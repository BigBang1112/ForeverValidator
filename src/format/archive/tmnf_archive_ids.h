#ifndef CMW_NOD_ARCHIVE_IDS_H
#define CMW_NOD_ARCHIVE_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
constexpr u32 CMwNodArchiveFacadeSentinel = 0xfacade01u;

constexpr u32 TMNF_CGameCtnCollectorChunk_0301A006 = 0x0301a006u;
constexpr u32 TMNF_CGameCtnCollectorChunk_0301A007 = 0x0301a007u;
constexpr u32 TMNF_CGameCtnCollectorChunk_LoadableNodRefAndId = 0x0301a009u;
constexpr u32 TMNF_CGameCtnCollectorChunk_CMwId0301A00A = 0x0301a00au;
constexpr u32 TMNF_CGameCtnCollectorChunk_SGameCtnIdentifier = 0x0301a00bu;

static inline u32 TMNF_CPlugSolidCanonicalChunkIdFromArchiveWord(u32 chunkId) {
    /*
     * Solid chunks use the canonical 0x09005000 selector family. Some packed
     * mobil and static-solid streams encode the same selector with an
     * alternate high word, so normalize that word before chunk dispatch.
     */
    const u32 low = chunkId & 0xffffu;
    if ((chunkId & 0xffff0000u) != 0x09000000u &&
        low >= 0x5000u &&
        low <= 0x5012u) {
        return 0x09000000u | low;
    }
    return chunkId;
}

static inline u32 TMNF_CPlugTreeCanonicalChunkIdFromArchiveWord(u32 chunkId) {
    /*
     * Tree chunks use selectors 0x0904f000 through 0x0904f01a. Packed Stadium
     * solids may encode those selectors as feedback words with a different
     * high byte; normalize them before archive dispatch.
     */
    const u32 low = chunkId & 0xffffu;
    if ((chunkId & 0xff000000u) != 0x09000000u &&
        low >= 0xf000u &&
        low <= 0xf01au) {
        return 0x09040000u | low;
    }
    return chunkId;
}
#endif
