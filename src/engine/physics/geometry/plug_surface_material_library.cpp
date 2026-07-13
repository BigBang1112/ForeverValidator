#include "engine/physics/geometry/plug_surface_material_library.h"
void CPlugSurfaceMaterialLibrary::EnsureInitialized() {
    if (initialized_) {
        return;
    }
    for (u32 id = 0; id < materials_.size(); ++id) {
        CPlugMaterial *material = new CPlugMaterial();
        material->SetSurfaceMaterialId(
                static_cast<EPlugSurfaceMaterialId>(id));
        materials_[id].MwSetNod(material);
    }
    initialized_ = true;
}

CPlugMaterial *CPlugSurfaceMaterialLibrary::Default(
        EPlugSurfaceMaterialId id) {
    return Default(static_cast<u32>(id));
}

CPlugMaterial *CPlugSurfaceMaterialLibrary::Default(u32 id) {
    EnsureInitialized();
    return id < materials_.size() ? materials_[id].Get() : nullptr;
}

CPlugSurfaceMaterialLibrary &PlugSurfaceMaterials() {
    static CPlugSurfaceMaterialLibrary library;
    return library;
}
