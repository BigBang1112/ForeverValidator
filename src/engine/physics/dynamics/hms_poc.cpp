#include "engine/physics/dynamics/hms_poc.h"
void CHmsSoundSource::ResetSoundParams(void) {
    volume = 1.0f;
    pitch = 1.0f;
    pan = 0.0f;
}

CHmsSoundSource::CHmsSoundSource(void) {
    ResetSoundParams();
}

CHmsSoundSource::~CHmsSoundSource(void) = default;
