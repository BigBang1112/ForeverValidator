#include "format/static_solid/static_solid_archive_surface_reader.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_graph_writer.h"
#include "format/static_solid/static_solid_archive_node_graph.h"
#include "format/static_solid/static_solid_material_definition_resolver.h"
#include <new>
CGameCtnReplayStaticSolidSurfaceDefinition
CGameCtnReplayStaticSolidSurfaceDefinition::ForArchiveSurface(
        StaticSolidArchiveId newPayload,
        u32 newSurfaceNodeIndex) {
    CGameCtnReplayStaticSolidSurfaceDefinition definition;
    definition.payload = newPayload;
    definition.surfaceNode =
            ArchiveNodeReference::FromIndex(newSurfaceNodeIndex);
    return definition;
}

int CGameCtnReplayStaticSolidSurfaceDefinition::ReadFromArchive(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }

    u32 materialCount = 0u;
    if (!nodeRefs->ReadNodeRef(&geomNode) ||
        !byteStream->ReadU32(&materialCount) ||
        materialCount > 0x100000u) {
        return 0;
    }

    try {
        materialEntries.clear();
        materialEntries.reserve(materialCount);
    } catch (const std::bad_alloc &) {
        return 0;
    }

    for (u32 i = 0u; i < materialCount; i++) {
        u32 usesExplicitRef = 0u;
        if (!byteStream->ReadBool(&usesExplicitRef)) {
            return 0;
        }

        if (usesExplicitRef != 0u) {
            ArchiveNodeReference materialNode =
                    ArchiveNodeReference::Invalid();
            if (!nodeRefs->ReadNodeRef(&materialNode)) {
                return 0;
            }
            CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry entry;
            entry.InstallExplicitRef(NodeIdentity(surfaceNode),
                                     i,
                                     NodeIdentity(materialNode));
            try {
                materialEntries.push_back(entry);
            } catch (const std::bad_alloc &) {
                return 0;
            }
        } else {
            u32 defaultMaterialId = 0u;
            if (!byteStream->ReadU16(&defaultMaterialId)) {
                return 0;
            }
            CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry entry;
            entry.InstallDefaultMaterial(NodeIdentity(surfaceNode),
                                         i,
                                         (uint16_t)defaultMaterialId);
            try {
                materialEntries.push_back(entry);
            } catch (const std::bad_alloc &) {
                return 0;
            }
        }
    }

    return 1;
}

u32 CGameCtnReplayStaticSolidSurfaceDefinition::ExplicitMaterialRefCount()
        const {
    u32 count = 0u;
    for (const CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry &entry :
         materialEntries) {
        if (entry.UsesExplicitRef()) {
            count++;
        }
    }
    return count;
}

u32 CGameCtnReplayStaticSolidSurfaceDefinition::DefaultMaterialIdCount()
        const {
    u32 count = 0u;
    for (const CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry &entry :
         materialEntries) {
        if (!entry.UsesExplicitRef()) {
            count++;
        }
    }
    return count;
}

int CGameCtnReplayStaticSolidSurfaceDefinition::Commit(
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        const SceneDescriptorFolderPaths *externalFolders,
        StaticSolidArchiveLoadSession *store) const {
    if (store != nullptr) {
        for (const CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry &entry :
             materialEntries) {
            if (entry.UsesExplicitRef() &&
                !StaticSolidMaterialAssetLinker::ResolveAndAppend(
                                archiveNodeGraph,
                                externalFolders,
                                store,
                                entry.Material())) {
                return 0;
            }
            if (!store->MutableArchiveGraph()
                         .SurfaceGraph()
                         .AddSurfaceMaterialEntry(entry)) {
                return 0;
            }
        }
    }

    CGameCtnReplayStaticSolidArchiveGraphWriter writer(
            store != nullptr ? &store->MutableArchiveGraph() : nullptr,
            payload);
    return writer.AddSurfaceGeometryLink(surfaceNode,
                                        geomNode,
                                        (u32)materialEntries.size(),
                                        ExplicitMaterialRefCount(),
                                        DefaultMaterialIdCount());
}

int CGameCtnReplayStaticSolidArchiveSurfaceReader::ParseSurfaceDefinition(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        const SceneDescriptorFolderPaths *externalFolders,
        StaticSolidArchiveLoadSession *store,
        StaticSolidArchiveId selectedPayload,
        u32 surfaceNodeIndex,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    CGameCtnReplayStaticSolidSurfaceDefinition surface =
            CGameCtnReplayStaticSolidSurfaceDefinition::ForArchiveSurface(
                    selectedPayload,
                    surfaceNodeIndex);
    return surface.ReadFromArchive(byteStream, nodeRefs) &&
           surface.Commit(archiveNodeGraph, externalFolders, store);
}
