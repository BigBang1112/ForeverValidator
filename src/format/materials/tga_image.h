#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace TgaFormat {

struct TrueColorImage {
    std::uint16_t width = 0u;
    std::uint16_t height = 0u;
    std::uint8_t bitsPerPixel = 0u;
    std::vector<std::uint8_t> storedPixels;

    bool HasDimensions(std::uint16_t expectedWidth,
                       std::uint16_t expectedHeight,
                       std::uint8_t expectedBitsPerPixel) const;
};

std::optional<TrueColorImage> DecodeUncompressedTrueColor(
        const std::uint8_t *bytes,
        std::size_t byteCount);

}  // namespace TgaFormat
