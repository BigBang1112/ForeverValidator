#pragma once

#include "engine/core/cmw_nod.h"
#include "engine/physics/world/hms_zone_element.h"
#include "engine/scene/plug_audio.h"
class CHmsSoundSource : public CHmsPoc {
public:
    CHmsSoundSource(void);
    ~CHmsSoundSource(void) override;

    void ResetSoundParams(void);

    CMwNodRef<CPlugSound> sound;
    float volume = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
    bool playing = false;
};
