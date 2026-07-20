#include "engine/rendering/plug_material.h"
#include <algorithm>
#include <new>
#include <optional>
#include <type_traits>
#include "engine/rendering/plug_bitmap.h"
#include "engine/rendering/plug_material_custom_parameters.h"
#include "engine/rendering/plug_tree.h"
#include "engine/rendering/plug_visual.h"
#include "engine/resources/system_fid.h"
#include "engine/resources/system_fid_parameters.h"
#include "format/archive/archive_class_ids.h"
namespace {
constexpr u32 CPlugMaterialShaderRefCount = 2u;

CMwNodRef<CPlugBitmapRender> CreateReplayBitmapRender(u32 classId) {
    if (classId == TMNF_CLASS_CPlugBitmapRenderWater) {
        return CMwNodRef<CPlugBitmapRender>(new CPlugBitmapRenderWater());
    }
    if (classId == TMNF_CLASS_CPlugBitmapRenderCubeMap) {
        return CMwNodRef<CPlugBitmapRender>(new CPlugBitmapRenderCubeMap());
    }
    if (classId == TMNF_CLASS_CPlugBitmapRenderScene3d) {
        return CMwNodRef<CPlugBitmapRender>(new CPlugBitmapRenderScene3d());
    }
    if (classId == TMNF_CLASS_CPlugBitmapRender) {
        return MakeMwNod<CPlugBitmapRender>();
    }
    return {};
}
}

UPlugRenderDevice CPlugMaterial::s_SupportedDevice{
        PlugRenderDevice_PC3,
        0xffu,
        PlugRenderQuality_VHigh};

static bool PlugRenderDeviceLess(
        const UPlugRenderDevice &left,
        const UPlugRenderDevice &right) {
    if (left.device != right.device) {
        return left.device < right.device;
    }
    if (left.subDevice != right.subDevice) {
        return left.subDevice < right.subDevice;
    }
    return left.quality < right.quality;
}

static bool PlugRenderDeviceEqual(
        const UPlugRenderDevice &left,
        const UPlugRenderDevice &right) {
    return left.device == right.device &&
           left.subDevice == right.subDevice &&
           left.quality == right.quality;
}

void CPlugMaterial::CommonConstructor(void) {
    surfaceMaterialId_ = EPlugSurfaceMaterialId_Concrete;
}

CPlugMaterial::CPlugMaterial(void) {
    CommonConstructor();
}

CPlugMaterial::CPlugMaterial(CPlugShader *shader)
        : CPlugMaterial() {
    ShaderAdd(
            shader,
            PlugRenderDevice_PC0,
            PlugRenderQuality_VLow);
}

CPlugMaterial::~CPlugMaterial(void) {
    ShaderRemoveAll();
    SetMaterialCustom(nullptr);
    materialModel.MwSetNod(nullptr);
}

void CPlugMaterial::SetReplayRenderDefinition(
        const MaterialRenderDefinition &definition) {
    try {
        MaterialRenderDefinition nextDefinition = definition;
        std::vector<SDeviceMat> nextDeviceMaterials;
        if (definition.Bitmaps().empty()) {
            static_assert(std::is_nothrow_move_assignable_v<
                          MaterialRenderDefinition>);
            replayRenderDefinition_ = std::move(nextDefinition);
            deviceMaterials.swap(nextDeviceMaterials);
            return;
        }

        const u32 key = definition.ShaderRenderDeviceWord();
        const u32 device = (key >> 16u) & 0xffffu;
        const u32 subDevice = (key >> 8u) & 0xffu;
        const u32 quality = key & 0xffu;
        if (device > PlugRenderDevice_PC3 ||
            quality > PlugRenderQuality_VHigh) {
            return;
        }

        CMwNodRef<CPlugShaderApply> shader =
                MakeMwNod<CPlugShaderApply>();
        shader->SetArchiveFlags(definition.ShaderFlags());
        for (const MaterialRenderBitmapDefinition &bitmapDefinition :
             definition.Bitmaps()) {
            CMwNodRef<CPlugBitmapRender> render =
                    CreateReplayBitmapRender(bitmapDefinition.renderClassId);
            if (!render) {
                if (bitmapDefinition.renderClassId == 0u) {
                    continue;
                }
                return;
            }
            CMwNodRef<CPlugBitmap> bitmap = MakeMwNod<CPlugBitmap>();
            bitmap->SetPixelUpdate(CPlugBitmap::PixelUpdate_Render,
                                   render.Get());
            CMwNodRef<CPlugBitmapAddress> address =
                    MakeMwNod<CPlugBitmapAddress>();
            address->CreateBitmapAddress(shader.Get(), bitmap.Get(), 0ul);
            CMwNodRef<CPlugShaderPass> pass =
                    MakeMwNod<CPlugShaderPass>();
            pass->AddBitmapAddress(address.Get());
            shader->AddPass(pass.Get());
        }
        if (shader->GetPassToRenderCount() != 0ul) {
            nextDeviceMaterials.emplace_back();
            SDeviceMat &selected = nextDeviceMaterials.back();
            selected.renderDevice = {
                    static_cast<EPlugRenderDevice>(device),
                    subDevice,
                    static_cast<EPlugRenderQuality>(quality)};
            selected.SetActiveShader(shader.Get());
        }
        replayRenderDefinition_ = std::move(nextDefinition);
        deviceMaterials.swap(nextDeviceMaterials);
    } catch (const std::bad_alloc &) {
        return;
    }
}

