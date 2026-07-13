#ifndef LZO1X_H
#define LZO1X_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>

constexpr int LZO_E_OK = 0;
constexpr int LZO_E_INPUT_OVERRUN = -4;
constexpr int LZO_E_OUTPUT_OVERRUN = -5;
constexpr int LZO_E_LOOKBEHIND_OVERRUN = -6;
constexpr int LZO_E_EOF_NOT_FOUND = -7;
constexpr int LZO_E_INPUT_NOT_CONSUMED = -8;

struct Lzo1xDictionary {
    static constexpr std::size_t EntryCount = 0x4000u;
    static constexpr std::size_t NoMatchOffset =
            std::numeric_limits<std::size_t>::max();

    std::array<std::size_t, EntryCount> sourceOffsets{};

    void Reset(void) noexcept {
        sourceOffsets.fill(NoMatchOffset);
    }
};

int lzo1x_decompress_safe(
        const unsigned char *src,
        size_t src_len,
        unsigned char *dst,
        size_t *dst_len);

int lzo1x_1_compress(
        const unsigned char *src,
        size_t src_len,
        unsigned char *dst,
        size_t *dst_len,
        Lzo1xDictionary &dictionary);

#endif
