#include "engine/rendering/plug_material_custom_parameters.h"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

#include "engine/core/binary32_math.h"
#include "engine/resources/system_fid.h"
namespace {

bool SameGpuScalarPrefix(const CPlugMaterialCustom::SGpuFx &left,
                         const CPlugMaterialCustom::SGpuFx &right,
                         size_t scalarCount) {
    const size_t leftCapacity = left.RegisterCount() * 4u;
    const size_t rightCapacity = right.RegisterCount() * 4u;
    if (scalarCount > leftCapacity || scalarCount > rightCapacity) {
        return false;
    }
    for (size_t index = 0u; index < scalarCount; ++index) {
        if (!Binary32::HaveSameEncoding(
                    left.RegisterAt(index / 4u)[index % 4u],
                    right.RegisterAt(index / 4u)[index % 4u])) {
            return false;
        }
    }
    return true;
}

}  // namespace

CPlugMaterialCustom::SFlags::SFlags(void)
        : shaderConfiguration_(),
          shaderApplyConfiguration_() {
    for (std::size_t setting = 18u; setting <= 24u; ++setting) {
        shaderConfiguration_.set(setting);
    }
    for (std::size_t setting = 8u; setting <= 14u; ++setting) {
        shaderApplyConfiguration_.set(setting);
    }
}

bool CPlugMaterialCustom::SFlags::IsDefaultConfiguration(void) const {
    return *this == SFlags{};
}

bool CPlugMaterialCustom::SFlags::operator==(
        const SFlags &other) const {
    return shaderConfiguration_ == other.shaderConfiguration_ &&
           shaderApplyConfiguration_ == other.shaderApplyConfiguration_;
}

bool CPlugMaterialCustom::SFlags::operator!=(
        const SFlags &other) const {
    return !(*this == other);
}

CPlugMaterialCustom::SBitmap::SBitmap(void)
        : SParam(BitmapParameterAudience(), 0),
          samplerId(),
          bitmapFid(nullptr) {}

CPlugMaterialCustom::SBitmap::SBitmap(
        const CMwId &sampler,
        CSystemFidFile *fid)
        : SParam(BitmapParameterAudience(), 0),
          samplerId(sampler),
          bitmapFid(fid) {}

CPlugMaterialCustom::SBitmap::~SBitmap(void) = default;

CSystemFidParameters::SParam *
CPlugMaterialCustom::SBitmap::Duplicate(void) const {
    return std::make_unique<SBitmap>(*this).release();
}

void CPlugMaterialCustom::SBitmap::CopyFrom(
        const CSystemFidParameters::SParam &source) {
    const auto *typed = dynamic_cast<const SBitmap *>(&source);
    if (typed == nullptr || typed == this) {
        return;
    }
    SParam::CopyFrom(*typed);
    samplerId = typed->samplerId;
    bitmapFid = typed->bitmapFid;
}

void CPlugMaterialCustom::SBitmap::Compare(
        const CSystemFidParameters::SParam &candidate,
        int &sameValue,
        int &differentKey) const {
    const auto *typed = dynamic_cast<const SBitmap *>(&candidate);
    const bool sameKey = typed != nullptr && samplerId == typed->samplerId;
    StoreCompareResult(sameKey && bitmapFid == typed->bitmapFid,
                       !sameKey,
                       sameValue,
                       differentKey);
}

int CPlugMaterialCustom::SBitmap::IsDefault(void) const {
    return bitmapFid == nullptr;
}

CPlugMaterialCustom::SGpuFx::SGpuFx(void)
        : SParam(ShaderAndPassParameterAudience(), 0),
          id_(),
          default_(true),
          componentCount_(1u),
          registers_(1u, SRegister{}) {}

CPlugMaterialCustom::SGpuFx::~SGpuFx(void) = default;

const CMwId &CPlugMaterialCustom::SGpuFx::Id(void) const {
    return id_;
}

void CPlugMaterialCustom::SGpuFx::SetId(const CMwId &newId) {
    id_ = newId;
}

void CPlugMaterialCustom::SGpuFx::SetDefault(bool isDefault) {
    default_ = isDefault;
}

u32 CPlugMaterialCustom::SGpuFx::ComponentCount(void) const {
    return componentCount_;
}

void CPlugMaterialCustom::SGpuFx::SetComponentCount(
        unsigned long componentCount) {
    if (componentCount < MinComponentCount ||
        componentCount > MaxComponentCount) {
        throw std::length_error(
                "GPU parameter component count must be between 1 and 4");
    }
    componentCount_ = static_cast<u32>(componentCount);
}

std::size_t CPlugMaterialCustom::SGpuFx::RegisterCount(void) const {
    return registers_.size();
}

