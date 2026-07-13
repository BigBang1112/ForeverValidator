#ifndef TMNF_SCENE_OBJECT_LINK_ARCHIVE_CHUNK_IDS_H
#define TMNF_SCENE_OBJECT_LINK_ARCHIVE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
enum class CSceneObjectLinkArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneObjectLink,
    MobilOrObjectRefBeforeIso = 0x0a014001u,
    MobilTreeIdAndInstallFlags = 0x0a014002u,
    ArchivedTreeId = 0x0a014003u,
    LegacyRoot = 0x0a00f000u,
    LegacyPositionAndActiveFlag = 0x0a00f001u,
    LegacyActiveFlag = 0x0a00f002u,
};

constexpr u32 ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCSceneObjectLinkArchiveChunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CSceneObjectLink ||
           chunkId == ArchiveChunkIdValue(
                              CSceneObjectLinkArchiveChunkId::MobilOrObjectRefBeforeIso) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneObjectLinkArchiveChunkId::MobilTreeIdAndInstallFlags) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneObjectLinkArchiveChunkId::ArchivedTreeId) ||
           chunkId == ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::LegacyRoot) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneObjectLinkArchiveChunkId::LegacyPositionAndActiveFlag) ||
           chunkId == ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::LegacyActiveFlag);
}

constexpr int IsCSceneObjectLinkInfo1Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CSceneObjectLink ||
           chunkId == ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::LegacyRoot) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneObjectLinkArchiveChunkId::LegacyPositionAndActiveFlag) ||
           chunkId == ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::LegacyActiveFlag);
}

constexpr int IsCSceneObjectLinkInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CSceneObjectLinkArchiveChunkId::MobilOrObjectRefBeforeIso) ||
           chunkId == ArchiveChunkIdValue(
                              CSceneObjectLinkArchiveChunkId::MobilTreeIdAndInstallFlags) ||
           chunkId == ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::ArchivedTreeId);
}

#endif
