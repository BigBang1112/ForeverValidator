#include "engine/core/fast_algo.h"
#include <cstdint>
#include <type_traits>

namespace {

bool IsPrime(unsigned long value) {
    if (value < 2u) {
        return false;
    }
    if ((value & 1u) == 0u) {
        return value == 2u;
    }
    for (unsigned long divisor = 3u;
         divisor <= value / divisor;
         divisor += 2u) {
        if (value % divisor == 0u) {
            return false;
        }
    }
    return true;
}

template <typename Character>
unsigned long HashText(const Character *text, unsigned long *lengthOut) {
    std::uint32_t hash = 0u;
    unsigned long length = 0u;
    if (text != nullptr) {
        while (text[length] != Character{}) {
            using SignedCharacter = std::make_signed_t<Character>;
            hash = (hash << 4u) + static_cast<std::uint32_t>(
                    static_cast<SignedCharacter>(text[length]));
            const std::uint32_t high = hash & 0xf0000000u;
            if (high != 0u) {
                hash = (hash & 0x0fffffffu) ^ (high >> 24u);
            }
            ++length;
        }
    }
    if (lengthOut != nullptr) {
        *lengthOut = length;
    }
    return hash;
}

}  // namespace

unsigned long CFastAlgo::ComputeHashSize(unsigned long value) {
    unsigned long candidate = value <= 8u
            ? 11u
            : ((value * 4u / 3u) | 1u);
    while (!IsPrime(candidate)) {
        candidate += 2u;
    }
    return candidate;
}

unsigned long CFastAlgo::ComputeHashVal(
        const char *text,
        unsigned long *lengthOut) {
    return HashText(text, lengthOut);
}

unsigned long CFastAlgo::ComputeHashVal(
        const wchar_t *text,
        unsigned long *lengthOut) {
    return HashText(text, lengthOut);
}
