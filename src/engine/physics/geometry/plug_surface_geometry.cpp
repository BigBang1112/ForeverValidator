#include "engine/physics/geometry/plug_surface.h"

#include <utility>

void CPlugSurfaceGeom::ComputeBoundingBox(void) {
    gmSurf->GetBoundingBox(bounds_);
}

CPlugSurfaceGeom::CPlugSurfaceGeom(void) {
    bounds_.center = {};
    bounds_.halfExtents = {-1.0f, -1.0f, -1.0f};
}

CPlugSurfaceGeom::~CPlugSurfaceGeom(void) = default;

void CPlugSurfaceGeom::CreateDefaultData(void) {
    gmSurf.reset();
    bounds_.center = {};
    bounds_.halfExtents = {-1.0f, -1.0f, -1.0f};
    hasDefaultData_ = true;
}

void CPlugSurfaceGeom::SetGmSurf(std::unique_ptr<GmSurf> surf) {
    gmSurf = std::move(surf);
    if (gmSurf != nullptr) {
        ComputeBoundingBox();
    }
}

const GmSurf *CPlugSurfaceGeom::Surface(void) const {
    return gmSurf.get();
}

const CMwId &CPlugSurfaceGeom::Id(void) const {
    return id_;
}

void CPlugSurfaceGeom::SetId(const CMwId &id) {
    id_ = id;
}

bool CPlugSurfaceGeom::HasDefaultData(void) const {
    return hasDefaultData_;
}

void CPlugSurfaceGeom::SetHasDefaultData(bool hasDefaultData) {
    hasDefaultData_ = hasDefaultData;
}

const GmBoxAligned &CPlugSurfaceGeom::Bounds(void) const {
    return bounds_;
}

void CPlugSurfaceGeom::TranslateMeshVerticesAbove(
        float yThreshold,
        float yDelta) {
    auto *mesh = dynamic_cast<GmSurfMesh *>(gmSurf.get());
    if (mesh == nullptr) {
        return;
    }
    mesh->TranslateVerticesAbove(yThreshold, yDelta);
    ComputeBoundingBox();
}

void CPlugSurfaceGeom::TransformByNOMat(const GmIso4 &transform) {
    auto *mesh = dynamic_cast<GmSurfMesh *>(gmSurf.get());
    if (mesh == nullptr) {
        return;
    }
    mesh->TransformByNOMat(transform);
    ComputeBoundingBox();
}
