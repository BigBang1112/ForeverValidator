#include "format/static_solid/static_solid_surface_assembler.h"
#include "engine/physics/geometry/plug_surface_material_library.h"
#include <stdint.h>
#include <utility>
#include <vector>

#include "format/archive/archive_binary32.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_node_kind.h"
#include <new>
namespace {

std::unique_ptr<GmSurf> CreateSurfaceGeometry(u32 surfaceType,
                                               uint16_t materialId) {
    std::unique_ptr<GmSurf> surface;
    switch (surfaceType) {
        case static_cast<u32>(GmSurf::EGmSurfType::Sphere):
            surface = std::make_unique<GmSurfSphere>();
            break;
        case static_cast<u32>(GmSurf::EGmSurfType::Ellipsoid):
            surface = std::make_unique<GmSurfEllipsoid>();
            break;
        case static_cast<u32>(GmSurf::EGmSurfType::Box):
            surface = std::make_unique<GmSurfBox>();
            break;
        case static_cast<u32>(GmSurf::EGmSurfType::Mesh):
            surface = std::make_unique<GmSurfMesh>();
            break;
        default:
            return nullptr;
    }
    surface->material = GmLocalMaterialIndex::FromIndex(materialId);
    return surface;
}

u32 StaticSolidArchiveReadU32(const uint8_t *bytes) {
    return static_cast<u32>(bytes[0]) |
           (static_cast<u32>(bytes[1]) << 8u) |
           (static_cast<u32>(bytes[2]) << 16u) |
           (static_cast<u32>(bytes[3]) << 24u);
}

uint16_t StaticSolidArchiveReadU16(const uint8_t *bytes) {
    return static_cast<uint16_t>(
            static_cast<uint16_t>(bytes[0]) |
            (static_cast<uint16_t>(bytes[1]) << 8u));
}

float StaticSolidArchiveReadFloat(const uint8_t *bytes) {
    return TmnfArchiveBinary32::Decode(StaticSolidArchiveReadU32(bytes));
}

GmVec3 StaticSolidArchiveReadVec3(const uint8_t *bytes) {
    return {
        StaticSolidArchiveReadFloat(bytes),
        StaticSolidArchiveReadFloat(bytes + 4u),
        StaticSolidArchiveReadFloat(bytes + 8u),
    };
}

int StaticSolidArchiveRecordSpanMatches(
        const StaticSolidArchiveDecodedBytes &span,
        u32 recordCount,
        u32 recordBytes) {
    return span.IsAvailable() &&
           recordBytes != 0u &&
           recordCount <= UINT32_MAX / recordBytes &&
           span.ByteCount() == recordCount * recordBytes;
}

struct DecodedStaticSolidArchiveMesh {
    std::vector<GmVec3> vertices;
    std::vector<GmSurfMeshTriangle> triangles;
    std::vector<GmMeshOctreeCell> octreeCells;
};

int DecodeStaticSolidArchiveMesh(
        const StaticSolidDecodedPayloads &decodedPayloads,
        StaticSolidArchiveId payload,
        const CGameCtnReplayStaticSolidArchiveMeshPayload &meshPayload,
        DecodedStaticSolidArchiveMesh *meshOut) {
    if (meshOut == nullptr) {
        return 0;
    }
    const StaticSolidArchiveDecodedBytes vertexRecords =
            decodedPayloads.Slice(payload, meshPayload.Vertices());
    const StaticSolidArchiveDecodedBytes triangleRecords =
            decodedPayloads.Slice(payload, meshPayload.Triangles());
    const StaticSolidArchiveDecodedBytes octreeCellRecords =
            decodedPayloads.Slice(payload, meshPayload.OctreeCells());
    if (!StaticSolidArchiveRecordSpanMatches(
                vertexRecords,
                meshPayload.VertexCount(),
                CGameCtnReplayStaticSolidArchiveMeshPayload::
                        VertexRecordBytes) ||
        !StaticSolidArchiveRecordSpanMatches(
                triangleRecords,
                meshPayload.TriangleCount(),
                CGameCtnReplayStaticSolidArchiveMeshPayload::
                        TriangleRecordBytes) ||
        !StaticSolidArchiveRecordSpanMatches(
                octreeCellRecords,
                meshPayload.OctreeCellCount(),
                CGameCtnReplayStaticSolidArchiveMeshPayload::
                        OctreeCellRecordBytes)) {
        return 0;
    }

    try {
        meshOut->vertices.resize(meshPayload.VertexCount());
        meshOut->triangles.resize(meshPayload.TriangleCount());
        meshOut->octreeCells.resize(meshPayload.OctreeCellCount());
    } catch (const std::bad_alloc &) {
        *meshOut = DecodedStaticSolidArchiveMesh{};
        return 0;
    }

    for (u32 index = 0u; index < meshPayload.VertexCount(); index++) {
        const uint8_t *record =
                vertexRecords.Bytes() + index *
                        CGameCtnReplayStaticSolidArchiveMeshPayload::
                                VertexRecordBytes;
        meshOut->vertices[index] = StaticSolidArchiveReadVec3(record);
    }
    for (u32 index = 0u; index < meshPayload.TriangleCount(); index++) {
        const uint8_t *record =
                triangleRecords.Bytes() + index *
                        CGameCtnReplayStaticSolidArchiveMeshPayload::
                                TriangleRecordBytes;
        GmSurfMeshTriangle &triangle = meshOut->triangles[index];
        triangle.normal = StaticSolidArchiveReadVec3(record);
        triangle.planeDistance = StaticSolidArchiveReadFloat(record + 12u);
        triangle.vertexIndex[0] = StaticSolidArchiveReadU32(record + 16u);
        triangle.vertexIndex[1] = StaticSolidArchiveReadU32(record + 20u);
        triangle.vertexIndex[2] = StaticSolidArchiveReadU32(record + 24u);
        triangle.material = GmLocalMaterialIndex::FromIndex(
                StaticSolidArchiveReadU16(record + 28u));
    }
    for (u32 index = 0u; index < meshPayload.OctreeCellCount(); index++) {
        const uint8_t *record =
                octreeCellRecords.Bytes() + index *
                        CGameCtnReplayStaticSolidArchiveMeshPayload::
                                OctreeCellRecordBytes;
        const u32 subtreeEntryCount = StaticSolidArchiveReadU32(record);
        const GmBoxAligned bounds = {
            StaticSolidArchiveReadVec3(record + 4u),
            StaticSolidArchiveReadVec3(record + 16u),
        };
        const int32_t triangleIndex =
                static_cast<int32_t>(StaticSolidArchiveReadU32(record + 28u));
        GmMeshOctreeCell cell = triangleIndex < 0
                ? GmMeshOctreeCell::Branch(bounds)
                : GmMeshOctreeCell::Triangle(
                        bounds, static_cast<u32>(triangleIndex));
        cell.SetSubtreeEntryCount(subtreeEntryCount);
        meshOut->octreeCells[index] = std::move(cell);
    }
    return 1;
}

}  // namespace

