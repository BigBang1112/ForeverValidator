#pragma once

#include "engine/physics/collision/gm_collision_buffer.h"
#include "engine/physics/geometry/gm_surface.h"
struct SMeshMeshCollide {
    GmIso4 bToA{};
    GmMat3 absBToARotation{};
    bool hit = false;
    const GmSurfMesh *meshA = nullptr;
    const GmSurfMesh *meshB = nullptr;
    CGmCollisionBuffer *collisionBuffer = nullptr;
    const GmIso4 *isoA = nullptr;

    void EmitCollision(const GmSurfMeshTriangle &triangleA,
                       const GmSurfMeshTriangle &triangleB);
    int TriTri(const GmMeshOctreeCell &cellA,
               const GmMeshOctreeCell &cellB);
    void TreeTree(unsigned long cellIndexA, unsigned long cellIndexB);
};