const MaterialRenderDefinition &
CPlugMaterial::ReplayRenderDefinition(void) const {
    return replayRenderDefinition_;
}

CMwNod *CPlugMaterial::MwNewCPlugMaterial(void) {
    return new CPlugMaterial();
}

UPlugRenderDevice CPlugMaterial::ShaderGetDevice(
        const CPlugShader *shader) {
    (void)shader;
    UPlugRenderDevice device;
    return device;
}

int CPlugMaterial::ShaderAdd(
        CPlugShader *shader,
        EPlugRenderDevice requestedDevice,
        EPlugRenderQuality requestedQuality) {
    if (shader == nullptr) {
        return 0;
    }

    UPlugRenderDevice device = ShaderGetDevice(shader);
    if (requestedQuality > device.quality) {
        device.quality = requestedQuality;
    }
    if (requestedDevice > device.device) {
        device.device = requestedDevice;
        device.subDevice = 0u;
    }

    size_t index = 0u;
    while (index < deviceMaterials.size() &&
           PlugRenderDeviceLess(deviceMaterials[index].renderDevice, device)) {
        index++;
    }

    int inserted = 0;
    if (index == deviceMaterials.size() ||
        !PlugRenderDeviceEqual(deviceMaterials[index].renderDevice, device)) {
        deviceMaterials.emplace(deviceMaterials.begin() + index);
        deviceMaterials[index].renderDevice = device;
        inserted = 1;
    }

    SDeviceMat &deviceMaterial = deviceMaterials[index];
    deviceMaterial.SetActiveShader(shader);
    CSystemFid *loadableFid = shader->fid != nullptr
            ? shader->fid->ParametrizedGetLoadableFid()
            : nullptr;
    deviceMaterial.SetShaderSourceAt(0u, loadableFid);
    deviceMaterial.SetShaderSourceAt(1u, loadableFid);
    return inserted;
}

void CPlugMaterial::ShaderRemoveAll(void) {
    for (SDeviceMat &deviceMaterial : deviceMaterials) {
        deviceMaterial.ReleaseShaders();
    }
    while (!deviceMaterials.empty()) {
        deviceMaterials.pop_back();
    }
}

int CPlugMaterial::DoesContainShader(
        CPlugShader *shader,
        unsigned long *indexOut) const {
    for (size_t deviceMatIndex = 0u;
         deviceMatIndex < deviceMaterials.size();
         deviceMatIndex++) {
        if (deviceMaterials[deviceMatIndex].ActiveShader() == shader) {
            if (indexOut != nullptr) {
                *indexOut = static_cast<unsigned long>(deviceMatIndex);
            }
            return 1;
        }
    }
    return 0;
}

unsigned long CPlugMaterial::GetSupportedDeviceMatIndex(void) const {
    if (deviceMaterials.empty()) {
        return InvalidEngineIndex;
    }

    UPlugRenderDevice supportedDevice = s_SupportedDevice;
    if (supportedDevice.device != PlugRenderDevice_PC2) {
        supportedDevice.subDevice = 0u;
    }

    int index = static_cast<int>(deviceMaterials.size()) - 1;
    while (PlugRenderDeviceLess(
            supportedDevice,
            deviceMaterials[static_cast<size_t>(index)].renderDevice)) {
        if (--index == -1) {
            return 0u;
        }
    }
    return static_cast<unsigned long>(index);
}

int CPlugMaterial::AreAllShadersLoaded(void) const {
    for (const SDeviceMat &deviceMaterial : deviceMaterials) {
        if (deviceMaterial.ActiveShader() == nullptr) {
            return 0;
        }
        for (u32 shaderIndex = 0u;
             shaderIndex < CPlugMaterialShaderRefCount;
             ++shaderIndex) {
            if (deviceMaterial.LoadedShaderAt(shaderIndex) == nullptr) {
                return 0;
            }
        }
    }
    return 1;
}

