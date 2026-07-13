#include "validation/evaluation/replay_validation_session.h"
#include <functional>

#include "format/replay/replay_file.h"
std::optional<std::int32_t> ReplayValidationReplay::ExpectedRaceTimeMs() const {
    return inputTimeline_.Metadata().raceTimeMs;
}

std::optional<std::int32_t> ReplayValidationReplay::ExpectedRespawns() const {
    if (!inputTimeline_.Metadata().respawnCount.has_value()) {
        return std::nullopt;
    }
    return static_cast<std::int32_t>(
            *inputTimeline_.Metadata().respawnCount);
}

std::size_t ReplayValidationReplay::ExpectedSampleCount(
        const ReplayGhostTrajectory &trajectory,
        std::int32_t inputDurationMs,
        std::optional<std::int32_t> expectedRaceTimeMs) {
    std::int32_t compareUntilMs = expectedRaceTimeMs.value_or(0);
    if (compareUntilMs == -1) {
        compareUntilMs = inputDurationMs;
    }
    return trajectory.CountThrough(compareUntilMs);
}

std::size_t ReplayValidationReplay::ExpectedSamples() const {
    return ExpectedSampleCount(
            ghostTrajectory_,
            static_cast<std::int32_t>(inputTimeline_.Metadata().durationMs),
            ExpectedRaceTimeMs());
}

std::uint32_t ReplayValidationReplay::RequestedSamples(
        const ReplayValidationConfiguration &configuration) const {
    const std::size_t expectedSamples = ExpectedSamples();
    if (expectedSamples >= configuration.requestedSamples) {
        return configuration.requestedSamples;
    }
    return static_cast<std::uint32_t>(expectedSamples);
}

ReplayValidationPlan ReplayValidationReplay::BuildStreamPlan(
        const ReplayValidationConfiguration &configuration) const {
    ReplayValidationPlan plan;
    plan.sampleCount = ghostTrajectory_.SampleCount();
    plan.samplePeriodMs = ghostTrajectory_.FixedStepMs();
    plan.requestedSamples = RequestedSamples(configuration);
    plan.expectedSamples = ExpectedSamples();
    plan.controlTickMs = configuration.controlTickMs;
    plan.validationPrestartMs = configuration.validationPrestartMs;
    plan.baseActions = configuration.baseActions;
    plan.inputTimeBaseMs = configuration.inputTimeBaseMs;
    plan.validationSeed = inputTimeline_.Metadata().validationSeed;
    plan.validationDurationMs = static_cast<std::int32_t>(
            inputTimeline_.Metadata().durationMs);
    plan.enableRaceSimulationAfterMs =
            static_cast<std::int32_t>(configuration.validationPrestartMs);
    plan.establishRaceSpawnAtMs = std::nullopt;

    // The validation schema uses zero for an unspecified race-time window,
    // while the expectation itself remains explicit.
    plan.expectedRaceTimeMs = ExpectedRaceTimeMs().value_or(0);
    plan.expectedStuntsScore = std::nullopt;
    plan.expectedRespawns = ExpectedRespawns();
    return plan;
}

ReplayValidationMetadata ReplayValidationReplay::BuildMetadata(
        const ReplayValidationConfiguration &configuration) const {
    ReplayValidationMetadata metadata;
    metadata.sampleCount = ghostTrajectory_.SampleCount();
    metadata.samplePeriodMs = ghostTrajectory_.FixedStepMs();
    metadata.inputDurationMs = static_cast<std::int32_t>(
            inputTimeline_.Metadata().durationMs);
    metadata.expectedRaceTimeMs = ExpectedRaceTimeMs();
    metadata.expectedRespawns = ExpectedRespawns();
    metadata.requestedSamples = RequestedSamples(configuration);
    metadata.expectedSamples = ExpectedSamples();
    metadata.actionCount = inputTimeline_.DefinedActionCount();
    metadata.eventCount = inputTimeline_.EventCount();
    return metadata;
}

ReplayFileValidationBuild ValidateReplayFile(
        const ReplayFile &replayFile,
        ReplaySimulationSession &simulationSession,
        const ReplaySimulationDefinition &simulationDefinition,
        const ReplayValidationConfiguration &configuration) {
    if (!configuration.IsValid()) {
        return ReplayFileValidationBuild::Failure(
                ReplayValidationExecutionResult::MissingInput);
    }

    ReplayValidationReplay replay(
            replayFile.InputTimeline(),
            replayFile.GhostTrajectory());
    const ReplayValidationPlan plan = replay.BuildStreamPlan(configuration);

    ReplayFileValidationResult result;
    result.metadata = replay.BuildMetadata(configuration);
    const ReplayGhostArchiveMetadata &ghostArchive =
            replayFile.GhostArchiveMetadata();
    result.metadata.encodedGhostSampleByteCount =
            ghostArchive.encodedSampleByteCount;
    result.metadata.encodedGhostStateByteCount =
            ghostArchive.encodedStateByteCount;

    ReplayValidationExecutionOutput execution;
    const ReplayValidationExecutionResult status = ExecuteReplayValidation(
            &execution,
            simulationSession,
            plan,
            simulationDefinition,
            replay.Trajectory(),
            replay.InputTimeline());
    if (status == ReplayValidationExecutionResult::Success) {
        result.validation = std::move(execution.validation);
        result.raceOutcome = std::move(execution.raceOutcome);
        return ReplayFileValidationBuild::Success(std::move(result));
    }
    return ReplayFileValidationBuild::Failure(status);
}
