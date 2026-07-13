#include "format/static_solid/static_solid_archive_payload_bytes.h"
#include <memory>
#include <vector>

#include <zlib.h>

#include "format/archive/classic_buffer_crypted.h"
#include <new>
static int inflate_static_solid_part(const unsigned char *input,
                                     u32 inputByteCount,
                                     unsigned char *output,
                                     u32 outputByteCount);

static int inflate_static_solid_part(const unsigned char *input,
                                     u32 inputByteCount,
                                     unsigned char *output,
                                     u32 outputByteCount) {
    z_stream stream{};
    stream.next_in = (Bytef *)input;
    stream.avail_in = inputByteCount;
    stream.next_out = output;
    stream.avail_out = outputByteCount;
    if (inflateInit(&stream) != Z_OK) {
        return 0;
    }
    const int status = inflate(&stream, Z_NO_FLUSH);
    const int ok = stream.total_out == outputByteCount &&
                   (status == Z_OK ||
                    status == Z_STREAM_END ||
                    stream.avail_out == 0u);
    inflateEnd(&stream);
    return ok;
}

int CGameCtnReplayStaticSolidRawPayloadDecoder::Decode(
        const SNat128 &key,
        const StaticSolidArchivePayload &payloadAsset,
        StaticSolidArchiveRawBytes rawPayload,
        CGameCtnReplayStaticSolidDecodedPayload *decodedOut) {
    if (!rawPayload.IsAvailable() || decodedOut == nullptr) {
        return 0;
    }
    decodedOut->Clear();
    const u32 packedByteCount = payloadAsset.PackedByteCount();
    if (rawPayload.ByteCount() < payloadAsset.RawByteCount()) {
        return 0;
    }
    if (payloadAsset.IsEncrypted() &&
        payloadAsset.RawByteCount() < packedByteCount + 8u) {
        return 0;
    }
    std::vector<unsigned char> packedBytes;
    try {
        packedBytes.resize(packedByteCount);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    if (payloadAsset.IsEncrypted()) {
        auto reader = CClassicBufferCrypted::CreateBlowfishReaderForMemory(
                rawPayload.Bytes(),
                rawPayload.ByteCount(),
                8ul,
                key);
        if (reader == nullptr ||
            reader->Read(
                    packedByteCount != 0u ? packedBytes.data() : nullptr,
                    packedByteCount) != packedByteCount) {
            return 0;
        }
    } else {
        if (rawPayload.ByteCount() < packedByteCount) {
            return 0;
        }
        if (packedByteCount != 0u) {
            packedBytes.assign(rawPayload.Bytes(),
                               rawPayload.Bytes() + packedByteCount);
        }
    }
    if (payloadAsset.IsCompressed()) {
        CGameCtnReplayStaticSolidDecodedPayload decoded;
        if (!decoded.ResizeForDecode(payloadAsset.UncompressedByteCount())) {
            return 0;
        }
        if (!inflate_static_solid_part(
                                       packedByteCount != 0u
                                               ? packedBytes.data()
                                               : nullptr,
                                       packedByteCount,
                                       decoded.MutableBytes(),
                                       payloadAsset.UncompressedByteCount())) {
            return 0;
        }
        decodedOut->TakeFrom(&decoded);
        return 1;
    }
    return decodedOut->CopyFrom(
            packedByteCount != 0u ? packedBytes.data() : nullptr,
            packedByteCount);
}
