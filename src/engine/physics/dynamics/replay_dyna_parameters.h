#ifndef TMNF_REPLAY_DYNA_PARAMETERS_H
#define TMNF_REPLAY_DYNA_PARAMETERS_H

#include "engine/core/gm_types.h"
struct ReplayDynaParameters {
    float mass = 0.0f;
    GmMat3 inverseBodyInertia{};
    float linearDampingScale = 0.0f;
    float angularDampingScale = 0.0f;
    float maxStepDistance = 0.0f;
    float forceScale = 0.0f;
    GmVec3 localCenterOfMass{};
};

#endif // TMNF_REPLAY_DYNA_PARAMETERS_H
