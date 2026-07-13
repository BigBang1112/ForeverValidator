#include "format/archive/classic_archive_reader.h"
#include <limits>

#include "format/archive/archive_binary32.h"
#include "format/archive/classic_archive_wire.h"
#include "format/archive/mw_id_archive_codec.h"
#include <new>
namespace TmnfFormat {
namespace {

namespace ArchiveWire = ClassicArchiveWire;

}  // namespace

bool ClassicArchiveReader::Open(
        const std::uint8_t *bytes,
        std::size_t byteCount,
        std::size_t maximumStoredIdentifiers) {
    bytes_ = bytes;
    byteCount_ = bytes != nullptr ? byteCount : 0u;
    offset_ = 0u;
    maximumStoredIdentifiers_ = maximumStoredIdentifiers;
    identifierEncoding_ = 0u;
    storedIdentifiers_.clear();
    if (bytes == nullptr && byteCount != 0u) {
        return false;
    }
    try {
        storedIdentifiers_.reserve(maximumStoredIdentifiers);
    } catch (const std::bad_alloc &) {
        maximumStoredIdentifiers_ = 0u;
        return false;
    }
    return true;
}

std::size_t ClassicArchiveReader::Offset() const {
    return offset_;
}

std::size_t ClassicArchiveReader::Remaining() const {
    return offset_ <= byteCount_ ? byteCount_ - offset_ : 0u;
}

bool ClassicArchiveReader::Seek(std::size_t offset) {
    if (offset > byteCount_) {
        return false;
    }
    offset_ = offset;
    return true;
}

bool ClassicArchiveReader::Skip(std::size_t byteCount) {
    if (byteCount > Remaining()) {
        return false;
    }
    offset_ += byteCount;
    return true;
}

bool ClassicArchiveReader::PeekU32(std::uint32_t &value) const {
    if (Remaining() < sizeof(std::uint32_t)) {
        return false;
    }
    value = ArchiveWire::DecodeNatural(bytes_ + offset_);
    return true;
}

bool ClassicArchiveReader::ReadU8(std::uint8_t &value) {
    if (Remaining() < sizeof(std::uint8_t)) {
        return false;
    }
    value = bytes_[offset_++];
    return true;
}

bool ClassicArchiveReader::ReadU32(std::uint32_t &value) {
    if (!PeekU32(value)) {
        return false;
    }
    offset_ += sizeof(std::uint32_t);
    return true;
}

bool ClassicArchiveReader::ReadF32(float &value) {
    std::uint32_t encoded = 0u;
    if (!ReadU32(encoded)) {
        return false;
    }
    value = TmnfArchiveBinary32::Decode(encoded);
    return true;
}

bool ClassicArchiveReader::ReadString(std::string &value) {
    std::uint32_t length = 0u;
    if (!ReadU32(length) ||
        length > ArchiveWire::MaximumStringByteCount ||
        length > Remaining()) {
        return false;
    }
    try {
        value.assign(reinterpret_cast<const char *>(bytes_ + offset_), length);
    } catch (const std::bad_alloc &) {
        value.clear();
        return false;
    }
    offset_ += length;
    return true;
}

bool ClassicArchiveReader::ReadEncodedIdentifier(
        std::uint32_t encodedValue,
        ClassicArchiveIdentifier &identifier) {
    identifier = ClassicArchiveIdentifier{};
    identifier.encodedValue = encodedValue;
    if (identifierEncoding_ == 1u) {
        return encodedValue == 0u || ReadString(identifier.text);
    }
    if (!(CMwIdArchiveCodec::ParseWord((encodedValue)).IsNamed())) {
        return true;
    }
    if (identifierEncoding_ == 2u) {
        return ReadString(identifier.text);
    }
    if (identifierEncoding_ != 3u) {
        return false;
    }

    const std::uint32_t oneBasedIndex =
            CMwIdArchiveCodec::ParseWord(encodedValue).payload;
    if (oneBasedIndex != 0u) {
        if (oneBasedIndex > storedIdentifiers_.size()) {
            return false;
        }
        try {
            identifier.text = storedIdentifiers_[oneBasedIndex - 1u];
        } catch (const std::bad_alloc &) {
            identifier.text.clear();
            return false;
        }
        return true;
    }

    if (storedIdentifiers_.size() >= maximumStoredIdentifiers_ ||
        !ReadString(identifier.text)) {
        return false;
    }
    try {
        storedIdentifiers_.push_back(identifier.text);
    } catch (const std::bad_alloc &) {
        identifier.text.clear();
        return false;
    }
    return true;
}

bool ClassicArchiveReader::ReadIdentifier(
        ClassicArchiveIdentifier &identifier) {
    std::uint32_t encodedValue = 0u;
    if (!ReadU32(encodedValue)) {
        return false;
    }
    if (identifierEncoding_ == 0u &&
        encodedValue >= 1u && encodedValue <= 3u) {
        identifierEncoding_ = encodedValue;
        if (!ReadU32(encodedValue)) {
            return false;
        }
    }
    return ReadEncodedIdentifier(encodedValue, identifier);
}

}  // namespace TmnfFormat
