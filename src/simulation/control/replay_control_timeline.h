#ifndef TMNF_REPLAY_CONTROL_TIMELINE_H
#define TMNF_REPLAY_CONTROL_TIMELINE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "engine/core/gm_types.h"
struct ReplayVehicleControlState {
    float lowSpeedGateA = 0.0f;
    float lowSpeedGateB = 0.0f;
    float steering = 0.0f;
};

enum class ReplayStuntInputAction : std::size_t {
    SteerLeft = 0u,
    SteerRight,
    Steer,
    Accelerate,
    Brake,
    Gas,
    Count,
};

struct ReplayStuntInputState {
    std::array<std::uint32_t,
               static_cast<std::size_t>(ReplayStuntInputAction::Count)>
            lastChangeTimeMs{};
};

struct ReplayRaceTransitionActions {
    bool establishRaceSpawn = false;
    bool suppressVehicleForceCallbacks = false;
    bool enableRaceSimulation = false;
    bool resetAtRaceStart = false;
    bool enableStuntsSimulation = false;
    bool finishRace = false;
    std::uint32_t stuntsTimeLimitMs = 0u;
    std::uint32_t respawnAtCheckpointCount = 0u;
};

struct ReplayControlTick {
    std::uint32_t periodMs = 0u;
    std::uint32_t timeMs = 0u;
    bool observe = false;
    ReplayRaceTransitionActions actions{};
    ReplayVehicleControlState controls{};
    ReplayStuntInputState stuntsInput{};
    std::optional<GmVec3> comparisonTarget;
};

#endif
