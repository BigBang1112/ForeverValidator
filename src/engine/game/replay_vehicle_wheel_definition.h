#ifndef TMNF_REPLAY_VEHICLE_WHEEL_DEFINITION_H
#define TMNF_REPLAY_VEHICLE_WHEEL_DEFINITION_H

#include <array>
#include <cstddef>
#include <optional>

#include "engine/game/replay_vehicle_collision_definition.h"
#include "engine/game/vehicle_wheel_axle.h"
#include "engine/game/vehicle_wheel_surface_id.h"
inline constexpr std::size_t OfficialVehicleWheelCount = 4u;

struct VehicleWheelContactSettings {
    VehicleWheelSurfaceId surfaceId = VehicleWheelSurfaceId::Invalid;
    bool killsLateralSpeedOnContact = false;
    VehicleWheelAxle axle = VehicleWheelAxle::Rear;
};

struct VehicleWheelDefinition {
    VehicleWheelSurfaceId surfaceId = VehicleWheelSurfaceId::Invalid;
    bool killsLateralSpeedOnContact = false;
    VehicleWheelAxle axle = VehicleWheelAxle::Rear;
    float rollingRadius = 0.0f;
    GmVec3 forceApplicationPoint{};
    GmVec3 initialSurfacePoint{};
    GmIso4 restSurfacePose{};
    VehicleCollisionRole collisionRole = VehicleCollisionRole::BodyMain;
};

struct VehicleWheelSetDefinition {
    static constexpr std::size_t OfficialWheelCount =
            OfficialVehicleWheelCount;

    // Canonical order: front-left, front-right, rear-right, rear-left.
    std::array<VehicleWheelDefinition, OfficialWheelCount> wheels{};
    float restDamperAbsorb = 0.0f;

    bool IsComplete() const;
};

struct ReplayVehicleSolidDefinition;
struct ReplayVehicleSourceDefinition;
struct ReplayVehicleTuningDefinition;

std::optional<VehicleWheelSetDefinition> BuildVehicleWheelSetDefinition(
        const ReplayVehicleSolidDefinition &solid,
        const ReplayVehicleTuningDefinition &tuning,
        const ReplayVehicleSourceDefinition &vehicle);

#endif