void StaticSolidArchiveVisualCollection::Clear() {
    providers.clear();
}

int StaticSolidArchiveVisualCollection::Allocate(
        u32 count) {
    try {
        providers.clear();
        providers.reserve(count);
        for (u32 index = 0u; index < count; ++index) {
            providers.push_back(MakeMwNod<StaticSolidArchiveVisual>());
        }
    } catch (const std::bad_alloc &) {
        Clear();
        return 0;
    }
    return 1;
}

StaticSolidArchiveVisual *
StaticSolidArchiveVisualCollection::At(u32 index) {
    return index < providers.size() ? providers[index].Get() : nullptr;
}

u32 StaticSolidArchiveVisualCollection::Count() const {
    return providers.size() <= UINT32_MAX
            ? (u32)providers.size()
            : UINT32_MAX;
}

void StaticSolidSurfaceAssembler::Free() {
    if (surfaces.HasObjects()) {
        for (u32 i = 0; i < surfaceCount; i++) {
            CPlugSurface *surface = surfaces.At(i);
            StaticSolidResourceIndex::
                    DetachSurfaceMaterials(surface);
        }
    }
    surfaces.Clear();
    surfaceGeoms.Clear();
    materials.Clear();
    visualProviders.Clear();
    resources.Clear();
    materialRemaps.Clear();
    surfaceCount = 0u;
    surfaceGeomCount = 0u;
    materialCount = 0u;
}

void StaticSolidSurfaceAssembler::
        EnsureDefaultSurfaceMaterials() {
    (void)PlugSurfaceMaterials().Default(0u);
}

CPlugMaterial *
StaticSolidSurfaceAssembler::DefaultSurfaceMaterial(
        u32 materialId) {
    return PlugSurfaceMaterials().Default(materialId);
}

