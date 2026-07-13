#ifndef TMNF_ARCHIVE_BINARY_H
#define TMNF_ARCHIVE_BINARY_H

#include <cstdint>

namespace TmnfFormat::ArchiveBinary {

inline std::uint16_t ReadU16LE(const unsigned char *bytes) {
    return static_cast<std::uint16_t>(bytes[0]) |
           static_cast<std::uint16_t>(bytes[1]) << 8u;
}

inline std::uint32_t ReadU32LE(const unsigned char *bytes) {
    return static_cast<std::uint32_t>(bytes[0]) |
           static_cast<std::uint32_t>(bytes[1]) << 8u |
           static_cast<std::uint32_t>(bytes[2]) << 16u |
           static_cast<std::uint32_t>(bytes[3]) << 24u;
}

inline std::uint64_t ReadU64LE(const unsigned char *bytes) {
    return static_cast<std::uint64_t>(ReadU32LE(bytes)) |
           static_cast<std::uint64_t>(ReadU32LE(bytes + 4u)) << 32u;
}

}  // namespace TmnfFormat::ArchiveBinary

#endif
