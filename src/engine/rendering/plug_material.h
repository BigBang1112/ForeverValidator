#pragma once

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include "engine/core/engine_types.h"
#include "engine/rendering/plug.h"
#include "engine/game/surface_material.h"
#include "engine/rendering/plug_bitmap_render.h"
#include "engine/rendering/plug_shader.h"
struct CPlugMaterialCustom;
struct CPlugTree;
struct CSystemFid;
struct CSystemFidFile;
struct CSystemFidParameters;
struct SPlugGpuParamSkipSampler;
struct SPlugVDcls;

enum EPlugRenderDevice {
    PlugRenderDevice_PC0 = 0,
    PlugRenderDevice_PC1 = 1,
    PlugRenderDevice_PC2 = 2,
    PlugRenderDevice_PC3 = 3,
};

enum EPlugRenderQuality {
    PlugRenderQuality_VLow = 0,
    PlugRenderQuality_Low = 1,
    PlugRenderQuality_Medium = 2,
    PlugRenderQuality_High = 3,
    PlugRenderQuality_VHigh = 4,
};

struct UPlugRenderDevice {
    EPlugRenderDevice device = PlugRenderDevice_PC0;
    u32 subDevice = 0u;
    EPlugRenderQuality quality = PlugRenderQuality_VLow;
};

struct CPlugMaterial : CPlug {
    struct SItAllShader {
        unsigned long deviceMatIndex = 0ul;
        unsigned long shaderRefIndex = 0ul;
    };

    struct SDeviceMat {
        struct ShaderVariant {
            CSystemFid *source = nullptr;
            CMwNodRef<CPlugShader> loadedShader;
        };

        UPlugRenderDevice renderDevice;

        SDeviceMat(void);
        ~SDeviceMat(void);
        SDeviceMat &operator=(const SDeviceMat &other);
        CPlugShader *LoadShader(CPlugMaterialCustom *customMaterial);
        void ReleaseShaders(void);
        CSystemFid *ShaderSourceAt(u32 index) const;
        CPlugShader *LoadedShaderAt(u32 index) const;
        void SetShaderSourceAt(u32 index, CSystemFid *source);
        void SetLoadedShaderAt(u32 index, CPlugShader *shader);
        CSystemFid *ActiveShaderSource(void) const;
        CPlugShader *ActiveShader(void) const;
        void SetActiveShaderSource(CSystemFid *source);
        void SetActiveShader(CPlugShader *shader);

    private:
        std::array<ShaderVariant, 2> shaderVariants_{};
        CSystemFid *activeShaderSource_ = nullptr;
        CMwNodRef<CPlugShader> activeShader_;
    };

    CPlugMaterial(void);
    explicit CPlugMaterial(CPlugShader *shader);
    ~CPlugMaterial(void) override;
    static CMwNod *MwNewCPlugMaterial(void);
    static UPlugRenderDevice ShaderGetDevice(const CPlugShader *shader);
    int ShaderAdd(CPlugShader *shader,
                  EPlugRenderDevice device,
                  EPlugRenderQuality quality);
    unsigned long GetSupportedDeviceMatIndex(void) const;
    int DoesContainShader(CPlugShader *shader,
                          unsigned long *indexOut) const;
    int AreAllShadersLoaded(void) const;
    int AllShadersHaveNext(const SItAllShader &iterator) const;
    CPlugShader *AllShadersGetNextShader(SItAllShader &iterator) const;
    int ShaderLoadAll(void);
    unsigned long AllShadersGetTexCoordMaxCount(void);
    void AllShadersGetNeedTangents(int &firstNeedTangents,
                                   int &secondNeedTangents);
    void AllShadersGetGeometryRequirement(SPlugVDcls &geometryRequirement);
    void AllShadersCheckGeometryRequirement(void);
    CPlugShader *GetSupportedShader(void);
    void SetMaterialCustom(CPlugMaterialCustom *materialCustom);
    EPlugSurfaceMaterialId SurfaceMaterialId(void) const;
    void SetSurfaceMaterialId(EPlugSurfaceMaterialId materialId);
    void ModifyTreeBeforeOptim(CPlugTree *tree);
    CPlugMaterial *ModelMaterial(void) const;
    CPlugMaterialCustom *CustomMaterial(void) const;

protected:
    static UPlugRenderDevice s_SupportedDevice;
    void ShaderRemoveAll(void);

private:
    std::vector<SDeviceMat> deviceMaterials;
    CMwNodRef<CPlugMaterial> materialModel;
    CMwNodRef<CPlugMaterialCustom> customMaterial;
    EPlugSurfaceMaterialId surfaceMaterialId_ =
            EPlugSurfaceMaterialId_Concrete;
    void CommonConstructor(void);
};

struct CPlugMaterialCustom : CPlug {
    struct SFlags;
    struct SBitmap;
    struct SGpuFx;
    struct SParamShaderFlags;
    struct SParamFuncShader;

    CPlugMaterialCustom(void);
    ~CPlugMaterialCustom(void) override;
    CPlugMaterialCustom(const CPlugMaterialCustom &) = delete;
    CPlugMaterialCustom &operator=(const CPlugMaterialCustom &) = delete;

    SFlags &ShaderFlags(void);
    const SFlags &ShaderFlags(void) const;
    SPlugVisibleFilter &VisibleFilter(void);
    const SPlugVisibleFilter &VisibleFilter(void) const;
    CPlugShader::SRequirement &ShaderRequirement(void);
    const CPlugShader::SRequirement &ShaderRequirement(void) const;
    std::vector<SPlugGpuParamSkipSampler> &SkipSamplerParameters(void);
    std::vector<SBitmap> &BitmapParameters(void);
    std::vector<SGpuFx> &VertexGpuFxParameters(void);
    std::vector<SGpuFx> &PixelGpuFxParameters(void);
    std::vector<std::reference_wrapper<CSystemFid>> &FuncShaderFids(void);
    const CSystemFidParameters &FidParameters(void) const;
    void UpdateFidParameters(void);

    static CPlugShader *ShaderLoadFromFidParam(
            CSystemFid *shaderFid,
            CPlugMaterialCustom *customMaterial);

protected:
    void SetMaterial(CPlugMaterial *material);

private:
    friend struct CPlugMaterial;
    CPlugMaterial *AssociatedMaterial(void) const;
    const CSystemFidParameters *AssociatedMaterialParameters(void) const;
    struct State;
    std::unique_ptr<State> state;
};

inline EPlugSurfaceMaterialId CPlugMaterial::SurfaceMaterialId(void) const {
    return surfaceMaterialId_;
}

inline void CPlugMaterial::SetSurfaceMaterialId(
        EPlugSurfaceMaterialId materialId) {
    surfaceMaterialId_ = materialId;
}