int StaticSolidSurfaceAssembler::
        MaterializeArchiveGraphWithDefaultMaterials(
                const CGameCtnReplayStaticSolidArchiveNodeDirectory &nodes,
                const CGameCtnReplayStaticSolidArchiveSurfaceGraph
                        &surfaceGraph,
                const StaticSolidDecodedPayloads
                        &decodedPayloads) {
    EnsureDefaultSurfaceMaterials();
    return MaterializeArchiveGraph(nodes, surfaceGraph, decodedPayloads);
}

int StaticSolidSurfaceAssembler::MaterializeArchiveGraph(
        const CGameCtnReplayStaticSolidArchiveNodeDirectory &nodes,
        const CGameCtnReplayStaticSolidArchiveSurfaceGraph &surfaceGraph,
        const StaticSolidDecodedPayloads &decodedPayloads) {
    if (!AllocateForGraph(nodes, surfaceGraph)) {
        return 0;
    }
    ConstructSurfacesAndMaterials(nodes);
    ApplyMaterialDefinitions(surfaceGraph);
    if (!InstallMaterialRemaps(surfaceGraph)) {
        return 0;
    }
    if (!MaterializeSurfaceGeometryDefinitions(surfaceGraph,
                                               decodedPayloads)) {
        return 0;
    }
    MaterializeVisualProviderDefinitions(surfaceGraph, decodedPayloads);
    if (!LinkSurfacesToGeoms(surfaceGraph)) {
        return 0;
    }
    ApplySurfaceMaterialEntries(surfaceGraph);
    return 1;
}

int StaticSolidSurfaceAssembler::AllocateForGraph(
        const CGameCtnReplayStaticSolidArchiveNodeDirectory &nodes,
        const CGameCtnReplayStaticSolidArchiveSurfaceGraph &surfaceGraph) {
    if (nodes.Empty()) {
        return 1;
    }
    nodes.ForEachSurface([&](u32, const auto &) {
        surfaceCount++;
        return 1;
    });
    nodes.ForEachMaterial([&](u32, const auto &) {
        materialCount++;
        return 1;
    });
    surfaceGraph.ForEachSurfaceGeometryDefinition([&](const auto &) {
        surfaceGeomCount++;
        return 1;
    });
    u32 visualProviderDefinitionCount = 0u;
    surfaceGraph.ForEachVisualProviderDefinition([&](const auto &) {
        visualProviderDefinitionCount++;
        return 1;
    });
    if (surfaceCount != 0u && !surfaces.Allocate(surfaceCount)) {
        return 0;
    }
    if (surfaceCount != 0u) {
        if (!resources.AllocateSurfaceMaterialSlots(surfaceCount)) {
            return 0;
        }
    }
    if (surfaceGeomCount != 0u) {
        if (!surfaceGeoms.Allocate(surfaceGeomCount)) {
            return 0;
        }
    }
    if (visualProviderDefinitionCount != 0u) {
        if (!visualProviders.Allocate(visualProviderDefinitionCount)) {
            return 0;
        }
    }
    if (materialCount != 0u && !materials.Allocate(materialCount)) {
        return 0;
    }
    return 1;
}

void StaticSolidSurfaceAssembler::ConstructSurfacesAndMaterials(
        const CGameCtnReplayStaticSolidArchiveNodeDirectory &nodes) {
    u32 surfaceIndex = 0u;
    u32 materialIndex = 0u;
    nodes.ForEachSurface([&](u32, const auto &node) {
        resources.RegisterSurface(node.Identity(), surfaceIndex);
        surfaceIndex++;
        return 1;
    });
    nodes.ForEachMaterial([&](u32, const auto &node) {
        CPlugMaterial *material = materials.At(materialIndex);
        if (material != nullptr) {
            material->SetSurfaceMaterialId(
                    EPlugSurfaceMaterialId_Concrete);
        }
        resources.RegisterMaterial(node.Identity(), materialIndex);
        materialIndex++;
        return 1;
    });
}

void StaticSolidSurfaceAssembler::ApplyMaterialDefinitions(
        const CGameCtnReplayStaticSolidArchiveSurfaceGraph &surfaceGraph) {
    surfaceGraph.ForEachMaterialDefinition([&](
            const CGameCtnReplayStaticSolidArchiveMaterialDefinition &definition) {
        const u32 materialIndex = MaterialIndexForNode(definition.Material());
        if (materialIndex == UINT32_MAX || materialIndex >= materialCount) {
            return 1;
        }
        CPlugMaterial *material = materials.At(materialIndex);
        definition.ApplyToMaterial(material);
        return 1;
    });
}

