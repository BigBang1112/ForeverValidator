#pragma once

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

struct PlugImageExtent {
    unsigned long width = 0ul;
    unsigned long height = 1ul;
    unsigned long depth = 1ul;

    bool Contains(unsigned long x, unsigned long y,
                  unsigned long z = 0ul) const {
        return x < width && y < height && z < depth;
    }
};

class PlugImagePixels {
public:
    const PlugImageExtent &Extent(void) const { return extent_; }
    unsigned long BytesPerPixel(void) const { return bytesPerPixel_; }

    std::size_t PixelCount(void) const {
        return CheckedProduct(extent_.width, extent_.height, extent_.depth);
    }

    std::size_t RowByteCount(void) const {
        return CheckedProduct(extent_.width, bytesPerPixel_, 1ul);
    }

    bool Empty(void) const { return bytes_.empty(); }

    const std::vector<unsigned char> &Bytes(void) const { return bytes_; }

    void Assign(PlugImageExtent extent,
                unsigned long bytesPerPixel,
                std::vector<unsigned char> bytes) {
        const std::size_t expectedByteCount =
                StorageByteCount(extent, bytesPerPixel);
        if (bytes.size() != expectedByteCount) {
            throw std::invalid_argument(
                    "pixel data does not match its image extent and format");
        }
        extent_ = extent;
        bytesPerPixel_ = bytesPerPixel;
        bytes_ = std::move(bytes);
    }

    unsigned char *Allocate(void) {
        bytes_.assign(StorageByteCount(extent_, bytesPerPixel_), 0u);
        return bytes_.empty() ? nullptr : bytes_.data();
    }

    void Clear(void) { bytes_.clear(); }

    unsigned char *Pixel(unsigned long x,
                         unsigned long y,
                         unsigned long z = 0ul) {
        if (!extent_.Contains(x, y, z) || bytesPerPixel_ == 0ul) {
            return nullptr;
        }
        return &bytes_[PixelByteOffset(x, y, z)];
    }

    const unsigned char *Pixel(unsigned long x,
                               unsigned long y,
                               unsigned long z = 0ul) const {
        if (!extent_.Contains(x, y, z) || bytesPerPixel_ == 0ul) {
            return nullptr;
        }
        return &bytes_[PixelByteOffset(x, y, z)];
    }

    void SetChannel(unsigned long x,
                    unsigned long y,
                    unsigned long channel,
                    unsigned char value) {
        if (channel >= bytesPerPixel_ || !extent_.Contains(x, y)) {
            return;
        }
        bytes_[PixelByteOffset(x, y, 0ul) + channel] = value;
    }

    void ResetFormat(PlugImageExtent extent,
                     unsigned long bytesPerPixel,
                     std::vector<unsigned char> bytes) {
        Assign(extent, bytesPerPixel, std::move(bytes));
    }

    void SetDescription(PlugImageExtent extent,
                        unsigned long bytesPerPixel) {
        if (!bytes_.empty()) {
            throw std::logic_error(
                    "cannot change an allocated image description");
        }
        (void)StorageByteCount(extent, bytesPerPixel);
        extent_ = extent;
        bytesPerPixel_ = bytesPerPixel;
    }

private:
    static std::size_t CheckedProduct(unsigned long first,
                                      unsigned long second,
                                      unsigned long third) {
        std::size_t result = static_cast<std::size_t>(first);
        const auto multiply = [&result](unsigned long factor) {
            if (factor != 0ul &&
                result > std::numeric_limits<std::size_t>::max() /
                                 static_cast<std::size_t>(factor)) {
                throw std::length_error("image pixel storage is too large");
            }
            result *= static_cast<std::size_t>(factor);
        };
        multiply(second);
        multiply(third);
        return result;
    }

    static std::size_t StorageByteCount(PlugImageExtent extent,
                                        unsigned long bytesPerPixel) {
        return CheckedProduct(
                static_cast<unsigned long>(CheckedProduct(
                        extent.width, extent.height, extent.depth)),
                bytesPerPixel,
                1ul);
    }

    std::size_t PixelByteOffset(unsigned long x,
                                unsigned long y,
                                unsigned long z) const {
        const std::size_t pixelIndex =
                (static_cast<std::size_t>(z) * extent_.height + y) *
                        extent_.width +
                x;
        return pixelIndex * bytesPerPixel_;
    }

    PlugImageExtent extent_{};
    unsigned long bytesPerPixel_ = 3ul;
    std::vector<unsigned char> bytes_;
};
