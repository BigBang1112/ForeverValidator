#include <cstdint>
#include <functional>
#include <limits>
#include <optional>

#include "validation/evaluation/replay_validation_session.h"
#include "simulation/control/replay_control_plan.h"
#include "simulation/runtime/replay_simulation_session.h"
#include "validation/evaluation/replay_validation_evaluation.h"
namespace {

struct ReplayRecordedInputOutcome {
    std::optional<std::int32_t> finishRaceTimeMs;
    std::uint32_t respawnCount = 0u;
};

ReplayRecordedInputOutcome ObserveRecordedInputOutcome(
        const ReplayInputTimeline &timeline,
        std::uint32_t inputTimeBaseMs) {
    ReplayRecordedInputOutcome outcome;
    const std::vector<ReplayInputEvent> &events = timeline.Events();
    for (const ReplayInputEvent &event : events) {
        if (event.action == ReplayInputActionKind::Respawn &&
            event.value.IsActive()) {
            ++outcome.respawnCount;
        }
    }

    const std::optional<std::int32_t> expectedRaceTimeMs =
            timeline.Metadata().raceTimeMs;
    if (!expectedRaceTimeMs.has_value() ||
        *expectedRaceTimeMs < 0 ||
        events.empty()) {
        return outcome;
    }
    const std::uint64_t expectedInputTimeMs =
            static_cast<std::uint64_t>(inputTimeBaseMs) +
            static_cast<std::uint32_t>(*expectedRaceTimeMs);
    if (expectedInputTimeMs >
        std::numeric_limits<std::uint32_t>::max()) {
        return outcome;
    }
    const ReplayInputEvent &finalEvent = events.back();
    if (finalEvent.action == ReplayInputActionKind::FinishLine &&
        finalEvent.value.IsCanonicalPress() &&
        finalEvent.timeMs == expectedInputTimeMs) {
        outcome.finishRaceTimeMs = static_cast<std::int32_t>(
                finalEvent.timeMs - inputTimeBaseMs);
    }
    return outcome;
}

bool ApplyRecordedInputOutcome(
        const ReplayValidationPlan &plan,
        const ReplayRecordedInputOutcome &recorded,
        ReplaySimulationTimelineResult *simulationResult) {
    if (simulationResult == nullptr) {
        return false;
    }
    simulationResult->executedRespawnCount = recorded.respawnCount;
    if (!recorded.finishRaceTimeMs.has_value()) {
        return true;
    }
    const std::uint64_t finishTickMs =
            static_cast<std::uint64_t>(plan.validationPrestartMs) +
            static_cast<std::uint32_t>(*recorded.finishRaceTimeMs);
    if (finishTickMs > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }
    simulationResult->raceCompleted = true;
    simulationResult->finishTimeMs = static_cast<std::uint32_t>(finishTickMs);
    return true;
}

ReplayValidationExecutionResult FromControlPlanResult(
        ReplayControlPlanBuildResult result) {
    switch (result) {
    case ReplayControlPlanBuildResult::Success:
        return ReplayValidationExecutionResult::Success;
    case ReplayControlPlanBuildResult::InvalidRequest:
        return ReplayValidationExecutionResult::ControlPlanInvalidRequest;
    case ReplayControlPlanBuildResult::TargetAllocationFailed:
        return ReplayValidationExecutionResult::ControlTargetAllocationFailed;
    case ReplayControlPlanBuildResult::TargetTimeOutOfRange:
        return ReplayValidationExecutionResult::ControlTargetTimeOutOfRange;
    case ReplayControlPlanBuildResult::NonFiniteTarget:
        return ReplayValidationExecutionResult::ControlTargetNonFinite;
    case ReplayControlPlanBuildResult::TickReservationFailed:
        return ReplayValidationExecutionResult::ControlTickReservationFailed;
    case ReplayControlPlanBuildResult::TickAllocationFailed:
        return ReplayValidationExecutionResult::ControlTickAllocationFailed;
    case ReplayControlPlanBuildResult::MissingComparisonTarget:
        return ReplayValidationExecutionResult::ControlTargetMissing;
    case ReplayControlPlanBuildResult::MissingOutput:
        return ReplayValidationExecutionResult::ControlOutputMissing;
    }
    return ReplayValidationExecutionResult::ControlPlanInvalidRequest;
}

ReplayValidationExecutionResult FromSimulationResult(
        ReplaySimulationRunResult result) {
    switch (result) {
    case ReplaySimulationRunResult::Success:
        return ReplayValidationExecutionResult::Success;
    case ReplaySimulationRunResult::InvalidControlTimeline:
        return ReplayValidationExecutionResult::PhysicsInputInvalid;
    case ReplaySimulationRunResult::MapStartUnavailable:
        return ReplayValidationExecutionResult::MapStartUnavailable;
    case ReplaySimulationRunResult::ObservationAllocationFailed:
        return ReplayValidationExecutionResult::ObservationAllocationFailed;
    case ReplaySimulationRunResult::DeterministicExecutionUnavailable:
        return ReplayValidationExecutionResult::
                DeterministicExecutionUnavailable;
    case ReplaySimulationRunResult::StaticSceneConstructionFailed:
    case ReplaySimulationRunResult::StaticSceneSourcesMissing:
    case ReplaySimulationRunResult::StaticSceneCorpusCountMismatch:
    case ReplaySimulationRunResult::StaticSceneInstallFailed:
    case ReplaySimulationRunResult::VehicleCollisionModelFailed:
        return ReplayValidationExecutionResult::PhysicsInputInvalid;
    }
    return ReplayValidationExecutionResult::PhysicsInputInvalid;
}

}  // namespace

