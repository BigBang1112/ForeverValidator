#pragma once

#include <array>
#include <optional>

#include "engine/game/replay_vehicle_simulation_definition.h"
// Typed values decoded from the default vehicle node. Archive routing and
// chunk identifiers stay in the format reader that creates this object.
struct ReplayVehicleSourceDefinition {
    std::optional<VehicleInitialParameters> initialParameters;
    std::array<std::optional<VehicleWheelContactSettings>,
               OfficialVehicleWheelCount> wheelContactSettings;
    std::optional<VehicleMaterialSetDefinition> materials;

    bool HasCompleteWheelContactSettings() const {
        for (const auto &settings : wheelContactSettings) {
            if (!settings.has_value()) {
                return false;
            }
        }
        return true;
    }

    bool HasCompleteMaterialSet() const {
        return materials.has_value() &&
               materials->fakeContactTexture.has_value() &&
               materials->fakeContactTexture->HasExpectedShape();
    }
};
