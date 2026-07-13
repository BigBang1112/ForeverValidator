#ifndef TMNF_REPLAY_RESULT_COMPARATOR_H
#define TMNF_REPLAY_RESULT_COMPARATOR_H

#include <cstdint>
#include <optional>

enum class ReplayValidationComparisonResult : std::int32_t {
    Invalid = 0,
    Valid = 1,
    WrongSimulation = 2,
};

struct ReplayValidationExpectations {
    std::optional<std::int32_t> raceTimeMs;
    std::optional<std::int32_t> stuntsScore;
    std::optional<std::int32_t> respawns;
};

struct ReplayValidationObservation {
    float deviationDistance = 0.0f;
    std::uint32_t respawnCount = 0u;
    std::optional<std::int32_t> deviationTimeMs;
    float deviationReportDistance = 0.0f;
    std::optional<std::int32_t> actualFinishTimeMs;
    std::optional<std::int32_t> actualStuntsScore;
    bool raceCompleted = false;
};

struct ReplayValidationComparison {
    ReplayValidationComparisonResult result =
            ReplayValidationComparisonResult::WrongSimulation;
    const char *messageFormat = nullptr;
    std::int32_t firstInteger = 0;
    std::int32_t secondInteger = 0;
    double reportDistance = 0.0;
};

ReplayValidationComparison ReplayValidationCompareResult(
        const ReplayValidationExpectations &expected,
        const ReplayValidationObservation &observed);

#endif
