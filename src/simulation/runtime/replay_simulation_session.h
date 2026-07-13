#ifndef TMNF_REPLAY_SIMULATION_SESSION_H
#define TMNF_REPLAY_SIMULATION_SESSION_H

#include <memory>
#include <vector>

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

    ReplaySimulationTimelineResult SimulateTimeline(
            const ReplaySimulationDefinition &simulationDefinition,
            const std::vector<ReplayControlTick> &controlTicks,
            std::uint32_t validationSeed);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
