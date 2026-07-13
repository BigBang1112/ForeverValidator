#pragma once

#include <cstdint>

struct GxBGRAColor {
    std::uint32_t value = 0u;

    constexpr std::uint8_t Blue(void) const {
        return static_cast<std::uint8_t>(value);
    }
    constexpr std::uint8_t Green(void) const {
        return static_cast<std::uint8_t>(value >> 8u);
    }
    constexpr std::uint8_t Red(void) const {
        return static_cast<std::uint8_t>(value >> 16u);
    }
    constexpr std::uint8_t Alpha(void) const {
        return static_cast<std::uint8_t>(value >> 24u);
    }

    void SetAlpha(std::uint8_t alpha) {
        value = (value & 0x00ffffffu) |
                (static_cast<std::uint32_t>(alpha) << 24u);
    }
};
