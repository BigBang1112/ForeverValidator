#include "format/static_solid/static_solid_resource_index.h"
#include <new>
void StaticSolidNodeIndex::Clear() {
    entries.clear();
}

int StaticSolidNodeIndex::Register(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node,
        u32 objectIndex) {
    try {
        for (Entry &entry : entries) {
            if (entry.node.Matches(node)) {
                entry.objectIndex = objectIndex;
                return 1;
            }
        }
        entries.push_back(Entry{node, objectIndex});
    } catch (const std::bad_alloc &) {
        return 0;
    }
    return 1;
}

u32 StaticSolidNodeIndex::Find(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node) const {
    for (const Entry &entry : entries) {
        if (entry.node.Matches(node)) {
            return entry.objectIndex;
        }
    }
    return UINT32_MAX;
}

u32 StaticSolidNodeIndex::FirstForPayload(
        StaticSolidArchiveId payload) const {
    for (const Entry &entry : entries) {
        if (entry.node.MatchesPayload(payload)) {
            return entry.objectIndex;
        }
    }
    return UINT32_MAX;
}

void StaticSolidResourceIndex::Clear() {
    surfaces.Clear();
    surfaceGeoms.Clear();
    visualProviders.Clear();
    materials.Clear();
}

int StaticSolidResourceIndex::
        AllocateSurfaceMaterialSlots(u32 surfaceCount) {
    (void)surfaceCount;
    return 1;
}

void StaticSolidResourceIndex::
        DetachSurfaceMaterials(CPlugSurface *surface) {
    if (surface != nullptr) {
        surface->ClearMaterials();
    }
}

int StaticSolidResourceIndex::AttachMaterialSlots(
        CPlugSurface *surface,
        u32 surfaceIndex,
        u32 materialSlotCount) {
    (void)surfaceIndex;
    return surface != nullptr && surface->SetMaterialCount(materialSlotCount);
}

int StaticSolidResourceIndex::AssignSurfaceMaterial(
        CPlugSurface *surface,
        u32 materialSlotIndex,
        CPlugMaterial *material) {
    return surface != nullptr &&
           surface->SetMaterialAt(materialSlotIndex, material);
}

int StaticSolidResourceIndex::RegisterSurface(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node,
        u32 surfaceIndex) {
    return surfaces.Register(node, surfaceIndex);
}

int StaticSolidResourceIndex::RegisterSurfaceGeom(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node,
        u32 surfaceGeomIndex) {
    return surfaceGeoms.Register(node, surfaceGeomIndex);
}

int StaticSolidResourceIndex::RegisterVisualProvider(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node,
        u32 visualProviderIndex) {
    return visualProviders.Register(node, visualProviderIndex);
}

int StaticSolidResourceIndex::RegisterMaterial(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node,
        u32 materialIndex) {
    return materials.Register(node, materialIndex);
}

u32 StaticSolidResourceIndex::Surface(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node) const {
    return surfaces.Find(node);
}

u32 StaticSolidResourceIndex::
        FirstSurfaceForPayload(
                StaticSolidArchiveId payload) const {
    return surfaces.FirstForPayload(payload);
}

u32 StaticSolidResourceIndex::SurfaceGeom(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node) const {
    return surfaceGeoms.Find(node);
}

u32 StaticSolidResourceIndex::VisualProvider(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node) const {
    return visualProviders.Find(node);
}

u32 StaticSolidResourceIndex::Material(
        CGameCtnReplayStaticSolidArchiveNodeIdentity node) const {
    return materials.Find(node);
}
