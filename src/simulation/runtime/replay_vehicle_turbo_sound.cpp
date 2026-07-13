#include "simulation/runtime/replay_vehicle_turbo_sound.h"
void ReplayVehicleTurboSound::Reset() {
    sound.ResetSoundParams();
    sound.playing = false;
    soundSource.AttachSound(sound);
}

void ReplayVehicleTurboSound::Install(CSceneVehicleCar &car) {
    Reset();
    car.BindTurboSound(soundSource, sound);
}
