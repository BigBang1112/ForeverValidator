#include "engine/rendering/plug_visual.h"
#include <algorithm>

#include "engine/rendering/plug_vertex_stream.h"
uint32_t CPlugVisual::VertexStreamTexCoordCount(void) const {
    uint32_t count = 0u;
    for (const CMwNodRef<CPlugVertexStream> &stream : vertexStreams) {
        if (stream) {
            count = std::max(count,
                             static_cast<uint32_t>(
                                     stream->TexCoordDeclarationCount()));
        }
    }
    return count;
}

bool CPlugVisual::VertexStreamsHave(EPlugVDcl declaration) const {
    return std::any_of(
            vertexStreams.begin(), vertexStreams.end(),
            [declaration](const CMwNodRef<CPlugVertexStream> &stream) {
                return stream && stream->HasDeclaration(declaration);
            });
}

bool CPlugVisual::RequiresTexCoordInference(void) const {
    return dynamic_cast<const CPlugVisualIndexed *>(this) == nullptr;
}

uint32_t CPlugVisual::AvailableTexCoordCountForShader(void) const {
    uint32_t availableTexCoordCount = TexCoordSetCount();
    const uint32_t inferredTexCoordCount = VertexStreamTexCoordCount();
    if (availableTexCoordCount < inferredTexCoordCount) {
        availableTexCoordCount = inferredTexCoordCount;
    }
    return availableTexCoordCount;
}

int CPlugVisual::EnsureShaderTexCoordSetCount(
        const CPlugShader &shader) {
    if (!RequiresTexCoordInference()) {
        return 0;
    }

    uint32_t availableTexCoordCount = AvailableTexCoordCountForShader();
    int changed = 0;
    while (availableTexCoordCount < shader.RequiredTexCoordCount()) {
        changed = 1;
        AddDefaultTexCoordSet();
        availableTexCoordCount++;
    }
    return changed;
}

bool CPlugVisual::HasShadowTexCoords(void) const {
    return VertexStreamsHave(EPlugVDcl_TangentU);
}

bool CPlugVisual::HasLightTexCoords(void) const {
    return VertexStreamsHave(EPlugVDcl_TangentV);
}

bool CPlugVisual::NeedsShadowTexCoords(
        const CPlugShader &shader) const {
    const CPlugVisual3D *visual3D = dynamic_cast<const CPlugVisual3D *>(this);
    return shader.RequiresShadowTexCoords() &&
           visual3D != nullptr &&
           visual3D->TangentCount() == 0u &&
           !HasShadowTexCoords();
}

bool CPlugVisual::NeedsLightTexCoords(
        const CPlugShader &shader) const {
    const CPlugVisual3D *visual3D = dynamic_cast<const CPlugVisual3D *>(this);
    return shader.RequiresLightTexCoords() &&
           visual3D != nullptr &&
           visual3D->ShaderCount() == 0u &&
           !HasLightTexCoords();
}

int CPlugVisual::AddDefaultTexCoordSet(void) {
    return static_cast<int>(
            AddTexCoordSet(1.0f, 1.0f, 0u, 0.0f, 0.0f));
}

int CPlugVisual::ComputeTangentsForRequirements(
        uint32_t requiresShadowTexCoords,
        uint32_t requiresLightTexCoords) {
    (void)requiresShadowTexCoords;
    (void)requiresLightTexCoords;
    return 0;
}



int CPlugVisual::UpdateRequirementChannels(
        uint32_t requiresShadowTexCoords,
        uint32_t requiresLightTexCoords) {
    return ComputeTangentsForRequirements(
            requiresShadowTexCoords,
            requiresLightTexCoords);
}

int CPlugVisual::UpdateVisualFromShaderRequirement(
        const CPlugShader *&shaderRef,
        CPlugShader *replacementShader) {
    const CPlugShader &shader = *shaderRef;
    int changed = EnsureShaderTexCoordSetCount(shader);

    if (shader.RequiresVertexNormal() && !HasVertexNormal()) {
        EnableVertexNormal(1);
        changed = 1;
    }
    if (shader.RequiresVertexColor() && !HasVertexColor()) {
        EnableVertexColor(1);
        changed = 1;
    }

    if (!shader.RequiresGeneratedTexCoords()) {
        return changed;
    }

    if (dynamic_cast<CPlugVisual3D *>(this) == nullptr) {
        shaderRef = replacementShader;
        return changed;
    }

    const bool needsShadowTexCoords =
            NeedsShadowTexCoords(shader);
    const bool needsLightTexCoords =
            NeedsLightTexCoords(shader);
    if (!needsShadowTexCoords && !needsLightTexCoords) {
        return changed;
    }

    if (UpdateRequirementChannels(
                shader.RequiresShadowTexCoords(),
                shader.RequiresLightTexCoords()) == 0) {
        shaderRef = replacementShader;
    }
    return 1;
}
