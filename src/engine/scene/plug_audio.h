#pragma once

#include "engine/core/engine_types.h"
#include "engine/core/mw_id.h"
#include "engine/rendering/plug.h"
#include "engine/resources/plug_file.h"
struct CPlugFileImg;

struct CPlugAudio : CPlug {
    CPlugAudio(void);
    ~CPlugAudio(void) override;

private:
    CMwId audioId;
};

struct CPlugSound : CPlugAudio {
    enum class EMode : u32 {
        Spatial = 2u,
    };

    CPlugSound(void);
    ~CPlugSound(void) override;

private:
    CMwNodRef<CPlugFileSnd> fileSnd;
    EMode mode = EMode::Spatial;
    float volume = 0.5f;
    bool looping = true;
    bool continuous = true;
    float priority = 1.0f;
    float referenceDistance = 10.0f;
    float maximumOmnidirectionalDistance = 100000.0f;
    bool dopplerEnabled = true;
    float dopplerFactor = 1.0f;
    u32 soundKind = 0u;
    u32 insideConeAngle = 360u;
    u32 outsideConeAngle = 360u;
    float coneOutsideAttenuation = 1.0f;
    float coneOutsideAttenuationHighFrequency = 1.0f;
    float directAttenuation = 1.0f;
    float directHighFrequencyAttenuation = 1.0f;
    float roomAttenuation = 1.0f;
    float roomHighFrequencyAttenuation = 1.0f;
    float rolloffFactor = 1.0f;
    float roomRolloffFactor = 1.0f;
    float airAbsorptionFactor = 1.0f;
};

struct CPlugSoundVideo : CPlugSound {
    CPlugSoundVideo(void);
    ~CPlugSoundVideo(void) override;
    void SetImage(CPlugFileImg *image);

private:
    CMwNodRef<CPlugFileImg> videoFileImage;
    CMwNodRef<CPlugFileImg> stillImage;
};
