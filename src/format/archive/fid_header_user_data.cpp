#include "format/archive/fid_header_user_data.h"
#include <utility>
#include <variant>

#include "format/archive/system_archive_nod.h"
namespace {

constexpr uint32_t HeaderChunkDeferredMask = 0x80000000u;
constexpr uint32_t HeaderChunkByteCountMask = 0x7fffffffu;
constexpr uint32_t HeaderChunkCountLimit = 0x32u;
constexpr uint32_t HeaderUserDataByteCountLimit = 0x200000u;
constexpr size_t HeaderDirectoryByteCount = 4u;
constexpr size_t HeaderChunkRecordByteCount = 8u;
constexpr unsigned char KnownAbsentCacheRecord = 0u;

bool ReadU32Le(
        const unsigned char *source,
        size_t sourceSize,
        size_t offset,
        uint32_t &value) {
    if (source == nullptr || offset > sourceSize || sourceSize - offset < 4u) {
        return false;
    }
    value = static_cast<uint32_t>(source[offset]) |
            (static_cast<uint32_t>(source[offset + 1u]) << 8u) |
            (static_cast<uint32_t>(source[offset + 2u]) << 16u) |
            (static_cast<uint32_t>(source[offset + 3u]) << 24u);
    return true;
}

void AppendU32Le(std::vector<unsigned char> &destination, uint32_t value) {
    destination.push_back(static_cast<unsigned char>(value));
    destination.push_back(static_cast<unsigned char>(value >> 8u));
    destination.push_back(static_cast<unsigned char>(value >> 16u));
    destination.push_back(static_cast<unsigned char>(value >> 24u));
}

}  // namespace

struct CSystemFid::SHeaderUserData::Impl {
    struct RetainedPayload {
        std::vector<unsigned char> bytes;
    };

    struct DeferredPayload {
        uint32_t byteCount = 0u;
    };

    using Payload = std::variant<RetainedPayload, DeferredPayload>;

    struct Chunk {
        uint32_t chunkId = 0u;
        Payload payload;

        bool IsDeferred(void) const {
            return std::holds_alternative<DeferredPayload>(payload);
        }

        size_t ByteCount(void) const {
            if (const RetainedPayload *retained =
                        std::get_if<RetainedPayload>(&payload)) {
                return retained->bytes.size();
            }
            return std::get<DeferredPayload>(payload).byteCount;
        }
    };

    std::vector<Chunk> chunks;
    mutable std::vector<unsigned char> cacheEncoding;
};

CSystemFid::SHeaderUserData::SHeaderUserData(void)
        : impl(std::make_unique<Impl>()) {}

CSystemFid::SHeaderUserData::~SHeaderUserData(void) = default;

void CSystemFid::SHeaderUserDataDeleter::operator()(
        SHeaderUserData *data) const {
    delete data;
}

unsigned long
CSystemFid::SHeaderUserData::ComputeByteSizeTotalInFile(void) const {
    uint64_t byteCount = HeaderDirectoryByteCount +
            HeaderChunkRecordByteCount * impl->chunks.size();
    for (const Impl::Chunk &chunk : impl->chunks) {
        byteCount += chunk.ByteCount();
    }
    return static_cast<unsigned long>(byteCount);
}

unsigned long
CSystemFid::SHeaderUserData::ComputeDataSizeTotalInMem(void) const {
    uint64_t byteCount = 0u;
    for (const Impl::Chunk &chunk : impl->chunks) {
        if (const Impl::RetainedPayload *retained =
                    std::get_if<Impl::RetainedPayload>(&chunk.payload)) {
            byteCount += retained->bytes.size();
        }
    }
    return static_cast<unsigned long>(byteCount);
}

CSystemFid::HeaderUserDataPtr
CSystemFid::SHeaderUserData::DecodeCache(
        const unsigned char *source,
        unsigned long size) {
    if (source == nullptr ||
        size < HeaderDirectoryByteCount ||
        size >= HeaderUserDataByteCountLimit) {
        return nullptr;
    }

    const size_t sourceSize = static_cast<size_t>(size);
    uint32_t chunkCount = 0u;
    if (!ReadU32Le(source, sourceSize, 0u, chunkCount) ||
        chunkCount >= HeaderChunkCountLimit) {
        return nullptr;
    }

    const size_t directoryByteCount = HeaderDirectoryByteCount +
            HeaderChunkRecordByteCount * static_cast<size_t>(chunkCount);
    if (directoryByteCount > sourceSize) {
        return nullptr;
    }

    HeaderUserDataPtr decoded(new SHeaderUserData());
    decoded->impl->chunks.reserve(chunkCount);

    struct CacheChunkDescriptor {
        uint32_t chunkId;
        uint32_t encodedSize;
    };
    std::vector<CacheChunkDescriptor> descriptors;
    descriptors.reserve(chunkCount);

    uint64_t fileByteCount = directoryByteCount;
    for (uint32_t index = 0u; index < chunkCount; ++index) {
        const size_t recordOffset = HeaderDirectoryByteCount +
                HeaderChunkRecordByteCount * static_cast<size_t>(index);
        uint32_t chunkId = 0u;
        uint32_t encodedSize = 0u;
        if (!ReadU32Le(source, sourceSize, recordOffset, chunkId) ||
            !ReadU32Le(source, sourceSize, recordOffset + 4u, encodedSize)) {
            return nullptr;
        }

        const uint32_t payloadByteCount =
                encodedSize & HeaderChunkByteCountMask;
        fileByteCount += payloadByteCount;
        if (fileByteCount >= HeaderUserDataByteCountLimit) {
            return nullptr;
        }
        descriptors.push_back({chunkId, encodedSize});
    }

    size_t payloadOffset = directoryByteCount;
    for (const CacheChunkDescriptor &descriptor : descriptors) {
        const uint32_t payloadByteCount =
                descriptor.encodedSize & HeaderChunkByteCountMask;
        Impl::Chunk chunk;
        chunk.chunkId = descriptor.chunkId;
        if ((descriptor.encodedSize & HeaderChunkDeferredMask) != 0u) {
            chunk.payload = Impl::DeferredPayload{payloadByteCount};
        } else {
            if (payloadOffset > sourceSize ||
                payloadByteCount > sourceSize - payloadOffset) {
                return nullptr;
            }
            chunk.payload = Impl::RetainedPayload{
                    std::vector<unsigned char>(
                            source + payloadOffset,
                            source + payloadOffset + payloadByteCount)};
            payloadOffset += payloadByteCount;
        }
        decoded->impl->chunks.push_back(std::move(chunk));
    }
    if (payloadOffset != sourceSize) {
        return nullptr;
    }
    return decoded;
}

