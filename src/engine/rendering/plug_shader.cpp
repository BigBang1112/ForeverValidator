#include "engine/rendering/plug_shader.h"
#include <algorithm>

#include "engine/rendering/plug_bitmap.h"
#include "engine/rendering/plug_bitmap_render.h"
#include "engine/resources/system_pack_desc.h"
#include "format/archive/archive_class_ids.h"
CPlugBitmapAddress::CPlugBitmapAddress(void) = default;
CPlugBitmapAddress::~CPlugBitmapAddress(void) = default;

void CPlugBitmapAddress::CreateBitmapAddress(
        CPlugShader *shader,
        CPlugBitmap *bitmap,
        unsigned long textureCoordinateSet) {
    shader_ = shader;
    bitmap_.MwSetNod(bitmap);
    textureCoordinateSet_ = textureCoordinateSet;
}

CPlugShader *CPlugBitmapAddress::Shader(void) const {
    return shader_;
}

CPlugBitmap *CPlugBitmapAddress::Bitmap(void) const {
    return bitmap_.Get();
}

CSystemFid *CPlugBitmapAddress::ImageFid(void) const {
    CPlugBitmap *bitmap = Bitmap();
    return bitmap != nullptr && bitmap->Image() != nullptr
            ? bitmap->Image()->fid
            : nullptr;
}

void CPlugBitmapAddress::SetBumpEnvironmentScale(float scale) {
    bumpEnvironmentScale_ = scale;
}

float CPlugBitmapAddress::BumpEnvironmentScale(void) const {
    return bumpEnvironmentScale_;
}

void CPlugBitmapAddress::SetTexCoordIndex(
        unsigned long textureCoordinateSet) {
    textureCoordinateSet_ = textureCoordinateSet;
}

unsigned long CPlugBitmapAddress::TextureCoordinateSet(void) const {
    return textureCoordinateSet_;
}

CPlugShaderPass::CPlugShaderPass(void) = default;
CPlugShaderPass::~CPlugShaderPass(void) {
    DetachShader();
}

void CPlugShaderPass::AttachShader(
        CPlugShader *shader,
        unsigned long passIndex) {
    shader_ = shader;
    passIndex_ = passIndex;
}

void CPlugShaderPass::DetachShader(void) {
    for (CMwNodRef<CPlugBitmapAddress> &address : bitmapAddresses_) {
        if (address->Shader() == shader_) {
            address->CreateBitmapAddress(
                    nullptr,
                    address->Bitmap(),
                    address->TextureCoordinateSet());
        }
    }
    shader_ = nullptr;
    passIndex_ = 0ul;
}

void CPlugShaderPass::AddBitmapAddress(CPlugBitmapAddress *address) {
    if (address == nullptr) {
        return;
    }
    const auto found = std::find_if(
            bitmapAddresses_.begin(),
            bitmapAddresses_.end(),
            [address](const CMwNodRef<CPlugBitmapAddress> &candidate) {
                return candidate.Get() == address;
            });
    if (found == bitmapAddresses_.end()) {
        if (shader_ != nullptr) {
            address->CreateBitmapAddress(
                    shader_, address->Bitmap(), address->TextureCoordinateSet());
        }
        bitmapAddresses_.emplace_back(address);
    }
}

void CPlugShaderPass::RemoveBitmapAddress(CPlugBitmapAddress *address) {
    bitmapAddresses_.erase(
            std::remove_if(
                    bitmapAddresses_.begin(),
                    bitmapAddresses_.end(),
                    [address](const CMwNodRef<CPlugBitmapAddress> &candidate) {
                        return candidate.Get() == address;
                    }),
            bitmapAddresses_.end());
}

const CPlugBitmapAddress *CPlugShaderPass::FirstBitmapAddress(void) const {
    return bitmapAddresses_.empty() ? nullptr : bitmapAddresses_.front().Get();
}

CPlugBitmapAddress *CPlugShaderPass::FirstMutableBitmapAddress(void) {
    return bitmapAddresses_.empty() ? nullptr : bitmapAddresses_.front().Get();
}