CPlugMaterialCustom::SGpuFx::SRegister &
CPlugMaterialCustom::SGpuFx::RegisterAt(std::size_t registerIndex) {
    return registers_.at(registerIndex);
}

const CPlugMaterialCustom::SGpuFx::SRegister &
CPlugMaterialCustom::SGpuFx::RegisterAt(std::size_t registerIndex) const {
    return registers_.at(registerIndex);
}

void CPlugMaterialCustom::SGpuFx::SetRegisterCountAndAlloc(
        unsigned long registerCount) {
    if (registerCount > MaxRegisterCount) {
        throw std::length_error("GPU parameter register count exceeds 1023");
    }
    const size_t oldCount = registers_.size();
    if (registerCount == oldCount) {
        return;
    }
    if (oldCount != 0u) {
        disabledFirstRegister_ = registers_.front();
        registers_.resize(registerCount, SRegister{});
        return;
    }
    registers_.resize(registerCount, SRegister{});
    if (registerCount == 1u) {
        registers_.front() = disabledFirstRegister_;
    }
}

CSystemFidParameters::SParam *
CPlugMaterialCustom::SGpuFx::Duplicate(void) const {
    auto duplicate = std::make_unique<SGpuFx>();
    duplicate->CopyFrom(*this);
    return duplicate.release();
}

void CPlugMaterialCustom::SGpuFx::CopyFrom(
        const CSystemFidParameters::SParam &source) {
    const auto *typed = dynamic_cast<const SGpuFx *>(&source);
    if (typed == nullptr || typed == this) {
        return;
    }
    SParam::CopyFrom(*typed);
    id_ = typed->id_;
    default_ = typed->default_;
    SetComponentCount(typed->componentCount_);
    SetRegisterCountAndAlloc(
            static_cast<unsigned long>(typed->registers_.size()));
    registers_ = typed->registers_;
}

void CPlugMaterialCustom::SGpuFx::Compare(
        const CSystemFidParameters::SParam &candidate,
        int &sameValue,
        int &differentKey) const {
    const auto *typed = dynamic_cast<const SGpuFx *>(&candidate);
    const size_t scalarCount = ComponentCount() * RegisterCount();
    const bool sameKey = typed != nullptr && Id() == typed->Id() &&
                         scalarCount ==
                                 typed->ComponentCount() *
                                         typed->RegisterCount();
    StoreCompareResult(
            sameKey && SameGpuScalarPrefix(*this, *typed, scalarCount),
            !sameKey,
            sameValue,
            differentKey);
}

int CPlugMaterialCustom::SGpuFx::IsDefault(void) const {
    return default_;
}

CPlugMaterialCustom::SParamShaderFlags::SParamShaderFlags(void)
        : SParam(ShaderFlagsParameterAudience(), 0),
          shaderFlags(),
          visibleFilter(),
          requirement(),
          defaultParameter(true) {
    visibleFilter.ClearSelection();
}

CPlugMaterialCustom::SParamShaderFlags::SParamShaderFlags(
        const SFlags &flags,
        const SPlugVisibleFilter &filter,
        const CPlugShader::SRequirement &shaderRequirement,
        AssetAudience parameterAudience,
        int optional)
        : SParam(std::move(parameterAudience), optional),
          shaderFlags(flags),
          visibleFilter(filter),
          requirement(shaderRequirement),
          defaultParameter(false) {}

CPlugMaterialCustom::SParamShaderFlags::~SParamShaderFlags(void) = default;

CSystemFidParameters::SParam *
CPlugMaterialCustom::SParamShaderFlags::Duplicate(void) const {
    return std::make_unique<SParamShaderFlags>(*this).release();
}

void CPlugMaterialCustom::SParamShaderFlags::CopyFrom(
        const CSystemFidParameters::SParam &source) {
    const auto *typed = dynamic_cast<const SParamShaderFlags *>(&source);
    if (typed == nullptr || typed == this) {
        return;
    }
    SParam::CopyFrom(*typed);
    shaderFlags = typed->shaderFlags;
    visibleFilter = typed->visibleFilter;
    requirement = typed->requirement;
    defaultParameter = typed->defaultParameter;
}

void CPlugMaterialCustom::SParamShaderFlags::Compare(
        const CSystemFidParameters::SParam &candidate,
        int &sameValue,
        int &differentKey) const {
    const auto *typed = dynamic_cast<const SParamShaderFlags *>(&candidate);
    if (typed == nullptr) {
        StoreCompareResult(0, 1, sameValue, differentKey);
        return;
    }

    const bool defaultnessMatches =
            defaultParameter == typed->defaultParameter;
    const bool flagsMatch = shaderFlags == typed->shaderFlags;
    const bool filterMatches =
            visibleFilter.HasEquivalentSelection(typed->visibleFilter);
    const bool requirementMatches = requirement == typed->requirement;
    StoreCompareResult(defaultnessMatches && flagsMatch && filterMatches &&
                               requirementMatches,
                       0,
                       sameValue,
                       differentKey);
}

