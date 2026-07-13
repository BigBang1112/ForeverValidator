#pragma once

#include <array>
#include <optional>

#include "engine/game/replay_vehicle_collision_definition.h"
#include "engine/game/replay_vehicle_wheel_definition.h"
// Semantic collision geometry and wheel placement decoded from the vehicle
// solid. This type deliberately has no archive offsets or node identities.
struct ReplayVehicleSolidDefinition {
    std::array<std::optional<VehicleWheelDefinition>,
               OfficialVehicleWheelCount> wheels;
    VehicleCollisionModelDefinition collisionModel;

    bool HasCompleteWheelSet() const {
        for (const auto &wheel : wheels) {
            if (!wheel.has_value()) {
                return false;
            }
        }
        return true;
    }

    bool IsComplete() const {
        return HasCompleteWheelSet() && collisionModel.IsComplete();
    }
};
