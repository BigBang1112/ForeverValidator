#ifndef TMNF_STATIC_SOLID_ARCHIVE_PAYLOAD_DECODE_H
#define TMNF_STATIC_SOLID_ARCHIVE_PAYLOAD_DECODE_H

#include "engine/core/engine_types.h"
#include <vector>

#include "format/pack/installed/byte_buffer.h"
struct CGameCtnReplayStaticSolidArchiveDecodeStats {
    u32 bodyOffsetParsed = 0u;
    u32 rootFeedbackApplied = 0u;
    u32 nestedFeedbackApplied = 0u;
    u32 archiveNodeCount = 0u;
    u32 archiveTreeCount = 0u;
    u32 archiveSurfaceCount = 0u;
    u32 archiveSurfaceGeomCount = 0u;
    u32 archiveMeshCount = 0u;
    u32 archiveTreeChildLinkCount = 0u;

    void Clear();
};

class CGameCtnReplayStaticSolidDecodedPayload {
public:
    CGameCtnReplayStaticSolidDecodedPayload() = default;
    CGameCtnReplayStaticSolidDecodedPayload(
            const CGameCtnReplayStaticSolidDecodedPayload &) = delete;
    CGameCtnReplayStaticSolidDecodedPayload &operator=(
            const CGameCtnReplayStaticSolidDecodedPayload &) = delete;
    ~CGameCtnReplayStaticSolidDecodedPayload() = default;

    void Clear();
    int ResizeForDecode(u32 count);
    int CopyFrom(const unsigned char *source, u32 byteCount);
    int TrimToByteCount(u32 count);
    void TakeFrom(CGameCtnReplayStaticSolidDecodedPayload *source);
    unsigned char *MutableBytes();
    const unsigned char *Bytes() const;
    u32 ByteCount() const;
    int IsReady() const;
    int CopyToByteBuffer(ByteBuffer *out) const;

private:
    std::vector<unsigned char> bytes;
    bool ready = false;
};

#endif
