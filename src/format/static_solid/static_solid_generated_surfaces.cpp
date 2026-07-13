#include "format/static_solid/static_solid_generated_surfaces.h"
#include <vector>
#include <new>

namespace {

GmSurfMeshTriangle GeneratedSurfaceTriangleFromIndices(
        const uint16_t *indices) {
    GmSurfMeshTriangle triangle{};
    triangle.vertexIndex[0] = indices[0];
    triangle.vertexIndex[1] = indices[1];
    triangle.vertexIndex[2] = indices[2];
    triangle.material = GmLocalMaterialIndex::FromIndex(0u);
    return triangle;
}

}  // namespace

void CGameCtnReplayStaticSolidGeneratedSurfaceBudget::Clear() {
    surfaces = 0u;
    vertices = 0u;
    triangles = 0u;
}

void CGameCtnReplayStaticSolidGeneratedSurfaceBudget::
        ReserveCandidateSurface() {
    surfaces++;
}

void CGameCtnReplayStaticSolidGeneratedSurfaceBudget::AddSurface(
        u32 vertexCount,
        u32 indexCount) {
    surfaces++;
    vertices += vertexCount;
    triangles += indexCount / 3u;
}

void CGameCtnReplayStaticSolidGeneratedSurfaceBudget::Add(
        const CGameCtnReplayStaticSolidGeneratedSurfaceBudget &budget) {
    surfaces += budget.surfaces;
    vertices += budget.vertices;
    triangles += budget.triangles;
}

int CGameCtnReplayStaticSolidGeneratedSurfaceBudget::Empty() const {
    return surfaces == 0u;
}

u32 CGameCtnReplayStaticSolidGeneratedSurfaceBudget::SurfaceCount() const {
    return surfaces;
}

u32 CGameCtnReplayStaticSolidGeneratedSurfaceBudget::VertexCount() const {
    return vertices;
}

u32 CGameCtnReplayStaticSolidGeneratedSurfaceBudget::TriangleCount() const {
    return triangles;
}

u32 CGameCtnReplayStaticSolidGeneratedSurfaceCursor::SurfaceIndex() const {
    return surfaceIndex;
}

u32 CGameCtnReplayStaticSolidGeneratedSurfaceCursor::VertexOffset() const {
    return vertexOffset;
}

u32 CGameCtnReplayStaticSolidGeneratedSurfaceCursor::TriangleOffset() const {
    return triangleOffset;
}

void CGameCtnReplayStaticSolidGeneratedSurfaceCursor::Advance(
        u32 vertexCount,
        u32 triangleCount) {
    surfaceIndex++;
    vertexOffset += vertexCount;
    triangleOffset += triangleCount;
}

CGameCtnReplayStaticSolidGeneratedSurfaceVisual::
        CGameCtnReplayStaticSolidGeneratedSurfaceVisual(
                CPlugVisual *newProvider)
        : visual(dynamic_cast<CPlugVisualIndexedTriangles *>(newProvider)) {}

int CGameCtnReplayStaticSolidGeneratedSurfaceVisual::CanBuildMesh() const {
    return visual != nullptr;
}

int CGameCtnReplayStaticSolidGeneratedSurfaceVisual::VertexCount()
        const {
    return visual != nullptr
            ? static_cast<int>(visual->GetTotalVertexCount())
            : 0;
}

u32 CGameCtnReplayStaticSolidGeneratedSurfaceVisual::IndexCount()
        const {
    return visual != nullptr ? visual->GetVertexIndexCount() : 0u;
}

void CGameCtnReplayStaticSolidGeneratedSurfaceVisual::WriteGxVertices(
        GxVertex *destination) const {
    if (visual != nullptr) {
        visual->GetTotalVertexs(destination);
    }
}

void CGameCtnReplayStaticSolidGeneratedSurfaceVisual::WriteIndices(
        uint16_t *destination) const {
    if (visual != nullptr) {
        visual->CopyIndicesTo(destination);
    }
}

void StaticSolidGeneratedSurfaces::Free() {
    surfaces.Clear();
    surfaceGeoms.Clear();
    materials.Clear();
    meshVertices.clear();
    meshTriangles.clear();
    surfaceCapacity = 0u;
    createdSurfaceCount = 0u;
}

int StaticSolidGeneratedSurfaces::Allocate(
        const CGameCtnReplayStaticSolidGeneratedSurfaceBudget &budget) {
    surfaceCapacity = budget.SurfaceCount();
    if (budget.Empty()) {
        return 1;
    }

    if (!surfaces.Allocate(budget.SurfaceCount()) ||
        !surfaceGeoms.Allocate(budget.SurfaceCount()) ||
        !materials.Allocate(budget.SurfaceCount())) {
        return 0;
    }
    try {
        meshVertices.assign(budget.VertexCount(), GmVec3{});
        meshTriangles.assign(budget.TriangleCount(), GmSurfMeshTriangle{});
    } catch (const std::bad_alloc &) {
        meshVertices.clear();
        meshTriangles.clear();
        return 0;
    }
    return 1;
}