int CPlugMaterialCustom::SParamShaderFlags::IsDefault(void) const {
    return defaultParameter;
}

CPlugMaterialCustom::SParamFuncShader::SParamFuncShader(void)
        : SParam(ShaderParameterAudience(), 0),
          functionShaderFids() {}

CPlugMaterialCustom::SParamFuncShader::~SParamFuncShader(void) = default;

CSystemFidParameters::SParam *
CPlugMaterialCustom::SParamFuncShader::Duplicate(void) const {
    return std::make_unique<SParamFuncShader>(*this).release();
}

void CPlugMaterialCustom::SParamFuncShader::CopyFrom(
        const CSystemFidParameters::SParam &source) {
    const auto *typed = dynamic_cast<const SParamFuncShader *>(&source);
    if (typed == nullptr || typed == this) {
        return;
    }
    SParam::CopyFrom(*typed);
    functionShaderFids = typed->functionShaderFids;
}

void CPlugMaterialCustom::SParamFuncShader::Compare(
        const CSystemFidParameters::SParam &candidate,
        int &sameValue,
        int &differentKey) const {
    const auto *typed = dynamic_cast<const SParamFuncShader *>(&candidate);
    if (typed == nullptr) {
        StoreCompareResult(0, 1, sameValue, differentKey);
        return;
    }
    const bool matches =
            functionShaderFids.size() == typed->functionShaderFids.size() &&
            std::equal(
                    functionShaderFids.begin(),
                    functionShaderFids.end(),
                    typed->functionShaderFids.begin(),
                    [](const std::reference_wrapper<CSystemFid> &left,
                       const std::reference_wrapper<CSystemFid> &right) {
                        return &left.get() == &right.get();
                    });
    StoreCompareResult(matches, 0, sameValue, differentKey);
}

int CPlugMaterialCustom::SParamFuncShader::IsDefault(void) const {
    return functionShaderFids.empty();
}

SPlugGpuParamSkipSampler::SPlugGpuParamSkipSampler(void)
        : SParam(AnyAssetAudience(), 0),
          samplerId(),
          skip(false),
          kind(EKind::Material) {}

SPlugGpuParamSkipSampler::SPlugGpuParamSkipSampler(
        const CMwId &sampler,
        EKind parameterKind,
        AssetAudience parameterAudience)
        : SParam(std::move(parameterAudience), 0),
          samplerId(sampler),
          skip(false),
          kind(parameterKind) {}

SPlugGpuParamSkipSampler::~SPlugGpuParamSkipSampler(void) = default;

CSystemFidParameters::SParam *
SPlugGpuParamSkipSampler::Duplicate(void) const {
    return std::make_unique<SPlugGpuParamSkipSampler>(*this).release();
}

void SPlugGpuParamSkipSampler::CopyFrom(
        const CSystemFidParameters::SParam &source) {
    const auto *typed = dynamic_cast<const SPlugGpuParamSkipSampler *>(&source);
    if (typed == nullptr || typed == this) {
        return;
    }
    SParam::CopyFrom(*typed);
    samplerId = typed->samplerId;
    kind = typed->kind;
    skip = typed->skip;
}

void SPlugGpuParamSkipSampler::Compare(
        const CSystemFidParameters::SParam &candidate,
        int &sameValue,
        int &differentKey) const {
    const auto *typed = dynamic_cast<const SPlugGpuParamSkipSampler *>(
            &candidate);
    const bool sameKey = typed != nullptr && kind == typed->Kind() &&
                         samplerId == typed->samplerId;
    StoreCompareResult(sameKey && skip == typed->skip,
                       !sameKey,
                       sameValue,
                       differentKey);
}

int SPlugGpuParamSkipSampler::IsDefault(void) const {
    return !skip;
}

struct CPlugMaterialCustom::State {
    State(void) {
        visibleFilter.ClearSelection();
    }

    CPlugMaterial *associatedMaterial = nullptr;
    SFlags shaderFlags;
    SPlugVisibleFilter visibleFilter;
    CPlugShader::SRequirement shaderRequirement;
    std::vector<SPlugGpuParamSkipSampler> skipSamplers;
    std::vector<SBitmap> bitmaps;
    std::vector<SGpuFx> vertexGpuFx;
    std::vector<SGpuFx> pixelGpuFx;
    std::vector<std::reference_wrapper<CSystemFid>> functionShaderFids;
    CSystemFidParameters fidParameters;
};

