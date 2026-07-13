#ifndef TMNF_REPLAY_VALIDATION_RESULT_H
#define TMNF_REPLAY_VALIDATION_RESULT_H

#include <cstdint>
#include <optional>

#include "engine/core/gm_types.h"
enum class ReplayValidationStatus {
    Valid,
    ValidPrefix,
    WrongSimulation,
    IncompleteStandaloneRun,
    RaceCompletionUnavailable,
    ExpectingCompletedRace,
    RaceTimeMismatch,
    StuntsScoreMismatch,
    RespawnCountMismatch,
    RespawnExpectationUnavailable,
    ObservationError,
};

enum class ReplayValidationOutcome {
    Invalid,
    Valid,
    WrongSimulation,
    Unavailable,
    Error,
};

enum class ReplayObservationError {
    NonFiniteDistance,
    ReplayMetadataUnavailable,
};

struct ReplayValidationDeviation {
    std::uint32_t comparisonOrdinal = 0u;
    std::int32_t ghostTimeMs = 0;
    std::int32_t simulationTimeMs = 0;
    float distance = 0.0f;
    GmVec3 simulatedPosition{};
    GmVec3 writePosition{};
    GmVec3 targetPosition{};
};

struct ReplayValidationResult {
    ReplayValidationStatus status = ReplayValidationStatus::Valid;
    ReplayValidationOutcome outcome = ReplayValidationOutcome::Valid;
    std::uint32_t measuredSamples = 0u;
    std::uint32_t expectedSamples = 0u;
    std::uint32_t comparedExactGhostStateCount = 0u;
    bool wrongSimulation = false;
    std::optional<ReplayValidationDeviation> firstDivergence;
    std::optional<ReplayValidationDeviation> firstExactDeviation;
    float maxDeviation = 0.0f;
    std::optional<std::int32_t> maxDeviationTimeMs;
    float maxDeviationDistance = 0.0f;
    std::optional<ReplayObservationError> observationError;
};

#endif
