#include "format/archive/tmnf_gbx_body_reader.h"
#include <stdint.h>

#include "format/archive/archive_binary.h"
using TmnfFormat::ArchiveBinary::ReadU32LE;

int GbxBodyOffsetReader::SkipString(u32 *offset) const {
    if (bytes == nullptr || offset == nullptr || *offset > byteCount ||
        byteCount - *offset < 4u) {
        return 0;
    }
    const u32 length = ReadU32LE(bytes + *offset);
    *offset += 4u;
    if (length > 0x0fffffffu || length > byteCount - *offset) {
        return 0;
    }
    *offset += length;
    return 1;
}

int GbxBodyOffsetReader::SkipSubfolders(u32 *offset, u32 depth) const {
    if (bytes == nullptr || offset == nullptr || depth > 64u ||
        *offset > byteCount || byteCount - *offset < 4u) {
        return 0;
    }
    const u32 count = ReadU32LE(bytes + *offset);
    *offset += 4u;
    if (count > 65536u) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        if (!SkipString(offset) ||
            !SkipSubfolders(offset, depth + 1u)) {
            return 0;
        }
    }
    return 1;
}

int GbxBodyOffsetReader::TryParse(
        const unsigned char *bytes,
        u32 byteCount,
        u32 *classIdOut,
        u32 *bodyOffsetOut) {
    if (bytes == nullptr || classIdOut == nullptr || bodyOffsetOut == nullptr ||
        byteCount < 17u ||
        bytes[0] != 'G' || bytes[1] != 'B' || bytes[2] != 'X') {
        return 0;
    }
    const u32 version = (u32)bytes[3] | ((u32)bytes[4] << 8u);
    const u32 classId = ReadU32LE(bytes + 9u);
    const u32 headerSize = ReadU32LE(bytes + 13u);
    if (headerSize > UINT32_MAX - 17u ||
        17u + headerSize > byteCount ||
        byteCount - (17u + headerSize) < 8u) {
        return 0;
    }
    u32 offset = 17u + headerSize;
    const u32 numNodes = ReadU32LE(bytes + offset);
    offset += 4u;
    const u32 externalCount = ReadU32LE(bytes + offset);
    offset += 4u;
    if (numNodes > 0x100000u || externalCount > 0x100000u) {
        return 0;
    }
    if (externalCount != 0u) {
        if (byteCount - offset < 4u) {
            return 0;
        }
        offset += 4u;
        GbxBodyOffsetReader reader;
        reader.bytes = bytes;
        reader.byteCount = byteCount;
        if (!reader.SkipSubfolders(&offset, 0u)) {
            return 0;
        }
        for (u32 i = 0; i < externalCount; i++) {
            if (byteCount - offset < 4u) {
                return 0;
            }
            const u32 flags = ReadU32LE(bytes + offset);
            offset += 4u;
            if ((flags & 4u) != 0u) {
                if (byteCount - offset < 4u) {
                    return 0;
                }
                offset += 4u;
            } else if (!reader.SkipString(&offset)) {
                return 0;
            }
            if (byteCount - offset < 4u) {
                return 0;
            }
            offset += 4u;
            if (version >= 5u) {
                if (byteCount - offset < 4u) {
                    return 0;
                }
                offset += 4u;
            }
            if ((flags & 4u) == 0u) {
                if (byteCount - offset < 4u) {
                    return 0;
                }
                offset += 4u;
            }
        }
    }
    *classIdOut = classId;
    *bodyOffsetOut = offset;
    return 1;
}
