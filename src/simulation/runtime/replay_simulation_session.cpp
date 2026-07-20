#include "simulation/runtime/replay_simulation_session.h"
#include <new>
#include <utility>

#include "engine/core/binary32_math.h"
#include "simulation/replay/replay_map_scene.h"
#include "simulation/runtime/replay_environment.h"
#include "simulation/runtime/replay_physics_world.h"
#include "simulation/runtime/replay_simulation_runtime.h"
#include "simulation/runtime/replay_vehicle_body.h"
#include "simulation/runtime/replay_vehicle_simulation.h"
#include "engine/game/trackmania_race.h"
#include "simulation/runtime/replay_deterministic_execution.h"
namespace {

ReplayTrajectoryObservation ObserveReplayTrajectory(
        const ReplaySimulationStepExecution &execution,
        const ReplayControlTick &tick) {
    ReplayTrajectoryObservation observation;
    observation.simulatedPosition = execution.simulatedFrame.position;
    observation.writePosition = execution.writeFrame.position;
    observation.finishTickMs = execution.finishTickMs;
    if (!tick.comparisonTarget.has_value()) {
        return observation;
    }

    ReplayTrajectoryDeviation comparison;
    comparison.targetPosition = *tick.comparisonTarget;
    comparison.delta = {
            observation.writePosition.x - comparison.targetPosition.x,
            observation.writePosition.y - comparison.targetPosition.y,
            observation.writePosition.z - comparison.targetPosition.z};
    const float horizontalDistanceSquared =
            comparison.delta.x * comparison.delta.x +
            comparison.delta.y * comparison.delta.y;
    comparison.distance = CIsqrt(
            horizontalDistanceSquared +
            comparison.delta.z * comparison.delta.z);
    observation.comparison = comparison;
    return observation;
}

}  // namespace

struct ReplaySimulationSession::Impl {
    CTrackManiaRace race;
    ReplayMapScene mapScene;
    std::unique_ptr<ReplaySimulationRuntime> runtime;

    void ResetRuntime() {
        runtime.reset();
    }
};

ReplaySimulationSession::ReplaySimulationSession()
    : impl(std::make_unique<Impl>()) {}

ReplaySimulationSession::~ReplaySimulationSession() = default;

void ReplaySimulationSession::Reset() {
    impl->ResetRuntime();
    impl->mapScene.Reset(impl->race);
}

bool ReplaySimulationSession::PreloadChallenge(
        CGameCtnChallengeConstruction &construction) {
    return impl->mapScene.PreloadChallenge(construction) ==
           ReplayMapSceneResult::Ready;
}

bool ReplaySimulationSession::InstallStaticScene(
        StaticSceneModelCollection models) {
    return impl->mapScene.InstallModels(std::move(models)) ==
           ReplayMapSceneResult::Ready;
}

void ReplaySimulationSession::ActivateStaticScene() {
    impl->mapScene.Activate();
}

void ReplaySimulationSession::ConfigureReplayRace(
        EChallengePlayMode playMode,
        bool isLapRace,
        std::uint32_t lapCount) {
    impl->race.SetReplayChallengePlayMode(playMode);
    impl->race.InitNbLapsAndCheckpoints(
            isLapRace ? lapCount : 1u);
}

ReplaySimulationTimelineResult ReplaySimulationSession::SimulateTimeline(
        const ReplaySimulationDefinition &simulationDefinition,
        const std::vector<ReplayControlTick> &controlTicks,
        std::uint32_t validationSeed) {
    ReplaySimulationTimelineResult result;
    if (controlTicks.empty()) {
        return result;
    }

    if (!tmnf::simulation::DeterministicExecutionScope::IsActive()) {
        result.result =
                ReplaySimulationRunResult::DeterministicExecutionUnavailable;
        return result;
    }
    impl->ResetRuntime();

    const ReplayMapSceneResult readyResult =
            impl->mapScene.EnsureReady(impl->race);
    if (readyResult != ReplayMapSceneResult::Ready) {
        result.result = MapReplaySceneResult(readyResult);
        return result;
    }
    GmIso4 startLocation;
    if (!impl->mapScene.FirstStartLineSpawnLocation(startLocation)) {
        result.result = ReplaySimulationRunResult::MapStartUnavailable;
        return result;
    }

    impl->runtime = std::make_unique<ReplaySimulationRuntime>(impl->race);
    result.result = impl->runtime->Start(
            simulationDefinition,
            impl->mapScene,
            startLocation,
            controlTicks.front(),
            validationSeed);
    if (result.result != ReplaySimulationRunResult::Success) {
        return result;
    }

    for (const ReplayControlTick &tick : controlTicks) {
        const ReplaySimulationStepExecution execution =
                impl->runtime->Step(tick);
        if (execution.result != ReplaySimulationRunResult::Success) {
            result.result = execution.result;
            return result;
        }
        result.executedRespawnCount +=
                execution.respawnExecutedCount;

        if (tick.observe) {
            try {
                result.observations.push_back(
                        ObserveReplayTrajectory(execution, tick));
            } catch (const std::bad_alloc &) {
                result.result =
                        ReplaySimulationRunResult::ObservationAllocationFailed;
                return result;
            }
        }
    }
    result.finishTimeMs = impl->runtime->FinishTimeMs();
    result.stuntsScore = impl->runtime->StuntsScore();
    result.raceCompleted = result.finishTimeMs.has_value();
    result.result = ReplaySimulationRunResult::Success;
    return result;
}

std::optional<std::uint32_t>
ReplaySimulationSession::ApplyReplayStuntTimePenalty(
        std::uint32_t overtimeMs) {
    if (!impl->runtime) {
        return std::nullopt;
    }
    return impl->runtime->ApplyReplayStuntTimePenalty(overtimeMs);
}
