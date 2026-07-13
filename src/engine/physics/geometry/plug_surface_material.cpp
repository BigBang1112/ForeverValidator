#include "engine/physics/geometry/plug_surface.h"

CPlugSurfaceMaterialData CPlugSurfaceMaterialData::ForMaterial(
        EPlugSurfaceMaterialId materialId) {
    switch (materialId) {
    case EPlugSurfaceMaterialId_Ice:
    case EPlugSurfaceMaterialId_Rubber:
    case EPlugSurfaceMaterialId_Test:
        return {0.0f, 0.0f};
    case EPlugSurfaceMaterialId_SlidingRubber:
        return {0.0f, -0.5f};
    case EPlugSurfaceMaterialId_GolfBall:
        return {1.0f, 0.95f};
    case EPlugSurfaceMaterialId_GolfWall:
    case EPlugSurfaceMaterialId_GolfGround:
        return {1.0f, 0.8f};
    default:
        return {1.0f, 0.5f};
    }
}

float CPlugSurfaceMaterialData::GetRestitutionCoefWith(
        const CPlugSurfaceMaterialData &other) {
    const float selfRestitution = restitutionCoef;
    const float otherRestitution = other.restitutionCoef;

    if (selfRestitution > 0.0f) {
        if (otherRestitution > 0.0f) {
            return (
                    otherRestitution * selfRestitution);
        }
        return otherRestitution;
    }
    if (otherRestitution > 0.0f) {
        return selfRestitution;
    }
    return (
            otherRestitution + selfRestitution);
}
