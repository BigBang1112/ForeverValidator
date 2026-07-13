#ifndef TMNF_REPLAY_VEHICLE_ASSEMBLY_H
#define TMNF_REPLAY_VEHICLE_ASSEMBLY_H

#include "engine/game/replay_vehicle_simulation_definition.h"
#include "engine/scene/scene_vehicle_car.h"
#include "engine/scene/scene_vehicle_struct.h"
void InstallReplayVehicleInitialParameters(
        const VehicleInitialParameters &parameters,
        CSceneVehicleCar &car);

void InstallReplayVehicleWheels(
        const VehicleWheelSetDefinition &definition,
        CSceneVehicleCar &car,
        CSceneVehicleStruct &vehicleStruct);

#endif // TMNF_REPLAY_VEHICLE_ASSEMBLY_H
