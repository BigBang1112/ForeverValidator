#ifndef TMNF_ARCHIVE_BINARY32_H
#define TMNF_ARCHIVE_BINARY32_H

#include <cstring>
#include <cstdint>
#include <limits>

#include "format/archive/classic_archive_wire.h"
namespace TmnfArchiveBinary32 {

static_assert(sizeof(float) == sizeof(uint32_t),
              "TMNF archives require 32-bit float storage");
static_assert(std::numeric_limits<float>::is_iec559,
              "TMNF archives require IEEE-754 binary32 floats");

inline void DecodeInto(uint32_t encoded, float &value) noexcept {
    std::memcpy(&value, &encoded, sizeof(value));
}

inline float Decode(uint32_t encoded) noexcept {
    float value = 0.0f;
    DecodeInto(encoded, value);
    return value;
}

inline uint32_t Encode(const float &value) noexcept {
    uint32_t encoded = 0u;
    std::memcpy(&encoded, &value, sizeof(encoded));
    return encoded;
}

inline uint32_t ReadU32LittleEndian(const unsigned char *bytes) {
    return TmnfFormat::ClassicArchiveWire::DecodeNatural(bytes);
}

inline void ReadLittleEndianInto(const unsigned char *bytes,
                                 float &value) noexcept {
    DecodeInto(ReadU32LittleEndian(bytes), value);
}

inline float ReadLittleEndian(const unsigned char *bytes) noexcept {
    float value = 0.0f;
    ReadLittleEndianInto(bytes, value);
    return value;
}

} // namespace TmnfArchiveBinary32

#endif
