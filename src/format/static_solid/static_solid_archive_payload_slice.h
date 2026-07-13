#ifndef TMNF_STATIC_SOLID_ARCHIVE_PAYLOAD_SLICE_H
#define TMNF_STATIC_SOLID_ARCHIVE_PAYLOAD_SLICE_H

#include "engine/core/engine_types.h"

class CGameCtnReplayStaticSolidArchivePayloadSlice {
public:
    static CGameCtnReplayStaticSolidArchivePayloadSlice Empty();
    static CGameCtnReplayStaticSolidArchivePayloadSlice FromRecords(
            u32 byteOffset,
            u32 recordCount,
            u32 recordStride);

    int HasRecords() const;
    u32 ByteOffset() const;
    u32 RecordCount() const;
    u32 RecordStride() const;
    u32 ByteCount() const;

private:
    bool hasRecords = false;
    u32 byteOffset = 0xffffffffu;
    u32 recordCount = 0u;
    u32 recordStride = 0u;
};

#endif