int StaticSolidSurfaceAssembler::InstallMaterialRemaps(
        const CGameCtnReplayStaticSolidArchiveSurfaceGraph &surfaceGraph) {
    EnsureDefaultSurfaceMaterials();
    materialRemaps.Clear();
    int ok = 1;
    surfaceGraph.ForEachMaterialDefinition([&](
            const CGameCtnReplayStaticSolidArchiveMaterialDefinition &definition) {
        const auto &terrainModifier =
                definition.Remaps().terrainModifierDirt;
        if (!terrainModifier) {
            return 1;
        }
        const u32 materialIndex = MaterialIndexForNode(definition.Material());
        if (materialIndex == UINT32_MAX || materialIndex >= materialCount) {
            return 1;
        }
        const u32 replacementMaterialId = static_cast<u32>(
                terrainModifier->surface.MaterialId());
        CPlugMaterial *replacement =
                PlugSurfaceMaterials().Default(replacementMaterialId);
        CPlugMaterial *decorationSkin = PlugSurfaceMaterials().Default(0u);
        if (!materialRemaps.Add(definition.Material().Payload(),
                                MaterialAt(materialIndex),
                                replacement,
                                decorationSkin)) {
            ok = 0;
            return 0;
        }
        return 1;
    });
    return ok;
}

int StaticSolidSurfaceAssembler::MaterializeSurfaceGeometryDefinition(
        u32 surfaceGeomIndex,
        const CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition &definition,
        const StaticSolidDecodedPayloads &decodedPayloads) {
    if (surfaceGeomIndex >= surfaceGeomCount) {
        return 1;
    }

    CPlugSurfaceGeom *geom = surfaceGeoms.At(surfaceGeomIndex);
    if (geom == nullptr) {
        return 1;
    }

    std::unique_ptr<GmSurf> surface;
    try {
        surface = CreateSurfaceGeometry(definition.SurfType(),
                                        definition.MaterialId());
    } catch (const std::bad_alloc &) {
        return 0;
    }
    if (surface == nullptr) {
        return 0;
    }

    if (definition.IsMesh()) {
        const CGameCtnReplayStaticSolidArchiveMeshPayload meshPayload =
                definition.MeshPayload();
        DecodedStaticSolidArchiveMesh decodedMesh;
        if (!DecodeStaticSolidArchiveMesh(
                    decodedPayloads,
                    definition.Geom().Payload(),
                    meshPayload,
                    &decodedMesh)) {
            return 0;
        }
        GmSurfMesh *mesh = dynamic_cast<GmSurfMesh *>(surface.get());
        if (mesh == nullptr) {
            return 0;
        }
        if (!mesh->SetGeometry(std::move(decodedMesh.vertices),
                               std::move(decodedMesh.triangles),
                               std::move(decodedMesh.octreeCells))) {
            return 0;
        }
    }

    geom->SetGmSurf(std::move(surface));
    resources.RegisterSurfaceGeom(definition.Geom(), surfaceGeomIndex);
    return 1;
}

int StaticSolidSurfaceAssembler::MaterializeSurfaceGeometryDefinitions(
        const CGameCtnReplayStaticSolidArchiveSurfaceGraph &surfaceGraph,
        const StaticSolidDecodedPayloads &decodedPayloads) {
    u32 surfaceGeomIndex = 0u;
    int ok = 1;
    surfaceGraph.ForEachSurfaceGeometryDefinition([&](
            const CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition &definition) {
        if (!definition.Geom().IsValid()) {
            surfaceGeomIndex++;
            return 1;
        }
        if (!MaterializeSurfaceGeometryDefinition(surfaceGeomIndex,
                                        definition,
                                        decodedPayloads)) {
            ok = 0;
            return 0;
        }
        surfaceGeomIndex++;
        return 1;
    });
    return ok;
}

