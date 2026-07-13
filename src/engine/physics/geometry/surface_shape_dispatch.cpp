// Surface objects, bounds, and type-directed collision dispatch.

#include <cmath>

#include "engine/core/binary32_math.h"
#include "engine/physics/geometry/geometry_helpers.h"
#include "engine/physics/collision/gm_collision_buffer.h"
#include "engine/physics/geometry/gmsurf_collision.h"
#include "engine/physics/geometry/physics_tolerances.h"
GmSurfSphere::GmSurfSphere(void) = default;

GmSurfEllipsoid::GmSurfEllipsoid(void) = default;

GmSurfEllipsoid::~GmSurfEllipsoid(void) = default;

GmSurfBox::GmSurfBox(void) = default;

GmSurfMesh::GmSurfMesh(void) = default;

GmSurfMesh::~GmSurfMesh(void) = default;

GmSurfPolygon::GmSurfPolygon(uint8_t vertexCount)
        : vertexCount(vertexCount), backSide(false) {}


namespace {

int FinishReversedCollision(u32 firstNewCollision,
                            int collided,
                            CGmCollisionBuffer &collisionBuffer) {
    if (!collided) {
        return 0;
    }
    const u32 collisionCount = collisionBuffer.GetCurrentCount();
    for (u32 collisionIndex = firstNewCollision;
         collisionIndex < collisionCount;
         collisionIndex++) {
        collisionBuffer.GetCollision(collisionIndex).Neg();
    }
    return 1;
}

}  // namespace

int GmSurfSphere::UsesSphereContactBuffer(void) const {
    return 1;
}

int GmSurfSphere::GetRollingRadius(float &outRadius) const {
    outRadius = radius;
    return 1;
}

int GmSurfEllipsoid::UsesSphereContactBuffer(void) const {
    return 1;
}

int GmSurfEllipsoid::GetRollingRadius(float &outRadius) const {
    outRadius = radii.y;
    return 1;
}

void GmSurfSphere::ComputeBoundingBox(GmBoxAligned &out) const {
    GetSphereBoundingBox(out);
}

void GmSurfEllipsoid::ComputeBoundingBox(GmBoxAligned &out) const {
    GetEllipsoidBoundingBox(out);
}

void GmSurfBox::ComputeBoundingBox(GmBoxAligned &out) const {
    out.center.x = (center.x);
    out.center.y = (center.y);
    out.center.z = (center.z);
    out.halfExtents.x = (halfExtents.x);
    out.halfExtents.y = (halfExtents.y);
    out.halfExtents.z = (halfExtents.z);
}

void GmSurfPolygon::ComputeBoundingBox(GmBoxAligned &out) const {
    (void)out;
}

void GmSurfMesh::ComputeBoundingBox(GmBoxAligned &out) const {
    GetMeshBoundingBox(out);
}

int GmSurfSphere::Collide(
        const LocatedGmSurf &self,
        const LocatedGmSurf &other,
        CGmCollisionBuffer &collisionBuffer) const {
    if (dynamic_cast<const GmSurfSphere *>(other.surf) != nullptr) {
        return GmCollision_Sphere_Sphere(self, other, collisionBuffer);
    }
    if (dynamic_cast<const GmSurfEllipsoid *>(other.surf) != nullptr) {
        return GmCollision_Sphere_Ellipsoid(self, other, collisionBuffer);
    }
    if (dynamic_cast<const GmSurfPolygon *>(other.surf) != nullptr) {
        return GmCollision_Sphere_Polygon(self, other, collisionBuffer);
    }
    if (dynamic_cast<const GmSurfBox *>(other.surf) != nullptr) {
        return GmCollision_Sphere_Box(self, other, collisionBuffer);
    }
    if (dynamic_cast<const GmSurfMesh *>(other.surf) != nullptr) {
        return GmCollision_Sphere_Mesh(self, other, collisionBuffer);
    }
    return GmCollision_NotHandled(self, other, collisionBuffer);
}

int GmSurfEllipsoid::Collide(
        const LocatedGmSurf &self,
        const LocatedGmSurf &other,
        CGmCollisionBuffer &collisionBuffer) const {
    if (dynamic_cast<const GmSurfSphere *>(other.surf) != nullptr) {
        const u32 firstNew = collisionBuffer.GetCurrentCount();
        return FinishReversedCollision(
                firstNew,
                GmCollision_Sphere_Ellipsoid(other, self, collisionBuffer),
                collisionBuffer);
    }
    if (dynamic_cast<const GmSurfPolygon *>(other.surf) != nullptr) {
        return GmCollision_Ellipsoid_Polygon(self, other, collisionBuffer);
    }
    if (dynamic_cast<const GmSurfMesh *>(other.surf) != nullptr) {
        return GmCollision_Ellipsoid_Mesh(self, other, collisionBuffer);
    }
    return GmCollision_NotHandled(self, other, collisionBuffer);
}

