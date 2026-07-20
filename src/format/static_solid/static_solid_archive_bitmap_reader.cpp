#include "format/static_solid/static_solid_archive_bitmap_reader.h"

#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_node_ref_reader.h"

namespace {

constexpr u32 MaxArchiveArrayCount = 0x100000u;
constexpr u32 MaxArchiveStringBytes = 0x10000u;
constexpr u32 SkipBlockMarker = 0x534b4950u;

constexpr u32 CPlugBitmapChunkImage14 = 0x09011014u;
constexpr u32 CPlugBitmapChunkImage15 = 0x09011015u;
constexpr u32 CPlugBitmapChunkImage18 = 0x09011018u;
constexpr u32 CPlugBitmapChunkFloat19 = 0x09011019u;
constexpr u32 CPlugBitmapChunkFlags1b = 0x0901101bu;
constexpr u32 CPlugBitmapChunkUv1c = 0x0901101cu;
constexpr u32 CPlugBitmapChunkShorts1d = 0x0901101du;
constexpr u32 CPlugBitmapChunkVec2Array1e = 0x0901101eu;
constexpr u32 CPlugBitmapChunkFlags1f = 0x0901101fu;
constexpr u32 CPlugBitmapChunkFloatArray20 = 0x09011020u;
constexpr u32 CPlugBitmapChunkWord21 = 0x09011021u;
constexpr u32 CPlugBitmapChunkImage22 = 0x09011022u;
constexpr u32 CPlugBitmapChunkWord23 = 0x09011023u;
constexpr u32 CPlugBitmapChunkFlags24 = 0x09011024u;
constexpr u32 CPlugBitmapChunkUv25 = 0x09011025u;
constexpr u32 CPlugBitmapChunkInt2_28 = 0x09011028u;
constexpr u32 CPlugBitmapChunkWord2e = 0x0901102eu;
constexpr u32 CPlugBitmapChunkEmpty30 = 0x09011030u;

constexpr u32 CPlugBitmapRenderChunkShorts03 = 0x09086003u;
constexpr u32 CPlugBitmapRenderChunkSkip08 = 0x09086008u;
constexpr u32 CPlugBitmapRenderChunkFloat0a = 0x0908600au;
constexpr u32 CPlugBitmapRenderChunkClear0b = 0x0908600bu;
constexpr u32 CPlugBitmapRenderChunkSub0c = 0x0908600cu;
constexpr u32 CPlugBitmapRenderChunkWord0d = 0x0908600du;
constexpr u32 CPlugBitmapRenderChunkWord0e = 0x0908600eu;
constexpr u32 CPlugBitmapRenderWaterChunkState05 = 0x09087005u;
constexpr u32 CPlugBitmapRenderWaterChunkState06 = 0x09087006u;

int SkipU32Array(CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    u32 count = 0u;
    return byteStream != nullptr && byteStream->ReadU32(&count) &&
           count <= MaxArchiveArrayCount &&
           byteStream->SkipCounted(count, 4u);
}

int SkipVec4Array(CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    u32 count = 0u;
    return byteStream != nullptr && byteStream->ReadU32(&count) &&
           count <= MaxArchiveArrayCount &&
           byteStream->SkipCounted(count, 16u);
}

int SkipSizedBlock(CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
                   u32 expectedByteCount) {
    u32 marker = 0u;
    u32 byteCount = 0u;
    return byteStream != nullptr && byteStream->ReadU32(&marker) &&
           marker == SkipBlockMarker && byteStream->ReadU32(&byteCount) &&
           byteCount == expectedByteCount && byteStream->Skip(byteCount);
}

int SkipBoundedString(CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    u32 byteCount = 0u;
    return byteStream != nullptr && byteStream->ReadU32(&byteCount) &&
           byteCount <= MaxArchiveStringBytes && byteStream->Skip(byteCount);
}

}  // namespace

