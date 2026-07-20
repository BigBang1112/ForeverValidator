#ifndef TMNF_STATIC_SOLID_ARCHIVE_BITMAP_READER_H
#define TMNF_STATIC_SOLID_ARCHIVE_BITMAP_READER_H

#include "engine/core/engine_types.h"

class CGameCtnReplayStaticSolidArchiveByteStream;
class CGameCtnReplayStaticSolidArchiveNodeRefReader;

struct CGameCtnReplayStaticSolidArchiveBitmapReader {
    static int ParseCPlugBitmapChunk(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
            u32 chunkId,
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs);
    static int ParseCPlugBitmapRenderWaterChunk(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
            u32 chunkId,
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs);
    static int ParseCPlugFileGenPayload(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs);
};

#endif
