#pragma once

#include "engine/game/material_definition.h"
struct MaterialSurfaceDefinitionFormatAccess {
    static MaterialSurfaceDefinition FromArchiveFlags(u32 flags) {
        MaterialSurfaceDefinition definition;
        definition.materialId_ = static_cast<EPlugSurfaceMaterialId>(
                flags & 0xffu);
        return definition;
    }
};