int StaticSolidSurfaceAssembler::LinkSurfaceToGeom(
        CGameCtnReplayStaticSolidArchiveNodeIdentity surfaceNode,
        CGameCtnReplayStaticSolidArchiveNodeIdentity geomNode,
        u32 materialSlotCount) {
    const u32 surfaceIndex = SurfaceIndexForNode(surfaceNode);
    const u32 geomIndex = SurfaceGeomIndexForNode(geomNode);
    if (surfaceIndex == UINT32_MAX || geomIndex == UINT32_MAX) {
        return 1;
    }

    CPlugSurface *surface = SurfaceAt(surfaceIndex);
    CPlugSurfaceGeom *geom = SurfaceGeomAt(geomIndex);
    if (surface == nullptr || geom == nullptr) {
        return 1;
    }

    surface->SetGeometry(geom);
    if (materialSlotCount != 0u &&
        !resources.AttachMaterialSlots(surface, surfaceIndex,
                                              materialSlotCount)) {
        return 0;
    }
    return 1;
}

int StaticSolidSurfaceAssembler::LinkSurfacesToGeoms(
        const CGameCtnReplayStaticSolidArchiveSurfaceGraph &surfaceGraph) {
    int ok = 1;
    surfaceGraph.ForEachSurfaceGeometryLink([&](
            const CGameCtnReplayStaticSolidArchiveSurfaceGeomLink &link) {
        if (!LinkSurfaceToGeom(link.Surface(),
                               link.Geom(),
                               link.MaterialEntryCount())) {
            ok = 0;
            return 0;
        }
        return 1;
    });
    return ok;
}

void StaticSolidSurfaceAssembler::ApplySurfaceMaterialEntry(
        CGameCtnReplayStaticSolidArchiveNodeIdentity surfaceNode,
        const CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry &entry) {
    const u32 surfaceIndex = SurfaceIndexForNode(surfaceNode);
    if (surfaceIndex == UINT32_MAX || surfaceIndex >= SurfaceCount()) {
        return;
    }

    CPlugSurface *surface = SurfaceAt(surfaceIndex);
    CPlugMaterial *material = nullptr;
    if (entry.UsesExplicitRef()) {
        const u32 materialIndex = MaterialIndexForNode(entry.Material());
        material = materialIndex != UINT32_MAX && materialIndex < MaterialCount()
                ? MaterialAt(materialIndex)
                : nullptr;
    } else {
        material = DefaultSurfaceMaterial(entry.DefaultMaterialId());
    }

    if (material == nullptr) {
        return;
    }
    resources.AssignSurfaceMaterial(
            surface, entry.MaterialSlotIndex(), material);
}

void StaticSolidSurfaceAssembler::ApplySurfaceMaterialEntries(
        const CGameCtnReplayStaticSolidArchiveSurfaceGraph &surfaceGraph) {
    surfaceGraph.ForEachSurfaceMaterialEntry([&](
            const CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry &entry) {
        ApplySurfaceMaterialEntry(entry.Surface(),
                                  entry);
        return 1;
    });
}

void StaticSolidSurfaceAssembler::MaterializeArchiveVisualProvider(
        u32 visualProviderIndex,
        const CGameCtnReplayStaticSolidArchiveVisualGeometryDefinition &definition,
        const StaticSolidDecodedPayloads &decodedPayloads) {
    MaterializeVisualProviderGeometry(
            visualProviderIndex,
            definition.VisualProvider(),
            StaticSolidDecodedVisualGeometry::FromArchiveDefinition(
                    definition,
                    decodedPayloads));
}

void StaticSolidSurfaceAssembler::MaterializeVisualProviderGeometry(
        u32 visualProviderIndex,
        CGameCtnReplayStaticSolidArchiveNodeIdentity visualProvider,
        StaticSolidDecodedVisualGeometry geometry) {
    StaticSolidArchiveVisual *provider =
            VisualProviderAt(visualProviderIndex);
    if (provider == nullptr) {
        return;
    }

    provider->ConfigureGeometry(geometry);
    resources.RegisterVisualProvider(visualProvider, visualProviderIndex);
}

void StaticSolidSurfaceAssembler::MaterializeVisualProviderDefinitions(
        const CGameCtnReplayStaticSolidArchiveSurfaceGraph &surfaceGraph,
        const StaticSolidDecodedPayloads &decodedPayloads) {
    u32 visualProviderIndex = 0u;
    surfaceGraph.ForEachVisualProviderDefinition([&](
            const CGameCtnReplayStaticSolidArchiveVisualGeometryDefinition &definition) {
        if (!definition.VisualProvider().IsValid() ||
            visualProviderIndex >= VisualProviderCount()) {
            visualProviderIndex++;
            return 1;
        }
        MaterializeArchiveVisualProvider(visualProviderIndex,
                                         definition,
                                         decodedPayloads);
        visualProviderIndex++;
        return 1;
    });
}

