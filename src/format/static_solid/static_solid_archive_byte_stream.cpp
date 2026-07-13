#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/archive/archive_binary.h"
#include <stdint.h>
#include <string.h>

#include "format/archive/classic_buffer_crypted.h"
#include "format/archive/archive_binary32.h"
#include "format/static_solid/static_solid_archive_feedback.h"
#include <new>
static constexpr const char *DefaultVehicleTuningsPackPath =
        "Vehicles\\CarCommon\\Tunings\\Stadium\\C2A.StadiumCar.Gbx";

CGameCtnReplayStaticSolidArchiveByteStream::
        ~CGameCtnReplayStaticSolidArchiveByteStream() {
    Close();
}

void CGameCtnReplayStaticSolidArchiveByteStream::Close() {
    if (zlibReady) {
        inflateEnd(&zlib);
        zlibReady = 0;
    }
}

int CGameCtnReplayStaticSolidArchiveByteStream::OpenDecodedPayload(
        const StaticSolidArchivePayload *payloadAsset,
        const unsigned char *decodedBytes,
        u32 decodedByteCount) {
    Close();
    if (payloadAsset == nullptr || decodedBytes == nullptr) {
        return 0;
    }
    activePayload = payloadAsset;
    feedback = nullptr;
    encryptedReader = nullptr;
    decodedPayloadBytes = (unsigned char *)decodedBytes;
    prefetchedCompressed.clear();
    offset = 0u;
    produced = decodedByteCount;
    compressedRead = 0u;
    outputWindowOffset = 0u;
    outputWindowCount = 0u;
    zlibEof = 1;
    failed = 0;
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::OpenEncryptedFeedbackPayload(
        const StaticSolidArchivePayload *payloadAsset,
        CClassicBufferCrypted *reader,
        unsigned char *decodedBytes,
        CGameCtnReplayStaticSolidArchiveFeedback *feedbackState) {
    Close();
    if (payloadAsset == nullptr || reader == nullptr || decodedBytes == nullptr ||
        feedbackState == nullptr) {
        return 0;
    }
    activePayload = payloadAsset;
    encryptedReader = reader;
    feedback = feedbackState;
    decodedPayloadBytes = decodedBytes;
    offset = 0u;
    produced = 0u;
    compressedRead = 0u;
    prefetchedCompressed.clear();
    prefetchedCompressedCount = 0u;
    zlibInputOffset = 0u;
    outputWindowOffset = 0u;
    outputWindowCount = 0u;
    zlibEof = 0;
    failed = 0;
    zlib = z_stream{};
    if (activePayload->IsCompressed()) {
        if (inflateInit(&zlib) != Z_OK) {
            return 0;
        }
        zlibReady = 1;
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::IsDefaultVehicleTunings()
        const {
    return activePayload != nullptr &&
           activePayload->MatchesSelectedDescriptor(DefaultVehicleTuningsPackPath);
}

int CGameCtnReplayStaticSolidArchiveByteStream::ApplyFeedbackValue(
        CGameCtnReplayStaticSolidArchiveFeedback &feedback,
        u32 value,
        int nested) {
    if (!feedback.ApplyValue(
                encryptedReader,
                value,
                nested,
                IsDefaultVehicleTunings())) {
        failed = 1;
        return 0;
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::ApplyFeedbackFloat(
        CGameCtnReplayStaticSolidArchiveFeedback &feedback,
        const float &value,
        int nested) {
    return ApplyFeedbackValue(
            feedback, TmnfArchiveBinary32::Encode(value), nested);
}

int CGameCtnReplayStaticSolidArchiveByteStream::PrefetchVehicleCompressedPage() {
    if (activePayload == nullptr ||
        encryptedReader == nullptr ||
        !activePayload->IsCompressed() ||
        !IsDefaultVehicleTunings()) {
        return 1;
    }
    if (compressedRead >= activePayload->CompressedByteCount()) {
        return 1;
    }
    if (prefetchedCompressed.empty()) {
        try {
            prefetchedCompressed.resize(
                    activePayload->CompressedByteCount() != 0u
                            ? activePayload->CompressedByteCount()
                            : 1u);
        } catch (const std::bad_alloc &) {
            failed = 1;
            return 0;
        }
    }
    if (feedback == nullptr || !feedback->FlushTo(encryptedReader)) {
        failed = 1;
        return 0;
    }
    u32 readCount = activePayload->CompressedByteCount() - compressedRead;
    if (readCount > EncryptedPageCapacity) {
        readCount = EncryptedPageCapacity;
    }
    if (encryptedReader->Read(
                prefetchedCompressed.data() + compressedRead,
                readCount) != readCount) {
        failed = 1;
        return 0;
    }
    compressedRead += readCount;
    prefetchedCompressedCount = compressedRead;
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::ReadEncryptedCompressedPage() {
    if (activePayload == nullptr ||
        feedback == nullptr ||
        encryptedReader == nullptr ||
        compressedRead >= activePayload->CompressedByteCount()) {
        return 0;
    }
    if (!feedback->FlushTo(encryptedReader)) {
        failed = 1;
        return 0;
    }
    u32 readCount = activePayload->CompressedByteCount() - compressedRead;
    if (readCount > EncryptedPageCapacity) {
        readCount = EncryptedPageCapacity;
    }
    if (encryptedReader->Read(encryptedInputPage.data(),
                              readCount) != readCount) {
        failed = 1;
        return 0;
    }
    compressedRead += readCount;
    zlib.next_in = encryptedInputPage.data();
    zlib.avail_in = readCount;
    return readCount != 0u;
}

int CGameCtnReplayStaticSolidArchiveByteStream::FillWindow() {
    if (failed || activePayload == nullptr) {
        return 0;
    }
    outputWindowOffset = 0u;
    outputWindowCount = 0u;
    if (!activePayload->IsCompressed()) {
        if (compressedRead >= activePayload->UncompressedByteCount()) {
            return 0;
        }
        u32 readCount = activePayload->UncompressedByteCount() - compressedRead;
        if (readCount > 4u) {
            readCount = 4u;
        }
        if (encryptedReader->Read(producedWindow.data(),
                                  readCount) !=
            readCount) {
            failed = 1;
            return 0;
        }
        compressedRead += readCount;
        outputWindowCount = readCount;
        if (compressedRead >= activePayload->UncompressedByteCount()) {
            zlibEof = 1;
        }
        return outputWindowCount != 0u;
    }
    const u32 outputCapacity = OutputWindowCapacity;
    const int isDefaultVehicleTunings = IsDefaultVehicleTunings();
    while (outputWindowCount < outputCapacity && !zlibEof) {
        if (zlib.avail_in == 0u) {
            if (isDefaultVehicleTunings) {
                if (feedback == nullptr) {
                    failed = 1;
                    return 0;
                }
                if (zlibInputOffset >= prefetchedCompressedCount) {
                    if (!PrefetchVehicleCompressedPage() ||
                        zlibInputOffset >= prefetchedCompressedCount) {
                        break;
                    }
                }
                zlib.next_in =
                        prefetchedCompressed.data() + zlibInputOffset;
                zlib.avail_in = prefetchedCompressedCount - zlibInputOffset;
                zlibInputOffset = prefetchedCompressedCount;
            } else {
                if (compressedRead >= activePayload->CompressedByteCount()) {
                    break;
                }
                if (!ReadEncryptedCompressedPage()) {
                    return 0;
                }
            }
        }
        zlib.next_out = producedWindow.data() + outputWindowCount;
        zlib.avail_out = (uInt)(outputCapacity - outputWindowCount);
        const uLong beforeOut = zlib.total_out;
        const int status = inflate(&zlib, Z_SYNC_FLUSH);
        outputWindowCount = (u32)zlib.total_out - produced;
        if (status == Z_STREAM_END) {
            zlibEof = 1;
            break;
        }
        if (status != Z_OK) {
            failed = 1;
            return 0;
        }
        if (zlib.total_out == beforeOut &&
            zlib.avail_in == 0u &&
            compressedRead >= activePayload->CompressedByteCount()) {
            break;
        }
    }
    return outputWindowCount != 0u;
}

int CGameCtnReplayStaticSolidArchiveByteStream::Read(
        unsigned char *out,
        u32 byteCount) {
    if (activePayload == nullptr ||
        decodedPayloadBytes == nullptr ||
        (out == nullptr && byteCount != 0u) ||
        offset > activePayload->UncompressedByteCount() ||
        byteCount > activePayload->UncompressedByteCount() - offset) {
        return 0;
    }
    u32 written = 0u;
    while (written < byteCount) {
        if (offset < produced) {
            u32 take = produced - offset;
            if (take > byteCount - written) {
                take = byteCount - written;
            }
            memcpy(out + written, decodedPayloadBytes + offset, take);
            offset += take;
            written += take;
            continue;
        }
        if (outputWindowOffset >= outputWindowCount && !FillWindow()) {
            return 0;
        }
        u32 take = outputWindowCount - outputWindowOffset;
        if (take > byteCount - written) {
            take = byteCount - written;
        }
        memcpy(out + written,
               producedWindow.data() + outputWindowOffset,
               take);
        memcpy(decodedPayloadBytes + produced,
               producedWindow.data() + outputWindowOffset,
               take);
        outputWindowOffset += take;
        offset += take;
        produced += take;
        written += take;
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::Skip(u32 byteCount) {
    unsigned char scratch[128];
    while (byteCount != 0u) {
        u32 take = byteCount;
        if (take > sizeof(scratch)) {
            take = sizeof(scratch);
        }
        if (!Read(scratch, take)) {
            return 0;
        }
        byteCount -= take;
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::SkipCounted(
        u32 count,
        u32 stride) {
    if (stride != 0u && count > UINT32_MAX / stride) {
        return 0;
    }
    return Skip(count * stride);
}

int CGameCtnReplayStaticSolidArchiveByteStream::Ensure(u32 size) {
    if (activePayload == nullptr || size > activePayload->UncompressedByteCount()) {
        return 0;
    }
    if (size <= produced) {
        return 1;
    }
    const u32 savedOffset = offset;
    offset = produced;
    const int ok = Skip(size - produced);
    offset = savedOffset;
    return ok;
}

int CGameCtnReplayStaticSolidArchiveByteStream::ReadU32(u32 *valueOut) {
    unsigned char bytes[4];
    if (valueOut == nullptr || !Read(bytes, sizeof(bytes))) {
        return 0;
    }
    *valueOut = TmnfFormat::ArchiveBinary::ReadU32LE(bytes);
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::ReadU16(u32 *valueOut) {
    unsigned char bytes[2];
    if (valueOut == nullptr || !Read(bytes, sizeof(bytes))) {
        return 0;
    }
    *valueOut = (u32)bytes[0] | ((u32)bytes[1] << 8u);
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::ReadF32(float *valueOut) {
    u32 bits = 0u;
    if (!ReadU32(&bits)) {
        return 0;
    }
    if (valueOut != nullptr) {
        TmnfArchiveBinary32::DecodeInto(bits, *valueOut);
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::ReadBool(u32 *valueOut) {
    return ReadU32(valueOut);
}

int CGameCtnReplayStaticSolidArchiveByteStream::ReadStringFixed(
        char *out,
        size_t outSize) {
    u32 length = 0u;
    if (out == nullptr || outSize == 0u ||
        !ReadU32(&length) ||
        length >= outSize ||
        length > 0x0fffffffu ||
        !Ensure(offset + length)) {
        return 0;
    }
    memcpy(out, decodedPayloadBytes + offset, length);
    out[length] = '\0';
    offset += length;
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::ReadStringSkip() {
    u32 length = 0u;
    if (!ReadU32(&length) || length > 0x0fffffffu) {
        return 0;
    }
    return Skip(length);
}

int CGameCtnReplayStaticSolidArchiveByteStream::ReadStringOwned(
        std::string *out) {
    u32 length = 0u;
    if (out == nullptr) {
        return 0;
    }
    out->clear();
    if (!ReadU32(&length) ||
        length > 0x0fffffffu ||
        !Ensure(offset + length)) {
        return 0;
    }
    try {
        out->assign((const char *)(decodedPayloadBytes + offset), (size_t)length);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    if (!Skip(length)) {
        out->clear();
        return 0;
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::PeekU32At(
        u32 cursor,
        u32 *valueOut) {
    if (valueOut == nullptr ||
        !Ensure(cursor + 4u)) {
        return 0;
    }
    *valueOut = TmnfFormat::ArchiveBinary::ReadU32LE(
            decodedPayloadBytes + cursor);
    return 1;
}

int CGameCtnReplayStaticSolidArchiveByteStream::PeekU32(u32 *valueOut) {
    return PeekU32At(offset, valueOut);
}

int CGameCtnReplayStaticSolidArchiveByteStream::SetOffset(u32 newOffset) {
    if (activePayload == nullptr || newOffset > activePayload->UncompressedByteCount()) {
        return 0;
    }
    offset = newOffset;
    return 1;
}

const StaticSolidArchivePayload *
CGameCtnReplayStaticSolidArchiveByteStream::PayloadAsset() const {
    return activePayload;
}

u32 CGameCtnReplayStaticSolidArchiveByteStream::PayloadUncompressedSize()
        const {
    return activePayload != nullptr ? activePayload->UncompressedByteCount() : 0u;
}

u32 CGameCtnReplayStaticSolidArchiveByteStream::Offset() const {
    return offset;
}

u32 CGameCtnReplayStaticSolidArchiveByteStream::Produced() const {
    return produced;
}

u32 CGameCtnReplayStaticSolidArchiveByteStream::CompressedRead() const {
    return compressedRead;
}

int CGameCtnReplayStaticSolidArchiveByteStream::Failed() const {
    return failed;
}