CPlugMaterialCustom::CPlugMaterialCustom(void)
        : CPlug(), state(std::make_unique<State>()) {}

CPlugMaterialCustom::~CPlugMaterialCustom(void) = default;

CPlugMaterialCustom::SFlags &CPlugMaterialCustom::ShaderFlags(void) {
    return state->shaderFlags;
}

const CPlugMaterialCustom::SFlags &
CPlugMaterialCustom::ShaderFlags(void) const {
    return state->shaderFlags;
}

SPlugVisibleFilter &CPlugMaterialCustom::VisibleFilter(void) {
    return state->visibleFilter;
}

const SPlugVisibleFilter &CPlugMaterialCustom::VisibleFilter(void) const {
    return state->visibleFilter;
}

CPlugShader::SRequirement &CPlugMaterialCustom::ShaderRequirement(void) {
    return state->shaderRequirement;
}

const CPlugShader::SRequirement &
CPlugMaterialCustom::ShaderRequirement(void) const {
    return state->shaderRequirement;
}

std::vector<SPlugGpuParamSkipSampler> &
CPlugMaterialCustom::SkipSamplerParameters(void) {
    return state->skipSamplers;
}

std::vector<CPlugMaterialCustom::SBitmap> &
CPlugMaterialCustom::BitmapParameters(void) {
    return state->bitmaps;
}

std::vector<CPlugMaterialCustom::SGpuFx> &
CPlugMaterialCustom::VertexGpuFxParameters(void) {
    return state->vertexGpuFx;
}

std::vector<CPlugMaterialCustom::SGpuFx> &
CPlugMaterialCustom::PixelGpuFxParameters(void) {
    return state->pixelGpuFx;
}

std::vector<std::reference_wrapper<CSystemFid>> &
CPlugMaterialCustom::FuncShaderFids(void) {
    return state->functionShaderFids;
}

const CSystemFidParameters &CPlugMaterialCustom::FidParameters(void) const {
    return state->fidParameters;
}

CPlugMaterial *CPlugMaterialCustom::AssociatedMaterial(void) const {
    return state->associatedMaterial;
}

void CPlugMaterialCustom::SetMaterial(CPlugMaterial *material) {
    if (state->associatedMaterial != material) {
        state->associatedMaterial = material;
        UpdateFidParameters();
    }
}

const CSystemFidParameters *
CPlugMaterialCustom::AssociatedMaterialParameters(void) const {
    if (state->associatedMaterial == nullptr ||
        state->associatedMaterial->ModelMaterial() == nullptr ||
        state->associatedMaterial->ModelMaterial()->fid == nullptr) {
        return nullptr;
    }
    return &state->associatedMaterial->ModelMaterial()->fid->Parameters();
}

void CPlugMaterial::SetMaterialCustom(CPlugMaterialCustom *materialCustom) {
    if (customMaterial.Get() == materialCustom) {
        return;
    }
    if (materialCustom != nullptr &&
        materialCustom->AssociatedMaterial() != nullptr &&
        materialCustom->AssociatedMaterial() != this) {
        return;
    }
    CPlugMaterialCustom *previous = customMaterial.Get();
    if (previous != nullptr) {
        previous->SetMaterial(nullptr);
    }
    if (materialCustom != nullptr) {
        materialCustom->SetMaterial(this);
    }
    customMaterial.MwSetNod(materialCustom);
}

void CPlugMaterialCustom::UpdateFidParameters(void) {
    state->fidParameters = CSystemFidParameters::None;

    const SParamShaderFlags flags(state->shaderFlags,
                                  state->visibleFilter,
                                  state->shaderRequirement,
                                  ShaderFlagsParameterAudience(),
                                  0);
    state->fidParameters.AddParam(flags);

    for (const SPlugGpuParamSkipSampler &skipSampler : state->skipSamplers) {
        if (!skipSampler.IsDefault()) {
            state->fidParameters.AddParam(skipSampler);
        }
    }
    for (const SBitmap &bitmap : state->bitmaps) {
        state->fidParameters.AddParam(bitmap);
    }
    const auto addGpuFx = [this](const std::vector<SGpuFx> &gpuEffects) {
        for (const SGpuFx &gpuEffect : gpuEffects) {
            if (!gpuEffect.IsDefault()) {
                state->fidParameters.AddParam(gpuEffect);
            }
        }
    };
    addGpuFx(state->vertexGpuFx);
    addGpuFx(state->pixelGpuFx);

    if (!state->functionShaderFids.empty()) {
        SParamFuncShader functions;
        functions.functionShaderFids = state->functionShaderFids;
        state->fidParameters.AddParam(functions);
    }
}