int CPlugMaterial::AllShadersHaveNext(
        const CPlugMaterial::SItAllShader &iterator) const {
    return iterator.deviceMatIndex < deviceMaterials.size() &&
           iterator.shaderRefIndex < CPlugMaterialShaderRefCount;
}

CPlugShader *CPlugMaterial::AllShadersGetNextShader(
        CPlugMaterial::SItAllShader &iterator) const {
    CPlugShader *shader = deviceMaterials[iterator.deviceMatIndex]
            .LoadedShaderAt(iterator.shaderRefIndex);

    iterator.shaderRefIndex++;
    if (iterator.shaderRefIndex >= CPlugMaterialShaderRefCount) {
        iterator.shaderRefIndex = 0u;
        iterator.deviceMatIndex++;
    }
    return shader;
}

int CPlugMaterial::ShaderLoadAll(void) {
    if (this->AreAllShadersLoaded()) {
        return 1;
    }

    int missingShader = 0;
    CPlugMaterialCustom *materialCustom = materialModel
            ? customMaterial.Get()
            : nullptr;

    for (SDeviceMat &deviceMaterial : deviceMaterials) {

        std::optional<u32> originalShaderFidIndex;
        for (u32 shaderFidIndex = 0;
             shaderFidIndex < CPlugMaterialShaderRefCount;
             shaderFidIndex++) {
            if (deviceMaterial.ShaderSourceAt(shaderFidIndex) ==
                deviceMaterial.ActiveShaderSource()) {
                originalShaderFidIndex = shaderFidIndex;
                break;
            }
        }
        if (!originalShaderFidIndex.has_value()) {
            originalShaderFidIndex = 0u;
        }

        for (u32 shaderRefIndex = 0;
             shaderRefIndex < CPlugMaterialShaderRefCount;
             shaderRefIndex++) {
            CSystemFid *shaderFid =
                    deviceMaterial.ShaderSourceAt(shaderRefIndex);
            if (deviceMaterial.ActiveShaderSource() != shaderFid) {
                deviceMaterial.SetActiveShaderSource(shaderFid);
                deviceMaterial.SetActiveShader(nullptr);
            }

            if (deviceMaterial.ActiveShader() == nullptr) {
                deviceMaterial.LoadShader(materialCustom);
            }

            CPlugShader *loadedShader = deviceMaterial.ActiveShader();
            if (loadedShader == nullptr) {
                missingShader = 1;
            }
            deviceMaterial.SetLoadedShaderAt(shaderRefIndex, loadedShader);
        }

        deviceMaterial.SetActiveShaderSource(
                deviceMaterial.ShaderSourceAt(*originalShaderFidIndex));
        CPlugShader *restoredShader =
                deviceMaterial.LoadedShaderAt(*originalShaderFidIndex);
        deviceMaterial.SetActiveShader(restoredShader);
    }

    return missingShader != 0 ? 0 : 1;
}

unsigned long CPlugMaterial::AllShadersGetTexCoordMaxCount(void) {
    this->ShaderLoadAll();

    u32 maxTexCoordCount = 0u;
    CPlugMaterial::SItAllShader iterator = {};
    while (this->AllShadersHaveNext(iterator)) {
        CPlugShader *shader = this->AllShadersGetNextShader(iterator);
        const u32 texCoordCount = shader->RequiredTexCoordCount();
        if (texCoordCount > maxTexCoordCount) {
            maxTexCoordCount = texCoordCount;
        }
    }
    return maxTexCoordCount;
}

void CPlugMaterial::AllShadersGetNeedTangents(
        int &firstNeedTangents,
        int &secondNeedTangents) {
    this->ShaderLoadAll();

    firstNeedTangents = 0;
    secondNeedTangents = 0;
    CPlugMaterial::SItAllShader iterator = {};
    while (this->AllShadersHaveNext(iterator)) {
        CPlugShader *shader = this->AllShadersGetNextShader(iterator);
        if (shader->RequiresFirstTangent()) {
            firstNeedTangents = 1;
        }
        if (shader->RequiresSecondTangent()) {
            secondNeedTangents = 1;
        }
    }
}

