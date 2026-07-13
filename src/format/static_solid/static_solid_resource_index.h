#pragma once

#include "engine/rendering/plug_material.h"
#include "engine/physics/geometry/plug_surface.h"
#include "engine/core/engine_types.h"
#include <stdint.h>
#include <vector>

#include "format/static_solid/static_solid_archive_identity.h"
#include "format/static_solid/static_solid_archive_id.h"
class StaticSolidNodeIndex {
public:
    void Clear();
    int Register(CGameCtnReplayStaticSolidArchiveNodeIdentity node,
                 u32 objectIndex);
    u32 Find(CGameCtnReplayStaticSolidArchiveNodeIdentity node) const;
    u32 FirstForPayload(StaticSolidArchiveId payload)
            const;

private:
    struct Entry {
        CGameCtnReplayStaticSolidArchiveNodeIdentity node =
                CGameCtnReplayStaticSolidArchiveNodeIdentity::Invalid();
        u32 objectIndex = UINT32_MAX;
    };

    std::vector<Entry> entries;
};

class StaticSolidResourceIndex {
public:
    void Clear();

    int AllocateSurfaceMaterialSlots(u32 surfaceCount);
    int AttachMaterialSlots(CPlugSurface *surface,
                            u32 surfaceIndex,
                            u32 materialSlotCount);
    int AssignSurfaceMaterial(CPlugSurface *surface,
                              u32 materialSlotIndex,
                              CPlugMaterial *material);
    static void DetachSurfaceMaterials(CPlugSurface *surface);

    int RegisterSurface(CGameCtnReplayStaticSolidArchiveNodeIdentity node,
                        u32 surfaceIndex);
    int RegisterSurfaceGeom(CGameCtnReplayStaticSolidArchiveNodeIdentity node,
                            u32 surfaceGeomIndex);
    int RegisterVisualProvider(
            CGameCtnReplayStaticSolidArchiveNodeIdentity node,
            u32 visualProviderIndex);
    int RegisterMaterial(CGameCtnReplayStaticSolidArchiveNodeIdentity node,
                         u32 materialIndex);

    u32 Surface(CGameCtnReplayStaticSolidArchiveNodeIdentity node) const;
    u32 FirstSurfaceForPayload(
            StaticSolidArchiveId payload) const;
    u32 SurfaceGeom(CGameCtnReplayStaticSolidArchiveNodeIdentity node) const;
    u32 VisualProvider(
            CGameCtnReplayStaticSolidArchiveNodeIdentity node) const;
    u32 Material(CGameCtnReplayStaticSolidArchiveNodeIdentity node) const;

private:
    StaticSolidNodeIndex surfaces;
    StaticSolidNodeIndex surfaceGeoms;
    StaticSolidNodeIndex visualProviders;
    StaticSolidNodeIndex materials;
};