const std::vector<unsigned char> &
CSystemFid::SHeaderUserData::EncodeCache(void) const {
    if (!impl->cacheEncoding.empty()) {
        return impl->cacheEncoding;
    }

    if (impl->chunks.size() >= HeaderChunkCountLimit) {
        return impl->cacheEncoding;
    }
    uint64_t fileByteCount = HeaderDirectoryByteCount +
            HeaderChunkRecordByteCount * impl->chunks.size();
    uint64_t encodedByteCount = fileByteCount;
    for (const Impl::Chunk &chunk : impl->chunks) {
        const size_t payloadByteCount = chunk.ByteCount();
        if (payloadByteCount > HeaderChunkByteCountMask) {
            return impl->cacheEncoding;
        }
        fileByteCount += payloadByteCount;
        if (!chunk.IsDeferred()) {
            encodedByteCount += payloadByteCount;
        }
    }
    if (fileByteCount >= HeaderUserDataByteCountLimit ||
        encodedByteCount >= HeaderUserDataByteCountLimit) {
        return impl->cacheEncoding;
    }

    impl->cacheEncoding.reserve(static_cast<size_t>(encodedByteCount));
    AppendU32Le(
            impl->cacheEncoding,
            static_cast<uint32_t>(impl->chunks.size()));
    for (const Impl::Chunk &chunk : impl->chunks) {
        AppendU32Le(impl->cacheEncoding, chunk.chunkId);
        const uint32_t encodedSize =
                static_cast<uint32_t>(chunk.ByteCount()) |
                (chunk.IsDeferred() ? HeaderChunkDeferredMask : 0u);
        AppendU32Le(impl->cacheEncoding, encodedSize);
    }
    for (const Impl::Chunk &chunk : impl->chunks) {
        const Impl::RetainedPayload *retained =
                std::get_if<Impl::RetainedPayload>(&chunk.payload);
        if (retained != nullptr) {
            impl->cacheEncoding.insert(
                    impl->cacheEncoding.end(),
                    retained->bytes.begin(),
                    retained->bytes.end());
        }
    }
    return impl->cacheEncoding;
}

void CSystemFid::LoadHeaderUserData(CClassicBuffer *buffer) {
    headerUserDataLoaded = false;
    headerUserData.reset();

    CSystemArchiveNod archive;
    archive.fid = this;
    (void)archive.DoFidLoadRefs(CSystemArchiveNod::Archive_Header, buffer);

    if (!headerUserDataLoaded) {
        headerUserDataLoaded = true;
    }
}

void CSystemFid::SetNoHeaderUserDatas(void) {
    headerUserData.reset();
    headerUserDataLoaded = true;
}

void CSystemFid::ResetHeaderUserDatas(void) {
    headerUserData.reset();
    headerUserDataLoaded = false;
    ClearAssetType();
}

void CSystemFid::HeaderUserData_DumpToCache(
        const unsigned char *&outData,
        unsigned long &outSize) const {
    outData = nullptr;
    outSize = 0u;
    if (!headerUserDataLoaded) {
        return;
    }
    if (headerUserData == nullptr) {
        outData = &KnownAbsentCacheRecord;
        return;
    }

    const std::vector<unsigned char> &encoding =
            headerUserData->EncodeCache();
    outData = encoding.data();
    outSize = static_cast<unsigned long>(encoding.size());
}

void CSystemFid::HeaderUserData_RestoreFromCache(
        const unsigned char *source,
        unsigned long size) {
    if (headerUserDataLoaded) {
        return;
    }
    if (size == 0u) {
        if (source != nullptr) {
            headerUserDataLoaded = true;
        }
        return;
    }

    HeaderUserDataPtr decoded = SHeaderUserData::DecodeCache(source, size);
    if (decoded == nullptr) {
        return;
    }
    headerUserData = std::move(decoded);
    headerUserDataLoaded = true;
}
