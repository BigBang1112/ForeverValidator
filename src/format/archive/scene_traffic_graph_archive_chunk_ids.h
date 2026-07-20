#ifndef TMNF_SCENE_TRAFFIC_GRAPH_ARCHIVE_CHUNK_IDS_H
#define TMNF_SCENE_TRAFFIC_GRAPH_ARCHIVE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"

enum class CSceneTrafficGraphArchiveChunkId : u32 {
    EmptyOld0 = TMNF_CLASS_CSceneTrafficGraph,
    HubLegacy0 = 0x0a062001u,
    EmptyOld1 = 0x0a062002u,
    HubLegacy1 = 0x0a062003u,
    HubsAndEdges = 0x0a062004u,
    HubLegacy2 = 0x0a062005u,
};

constexpr u32 ArchiveChunkIdValue(CSceneTrafficGraphArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCSceneTrafficGraphInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CSceneTrafficGraphArchiveChunkId::EmptyOld0) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneTrafficGraphArchiveChunkId::HubLegacy0) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneTrafficGraphArchiveChunkId::EmptyOld1) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneTrafficGraphArchiveChunkId::HubLegacy1) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneTrafficGraphArchiveChunkId::HubLegacy2);
}

constexpr int IsCSceneTrafficGraphInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CSceneTrafficGraphArchiveChunkId::HubsAndEdges);
}

constexpr int IsCSceneTrafficGraphLegacyHubChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CSceneTrafficGraphArchiveChunkId::HubLegacy0) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneTrafficGraphArchiveChunkId::HubLegacy1) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneTrafficGraphArchiveChunkId::HubLegacy2);
}

#endif