CPlugSurface *StaticSolidSurfaceAssembler::SurfaceForPayload(
        StaticSolidArchiveId payload) {
    const u32 surfaceIndex = resources.FirstSurfaceForPayload(payload);
    return surfaceIndex != UINT32_MAX && surfaceIndex < SurfaceCount()
            ? SurfaceAt(surfaceIndex)
            : nullptr;
}

CPlugSurface *StaticSolidSurfaceAssembler::SurfaceForNode(
        CGameCtnReplayStaticSolidArchiveNodeIdentity surface) {
    const u32 surfaceIndex = SurfaceIndexForNode(surface);
    return surfaceIndex != UINT32_MAX ? SurfaceAt(surfaceIndex) : nullptr;
}

CPlugMaterial *StaticSolidSurfaceAssembler::MaterialForNode(
        CGameCtnReplayStaticSolidArchiveNodeIdentity material) {
    const u32 materialIndex = MaterialIndexForNode(material);
    return materialIndex != UINT32_MAX ? MaterialAt(materialIndex) : nullptr;
}

StaticSolidArchiveVisual *
StaticSolidSurfaceAssembler::VisualProviderForNode(
        CGameCtnReplayStaticSolidArchiveNodeIdentity visual) {
    const u32 visualIndex = VisualProviderIndexForNode(visual);
    return visualIndex != UINT32_MAX ? VisualProviderAt(visualIndex) : nullptr;
}

CPlugMaterial *StaticSolidSurfaceAssembler::
        MaterialRemapForSource(
                StaticSolidArchiveId payload,
                StaticSolidMaterialRemap kind,
                const CPlugMaterial *source) {
    EnsureDefaultSurfaceMaterials();
    return materialRemaps.Resolve(payload, kind, source);
}

CPlugMaterial *StaticSolidSurfaceAssembler::RemapMaterialRef(
        StaticSolidArchiveId payload,
        StaticSolidMaterialRemap kind,
        CPlugMaterial *material) {
    CPlugMaterial *remapped =
            MaterialRemapForSource(payload, kind, material);
    return remapped != nullptr ? remapped : material;
}

void StaticSolidSurfaceAssembler::
        ApplyMaterialRemapToSurface(
                CPlugSurface *surface,
                StaticSolidArchiveId payload,
                StaticSolidMaterialRemap kind) {
    if (surface == nullptr) {
        return;
    }
    const u32 materialCount = surface->MaterialCount();
    for (u32 i = 0; i < materialCount; i++) {
        CPlugMaterial *material = surface->MaterialAt(i);
        if (material == nullptr) {
            continue;
        }
        surface->SetMaterialAt(
                i,
                RemapMaterialRef(payload, kind, material));
    }
}

void StaticSolidSurfaceAssembler::
        ApplyMaterialRemapToSubtree(
                CPlugTree *tree,
                StaticSolidArchiveId payload,
                StaticSolidMaterialRemap kind) {
    if (tree == nullptr) {
        return;
    }
    tree->SetMaterial(RemapMaterialRef(payload, kind, tree->Material()));
    ApplyMaterialRemapToSurface(tree->Surface(), payload, kind);

    const u32 childCount = tree->GetChildCount();
    for (u32 i = 0; i < childCount; i++) {
        ApplyMaterialRemapToSubtree(tree->GetChild(i),
                                    payload,
                                    kind);
    }
}

void StaticSolidSurfaceAssembler::ApplyMaterialRemapToTree(
        CPlugTree *tree,
        StaticSolidArchiveId payload,
        StaticSolidMaterialRemap kind) {
    if (tree == nullptr) {
        return;
    }
    EnsureDefaultSurfaceMaterials();
    // Keep the cloned tree and its collision surfaces on the same material map.
    ApplyMaterialRemapToSubtree(tree, payload, kind);
}

u32 StaticSolidSurfaceAssembler::SurfaceCount() const {
    return surfaceCount;
}

u32 StaticSolidSurfaceAssembler::SurfaceGeomCount() const {
    return surfaceGeomCount;
}

u32 StaticSolidSurfaceAssembler::VisualProviderCount() const {
    return visualProviders.Count();
}

u32 StaticSolidSurfaceAssembler::MaterialCount() const {
    return materialCount;
}
