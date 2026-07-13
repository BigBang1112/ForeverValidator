#ifndef TMNF_STATIC_SOLID_ARCHIVE_BYTE_STREAM_H
#define TMNF_STATIC_SOLID_ARCHIVE_BYTE_STREAM_H

#include "engine/core/engine_types.h"
#include <stddef.h>

#include <array>
#include <string>
#include <vector>
#include <zlib.h>

#include "format/static_solid/static_scene_archive_loader.h"
class CClassicBufferCrypted;
struct CGameCtnReplayStaticSolidArchiveFeedback;

class CGameCtnReplayStaticSolidArchiveByteStream {
public:
    ~CGameCtnReplayStaticSolidArchiveByteStream();

    int OpenDecodedPayload(
            const StaticSolidArchivePayload *payloadAsset,
            const unsigned char *decodedBytes,
            u32 decodedByteCount);
    int OpenEncryptedFeedbackPayload(
            const StaticSolidArchivePayload *payloadAsset,
            CClassicBufferCrypted *reader,
            unsigned char *decodedBytes,
            CGameCtnReplayStaticSolidArchiveFeedback *feedbackState);
    void Close();

    int ApplyFeedbackValue(
            CGameCtnReplayStaticSolidArchiveFeedback &feedback,
            u32 value,
            int nested);
    int ApplyFeedbackFloat(
            CGameCtnReplayStaticSolidArchiveFeedback &feedback,
            const float &value,
            int nested);

    int Read(unsigned char *out, u32 byteCount);
    int Skip(u32 byteCount);
    int SkipCounted(u32 count, u32 stride);
    int Ensure(u32 targetOffset);
    int ReadU32(u32 *valueOut);
    int ReadU16(u32 *valueOut);
    int ReadF32(float *valueOut);
    int ReadBool(u32 *valueOut);
    int ReadStringFixed(char *out, size_t outSize);
    int ReadStringSkip();
    int ReadStringOwned(std::string *out);

    int PeekU32At(u32 cursor, u32 *valueOut);
    int PeekU32(u32 *valueOut);
    int SetOffset(u32 newOffset);

    const StaticSolidArchivePayload *PayloadAsset() const;
    u32 PayloadUncompressedSize() const;
    u32 Offset() const;
    u32 Produced() const;
    u32 CompressedRead() const;
    int Failed() const;

private:
    static constexpr u32 OutputWindowCapacity = 0x400u;
    static constexpr u32 EncryptedPageCapacity = 0x100u;

    const StaticSolidArchivePayload *activePayload = nullptr;
    CClassicBufferCrypted *encryptedReader = nullptr;
    CGameCtnReplayStaticSolidArchiveFeedback *feedback = nullptr;
    z_stream zlib{};
    unsigned char *decodedPayloadBytes = nullptr;
    std::vector<unsigned char> prefetchedCompressed;
    u32 prefetchedCompressedCount = 0u;
    u32 zlibInputOffset = 0u;
    std::array<unsigned char, EncryptedPageCapacity> encryptedInputPage{};
    u32 offset = 0u;
    u32 produced = 0u;
    u32 compressedRead = 0u;
    std::array<unsigned char, OutputWindowCapacity> producedWindow{};
    u32 outputWindowOffset = 0u;
    u32 outputWindowCount = 0u;
    int zlibReady = 0;
    int zlibEof = 0;
    int failed = 0;

    int IsDefaultVehicleTunings() const;
    int PrefetchVehicleCompressedPage();
    int ReadEncryptedCompressedPage();
    int FillWindow();
};

#endif
