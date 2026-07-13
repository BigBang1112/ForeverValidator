#ifndef TMNF_REPLAY_COLLECTION_ARCHIVE_CHUNK_IDS_H
#define TMNF_REPLAY_COLLECTION_ARCHIVE_CHUNK_IDS_H

#include <stdint.h>

enum class CGameCtnCollectionArchiveChunkId : uint32_t {
    ZoneBufferRefs = 0x03033009u,
    SurfaceReplacementPairs = 0x0303301du
};

constexpr uint32_t ArchiveChunkIdValue(
        CGameCtnCollectionArchiveChunkId chunkId) {
    return static_cast<uint32_t>(chunkId);
}

#endif
