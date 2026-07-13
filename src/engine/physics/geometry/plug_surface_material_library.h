#pragma once

#include <array>

#include "engine/core/cmw_nod.h"
#include "engine/rendering/plug_material.h"
class CPlugSurfaceMaterialLibrary {
public:
    CPlugMaterial *Default(EPlugSurfaceMaterialId id);
    CPlugMaterial *Default(u32 id);

private:
    void EnsureInitialized();

    bool initialized_ = false;
    std::array<CMwNodRef<CPlugMaterial>, EPlugSurfaceMaterialId_Count>
            materials_;
};

CPlugSurfaceMaterialLibrary &PlugSurfaceMaterials();
