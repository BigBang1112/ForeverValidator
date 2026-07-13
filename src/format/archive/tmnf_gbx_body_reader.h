#ifndef TMNF_GBX_BODY_READER_H
#define TMNF_GBX_BODY_READER_H

#include "engine/core/engine_types.h"
struct GbxBodyOffsetReader {
    const unsigned char *bytes;
    u32 byteCount;

    int SkipString(u32 *offset) const;
    int SkipSubfolders(u32 *offset, u32 depth) const;
    static int TryParse(
            const unsigned char *bytes,
            u32 byteCount,
            u32 *classIdOut,
            u32 *bodyOffsetOut);
};

#endif
