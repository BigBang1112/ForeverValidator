#ifndef CCLASSIC_ARCHIVE_CHUNK_INFO_H
#define CCLASSIC_ARCHIVE_CHUNK_INFO_H

#include <stdint.h>

struct CClassicArchiveChunkInfo {
    enum : uint32_t {
        Zero = 0u,
        One = 1u,
        Streamed = 3u,
        SkipWrapped = 0x11u,
        SkipWrappedStop = 0x13u,
        HasSkipPayloadMask = 0x10u,
    };

    explicit CClassicArchiveChunkInfo(uint32_t rawValue)
            : rawValue(rawValue) {}

    uint32_t Raw(void) const { return rawValue; }

    bool HasSkipPayload(void) const {
        return (rawValue & HasSkipPayloadMask) != 0u;
    }

private:
    uint32_t rawValue;
};

#endif
