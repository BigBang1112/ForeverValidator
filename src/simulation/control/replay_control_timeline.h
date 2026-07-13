#ifndef TMNF_REPLAY_CONTROL_TIMELINE_H
#define TMNF_REPLAY_CONTROL_TIMELINE_H

#include <cstdint>
#include <optional>

#include "engine/core/gm_types.h"
struct ReplayVehicleControlState {
    float lowSpeedGateA = 0.0f;
    float lowSpeedGateB = 0.0f;
    float steering = 0.0f;
};

struct ReplayRaceTransitionActions {
    bool establishRaceSpawn = false;
    bool suppressVehicleForceCallbacks = false;
    bool enableRaceSimulation = false;
    bool resetAtRaceStart = false;
    std::uint32_t respawnAtCheckpointCount = 0u;
};

struct ReplayControlTick {
    std::uint32_t periodMs = 0u;
    std::uint32_t timeMs = 0u;
    bool observe = false;
    ReplayRaceTransitionActions actions{};
    ReplayVehicleControlState controls{};
    std::optional<GmVec3> comparisonTarget;
};

#endif
