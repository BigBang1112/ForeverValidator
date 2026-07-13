#pragma once

#include "engine/rendering/plug_visual.h"
#include "engine/rendering/plug_material.h"
#include "engine/physics/geometry/plug_surface.h"
#include "engine/core/gm_types.h"
#include "engine/core/engine_types.h"
#include "format/static_solid/static_solid_geometry_decoder.h"
class CGameCtnReplayStaticSolidGeneratedSurfaceBudget {
public:
    void Clear();
    void ReserveCandidateSurface();
    void AddSurface(u32 vertexCount, u32 indexCount);
    void Add(const CGameCtnReplayStaticSolidGeneratedSurfaceBudget &budget);
    int Empty() const;
    u32 SurfaceCount() const;
    u32 VertexCount() const;
    u32 TriangleCount() const;

private:
    u32 surfaces = 0u;
    u32 vertices = 0u;
    u32 triangles = 0u;
};

class CGameCtnReplayStaticSolidGeneratedSurfaceCursor {
public:
    u32 SurfaceIndex() const;
    u32 VertexOffset() const;
    u32 TriangleOffset() const;
    void Advance(u32 vertexCount, u32 triangleCount);

private:
    u32 surfaceIndex = 0u;
    u32 vertexOffset = 0u;
    u32 triangleOffset = 0u;
};

class CGameCtnReplayStaticSolidGeneratedSurfaceVisual {
public:
    explicit CGameCtnReplayStaticSolidGeneratedSurfaceVisual(
            CPlugVisual *provider);

    int CanBuildMesh() const;
    int VertexCount() const;
    u32 IndexCount() const;
    void WriteGxVertices(GxVertex *destination) const;
    void WriteIndices(uint16_t *destination) const;

private:
    CPlugVisualIndexedTriangles *visual = nullptr;
};

class StaticSolidGeneratedSurfaces {
public:
    void Free();
    int Allocate(
            const CGameCtnReplayStaticSolidGeneratedSurfaceBudget &budget);
    CPlugSurface *SurfaceAt(u32 index);
    int AddVisualSurfaceBudget(
            CPlugVisual *provider,
            CGameCtnReplayStaticSolidGeneratedSurfaceBudget *budget)
            const;
    int CreateSurfaceFromVisual(
            CPlugVisual *visual,
            CGameCtnReplayStaticSolidGeneratedSurfaceCursor *cursor);

private:
    int AttachSingleMaterialToSurface(u32 surfaceIndex,
                                      CPlugSurface *surface,
                                      CPlugMaterial *material);

    StaticSolidNodes<CPlugSurface> surfaces;
    StaticSolidNodes<CPlugSurfaceGeom> surfaceGeoms;
    StaticSolidNodes<CPlugMaterial> materials;
    std::vector<GmVec3> meshVertices;
    std::vector<GmSurfMeshTriangle> meshTriangles;
    u32 surfaceCapacity = 0u;
    u32 createdSurfaceCount = 0u;
};
