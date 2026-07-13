#pragma once

#include "engine/core/engine_types.h"
#include <bitset>
#include <cstddef>
#include <functional>

#include "engine/rendering/plug_material.h"
#include "engine/rendering/plug_shader.h"
#include "engine/resources/system_fid_parameter_types.h"
namespace TmnfFormat {
struct MaterialCustomShaderFlagsArchiveCodec;
}

struct CPlugMaterialCustom::SFlags {
    SFlags(void);

    bool IsDefaultConfiguration(void) const;
    bool operator==(const SFlags &other) const;
    bool operator!=(const SFlags &other) const;

private:
    friend struct TmnfFormat::MaterialCustomShaderFlagsArchiveCodec;

    std::bitset<31> shaderConfiguration_;
    std::bitset<32> shaderApplyConfiguration_;
};

struct CPlugMaterialCustom::SBitmap final
        : CSystemFidParameters::SParam {
    CMwId samplerId;
    CSystemFidFile *bitmapFid;

    SBitmap(void);
    SBitmap(const CMwId &samplerId, CSystemFidFile *bitmapFid);
    ~SBitmap(void) override;

    SParam *Duplicate(void) const override;
    void CopyFrom(const SParam &source) override;
    void Compare(const SParam &candidate,
                 int &sameValue,
                 int &differentKey) const override;
    int IsDefault(void) const override;
};

struct CPlugMaterialCustom::SGpuFx final
        : CSystemFidParameters::SParam {
    using SRegister = std::array<float, 4>;

    static constexpr u32 MinComponentCount = 1u;
    static constexpr u32 MaxComponentCount = 4u;
    static constexpr u32 MaxRegisterCount = 1023u;

    SGpuFx(void);
    ~SGpuFx(void) override;

    const CMwId &Id(void) const;
    void SetId(const CMwId &newId);
    void SetDefault(bool isDefault);
    u32 ComponentCount(void) const;
    void SetComponentCount(unsigned long componentCount);
    std::size_t RegisterCount(void) const;
    SRegister &RegisterAt(std::size_t registerIndex);
    const SRegister &RegisterAt(std::size_t registerIndex) const;
    void SetRegisterCountAndAlloc(unsigned long registerCount);
    SParam *Duplicate(void) const override;
    void CopyFrom(const SParam &source) override;
    void Compare(const SParam &candidate,
                 int &sameValue,
                 int &differentKey) const override;
    int IsDefault(void) const override;

private:
    CMwId id_;
    bool default_;
    u32 componentCount_;
    std::vector<SRegister> registers_;
    SRegister disabledFirstRegister_{};
};

struct CPlugMaterialCustom::SParamShaderFlags final
        : CSystemFidParameters::SParam {
    SFlags shaderFlags;
    SPlugVisibleFilter visibleFilter;
    CPlugShader::SRequirement requirement;

    SParamShaderFlags(void);
    SParamShaderFlags(const SFlags &shaderFlags,
                      const SPlugVisibleFilter &visibleFilter,
                      const CPlugShader::SRequirement &requirement,
                      AssetAudience audience,
                      int optional);
    ~SParamShaderFlags(void) override;

    SParam *Duplicate(void) const override;
    void CopyFrom(const SParam &source) override;
    void Compare(const SParam &candidate,
                 int &sameValue,
                 int &differentKey) const override;
    int IsDefault(void) const override;

private:
    bool defaultParameter;
};

struct CPlugMaterialCustom::SParamFuncShader final
        : CSystemFidParameters::SParam {
    std::vector<std::reference_wrapper<CSystemFid>> functionShaderFids;

    SParamFuncShader(void);
    ~SParamFuncShader(void) override;

    SParam *Duplicate(void) const override;
    void CopyFrom(const SParam &source) override;
    void Compare(const SParam &candidate,
                 int &sameValue,
                 int &differentKey) const override;
    int IsDefault(void) const override;
};

struct SPlugGpuParamSkipSampler final : CSystemFidParameters::SParam {
    enum class EKind : u32 {
        Material = 0u,
        ShaderPass = 1u,
    };

    CMwId samplerId;
    bool skip;

    SPlugGpuParamSkipSampler(void);
    SPlugGpuParamSkipSampler(const CMwId &samplerId,
                             EKind kind,
                             AssetAudience audience);
    ~SPlugGpuParamSkipSampler(void) override;

    EKind Kind(void) const { return kind; }

    SParam *Duplicate(void) const override;
    void CopyFrom(const SParam &source) override;
    void Compare(const SParam &candidate,
                 int &sameValue,
                 int &differentKey) const override;
    int IsDefault(void) const override;

private:
    EKind kind;
};
