#ifndef TMNF_REPLAY_VEHICLE_TURBO_SOUND_H
#define TMNF_REPLAY_VEHICLE_TURBO_SOUND_H

#include "engine/physics/dynamics/hms_poc.h"
#include "engine/scene/scene_object.h"
#include "engine/scene/scene_vehicle_car.h"
class ReplayVehicleTurboSound {
public:
    void Reset();
    void Install(CSceneVehicleCar &car);

private:
    CSceneSoundSource soundSource;
    CHmsSoundSource sound;
};

#endif // TMNF_REPLAY_VEHICLE_TURBO_SOUND_H
