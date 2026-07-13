#include "engine/physics/geometry/gm_surface.h"
#include <cmath>
#include <cstddef>
#include <new>
#include <utility>

#include "engine/physics/geometry/geometry_helpers.h"
#include "engine/physics/geometry/physics_tolerances.h"

GmSurf::GmSurf(void) = default;

GmSurf::~GmSurf(void) = default;

int GmSurf::UsesSphereContactBuffer(void) const {
    return 0;
}

int GmSurf::GetRollingRadius(float &outRadius) const {
    (void)outRadius;
    return 0;
}

void GmSurfMesh::ComputePlane(unsigned long triangleIndex) {
    GmSurfMeshTriangle &triangle = MutableTriangle(triangleIndex);
    const GmVec3 &vertex0 = Vertex(triangle.vertexIndex[0]);
    const GmVec3 edge10 = GmMath::Subtract(
            Vertex(triangle.vertexIndex[1]), vertex0);
    const GmVec3 edge20 = GmMath::Subtract(
            Vertex(triangle.vertexIndex[2]), vertex0);
    const GmVec3 normalSeed = GmMath::Cross(edge10, edge20);
    triangle.normal = GmMath::NormalizeOr(
            normalSeed,
            normalSeed,
            PhysicsTolerance::SurfaceDirectionLengthSquared);
    triangle.planeDistance =
            (-triangle.normal.x * vertex0.x -
             triangle.normal.y * vertex0.y) -
            triangle.normal.z * vertex0.z;
}

void GmSurfSphere::GetSphereBoundingBox(GmBoxAligned &out) const {
    out.center = {};
    out.halfExtents = {radius, radius, radius};
}

void GmSurfEllipsoid::GetEllipsoidBoundingBox(GmBoxAligned &out) const {
    out.center = {};
    out.halfExtents = radii;
}


static GmMeshOctreeCell BuildTriangleCell(
        const GmSurfMesh *mesh,
        const GmSurfMeshTriangle *triangle,
        uint32_t triangleIndex) {
    const GmVec3 *vertex0 = &mesh->Vertex(triangle->vertexIndex[0]);
    GmVec3 min = *vertex0;
    GmVec3 max = *vertex0;

    for (uint32_t vertexSlot = 1u; vertexSlot < 3u; vertexSlot++) {
        const GmVec3 *vertex = &mesh->Vertex(triangle->vertexIndex[vertexSlot]);
        if (vertex->x < min.x) {
            min.x = vertex->x;
        }
        if (vertex->y < min.y) {
            min.y = vertex->y;
        }
        if (vertex->z < min.z) {
            min.z = vertex->z;
        }
        if (max.x < vertex->x) {
            max.x = vertex->x;
        }
        if (max.y < vertex->y) {
            max.y = vertex->y;
        }
        if (max.z < vertex->z) {
            max.z = vertex->z;
        }
    }

    const GmBoxAligned bounds = {
        {(max.x + min.x) * 0.5f,
         (max.y + min.y) * 0.5f,
         (max.z + min.z) * 0.5f},
        {(max.x - min.x) * 0.5f,
         (max.y - min.y) * 0.5f,
         (max.z - min.z) * 0.5f},
    };
    return GmMeshOctreeCell::Triangle(bounds, triangleIndex);
}

