#ifndef TMNF_REPLAY_DYNA_FRAME_STATE_H
#define TMNF_REPLAY_DYNA_FRAME_STATE_H

#include "engine/core/gm_types.h"

struct ReplayDynaFrameState {
    GmQuat rotationQuaternion{};
    GmMat3 rotation{};
    GmVec3 position{};
    GmVec3 linearSpeed{};
    GmVec3 linearCorrectionSpeed{};
    GmVec3 angularSpeed{};
    GmVec3 force{};
    GmVec3 torque{};
    GmMat3 inverseInertiaWorld{};

    GmIso4 Location() const {
        GmIso4 location{};
        location.Set(rotation, position);
        return location;
    }
};

#endif