CPlugSurface *
StaticSolidGeneratedSurfaces::SurfaceAt(u32 index) {
    return index < surfaceCapacity ? surfaces.At(index) : nullptr;
}

int StaticSolidGeneratedSurfaces::
        AddVisualSurfaceBudget(
        CPlugVisual *provider,
        CGameCtnReplayStaticSolidGeneratedSurfaceBudget *budget) const {
    const CGameCtnReplayStaticSolidGeneratedSurfaceVisual visual(provider);
    if (!visual.CanBuildMesh()) {
        return 0;
    }
    const int vertexCount = visual.VertexCount();
    const u32 indexCount = visual.IndexCount();
    if (indexCount < 3u || vertexCount <= 0) {
        return 0;
    }
    if (budget != nullptr) {
        budget->AddSurface((u32)vertexCount, indexCount);
    }
    return 1;
}

int StaticSolidGeneratedSurfaces::
        AttachSingleMaterialToSurface(u32 surfaceIndex,
                                      CPlugSurface *surface,
                                      CPlugMaterial *material) {
    if (surface == nullptr || material == nullptr ||
        surfaceIndex >= surfaceCapacity) {
        return 0;
    }
    surface->AttachSingleMaterial(*material);
    return 1;
}

int StaticSolidGeneratedSurfaces::
        CreateSurfaceFromVisual(
                CPlugVisual *visual,
                CGameCtnReplayStaticSolidGeneratedSurfaceCursor
                        *cursor) {
    if (cursor == nullptr ||
        cursor->SurfaceIndex() >= surfaceCapacity ||
        !surfaces.HasObjects() ||
        !surfaceGeoms.HasObjects() ||
        !materials.HasObjects()) {
        return 0;
    }
    const CGameCtnReplayStaticSolidGeneratedSurfaceVisual source(visual);
    if (!source.CanBuildMesh()) {
        return 0;
    }
    const u32 indexCount = source.IndexCount();
    const u32 triangleCount = indexCount / 3u;
    if (triangleCount == 0u) {
        return 0;
    }
    const int vertexCountSigned = source.VertexCount();
    if (vertexCountSigned <= 0) {
        return 0;
    }
    const u32 vertexCount = (u32)vertexCountSigned;

    const u32 surfaceIndex = cursor->SurfaceIndex();
    CPlugSurface *surface = surfaces.At(surfaceIndex);
    CPlugSurfaceGeom *geom = surfaceGeoms.At(surfaceIndex);
    std::unique_ptr<GmSurfMesh> ownedMesh;
    try {
        ownedMesh = std::make_unique<GmSurfMesh>();
    } catch (const std::bad_alloc &) {
        return 0;
    }

    std::vector<GxVertex> gxVertices;
    std::vector<uint16_t> indices;
    try {
        gxVertices.assign(vertexCount, GxVertex{});
        indices.assign(indexCount, 0u);
    } catch (const std::bad_alloc &) {
        return 0;
    }

    source.WriteGxVertices(gxVertices.data());
    source.WriteIndices(indices.data());

    if (cursor->VertexOffset() + vertexCount > meshVertices.size() ||
        cursor->TriangleOffset() + triangleCount > meshTriangles.size()) {
        return 0;
    }
    GmVec3 *vertices = meshVertices.data() + cursor->VertexOffset();
    GmSurfMeshTriangle *triangles =
            meshTriangles.data() + cursor->TriangleOffset();
    for (u32 i = 0; i < vertexCount; i++) {
        vertices[i] = gxVertices[i].position;
    }

    GmSurfMesh *mesh = ownedMesh.get();
    mesh->material = GmLocalMaterialIndex::FromIndex(0u);
    for (u32 i = 0; i < triangleCount; i++) {
        triangles[i] = GeneratedSurfaceTriangleFromIndices(
                indices.data() + (size_t)i * 3u);
    }
    std::vector<GmVec3> ownedVertices;
    if (vertexCount != 0u) {
        ownedVertices.assign(vertices, vertices + vertexCount);
    }
    std::vector<GmSurfMeshTriangle> ownedTriangles;
    if (triangleCount != 0u) {
        ownedTriangles.assign(triangles, triangles + triangleCount);
    }
    if (!mesh->SetGeometry(std::move(ownedVertices),
                           std::move(ownedTriangles))) {
        return 0;
    }

    CPlugMaterial *material = materials.At(surfaceIndex);
    if (material != nullptr) {
        material->SetSurfaceMaterialId(
                EPlugSurfaceMaterialId_Concrete);
    }
    if (!AttachSingleMaterialToSurface(surfaceIndex, surface, material)) {
        return 0;
    }
    geom->SetGmSurf(std::move(ownedMesh));
    surface->SetGeometry(geom);

    cursor->Advance(vertexCount, triangleCount);
    createdSurfaceCount++;
    return 1;
}