unsigned long CPlugShader::SRequirement::TextureCoordinateCount(void) const {
    return textureCoordinateCount_;
}

void CPlugShader::SRequirement::SetTextureCoordinateCount(
        unsigned long count) {
    textureCoordinateCount_ = count;
}

bool CPlugShader::SRequirement::RequiresVertexNormal(void) const {
    return vertexNormal_;
}

void CPlugShader::SRequirement::SetRequiresVertexNormal(bool required) {
    vertexNormal_ = required;
}

bool CPlugShader::SRequirement::RequiresVertexColor(void) const {
    return vertexColor_;
}

void CPlugShader::SRequirement::SetRequiresVertexColor(bool required) {
    vertexColor_ = required;
}

bool CPlugShader::SRequirement::RequiresFirstTangent(void) const {
    return firstTangent_;
}

void CPlugShader::SRequirement::SetRequiresFirstTangent(bool required) {
    firstTangent_ = required;
}

bool CPlugShader::SRequirement::RequiresSecondTangent(void) const {
    return secondTangent_;
}

void CPlugShader::SRequirement::SetRequiresSecondTangent(bool required) {
    secondTangent_ = required;
}

bool CPlugShader::SRequirement::operator==(
        const SRequirement &other) const {
    return textureCoordinateCount_ == other.textureCoordinateCount_ &&
           vertexNormal_ == other.vertexNormal_ &&
           vertexColor_ == other.vertexColor_ &&
           firstTangent_ == other.firstTangent_ &&
           secondTangent_ == other.secondTangent_;
}

bool CPlugShader::SRequirement::operator!=(
        const SRequirement &other) const {
    return !(*this == other);
}

CPlugShader::CPlugShader(void) = default;
CPlugShader::~CPlugShader(void) {
    for (CMwNodRef<CPlugShaderPass> &pass : passes_) {
        pass->DetachShader();
    }
}

CPlugShader::SRequirement &CPlugShader::Requirement(void) {
    return requirement_;
}

const CPlugShader::SRequirement &CPlugShader::Requirement(void) const {
    return requirement_;
}

void CPlugShader::AddPass(CPlugShaderPass *pass) {
    if (pass == nullptr) {
        return;
    }
    const auto found = std::find_if(
            passes_.begin(),
            passes_.end(),
            [pass](const CMwNodRef<CPlugShaderPass> &candidate) {
                return candidate.Get() == pass;
            });
    if (found == passes_.end()) {
        pass->AttachShader(this, static_cast<unsigned long>(passes_.size()));
        passes_.emplace_back(pass);
        SetDirty(1, 1);
    }
}

unsigned long CPlugShader::GetPassToRenderCount(void) const {
    return static_cast<unsigned long>(passes_.size());
}

const CPlugShaderPass *CPlugShader::GetPassToRender(
        unsigned long index) const {
    return index < passes_.size() ? passes_[index].Get() : nullptr;
}

void CPlugShader::CheckGeometryRequirement(void) {
    requirement_.SetRequiresVertexNormal(true);
}

void CPlugShader::RemovePasses(void) {
    for (CMwNodRef<CPlugShaderPass> &pass : passes_) {
        pass->DetachShader();
    }
    passes_.clear();
    SetDirty(1, 1);
}

u32 CPlugShader::RequiredTexCoordCount(void) const {
    return static_cast<u32>(requirement_.TextureCoordinateCount());
}

bool CPlugShader::RequiresVertexNormal(void) const {
    return requirement_.RequiresVertexNormal();
}

bool CPlugShader::RequiresVertexColor(void) const {
    return requirement_.RequiresVertexColor();
}

bool CPlugShader::RequiresFirstTangent(void) const {
    return requirement_.RequiresFirstTangent();
}

bool CPlugShader::RequiresSecondTangent(void) const {
    return requirement_.RequiresSecondTangent();
}

bool CPlugShader::RequiresShadowTexCoords(void) const {
    return RequiresFirstTangent();
}

