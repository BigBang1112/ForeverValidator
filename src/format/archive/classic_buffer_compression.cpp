#include "engine/core/classic_buffer.h"
#include "format/archive/classic_archive_wire.h"
#include "format/compression/lzo1x.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

namespace ArchiveWire = TmnfFormat::ClassicArchiveWire;

constexpr std::uint32_t MaximumUncompressedBlockSize = 0x10000000u;
constexpr std::uint32_t LargestUncompressedBlock =
        MaximumUncompressedBlockSize - 1u;
constexpr std::uint32_t MaximumCompressedBlockSize =
        LargestUncompressedBlock + (LargestUncompressedBlock >> 6u) + 19u;

struct CompressedBlockHeader {
    std::uint32_t uncompressedByteCount = 0u;
    std::uint32_t compressedByteCount = 0u;
};

bool WriteWireNatural(CClassicBuffer &buffer, std::uint32_t value) {
    const ArchiveWire::NaturalBytes bytes =
            ArchiveWire::EncodeNatural(value);
    return buffer.WriteAll(bytes.data(), bytes.size()) != 0;
}

bool ReadWireNatural(
        CClassicBuffer &buffer,
        std::uint32_t &valueOut) {
    ArchiveWire::NaturalBytes bytes{};
    if (buffer.ReadAll(bytes.data(), bytes.size()) == 0) {
        return false;
    }
    valueOut = ArchiveWire::DecodeNatural(bytes.data());
    return true;
}

bool WriteCompressedBlockHeader(
        CClassicBuffer &buffer,
        const CompressedBlockHeader &header) {
    return WriteWireNatural(buffer, header.uncompressedByteCount) &&
           WriteWireNatural(buffer, header.compressedByteCount);
}

bool ReadCompressedBlockHeader(
        CClassicBuffer &buffer,
        CompressedBlockHeader &header) {
    return ReadWireNatural(buffer, header.uncompressedByteCount) &&
           ReadWireNatural(buffer, header.compressedByteCount);
}

bool ReadCompressedPayload(
        CClassicBuffer &buffer,
        std::uint32_t compressedSize,
        std::vector<unsigned char> &compressedOut) {
    if (compressedSize == 0u ||
        compressedSize > MaximumCompressedBlockSize) {
        return false;
    }

    if (buffer.IsSeekable() != 0) {
        const unsigned long position = buffer.GetCurOffset();
        const unsigned long size = buffer.GetActualSize();
        if (position > size || compressedSize > size - position) {
            return false;
        }
    }

    std::array<unsigned char, 4096> chunk{};
    std::uint32_t remaining = compressedSize;
    while (remaining != 0u) {
        const unsigned long batch = std::min<unsigned long>(
                remaining,
                static_cast<unsigned long>(chunk.size()));
        if (buffer.ReadAll(chunk.data(), batch) == 0) {
            return false;
        }
        compressedOut.insert(
                compressedOut.end(),
                chunk.begin(),
                chunk.begin() + batch);
        remaining -= static_cast<std::uint32_t>(batch);
    }
    return true;
}

}  // namespace

void CClassicBuffer::AddCompressedBlock(
        const CClassicBufferMemory &memoryBuffer) {
    const unsigned long sourceSize = memoryBuffer.GetActualSize();
    if (sourceSize > std::numeric_limits<std::uint32_t>::max()) {
        return;
    }

    const std::uint32_t sourceByteCount =
            static_cast<std::uint32_t>(sourceSize);
    const std::uint64_t capacity64 =
            static_cast<std::uint64_t>(sourceByteCount) +
            (sourceByteCount >> 6u) + 19u;
    if (capacity64 > std::numeric_limits<std::uint32_t>::max()) {
        return;
    }

    std::vector<unsigned char> compressed(
            static_cast<std::size_t>(capacity64));
    std::size_t compressedByteCount = compressed.size();
    Lzo1xDictionary dictionary;
    const int compressionResult = lzo1x_1_compress(
            memoryBuffer.Data(),
            sourceByteCount,
            compressed.data(),
            &compressedByteCount,
            dictionary);
    if (compressionResult != LZO_E_OK ||
        compressedByteCount > std::numeric_limits<std::uint32_t>::max()) {
        return;
    }

    const CompressedBlockHeader header{
            sourceByteCount,
            static_cast<std::uint32_t>(compressedByteCount),
    };
    if (!WriteCompressedBlockHeader(*this, header)) {
        return;
    }
    (void)WriteAll(compressed.data(), header.compressedByteCount);
}

CClassicBufferMemory *CClassicBuffer::CreateUncompressedBlock(void) {
    CompressedBlockHeader header;
    if (!ReadCompressedBlockHeader(*this, header) ||
        header.uncompressedByteCount >= MaximumUncompressedBlockSize) {
        return nullptr;
    }

    std::vector<unsigned char> compressed;
    if (!ReadCompressedPayload(
                *this, header.compressedByteCount, compressed)) {
        return nullptr;
    }

    std::vector<unsigned char> decompressed(header.uncompressedByteCount);
    unsigned char emptyOutput = 0u;
    unsigned char *output = decompressed.empty()
            ? &emptyOutput
            : decompressed.data();
    std::size_t decompressedByteCount = header.uncompressedByteCount;
    const int decompressionResult = lzo1x_decompress_safe(
            compressed.data(),
            compressed.size(),
            output,
            &decompressedByteCount);
    if (decompressionResult != LZO_E_OK ||
        decompressedByteCount > header.uncompressedByteCount) {
        return nullptr;
    }

    CClassicBufferMemory *result = sBufferMemoryGetNew();
    if (result == nullptr) {
        return nullptr;
    }
    result->Empty();
    result->PreAlloc(header.uncompressedByteCount);
    if (decompressedByteCount != 0u &&
        result->Write(decompressed.data(), decompressedByteCount) !=
                decompressedByteCount) {
        sBufferMemoryFree(result);
        return nullptr;
    }
    result->Reset();
    return result;
}