int GmSurfPolygon::Collide(
        const LocatedGmSurf &self,
        const LocatedGmSurf &other,
        CGmCollisionBuffer &collisionBuffer) const {
    const u32 firstNew = collisionBuffer.GetCurrentCount();
    if (dynamic_cast<const GmSurfSphere *>(other.surf) != nullptr) {
        return FinishReversedCollision(
                firstNew,
                GmCollision_Sphere_Polygon(other, self, collisionBuffer),
                collisionBuffer);
    }
    if (dynamic_cast<const GmSurfEllipsoid *>(other.surf) != nullptr) {
        return FinishReversedCollision(
                firstNew,
                GmCollision_Ellipsoid_Polygon(other, self, collisionBuffer),
                collisionBuffer);
    }
    return GmCollision_NotHandled(self, other, collisionBuffer);
}

int GmSurfBox::Collide(
        const LocatedGmSurf &self,
        const LocatedGmSurf &other,
        CGmCollisionBuffer &collisionBuffer) const {
    if (dynamic_cast<const GmSurfSphere *>(other.surf) != nullptr) {
        const u32 firstNew = collisionBuffer.GetCurrentCount();
        return FinishReversedCollision(
                firstNew,
                GmCollision_Sphere_Box(other, self, collisionBuffer),
                collisionBuffer);
    }
    if (dynamic_cast<const GmSurfBox *>(other.surf) != nullptr) {
        return GmCollision_Box_Box(self, other, collisionBuffer);
    }
    if (dynamic_cast<const GmSurfMesh *>(other.surf) != nullptr) {
        return GmCollision_Box_Mesh(self, other, collisionBuffer);
    }
    return GmCollision_NotHandled(self, other, collisionBuffer);
}

int GmSurfMesh::Collide(
        const LocatedGmSurf &self,
        const LocatedGmSurf &other,
        CGmCollisionBuffer &collisionBuffer) const {
    const u32 firstNew = collisionBuffer.GetCurrentCount();
    if (dynamic_cast<const GmSurfSphere *>(other.surf) != nullptr) {
        return FinishReversedCollision(
                firstNew,
                GmCollision_Sphere_Mesh(other, self, collisionBuffer),
                collisionBuffer);
    }
    if (dynamic_cast<const GmSurfEllipsoid *>(other.surf) != nullptr) {
        return FinishReversedCollision(
                firstNew,
                GmCollision_Ellipsoid_Mesh(other, self, collisionBuffer),
                collisionBuffer);
    }
    if (dynamic_cast<const GmSurfBox *>(other.surf) != nullptr) {
        return FinishReversedCollision(
                firstNew,
                GmCollision_Box_Mesh(other, self, collisionBuffer),
                collisionBuffer);
    }
    if (dynamic_cast<const GmSurfMesh *>(other.surf) != nullptr) {
        return GmCollision_Mesh_Mesh(self, other, collisionBuffer);
    }
    return GmCollision_NotHandled(self, other, collisionBuffer);
}

void GmSurfMesh::GetMeshBoundingBox(GmBoxAligned &outRef) const {
    const GmSurfMesh *mesh = this;
    GmBoxAligned *out = &outRef;
    if (mesh->TriangleCount() != 0u && mesh->OctreeCellCount() != 0u) {
        const GmMeshOctreeCell *rootCell = &mesh->OctreeCell(0u);
        out->center.x = (rootCell->Bounds().center.x);
        out->center.y = (rootCell->Bounds().center.y);
        out->center.z = (rootCell->Bounds().center.z);
        out->halfExtents.x = (rootCell->Bounds().halfExtents.x);
        out->halfExtents.y = (rootCell->Bounds().halfExtents.y);
        out->halfExtents.z = (rootCell->Bounds().halfExtents.z);
    } else if (mesh->TriangleCount() != 0u) {
        const GmSurfMeshTriangle &firstTriangle = mesh->Triangle(0u);
        GmVec3 min = mesh->Vertex(firstTriangle.vertexIndex[0]);
        GmVec3 max = min;
        for (u32 triangleIndex = 0u;
             triangleIndex < mesh->TriangleCount();
             triangleIndex++) {
            const GmSurfMeshTriangle &triangle =
                    mesh->Triangle(triangleIndex);
            for (u32 vertexSlot = 0u; vertexSlot < 3u; vertexSlot++) {
                const GmVec3 &vertex =
                        mesh->Vertex(triangle.vertexIndex[vertexSlot]);
                if (vertex.x < min.x) min.x = vertex.x;
                if (vertex.y < min.y) min.y = vertex.y;
                if (vertex.z < min.z) min.z = vertex.z;
                if (max.x < vertex.x) max.x = vertex.x;
                if (max.y < vertex.y) max.y = vertex.y;
                if (max.z < vertex.z) max.z = vertex.z;
            }
        }
        out->center.x = (max.x + min.x) * 0.5f;
        out->center.y = (max.y + min.y) * 0.5f;
        out->center.z = (max.z + min.z) * 0.5f;
        out->halfExtents.x = (max.x - min.x) * 0.5f;
        out->halfExtents.y = (max.y - min.y) * 0.5f;
        out->halfExtents.z = (max.z - min.z) * 0.5f;
    } else {
        out->center.z = 0.0f;
        out->center.y = 0.0f;
        out->center.x = 0.0f;
        out->halfExtents.x = -1.0f;
        out->halfExtents.y = -1.0f;
        out->halfExtents.z = -1.0f;
    }
}

void GmSurf::GetBoundingBox(GmBoxAligned &outRef) const {
    ComputeBoundingBox(outRef);
}