ReplayValidationExecutionResult ExecuteReplayValidation(
        ReplayValidationExecutionOutput *out,
        ReplaySimulationSession &simulationSession,
        const ReplayValidationPlan &plan,
        const ReplaySimulationDefinition &simulationDefinition,
        const ReplayGhostTrajectory &trajectory,
        const ReplayInputTimeline &inputTimeline) {
    if (out == nullptr) {
        return ReplayValidationExecutionResult::MissingInput;
    }
    if (plan.sampleCount != 0u && trajectory.Empty()) {
        return ReplayValidationExecutionResult::MissingInput;
    }
    if (plan.controlTickMs == 0u ||
        plan.expectedSamples >
                std::numeric_limits<std::uint32_t>::max()) {
        return ReplayValidationExecutionResult::InvalidPlan;
    }
    if (plan.sampleCount == 0u) {
        if (plan.samplePeriodMs != 0u ||
            plan.requestedSamples != 0u ||
            plan.expectedSamples != 0u ||
            !trajectory.Empty()) {
            return ReplayValidationExecutionResult::InvalidPlan;
        }
    } else if (trajectory.SampleCount() != plan.sampleCount ||
               trajectory.FixedStepMs() != plan.samplePeriodMs ||
               plan.requestedSamples == 0u ||
               plan.requestedSamples > plan.sampleCount) {
        return ReplayValidationExecutionResult::InvalidPlan;
    }

    ReplayControlPlanRequest controlRequest(inputTimeline);
    controlRequest.controlTickMs = plan.controlTickMs;
    controlRequest.requestedSamples = plan.requestedSamples;
    controlRequest.validationDurationMs = plan.validationDurationMs;
    controlRequest.validationPrestartMs = plan.validationPrestartMs;
    controlRequest.inputTimeBaseMs = plan.inputTimeBaseMs;
    controlRequest.baseActions = plan.baseActions;
    controlRequest.enableRaceSimulationAfterMs =
            plan.enableRaceSimulationAfterMs;
    controlRequest.establishRaceSpawnAtMs = plan.establishRaceSpawnAtMs;
    if (plan.sampleCount != 0u) {
        controlRequest.trajectory = std::cref(trajectory);
    }

    ReplayControlPlan controlPlan;
    const ReplayControlPlanBuildResult controlResult =
            BuildReplayControlPlan(controlRequest, &controlPlan);
    if (controlResult != ReplayControlPlanBuildResult::Success) {
        return FromControlPlanResult(controlResult);
    }
    const std::uint32_t expectedTargetCount =
            plan.requestedSamples != 0u ? plan.requestedSamples : 1u;
    if (controlPlan.ticks.empty() ||
        controlPlan.comparisonTargetCount != expectedTargetCount ||
        controlPlan.ticks.size() >
                std::numeric_limits<std::uint32_t>::max()) {
        return ReplayValidationExecutionResult::InvalidControlPlan;
    }

    ReplaySimulationTimelineResult simulationResult =
            simulationSession.SimulateTimeline(
                    simulationDefinition,
                    controlPlan.ticks,
                    plan.validationSeed);
    if (simulationResult.result != ReplaySimulationRunResult::Success) {
        return FromSimulationResult(simulationResult.result);
    }
    // TMNF validation playback still runs the world and vehicle. The replay's
    // final fake-finish and respawn events carry the recorded race outcome that
    // the game applies after the physics playback has completed.
    const ReplayRecordedInputOutcome recordedOutcome =
            ObserveRecordedInputOutcome(inputTimeline, plan.inputTimeBaseMs);
    if (!ApplyRecordedInputOutcome(
                plan, recordedOutcome, &simulationResult)) {
        return ReplayValidationExecutionResult::InvalidPlan;
    }

    *out = EvaluateReplayValidation(
            plan,
            simulationResult.observations,
            simulationResult);
    return ReplayValidationExecutionResult::Success;
}
