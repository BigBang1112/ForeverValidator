#pragma once

#include "engine/game/replay_vehicle_source_definition.h"
#include "engine/game/replay_vehicle_solid_definition.h"
#include "engine/game/replay_vehicle_tuning_definition.h"
struct ReplayVehicleSourceBundle {
    ReplayVehicleSolidDefinition solid;
    ReplayVehicleTuningDefinition tuning;
    ReplayVehicleSourceDefinition vehicle;

    bool HasRequiredAssets() const {
        return solid.IsComplete() &&
               vehicle.HasCompleteWheelContactSettings() &&
               vehicle.HasCompleteMaterialSet();
    }

    bool IsComplete() const {
        return HasRequiredAssets() && vehicle.initialParameters.has_value();
    }
};
