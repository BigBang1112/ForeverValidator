#include "engine/game/material_definition.h"
#include "engine/rendering/plug_material.h"
bool MaterialSurfaceDefinition::IsDefined() const {
    return materialId_.has_value();
}

EPlugSurfaceMaterialId MaterialSurfaceDefinition::MaterialId() const {
    return materialId_.value_or(EPlugSurfaceMaterialId_Concrete);
}

void MaterialSurfaceDefinition::ApplyTo(CPlugMaterial &material) const {
    if (materialId_.has_value()) {
        material.SetSurfaceMaterialId(*materialId_);
    }
}
