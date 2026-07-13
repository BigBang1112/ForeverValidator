#include "format/compression/lzo1x.h"
#include <limits>
#include <stdint.h>
#include <string.h>

static unsigned lzo_get_le16(const unsigned char *p) {
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static size_t lzo1x_1_do_compress(
        size_t src_len,
        const unsigned char *src,
        unsigned char *dst,
        size_t *dst_len,
        Lzo1xDictionary &dictionary) {
    /*
     * Hash four-byte windows into a 0x4000-entry dictionary, emit pending
     * literal runs before each match, then encode the standard LZO1X
     * M2/M3/M4 match forms. Return the remaining literal byte count.
     */
    size_t *dict = dictionary.sourceOffsets.data();
    unsigned char *op = dst;
    const unsigned char *anchor = src;
    const unsigned char *in_end = src + src_len;
    const unsigned char *ip = src + 4u;

    do {
        const size_t ipOffset = static_cast<size_t>(ip - src);
        const unsigned hash =
                ((33u * ((unsigned)ip[0] ^
                         (32u * ((unsigned)ip[1] ^
                                 (32u * ((unsigned)ip[2] ^
                                         ((unsigned)ip[3] << 6u))))))) >> 5u) &
                0x3fffu;
        size_t matchOffset = dict[hash];
        size_t *dict_slot = &dict[hash];
        const unsigned char *match = nullptr;
        size_t distance = 0u;
        size_t distance_minus_one = 0u;

        if (matchOffset == Lzo1xDictionary::NoMatchOffset ||
            matchOffset >= ipOffset) {
            goto no_match;
        }
        match = src + matchOffset;

        distance = ipOffset - matchOffset;
        distance_minus_one = distance - 1u;
        if (distance_minus_one > 0xbffeu) {
            goto no_match;
        }

        if (distance > 0x800u && match[3] != ip[3]) {
            const unsigned hash2 =
                    (((33u * ((unsigned)ip[0] ^
                              (32u * ((unsigned)ip[1] ^
                                      (32u * ((unsigned)ip[2] ^
                                              ((unsigned)ip[3] << 6u))))))) >> 5u) &
                     0x7ffu) ^
                    0x201fu;
            matchOffset = dict[hash2];
            dict_slot = &dict[hash2];
            if (matchOffset == Lzo1xDictionary::NoMatchOffset ||
                matchOffset >= ipOffset) {
                goto no_match;
            }
            match = src + matchOffset;
            distance = ipOffset - matchOffset;
            distance_minus_one = distance - 1u;
            if (distance_minus_one > 0xbffeu ||
                (distance > 0x800u && match[3] != ip[3])) {
                goto no_match;
            }
        }

        if (match[0] == ip[0] &&
            match[1] == ip[1] &&
            match[2] == ip[2]) {
            const unsigned char *literal = anchor;
            size_t literal_len = (size_t)(ip - anchor);
            *dict_slot = static_cast<size_t>(ip - src);

            if (literal_len != 0u) {
                if (literal_len > 3u) {
                    unsigned char token;
                    if (literal_len > 0x12u) {
                        size_t zero_count;
                        token = (unsigned char)(literal_len - 18u);
                        *op++ = 0u;
                        if (literal_len - 18u > 0xffu) {
                            zero_count = (literal_len - 274u) / 0xffu + 1u;
                            memset(op, 0, zero_count);
                            op += zero_count;
                            do {
                                token++;
                                zero_count--;
                            } while (zero_count != 0u);
                        }
                    } else {
                        token = (unsigned char)(literal_len - 3u);
                    }
                    *op++ = token;
                } else {
                    op[-2] |= (unsigned char)literal_len;
                }
                do {
                    *op++ = *literal++;
                    literal_len--;
                } while (literal_len != 0u);
                anchor = literal;
            }

            const unsigned char fourth = ip[3];
            ip += 4u;
            if (match[3] != fourth) {
                goto short_match;
            }
            if (match[4] != *ip++) {
                goto short_match;
            }
            if (match[5] != *ip++ ||
                match[6] != *ip++ ||
                match[7] != *ip++ ||
                match[8] != *ip++) {
short_match:
                ip--;
                const size_t match_len = (size_t)(ip - anchor);
                if (distance <= 0x800u) {
                    *op++ = (unsigned char)(
                            4u * ((distance_minus_one & 7u) |
                                  (8u * (match_len - 1u))));
                    *op++ = (unsigned char)(distance_minus_one >> 3u);
                    anchor = ip;
                    continue;
                }

                const size_t code_len = match_len - 2u;
                size_t distance_code;
                if (distance > 0x4000u) {
                    distance_code = distance - 0x4000u;
                    *op++ = (unsigned char)(
                            code_len |
                            ((distance_code >> 11u) & 8u) |
                            0x10u);
                } else {
                    distance_code = distance_minus_one;
                    *op++ = (unsigned char)(code_len | 0x20u);
                }
                *op++ = (unsigned char)(4u * distance_code);
                *op++ = (unsigned char)(distance_code >> 6u);
                anchor = ip;
                continue;
            }

            const unsigned char *scan = match + 9u;
            while (ip < in_end && *scan == *ip) {
                scan++;
                ip++;
            }

            const size_t match_len = (size_t)(ip - anchor);
            size_t len_code;
            size_t distance_code;
            if (distance > 0x4000u) {
                distance_code = distance - 0x4000u;
                if (match_len <= 9u) {
                    *op++ = (unsigned char)(
                            (match_len - 2u) |
                            ((distance_code >> 11u) & 8u) |
                            0x10u);
                    *op++ = (unsigned char)(4u * distance_code);
                    *op++ = (unsigned char)(distance_code >> 6u);
                    anchor = ip;
                    continue;
                }
                len_code = match_len - 9u;
                *op++ = (unsigned char)(((distance_code >> 11u) & 8u) | 0x10u);
            } else {
                distance_code = distance_minus_one;
                if (match_len <= 0x21u) {
                    *op++ = (unsigned char)((match_len - 2u) | 0x20u);
                    *op++ = (unsigned char)(4u * distance_code);
                    *op++ = (unsigned char)(distance_code >> 6u);
                    anchor = ip;
                    continue;
                }
                len_code = match_len - 33u;
                *op++ = 32u;
            }

            if (len_code > 0xffu) {
                size_t zero_count = (len_code - 256u) / 0xffu + 1u;
                memset(op, 0, zero_count);
                op += zero_count;
                do {
                    len_code = (unsigned char)(len_code + 1u);
                    zero_count--;
                } while (zero_count != 0u);
            }
            *op++ = (unsigned char)len_code;
            *op++ = (unsigned char)(4u * distance_code);
            *op++ = (unsigned char)(distance_code >> 6u);
            anchor = ip;
            continue;
        }

no_match:
        *dict_slot = static_cast<size_t>(ip - src);
        ip++;
    } while (ip < in_end - 13u);

    *dst_len = (size_t)(op - dst);
    return (size_t)(in_end - anchor);
}

int lzo1x_1_compress(
        const unsigned char *src,
        size_t src_len,
        unsigned char *dst,
        size_t *dst_len,
        Lzo1xDictionary &dictionary) {
    /*
     * for inputs above 13 bytes, emit the final literal run, then append the
     * 0x11 0x00 0x00 end marker.
     */
    if ((src == nullptr && src_len != 0u) ||
        dst == nullptr || dst_len == nullptr) {
        if (dst_len != nullptr) {
            *dst_len = 0u;
        }
        return LZO_E_INPUT_OVERRUN;
    }

    unsigned char *out_start = dst;
    unsigned char *op = dst;
    size_t tail_len = src_len;
    if (src_len > 0x0du) {
        dictionary.Reset();
        tail_len = lzo1x_1_do_compress(
                src_len,
                src,
                dst,
                dst_len,
                dictionary);
        op = dst + *dst_len;
    }

    if (tail_len != 0u) {
        const unsigned char *tail = src + src_len - tail_len;
        if (op == out_start && tail_len <= 0xeeu) {
            *op++ = (unsigned char)(tail_len + 17u);
        } else {
            if (tail_len <= 3u) {
                op[-2] |= (unsigned char)tail_len;
                goto copy_tail;
            }
            unsigned char token;
            if (tail_len > 0x12u) {
                token = (unsigned char)(tail_len - 18u);
                *op++ = 0u;
                if (tail_len - 18u > 0xffu) {
                    size_t zero_count = (tail_len - 274u) / 0xffu + 1u;
                    memset(op, 0, zero_count);
                    op += zero_count;
                    do {
                        token++;
                        zero_count--;
                    } while (zero_count != 0u);
                }
            } else {
                token = (unsigned char)(tail_len - 3u);
            }
            *op++ = token;
        }
        do {
copy_tail:
            *op++ = *tail++;
            tail_len--;
        } while (tail_len != 0u);
    }

    *op++ = 17u;
    *op++ = 0u;
    *op++ = 0u;
    *dst_len = (size_t)(op - out_start);
    return LZO_E_OK;
}

static int lzo_copy_literals_before_token(
        const unsigned char *src,
        size_t src_len,
        size_t *inputOffset,
        unsigned char *dst,
        size_t dst_capacity,
        size_t *outputOffset,
        size_t count) {
    if (*outputOffset > dst_capacity || count > dst_capacity - *outputOffset) {
        return LZO_E_OUTPUT_OVERRUN;
    }
    if (*inputOffset > src_len || count >= src_len - *inputOffset) {
        return LZO_E_INPUT_OVERRUN;
    }
    if (count != 0u) {
        memcpy(dst + *outputOffset, src + *inputOffset, count);
    }
    *inputOffset += count;
    *outputOffset += count;
    return LZO_E_OK;
}

static int lzo_copy_match(
        unsigned char *dst,
        size_t dst_capacity,
        size_t *outputOffset,
        size_t distance,
        size_t count) {
    if (distance == 0u || distance > *outputOffset) {
        return LZO_E_LOOKBEHIND_OVERRUN;
    }
    if (*outputOffset > dst_capacity || count > dst_capacity - *outputOffset) {
        return LZO_E_OUTPUT_OVERRUN;
    }
    while (count-- != 0u) {
        dst[*outputOffset] = dst[*outputOffset - distance];
        (*outputOffset)++;
    }
    return LZO_E_OK;
}

static int lzo_read_length(
        const unsigned char *src,
        size_t src_len,
        size_t *inputOffset,
        size_t initial,
        size_t base,
        size_t *lengthOut) {
    size_t length = initial;
    if (length == 0u) {
        while (*inputOffset < src_len && src[*inputOffset] == 0u) {
            if (length > std::numeric_limits<size_t>::max() - 255u) {
                return LZO_E_INPUT_OVERRUN;
            }
            length += 255u;
            (*inputOffset)++;
        }
        if (*inputOffset >= src_len) {
            return LZO_E_INPUT_OVERRUN;
        }
        const size_t tail = base + src[*inputOffset];
        (*inputOffset)++;
        if (length > std::numeric_limits<size_t>::max() - tail) {
            return LZO_E_INPUT_OVERRUN;
        }
        length += tail;
    }
    *lengthOut = length;
    return LZO_E_OK;
}

int lzo1x_decompress_safe(
        const unsigned char *src,
        size_t src_len,
        unsigned char *dst,
        size_t *dst_len) {
    if (src == nullptr || dst_len == nullptr ||
        (dst == nullptr && *dst_len != 0u)) {
        return LZO_E_INPUT_OVERRUN;
    }

    const size_t outputCapacity = *dst_len;
    size_t inputOffset = 0u;
    size_t outputOffset = 0u;
    size_t token = 0u;

    enum class DecodeState {
        Main,
        FirstLiteralMatch,
        Match,
        MatchNext,
    };

    if (src_len == 0u) {
        *dst_len = 0u;
        return LZO_E_EOF_NOT_FOUND;
    }

    token = src[inputOffset++];
    DecodeState state = DecodeState::Main;
    if (token > 17u) {
        token -= 17u;
        if (token < 4u) {
            state = DecodeState::MatchNext;
        } else {
            const int result = lzo_copy_literals_before_token(
                    src, src_len, &inputOffset,
                    dst, outputCapacity, &outputOffset, token);
            if (result != LZO_E_OK) {
                *dst_len = outputOffset;
                return result;
            }
            token = src[inputOffset++];
            state = DecodeState::FirstLiteralMatch;
        }
    }

    for (;;) {
        size_t distance = 0u;
        size_t matchLength = 0u;

        if (state == DecodeState::MatchNext) {
            const int result = lzo_copy_literals_before_token(
                    src, src_len, &inputOffset,
                    dst, outputCapacity, &outputOffset, token);
            if (result != LZO_E_OK) {
                *dst_len = outputOffset;
                return result;
            }
            token = src[inputOffset++];
            state = DecodeState::Match;
            continue;
        }

        if (state == DecodeState::Main && token < 16u) {
            if (token == 0u) {
                const int result = lzo_read_length(
                        src, src_len, &inputOffset,
                        token, 15u, &token);
                if (result != LZO_E_OK) {
                    *dst_len = outputOffset;
                    return result;
                }
            }
            if (token > std::numeric_limits<size_t>::max() - 3u) {
                *dst_len = outputOffset;
                return LZO_E_INPUT_OVERRUN;
            }
            const int result = lzo_copy_literals_before_token(
                    src, src_len, &inputOffset,
                    dst, outputCapacity, &outputOffset, token + 3u);
            if (result != LZO_E_OK) {
                *dst_len = outputOffset;
                return result;
            }
            token = src[inputOffset++];
            state = DecodeState::FirstLiteralMatch;
            continue;
        }

        if (state == DecodeState::FirstLiteralMatch && token < 16u) {
            if (inputOffset >= src_len) {
                *dst_len = outputOffset;
                return LZO_E_INPUT_OVERRUN;
            }
            distance = 0x0801u + (token >> 2u) +
                    (static_cast<size_t>(src[inputOffset++]) << 2u);
            const int result = lzo_copy_match(
                    dst, outputCapacity, &outputOffset, distance, 3u);
            if (result != LZO_E_OK) {
                *dst_len = outputOffset;
                return result;
            }
        } else {
            if (token >= 64u) {
                if (inputOffset >= src_len) {
                    *dst_len = outputOffset;
                    return LZO_E_INPUT_OVERRUN;
                }
                distance = 1u + ((token >> 2u) & 7u) +
                        (static_cast<size_t>(src[inputOffset++]) << 3u);
                matchLength = (token >> 5u) + 1u;
            } else if (token >= 32u) {
                token &= 31u;
                int result = lzo_read_length(
                        src, src_len, &inputOffset,
                        token, 31u, &token);
                if (result != LZO_E_OK) {
                    *dst_len = outputOffset;
                    return result;
                }
                if (inputOffset > src_len || src_len - inputOffset < 2u) {
                    *dst_len = outputOffset;
                    return LZO_E_INPUT_OVERRUN;
                }
                distance = 1u + (lzo_get_le16(src + inputOffset) >> 2u);
                inputOffset += 2u;
                if (token > std::numeric_limits<size_t>::max() - 2u) {
                    *dst_len = outputOffset;
                    return LZO_E_INPUT_OVERRUN;
                }
                matchLength = token + 2u;
            } else if (token >= 16u) {
                const size_t distanceHigh = (token & 8u) << 11u;
                token &= 7u;
                int result = lzo_read_length(
                        src, src_len, &inputOffset,
                        token, 7u, &token);
                if (result != LZO_E_OK) {
                    *dst_len = outputOffset;
                    return result;
                }
                if (inputOffset > src_len || src_len - inputOffset < 2u) {
                    *dst_len = outputOffset;
                    return LZO_E_INPUT_OVERRUN;
                }
                const size_t distanceLow =
                        lzo_get_le16(src + inputOffset) >> 2u;
                inputOffset += 2u;
                if (distanceHigh == 0u && distanceLow == 0u) {
                    *dst_len = outputOffset;
                    return inputOffset == src_len
                            ? LZO_E_OK
                            : LZO_E_INPUT_NOT_CONSUMED;
                }
                if (distanceHigh >
                    std::numeric_limits<size_t>::max() - distanceLow -
                            0x4000u) {
                    *dst_len = outputOffset;
                    return LZO_E_LOOKBEHIND_OVERRUN;
                }
                distance = distanceHigh + distanceLow + 0x4000u;
                if (token > std::numeric_limits<size_t>::max() - 2u) {
                    *dst_len = outputOffset;
                    return LZO_E_INPUT_OVERRUN;
                }
                matchLength = token + 2u;
            } else {
                if (inputOffset >= src_len) {
                    *dst_len = outputOffset;
                    return LZO_E_INPUT_OVERRUN;
                }
                distance = 1u + (token >> 2u) +
                        (static_cast<size_t>(src[inputOffset++]) << 2u);
                matchLength = 2u;
            }
            {
                const int result = lzo_copy_match(
                        dst, outputCapacity, &outputOffset,
                        distance, matchLength);
                if (result != LZO_E_OK) {
                    *dst_len = outputOffset;
                    return result;
                }
            }
        }

        if (inputOffset < 2u) {
            *dst_len = outputOffset;
            return LZO_E_INPUT_OVERRUN;
        }
        token = src[inputOffset - 2u] & 3u;
        if (token == 0u) {
            if (inputOffset >= src_len) {
                *dst_len = outputOffset;
                return LZO_E_EOF_NOT_FOUND;
            }
            token = src[inputOffset++];
            state = DecodeState::Main;
            continue;
        }
        state = DecodeState::MatchNext;
    }
}
