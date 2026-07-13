#include "format/static_solid/static_solid_archive_payload_decode.h"
#include <string.h>
#include <utility>
#include <new>

void CGameCtnReplayStaticSolidArchiveDecodeStats::Clear() {
    *this = CGameCtnReplayStaticSolidArchiveDecodeStats{};
}

void CGameCtnReplayStaticSolidDecodedPayload::Clear() {
    bytes.clear();
    ready = false;
}

int CGameCtnReplayStaticSolidDecodedPayload::ResizeForDecode(u32 count) {
    Clear();
    try {
        bytes.resize(count);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    ready = true;
    return 1;
}

int CGameCtnReplayStaticSolidDecodedPayload::CopyFrom(
        const unsigned char *source,
        u32 byteCount) {
    if (!ResizeForDecode(byteCount)) {
        return 0;
    }
    if (byteCount != 0u) {
        if (source == nullptr) {
            Clear();
            return 0;
        }
        memcpy(bytes.data(), source, byteCount);
    }
    return 1;
}

int CGameCtnReplayStaticSolidDecodedPayload::TrimToByteCount(u32 count) {
    if (!ready || count > bytes.size()) {
        return 0;
    }
    try {
        bytes.resize(count);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    return 1;
}

void CGameCtnReplayStaticSolidDecodedPayload::TakeFrom(
        CGameCtnReplayStaticSolidDecodedPayload *source) {
    Clear();
    if (source == nullptr) {
        return;
    }
    bytes = std::move(source->bytes);
    ready = source->ready;
    source->bytes.clear();
    source->ready = false;
}

unsigned char *CGameCtnReplayStaticSolidDecodedPayload::MutableBytes() {
    static unsigned char emptyDecodedPayloadByte = 0u;
    return ready ? (bytes.empty() ? &emptyDecodedPayloadByte : bytes.data()) :
                   nullptr;
}

const unsigned char *CGameCtnReplayStaticSolidDecodedPayload::Bytes() const {
    static const unsigned char emptyDecodedPayloadByte = 0u;
    return ready ? (bytes.empty() ? &emptyDecodedPayloadByte : bytes.data()) :
                   nullptr;
}

u32 CGameCtnReplayStaticSolidDecodedPayload::ByteCount() const {
    return ready && bytes.size() <= UINT32_MAX ? (u32)bytes.size() : 0u;
}

int CGameCtnReplayStaticSolidDecodedPayload::IsReady() const {
    return ready;
}

int CGameCtnReplayStaticSolidDecodedPayload::CopyToByteBuffer(
        ByteBuffer *out) const {
    if (out == nullptr || !ready) {
        return 0;
    }
    out->Clear();
    if (!out->Resize(ByteCount())) {
        return 0;
    }
    if (!bytes.empty()) {
        memcpy(out->Data(), bytes.data(), bytes.size());
    }
    return 1;
}
