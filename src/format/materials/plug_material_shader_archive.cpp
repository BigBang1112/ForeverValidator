#include "engine/rendering/plug_material.h"
#include <optional>

#include "engine/resources/system_fid.h"
#include "engine/resources/system_fid_parameters.h"
CPlugShader *CPlugMaterialCustom::ShaderLoadFromFidParam(
        CSystemFid *shaderFid,
        CPlugMaterialCustom *customMaterial) {
    CSystemFidParameters customParameters;
    CMwNod *loadedShader = nullptr;
    {
        std::optional<CSystemFidParameters::Scope> parameterScope;
        if (customMaterial != nullptr) {
            customMaterial->UpdateFidParameters();
            const CSystemFidParameters *materialParameters =
                    customMaterial->AssociatedMaterialParameters();
            if (materialParameters != nullptr) {
                customParameters.MergeForFid(
                        *materialParameters,
                        shaderFid);
            }
            customParameters.MergeForFid(
                    customMaterial->FidParameters(),
                    shaderFid);
            parameterScope.emplace(customParameters, 1);
        }

        if (shaderFid != nullptr) {
            shaderFid->LoadNode(loadedShader);
        }
    }
    return dynamic_cast<CPlugShader *>(loadedShader);
}
