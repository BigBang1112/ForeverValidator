#pragma once

#include <cstdint>

enum class VehicleWheelAxle : std::uint8_t {
    Rear,
    Front,
};

constexpr bool IsFrontVehicleWheel(VehicleWheelAxle axle) {
    return axle == VehicleWheelAxle::Front;
}