bool CPlugShader::RequiresLightTexCoords(void) const {
    return RequiresSecondTangent();
}

bool CPlugShader::RequiresGeneratedTexCoords(void) const {
    return RequiresShadowTexCoords() || RequiresLightTexCoords();
}

u32 CPlugShader::GetRenderBeforeCount(void) const {
    return static_cast<u32>(GetPassToRenderCount());
}

const CPlugBitmapAddress *CPlugShader::RenderBeforeAddress(u32 index) const {
    const CPlugShaderPass *pass = GetPassToRender(index);
    return pass != nullptr ? pass->FirstBitmapAddress() : nullptr;
}

CPlugBitmapAddress *CPlugShader::GetRenderBeforeAt(u32 index) {
    if (index >= passes_.size()) {
        return nullptr;
    }
    return passes_[index]->FirstMutableBitmapAddress();
}

int CPlugShader::RenderBeforeEntryMatchesPackDesc(
        u32 index,
        const CSystemPackDesc &packDesc) const {
    const CPlugBitmapAddress *address = RenderBeforeAddress(index);
    return address != nullptr && packDesc.ReferencesFid(address->ImageFid());
}

void CPlugShader::SetDirty(int dirty, int visionDirty) {
    visionDirty_ = visionDirty != 0;
    dirty_ = dirty != 0;
}

bool CPlugShader::IsDirty(void) const {
    return dirty_;
}

bool CPlugShader::IsVisionDirty(void) const {
    return visionDirty_;
}

void CPlugShader::SetArchiveFlags(u32 flags) {
    archiveFlags_ = flags;
}

u32 CPlugShader::ArchiveFlags(void) const {
    return archiveFlags_;
}

CPlugBitmapRender *CPlugShader::FindBitmapRenderByClassId(
        unsigned long classId,
        CPlugBitmap **outBitmap,
        CPlugBitmapAddress **outAddress) {
    for (u32 layerIndex = 0u;
         layerIndex < GetRenderBeforeCount(); ++layerIndex) {
        CPlugBitmapAddress *address = GetRenderBeforeAt(layerIndex);
        CPlugBitmap *bitmap = address != nullptr ? address->Bitmap() : nullptr;
        CPlugBitmapRender *render =
                bitmap != nullptr &&
                        bitmap->PixelUpdateKind() ==
                                CPlugBitmap::PixelUpdate_Render
                ? bitmap->RenderPayload()
                : nullptr;
        if (render == nullptr || !render->MwIsKindOf(classId)) {
            continue;
        }
        if (outBitmap != nullptr) {
            *outBitmap = bitmap;
        }
        if (outAddress != nullptr) {
            *outAddress = address;
        }
        return render;
    }
    return nullptr;
}

CPlugShaderGeneric::CPlugShaderGeneric(void) = default;
CPlugShaderGeneric::~CPlugShaderGeneric(void) = default;

void CPlugShaderGeneric::SetMaterial(const SMaterial &material) {
    material_ = material;
    SetDirty(1, 1);
}

const CPlugShaderGeneric::SMaterial &CPlugShaderGeneric::Material(void) const {
    return material_;
}

CPlugShaderApply::CPlugShaderApply(void) = default;
CPlugShaderApply::~CPlugShaderApply(void) = default;

CMwNod *CPlugShaderApply::MwNewCPlugShaderApply(void) {
    return new CPlugShaderApply();
}

void CPlugShaderApply::RemoveLayers(void) {
    RemovePasses();
}

float CPlugShaderApply::DefaultApplyValue(void) const {
    return defaultApplyValue_;
}

unsigned char CPlugShaderApply::PrimaryAlphaReference(void) const {
    return primaryAlphaReference_;
}

unsigned char CPlugShaderApply::SecondaryAlphaReference(void) const {
    return secondaryAlphaReference_;
}

GxBGRAColor CPlugShaderApply::TextureOperationConstant(void) const {
    return textureOperationConstant_;
}
