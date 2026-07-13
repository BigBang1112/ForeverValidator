#pragma once

#include <cstddef>
#include <cstdint>

enum class VehicleWheelSurfaceId : std::uint8_t {
    Invalid = 0u,
    FrontLeft = 1u,
    FrontRight = 2u,
    RearRight = 3u,
    RearLeft = 4u,
};

constexpr VehicleWheelSurfaceId VehicleWheelSurfaceIdForIndex(
        std::size_t wheelIndex) {
    switch (wheelIndex) {
    case 0u: return VehicleWheelSurfaceId::FrontLeft;
    case 1u: return VehicleWheelSurfaceId::FrontRight;
    case 2u: return VehicleWheelSurfaceId::RearRight;
    case 3u: return VehicleWheelSurfaceId::RearLeft;
    default: return VehicleWheelSurfaceId::Invalid;
    }
}
