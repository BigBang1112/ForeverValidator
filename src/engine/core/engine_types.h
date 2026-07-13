#pragma once

#include <array>
#include <cstdint>

using u32 = std::uint32_t;

inline constexpr unsigned long InvalidEngineIndex =
        static_cast<unsigned long>(UINT32_MAX);

struct SNat128 {
    std::array<u32, 4> words{};

    constexpr bool IsNull(void) const {
        return words[0] == 0u && words[1] == 0u &&
               words[2] == 0u && words[3] == 0u;
    }
};

constexpr bool operator==(const SNat128 &left, const SNat128 &right) {
    return left.words[0] == right.words[0] &&
           left.words[1] == right.words[1] &&
           left.words[2] == right.words[2] &&
           left.words[3] == right.words[3];
}

constexpr bool operator!=(const SNat128 &left, const SNat128 &right) {
    return !(left == right);
}