int CGameCtnReplayStaticSolidArchiveBitmapReader::ParseCPlugFileGenPayload(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    u32 archivedVersion = 0u;
    if (byteStream == nullptr || nodeRefs == nullptr ||
        !byteStream->ReadU32(&archivedVersion)) {
        return 0;
    }
    u32 genKind = archivedVersion;
    if ((archivedVersion & 0x80000000u) != 0u) {
        const u32 version = archivedVersion & 0x7fffffffu;
        if (version > 5u || !byteStream->ReadU32(&genKind) ||
            !SkipU32Array(byteStream) || !SkipVec4Array(byteStream) ||
            !SkipU32Array(byteStream)) {
            return 0;
        }
        if (version > 2u && !SkipBoundedString(byteStream)) {
            return 0;
        }
        if (version > 3u) {
            u32 count = 0u;
            if (!byteStream->ReadU32(&count) ||
                count > MaxArchiveArrayCount) {
                return 0;
            }
            for (u32 index = 0u; index < count; ++index) {
                if (!nodeRefs->ReadNodPtr(nullptr)) {
                    return 0;
                }
            }
        }
    } else if (archivedVersion > 0x32u || !SkipU32Array(byteStream) ||
               !SkipVec4Array(byteStream)) {
        return 0;
    }
    return genKind != 9u;
}

int CGameCtnReplayStaticSolidArchiveBitmapReader::ParseCPlugBitmapChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    switch (chunkId) {
    case CPlugBitmapChunkFloat19:
    case CPlugBitmapChunkFlags1b:
    case CPlugBitmapChunkFlags1f:
    case CPlugBitmapChunkWord21:
    case CPlugBitmapChunkWord23:
    case CPlugBitmapChunkFlags24:
    case CPlugBitmapChunkWord2e:
        return byteStream->Skip(4u);
    case CPlugBitmapChunkUv1c:
        return byteStream->Skip(20u);
    case CPlugBitmapChunkShorts1d:
        return byteStream->Skip(4u);
    case CPlugBitmapChunkVec2Array1e: {
        u32 count = 0u;
        return byteStream->ReadU32(&count) &&
               count <= MaxArchiveArrayCount &&
               byteStream->SkipCounted(count, 8u);
    }
    case CPlugBitmapChunkFloatArray20:
        return SkipU32Array(byteStream);
    case CPlugBitmapChunkImage14:
    case CPlugBitmapChunkImage15:
    case CPlugBitmapChunkImage18:
    case CPlugBitmapChunkImage22: {
        ArchiveNodeReference image = ArchiveNodeReference::Invalid();
        int imageIsInternal = 0;
        if (!nodeRefs->ReadNodeRefWithInlineState(
                    &image, &imageIsInternal) ||
            !byteStream->Skip(24u)) {
            return 0;
        }
        if ((chunkId == CPlugBitmapChunkImage18 ||
             chunkId == CPlugBitmapChunkImage22) &&
            image.IsValid() && imageIsInternal) {
            return nodeRefs->ReadNodPtr(nullptr);
        }
        return 1;
    }
    case CPlugBitmapChunkUv25:
        return byteStream->Skip(24u);
    case CPlugBitmapChunkInt2_28:
        return byteStream->Skip(8u);
    case CPlugBitmapChunkEmpty30:
        return 1;
    default:
        return 0;
    }
}

int CGameCtnReplayStaticSolidArchiveBitmapReader::
ParseCPlugBitmapRenderWaterChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    switch (chunkId) {
    case CPlugBitmapRenderChunkShorts03:
        return byteStream->Skip(8u);
    case CPlugBitmapRenderChunkSkip08:
        return SkipSizedBlock(byteStream, 8u);
    case CPlugBitmapRenderChunkFloat0a:
    case CPlugBitmapRenderChunkWord0d:
    case CPlugBitmapRenderChunkWord0e:
        return byteStream->Skip(4u);
    case CPlugBitmapRenderChunkClear0b:
        return byteStream->Skip(8u) && nodeRefs->ReadNodPtr(nullptr) &&
               byteStream->Skip(8u);
    case CPlugBitmapRenderChunkSub0c:
        return nodeRefs->ReadNodPtr(nullptr);
    case CPlugBitmapRenderWaterChunkState05:
        return SkipSizedBlock(byteStream, 92u);
    case CPlugBitmapRenderWaterChunkState06:
        return SkipSizedBlock(byteStream, 4u);
    default:
        return 0;
    }
}
