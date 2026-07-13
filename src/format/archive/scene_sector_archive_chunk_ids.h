#ifndef TMNF_SCENE_SECTOR_ARCHIVE_CHUNK_IDS_H
#define TMNF_SCENE_SECTOR_ARCHIVE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
enum class CSceneSectorArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneSector,
    Iso = 0x0a004001u,
    Id = 0x0a004002u,
    BoxAlignedOld1 = 0x0a004003u,
    BoxAligned = 0x0a004004u,
};

constexpr u32 ArchiveChunkIdValue(CSceneSectorArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCSceneSectorInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CSceneSectorArchiveChunkId::BoxAlignedOld1);
}

constexpr int IsCSceneSectorInfo3Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CSceneSector ||
           chunkId == ArchiveChunkIdValue(CSceneSectorArchiveChunkId::Iso) ||
           chunkId == ArchiveChunkIdValue(CSceneSectorArchiveChunkId::Id) ||
           chunkId == ArchiveChunkIdValue(CSceneSectorArchiveChunkId::BoxAligned);
}

#endif