void CPlugMaterial::AllShadersGetGeometryRequirement(
        SPlugVDcls &geometryRequirement) {
    this->ShaderLoadAll();

    geometryRequirement.Clear();
    CPlugMaterial::SItAllShader iterator = {};
    while (this->AllShadersHaveNext(iterator)) {
        CPlugShader *shader = this->AllShadersGetNextShader(iterator);
        if (shader->RequiresVertexNormal()) {
            geometryRequirement.Require(EPlugVDcl_Normal);
        }
        if (shader->RequiresVertexColor()) {
            geometryRequirement.Require(EPlugVDcl_Color0);
        }
        if (shader->RequiresFirstTangent()) {
            geometryRequirement.Require(EPlugVDcl_TangentU);
        }
        if (shader->RequiresSecondTangent()) {
            geometryRequirement.Require(EPlugVDcl_TangentV);
        }
    }
}

void CPlugMaterial::AllShadersCheckGeometryRequirement(void) {
    this->ShaderLoadAll();

    CPlugMaterial::SItAllShader iterator = {};
    while (this->AllShadersHaveNext(iterator)) {
        CPlugShader *shader = this->AllShadersGetNextShader(iterator);
        if (shader != nullptr) {
            shader->CheckGeometryRequirement();
        }
    }
}

CPlugMaterial::SDeviceMat::SDeviceMat(void) = default;

CPlugMaterial::SDeviceMat::~SDeviceMat(void) = default;

CPlugMaterial::SDeviceMat &CPlugMaterial::SDeviceMat::operator=(
        const CPlugMaterial::SDeviceMat &other) {
    if (this != &other) {
        renderDevice = other.renderDevice;
        shaderVariants_ = other.shaderVariants_;
        activeShaderSource_ = other.activeShaderSource_;
        activeShader_ = other.activeShader_;
    }
    return *this;
}

CSystemFid *CPlugMaterial::SDeviceMat::ShaderSourceAt(u32 index) const {
    return shaderVariants_.at(index).source;
}

CPlugShader *CPlugMaterial::SDeviceMat::LoadedShaderAt(u32 index) const {
    return shaderVariants_.at(index).loadedShader;
}

void CPlugMaterial::SDeviceMat::SetShaderSourceAt(
        u32 index,
        CSystemFid *source) {
    shaderVariants_.at(index).source = source;
}

void CPlugMaterial::SDeviceMat::SetLoadedShaderAt(
        u32 index,
        CPlugShader *shader) {
    shaderVariants_.at(index).loadedShader = shader;
}

CSystemFid *CPlugMaterial::SDeviceMat::ActiveShaderSource(void) const {
    return activeShaderSource_;
}

CPlugShader *CPlugMaterial::SDeviceMat::ActiveShader(void) const {
    return activeShader_;
}

void CPlugMaterial::SDeviceMat::SetActiveShaderSource(CSystemFid *source) {
    activeShaderSource_ = source;
}

void CPlugMaterial::SDeviceMat::SetActiveShader(CPlugShader *shader) {
    activeShader_ = shader;
}

CPlugShader *CPlugMaterial::SDeviceMat::LoadShader(
        CPlugMaterialCustom *customMaterial) {
    if (ActiveShaderSource() == nullptr) {
        return nullptr;
    }

    CPlugShader *loadedShader =
            CPlugMaterialCustom::ShaderLoadFromFidParam(
                    ActiveShaderSource(), customMaterial);
    SetActiveShader(loadedShader);
    return ActiveShader();
}

void CPlugMaterial::SDeviceMat::ReleaseShaders(void) {
    SetActiveShader(nullptr);
    for (ShaderVariant &variant : shaderVariants_) {
        variant.loadedShader.MwSetNod(nullptr);
    }
}

CPlugShader *CPlugMaterial::GetSupportedShader(void) {
    const unsigned long index = this->GetSupportedDeviceMatIndex();
    if (index >= deviceMaterials.size()) {
        return nullptr;
    }

    CPlugMaterial::SDeviceMat &deviceMaterial = deviceMaterials[index];
    if (deviceMaterial.ActiveShader() != nullptr) {
        return deviceMaterial.ActiveShader();
    }
    CPlugMaterialCustom *materialCustom = materialModel
            ? customMaterial.Get()
            : nullptr;
    return deviceMaterial.LoadShader(materialCustom);
}



void CPlugMaterial::ModifyTreeBeforeOptim(CPlugTree *tree) {
    if (tree != nullptr) {
        tree->SetMaterial(this);
    }
}

CPlugMaterial *CPlugMaterial::ModelMaterial(void) const {
    return materialModel;
}

CPlugMaterialCustom *CPlugMaterial::CustomMaterial(void) const {
    return customMaterial;
}
