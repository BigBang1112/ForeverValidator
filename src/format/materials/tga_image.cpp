#include "format/materials/tga_image.h"
#include <limits>
#include <new>

namespace TgaFormat {
namespace {

constexpr std::size_t HeaderSize = 18u;
constexpr std::uint8_t UncompressedTrueColor = 2u;

std::uint16_t ReadU16(const std::uint8_t *bytes) {
    return static_cast<std::uint16_t>(bytes[0]) |
           static_cast<std::uint16_t>(bytes[1] << 8u);
}

}  // namespace

bool TrueColorImage::HasDimensions(
        std::uint16_t expectedWidth,
        std::uint16_t expectedHeight,
        std::uint8_t expectedBitsPerPixel) const {
    if (width == 0u || height == 0u ||
        width != expectedWidth || height != expectedHeight ||
        bitsPerPixel != expectedBitsPerPixel || bitsPerPixel % 8u != 0u) {
        return false;
    }
    const std::size_t bytesPerPixel = bitsPerPixel / 8u;
    return height <= std::numeric_limits<std::size_t>::max() / width &&
            static_cast<std::size_t>(width) * height <=
                    std::numeric_limits<std::size_t>::max() / bytesPerPixel &&
            storedPixels.size() ==
                    static_cast<std::size_t>(width) * height * bytesPerPixel;
}

std::optional<TrueColorImage> DecodeUncompressedTrueColor(
        const std::uint8_t *bytes,
        std::size_t byteCount) {
    if (bytes == nullptr || byteCount < HeaderSize || bytes[1] != 0u ||
        bytes[2] != UncompressedTrueColor || bytes[16] == 0u ||
        bytes[16] % 8u != 0u) {
        return std::nullopt;
    }

    TrueColorImage image;
    image.width = ReadU16(bytes + 12u);
    image.height = ReadU16(bytes + 14u);
    image.bitsPerPixel = bytes[16];
    const std::size_t bytesPerPixel = image.bitsPerPixel / 8u;
    if (image.width == 0u || image.height == 0u ||
        image.height > std::numeric_limits<std::size_t>::max() / image.width ||
        static_cast<std::size_t>(image.width) * image.height >
                std::numeric_limits<std::size_t>::max() / bytesPerPixel) {
        return std::nullopt;
    }
    const std::size_t pixelByteCount =
            static_cast<std::size_t>(image.width) * image.height *
            bytesPerPixel;
    const std::size_t pixelOffset = HeaderSize + bytes[0];
    if (pixelOffset > byteCount || pixelByteCount > byteCount - pixelOffset) {
        return std::nullopt;
    }
    try {
        image.storedPixels.assign(bytes + pixelOffset,
                                  bytes + pixelOffset + pixelByteCount);
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
    return image;
}

}  // namespace TgaFormat
