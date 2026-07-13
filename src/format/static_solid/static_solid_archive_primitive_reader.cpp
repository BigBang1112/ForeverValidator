#include "format/static_solid/static_solid_archive_primitive_reader.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/archive/tmnf_archive_ids.h"
int CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    if (byteStream == nullptr) {
        return 0;
    }
    u32 count = 0u;
    return byteStream->ReadU32(&count) &&
           count <= 0x10000000u &&
           byteStream->SkipCounted(count, 4u);
}

int CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 count) {
    return byteStream != nullptr &&
           byteStream->SkipCounted(count, 4u);
}

int CGameCtnReplayStaticSolidArchivePrimitiveReader::ReadIso4(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        float iso[12]) {
    return byteStream != nullptr &&
           byteStream->Read((unsigned char *)iso, sizeof(float) * 12u);
}

int CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipMat3(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    /*
     * A matrix payload contains nine binary32 components.
     */
    return byteStream != nullptr &&
           byteStream->Skip(9u * 4u);
}

int CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipIso4(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    /*
     * A rigid transform contains a matrix followed by three translation
     * components.
     */
    return byteStream != nullptr &&
           byteStream->Skip(12u * 4u);
}

int CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipBoxAligned(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    /*
     * An aligned box occupies six binary32 components. Frustum records reserve
     * the same 24 bytes when their box-presence flag is clear.
     */
    return byteStream != nullptr &&
           byteStream->Skip(6u * 4u);
}

int CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFrustumIso4(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    /*
     * A frustum transform contains a box-presence flag, a 24-byte box slot,
     * a 3x3 orientation matrix, and three translation components.
     */
    if (byteStream == nullptr) {
        return 0;
    }
    u32 hasBox = 0u;
    if (!byteStream->ReadBool(&hasBox) ||
        !SkipBoxAligned(byteStream) ||
        !SkipMat3(byteStream) ||
        !byteStream->Skip(3u * 4u)) {
        return 0;
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipInlineArchivePayload(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    if (byteStream == nullptr) {
        return 0;
    }
    const u32 payloadUncompressedSize = byteStream->PayloadUncompressedSize();
    if (byteStream->Offset() > payloadUncompressedSize ||
        payloadUncompressedSize - byteStream->Offset() < 4u) {
        return 0;
    }
    for (u32 cursor = byteStream->Offset();
         cursor <= payloadUncompressedSize - 4u;
         cursor++) {
        if (!byteStream->Ensure(cursor + 4u)) {
            return 0;
        }
        u32 word = 0u;
        if (!byteStream->PeekU32At(cursor, &word)) {
            return 0;
        }
        if (word == CMwNodArchiveFacadeSentinel) {
            return byteStream->SetOffset(cursor + 4u);
        }
    }
    return 0;
}

int CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipD3dFormatArray(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    if (byteStream == nullptr) {
        return 0;
    }
    u32 count = 0u;
    if (!byteStream->ReadU32(&count) ||
        count > 0x10000000u) {
        return 0;
    }
    return byteStream->SkipCounted(count, 4u);
}
