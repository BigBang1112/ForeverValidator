#ifndef TMNF_REPLAY_VEHICLE_COLLISION_DEFINITION_H
#define TMNF_REPLAY_VEHICLE_COLLISION_DEFINITION_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

#include "engine/core/gm_types.h"
#include "engine/game/surface_material.h"
enum class VehicleCollisionRole : std::uint8_t {
    BodyLongitudinal,
    BodyMain,
    BodyRear,
    BodyFront,
    FrontLeftWheel,
    FrontRightWheel,
    RearLeftWheel,
    RearRightWheel,
};

inline constexpr std::array<VehicleCollisionRole, 8u>
        VehicleCollisionRolesInDetectorOrder{
                VehicleCollisionRole::BodyLongitudinal,
                VehicleCollisionRole::BodyMain,
                VehicleCollisionRole::BodyRear,
                VehicleCollisionRole::BodyFront,
                VehicleCollisionRole::FrontLeftWheel,
                VehicleCollisionRole::FrontRightWheel,
                VehicleCollisionRole::RearLeftWheel,
                VehicleCollisionRole::RearRightWheel,
        };

inline constexpr std::size_t VehicleCollisionRoleIndex(
        VehicleCollisionRole role) {
    switch (role) {
        case VehicleCollisionRole::BodyLongitudinal: return 0u;
        case VehicleCollisionRole::BodyMain: return 1u;
        case VehicleCollisionRole::BodyRear: return 2u;
        case VehicleCollisionRole::BodyFront: return 3u;
        case VehicleCollisionRole::FrontLeftWheel: return 4u;
        case VehicleCollisionRole::FrontRightWheel: return 5u;
        case VehicleCollisionRole::RearLeftWheel: return 6u;
        case VehicleCollisionRole::RearRightWheel: return 7u;
    }
    return VehicleCollisionRolesInDetectorOrder.size();
}

inline constexpr bool IsVehicleWheelCollisionRole(VehicleCollisionRole role) {
    return role == VehicleCollisionRole::FrontLeftWheel ||
           role == VehicleCollisionRole::FrontRightWheel ||
           role == VehicleCollisionRole::RearLeftWheel ||
           role == VehicleCollisionRole::RearRightWheel;
}

inline constexpr VehicleCollisionRole VehicleWheelCollisionRole(
        std::size_t wheelIndex) {
    switch (wheelIndex) {
        case 0u: return VehicleCollisionRole::FrontLeftWheel;
        case 1u: return VehicleCollisionRole::FrontRightWheel;
        case 2u: return VehicleCollisionRole::RearRightWheel;
        case 3u: return VehicleCollisionRole::RearLeftWheel;
        default: return VehicleCollisionRole::BodyMain;
    }
}

struct VehicleCollisionShapeDefinition {
    GmIso4 localPose{};
    GmBoxAligned localBounds{};
    GmLocalMaterialIndex localMaterial{};
    EPlugSurfaceMaterialId surfaceMaterial =
            EPlugSurfaceMaterialId_Concrete;
};

class VehicleCollisionModelDefinition {
public:
    bool SetShape(VehicleCollisionRole role,
                  VehicleCollisionShapeDefinition shape) {
        const std::size_t index = VehicleCollisionRoleIndex(role);
        if (index >= shapes.size() || shapes[index].has_value()) {
            return false;
        }
        shapes[index] = std::move(shape);
        return true;
    }

    const VehicleCollisionShapeDefinition *Shape(
            VehicleCollisionRole role) const {
        const std::size_t index = VehicleCollisionRoleIndex(role);
        return index < shapes.size() && shapes[index].has_value()
                ? &*shapes[index]
                : nullptr;
    }

    bool IsComplete() const {
        for (const auto &shape : shapes) {
            if (!shape.has_value()) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<std::optional<VehicleCollisionShapeDefinition>,
               VehicleCollisionRolesInDetectorOrder.size()> shapes{};
};

#endif
