#ifndef TMNF_BLOCKUNITINFO_ARCHIVE_CHUNK_IDS_H
#define TMNF_BLOCKUNITINFO_ARCHIVE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "engine/game/game_ctn_block_info.h"
#include "format/archive/archive_class_ids.h"
enum class CGameCtnBlockUnitInfoArchiveChunkId : u32 {
    Root = TMNF_CLASS_CGameCtnBlockUnitInfo,
    SurfaceAndOffset = 0x03036001u,
    Underground = 0x03036002u,
    ReplacementAndJunction = 0x03036003u,
    HelperMask = 0x03036004u,
    TerrainModifier = 0x03036005u,
};

constexpr u32 ArchiveChunkIdValue(CGameCtnBlockUnitInfoArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCGameCtnBlockUnitInfoInfo3Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CGameCtnBlockUnitInfo ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockUnitInfoArchiveChunkId::SurfaceAndOffset) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockUnitInfoArchiveChunkId::
                                      ReplacementAndJunction) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockUnitInfoArchiveChunkId::HelperMask) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockUnitInfoArchiveChunkId::TerrainModifier);
}

constexpr int IsCGameCtnBlockUnitInfoSkippedChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CGameCtnBlockUnitInfoArchiveChunkId::Underground);
}

#endif
