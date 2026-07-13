#ifndef TMNF_REPLAY_VEHICLE_DYNA_DEFINITION_H
#define TMNF_REPLAY_VEHICLE_DYNA_DEFINITION_H

#include <optional>

#include "engine/physics/dynamics/replay_dyna_parameters.h"
#include "engine/game/replay_vehicle_solid_definition.h"
#include "engine/game/replay_vehicle_tuning_definition.h"
std::optional<ReplayDynaParameters> BuildVehicleDynaDefinition(
        const ReplayVehicleTuningDefinition &tuning,
        const ReplayVehicleSolidDefinition &solid);

#endif