namespace {

bool HasValidBounds(const GmBoxAligned &bounds) {
    return std::isfinite(bounds.center.x) &&
           std::isfinite(bounds.center.y) &&
           std::isfinite(bounds.center.z) &&
           std::isfinite(bounds.halfExtents.x) &&
           std::isfinite(bounds.halfExtents.y) &&
           std::isfinite(bounds.halfExtents.z) &&
           bounds.halfExtents.x >= 0.0f &&
           bounds.halfExtents.y >= 0.0f &&
           bounds.halfExtents.z >= 0.0f;
}

bool BoundsContainsPoint(
        const GmBoxAligned &bounds,
        const GmVec3 &point) {
    return point.x >= bounds.center.x - bounds.halfExtents.x &&
           point.x <= bounds.center.x + bounds.halfExtents.x &&
           point.y >= bounds.center.y - bounds.halfExtents.y &&
           point.y <= bounds.center.y + bounds.halfExtents.y &&
           point.z >= bounds.center.z - bounds.halfExtents.z &&
           point.z <= bounds.center.z + bounds.halfExtents.z;
}

bool BoundsContainsBounds(
        const GmBoxAligned &outer,
        const GmBoxAligned &inner) {
    return inner.center.x - inner.halfExtents.x >=
                   outer.center.x - outer.halfExtents.x &&
           inner.center.x + inner.halfExtents.x <=
                   outer.center.x + outer.halfExtents.x &&
           inner.center.y - inner.halfExtents.y >=
                   outer.center.y - outer.halfExtents.y &&
           inner.center.y + inner.halfExtents.y <=
                   outer.center.y + outer.halfExtents.y &&
           inner.center.z - inner.halfExtents.z >=
                   outer.center.z - outer.halfExtents.z &&
           inner.center.z + inner.halfExtents.z <=
                   outer.center.z + outer.halfExtents.z;
}

bool ValidateOctreeNode(const std::vector<GmMeshOctreeCell> &cells,
                        const std::vector<GmVec3> &vertices,
                        const std::vector<GmSurfMeshTriangle> &triangles,
                        u32 nodeIndex,
                        u32 containingEnd,
                        std::vector<bool> &referencedTriangles) {
    if (nodeIndex >= containingEnd || nodeIndex >= cells.size()) {
        return false;
    }

    const GmMeshOctreeCell &cell = cells[nodeIndex];
    const u32 subtreeEntryCount = cell.SubtreeEntryCount();
    if (subtreeEntryCount == 0u ||
        subtreeEntryCount > containingEnd - nodeIndex ||
        subtreeEntryCount > cells.size() - nodeIndex ||
        !HasValidBounds(cell.Bounds())) {
        return false;
    }

    if (cell.ContainsTriangle()) {
        const u32 triangleIndex = cell.TriangleIndex();
        if (subtreeEntryCount != 1u ||
            triangleIndex >= triangles.size() ||
            referencedTriangles[triangleIndex]) {
            return false;
        }
        const GmSurfMeshTriangle &triangle = triangles[triangleIndex];
        for (const u32 vertexIndex : triangle.vertexIndex) {
            if (!BoundsContainsPoint(
                        cell.Bounds(), vertices[vertexIndex])) {
                return false;
            }
        }
        referencedTriangles[triangleIndex] = true;
        return true;
    }

    if (subtreeEntryCount == 1u) {
        return false;
    }

    const u32 subtreeEnd = nodeIndex + subtreeEntryCount;
    u32 childIndex = nodeIndex + 1u;
    while (childIndex < subtreeEnd) {
        if (!BoundsContainsBounds(
                    cell.Bounds(), cells[childIndex].Bounds())) {
            return false;
        }
        if (!ValidateOctreeNode(cells,
                                vertices,
                                triangles,
                                childIndex,
                                subtreeEnd,
                                referencedTriangles)) {
            return false;
        }
        childIndex += cells[childIndex].SubtreeEntryCount();
    }
    return childIndex == subtreeEnd;
}

bool ValidateArchivedOctree(
        const std::vector<GmMeshOctreeCell> &cells,
        const std::vector<GmVec3> &vertices,
        const std::vector<GmSurfMeshTriangle> &triangles) {
    if (triangles.empty()) {
        return cells.empty();
    }
    if (cells.empty() || cells.front().ContainsTriangle() ||
        cells.front().SubtreeEntryCount() != cells.size()) {
        return false;
    }

    std::vector<bool> referencedTriangles(triangles.size(), false);
    if (!ValidateOctreeNode(cells,
                            vertices,
                            triangles,
                            0u,
                            static_cast<u32>(cells.size()),
                            referencedTriangles)) {
        return false;
    }
    for (const bool referenced : referencedTriangles) {
        if (!referenced) {
            return false;
        }
    }
    return true;
}

bool ValidateTriangleIndices(
        const std::vector<GmVec3> &vertices,
        const std::vector<GmSurfMeshTriangle> &triangles) {
    for (const GmSurfMeshTriangle &triangle : triangles) {
        for (const u32 vertexIndex : triangle.vertexIndex) {
            if (vertexIndex >= vertices.size()) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

void GmSurfMesh::BuildOctree(void) {
    GmSurfMesh *mesh = this;
    const uint32_t triangleCount = mesh->TriangleCount();
    if (triangleCount == 0u) {
        mesh->SetOctree({});
        return;
    }
    std::vector<GmMeshOctreeCell> sourceCells(triangleCount);

    for (uint32_t triangleIndex = 0u;
         triangleIndex < triangleCount;
         triangleIndex++) {
        const GmSurfMeshTriangle *triangle = &mesh->Triangle(triangleIndex);
        sourceCells[triangleIndex] =
                BuildTriangleCell(mesh, triangle, triangleIndex);
    }

    GmMeshOctree builtOctree;
    builtOctree.Build(
            triangleCount,
            sourceCells.data(),
            1u,
            0u,
            0u,
            0.0f);
    mesh->SetOctree(builtOctree.TakeCells());
}
bool GmSurfMesh::SetGeometry(
        std::vector<GmVec3> meshVertices,
        std::vector<GmSurfMeshTriangle> meshTriangles,
        std::vector<GmMeshOctreeCell> meshOctreeCells) {
    if (!ValidateTriangleIndices(meshVertices, meshTriangles)) {
        return false;
    }

    try {
        GmSurfMesh candidate;
        candidate.vertices = std::move(meshVertices);
        candidate.triangles = std::move(meshTriangles);
        candidate.RecomputeAllPlanes();

        if (candidate.triangles.empty()) {
            candidate.octreeCells.clear();
        } else if (ValidateArchivedOctree(
                           meshOctreeCells,
                           candidate.vertices,
                           candidate.triangles)) {
            candidate.octreeCells = std::move(meshOctreeCells);
        } else {
            candidate.BuildOctree();
        }

        vertices.swap(candidate.vertices);
        triangles.swap(candidate.triangles);
        octreeCells.swap(candidate.octreeCells);
    } catch (const std::bad_alloc &) {
        return false;
    }
    return true;
}

void GmSurfMesh::ClearGeometry(void) {
    vertices.clear();
    triangles.clear();
    octreeCells.clear();
}

void GmSurfMesh::RefreshDerivedGeometry(void) {
    RecomputeAllPlanes();
    BuildOctree();
}

void GmSurfMesh::TranslateVerticesAbove(float yThreshold, float yDelta) {
    for (GmVec3 &vertex : vertices) {
        if (vertex.y > yThreshold) {
            vertex.y += yDelta;
        }
    }
    RefreshDerivedGeometry();
}

void GmSurfMesh::TransformByNOMat(const GmIso4 &transform) {
    for (GmVec3 &vertex : vertices) {
        vertex.Mult(transform);
    }
    RefreshDerivedGeometry();
}
