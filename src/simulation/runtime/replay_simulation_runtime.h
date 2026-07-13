#ifndef TMNF_REPLAY_SIMULATION_RUNTIME_H
#define TMNF_REPLAY_SIMULATION_RUNTIME_H

#include <memory>
#include <optional>

#include "simulation/control/replay_control_timeline.h"
#include "simulation/runtime/replay_dyna_frame_state.h"
#include "simulation/runtime/replay_simulation_result.h"
class CTrackManiaRace;
class ReplayMapScene;
struct ReplaySimulationDefinition;

struct ReplaySimulationStepExecution {
    ReplaySimulationRunResult result = ReplaySimulationRunResult::Success;
    ReplayDynaFrameState simulatedFrame{};
    ReplayDynaFrameState writeFrame{};
    std::optional<std::uint32_t> finishTickMs;
    std::uint32_t respawnExecutedCount = 0u;
};

class ReplaySimulationRuntime {
public:
    explicit ReplaySimulationRuntime(CTrackManiaRace &race);
    ~ReplaySimulationRuntime();

    ReplaySimulationRuntime(const ReplaySimulationRuntime &) = delete;
    ReplaySimulationRuntime &operator=(const ReplaySimulationRuntime &) = delete;

    ReplaySimulationRunResult Start(
            const ReplaySimulationDefinition &definition,
            ReplayMapScene &mapScene,
            const GmIso4 &spawnLocation,
            const ReplayControlTick &firstTick,
            std::uint32_t validationSeed);
    ReplaySimulationStepExecution Step(
            const ReplayControlTick &tick);
    std::optional<std::uint32_t> FinishTimeMs() const;

private:
    struct State;
    std::unique_ptr<State> state_;
};

#endif
