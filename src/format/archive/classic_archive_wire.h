#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace TmnfFormat::ClassicArchiveWire {

using Natural = std::uint32_t;
using SmallNatural = std::uint16_t;
using NaturalBytes = std::array<std::uint8_t, 4>;
using SmallNaturalBytes = std::array<std::uint8_t, 2>;

inline constexpr std::size_t NaturalByteCount = NaturalBytes{}.size();
inline constexpr std::size_t SmallNaturalByteCount =
        SmallNaturalBytes{}.size();
inline constexpr Natural MaximumStringByteCount = 0x0fffffffu;

static_assert(std::numeric_limits<std::uint8_t>::digits == 8,
              "classic archives require eight-bit bytes");

constexpr NaturalBytes EncodeNatural(Natural value) noexcept {
    return {{
            static_cast<std::uint8_t>(value),
            static_cast<std::uint8_t>(value >> 8u),
            static_cast<std::uint8_t>(value >> 16u),
            static_cast<std::uint8_t>(value >> 24u),
    }};
}

template <typename Byte>
constexpr Natural DecodeNatural(const Byte *bytes) noexcept {
    return static_cast<Natural>(bytes[0]) |
           (static_cast<Natural>(bytes[1]) << 8u) |
           (static_cast<Natural>(bytes[2]) << 16u) |
           (static_cast<Natural>(bytes[3]) << 24u);
}

constexpr SmallNaturalBytes EncodeSmallNatural(
        SmallNatural value) noexcept {
    return {{
            static_cast<std::uint8_t>(value),
            static_cast<std::uint8_t>(value >> 8u),
    }};
}

template <typename Byte>
constexpr SmallNatural DecodeSmallNatural(const Byte *bytes) noexcept {
    return static_cast<SmallNatural>(
            static_cast<SmallNatural>(bytes[0]) |
            (static_cast<SmallNatural>(bytes[1]) << 8u));
}

}  // namespace TmnfFormat::ClassicArchiveWire
