#ifndef TMNF_GBX_ARCHIVE_H
#define TMNF_GBX_ARCHIVE_H

#include <stdint.h>

constexpr uint32_t TmnfGbxArchiveMagic =
        static_cast<uint32_t>("GBX"[0]) |
        (static_cast<uint32_t>("GBX"[1]) << 8) |
        (static_cast<uint32_t>("GBX"[2]) << 16);
constexpr uint32_t TmnfGbxArchiveVersion6 = 6u;
constexpr uint32_t TmnfGbxArchiveFlagBinary = 0x42u;
constexpr uint32_t TmnfGbxArchiveFlagUncompressed = 0x55u;
constexpr uint32_t TmnfGbxArchiveFlagCompressed = 0x43u;
constexpr uint32_t TmnfGbxArchiveFlagReference = 0x52u;
constexpr uint32_t TmnfGbxArchiveHeaderExternalMarker = 0x80000000u;
constexpr uint32_t TmnfGbxArchiveHeaderLengthMask = 0x7fffffffu;
constexpr uint32_t TmnfGbxArchiveMaxHeaderChunks = 0x1000u;

#endif
