#ifndef TMNF_REPLAY_SIMULATION_SESSION_H
#define TMNF_REPLAY_SIMULATION_SESSION_H

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "engine/game/game_ctn_types.h"
#include "simulation/replay/replay_challenge_construction.h"
#include "simulation/control/replay_control_timeline.h"
#include "simulation/runtime/replay_simulation_definition.h"
#include "simulation/runtime/replay_simulation_result.h"
#include "simulation/runtime/replay_trajectory_observation.h"
#include "engine/scene/static_scene_model.h"
struct ReplaySimulationTimelineResult {
    ReplaySimulationRunResult result =
            ReplaySimulationRunResult::InvalidControlTimeline;
    std::vector<ReplayTrajectoryObservation> observations;
    bool raceCompleted = false;
    std::optional<std::uint32_t> finishTimeMs;
    std::optional<std::uint32_t> stuntsScore;
    std::uint32_t executedRespawnCount = 0u;
};

class ReplaySimulationSession {
public:
    ReplaySimulationSession();
    ~ReplaySimulationSession();

    ReplaySimulationSession(const ReplaySimulationSession &) = delete;
    ReplaySimulationSession &operator=(const ReplaySimulationSession &) = delete;

    void Reset();
    bool PreloadChallenge(CGameCtnChallengeConstruction &construction);
    bool InstallStaticScene(StaticSceneModelCollection models);
    void ActivateStaticScene();
    void ConfigureReplayRace(EChallengePlayMode playMode,
                             bool isLapRace,
                             std::uint32_t lapCount);

    ReplaySimulationTimelineResult SimulateTimeline(
            const ReplaySimulationDefinition &simulationDefinition,
            const std::vector<ReplayControlTick> &controlTicks,
            std::uint32_t validationSeed);
    std::optional<std::uint32_t> ApplyReplayStuntTimePenalty(
            std::uint32_t overtimeMs);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
