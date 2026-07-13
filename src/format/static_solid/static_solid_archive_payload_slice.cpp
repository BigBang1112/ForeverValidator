#include "format/static_solid/static_solid_archive_payload_slice.h"
CGameCtnReplayStaticSolidArchivePayloadSlice
CGameCtnReplayStaticSolidArchivePayloadSlice::Empty() {
    return CGameCtnReplayStaticSolidArchivePayloadSlice{};
}

CGameCtnReplayStaticSolidArchivePayloadSlice
CGameCtnReplayStaticSolidArchivePayloadSlice::FromRecords(
        u32 byteOffset,
        u32 recordCount,
        u32 recordStride) {
    CGameCtnReplayStaticSolidArchivePayloadSlice slice;
    if (recordCount != 0u && recordStride != 0u &&
        recordCount <= UINT32_MAX / recordStride) {
        slice.hasRecords = true;
        slice.byteOffset = byteOffset;
        slice.recordCount = recordCount;
        slice.recordStride = recordStride;
    }
    return slice;
}

int CGameCtnReplayStaticSolidArchivePayloadSlice::HasRecords() const {
    return hasRecords &&
           recordCount != 0u &&
           recordStride != 0u &&
           recordCount <= UINT32_MAX / recordStride;
}

u32 CGameCtnReplayStaticSolidArchivePayloadSlice::ByteOffset() const {
    return byteOffset;
}

u32 CGameCtnReplayStaticSolidArchivePayloadSlice::RecordCount() const {
    return recordCount;
}

u32 CGameCtnReplayStaticSolidArchivePayloadSlice::RecordStride() const {
    return recordStride;
}

u32 CGameCtnReplayStaticSolidArchivePayloadSlice::ByteCount() const {
    return HasRecords() ? recordCount * recordStride : 0u;
}
