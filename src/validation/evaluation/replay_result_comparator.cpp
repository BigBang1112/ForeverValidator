#include "validation/evaluation/replay_result_comparator.h"
namespace {

constexpr float ReplayValidationDeviationThreshold = 0.1f;
constexpr std::int64_t ReplayValidationRaceTimeToleranceMs = 10;

void SetInvalidComparison(
        ReplayValidationComparison &comparison,
        const char *messageFormat) {
    comparison.result = ReplayValidationComparisonResult::Invalid;
    comparison.messageFormat = messageFormat;
}

bool RaceTimeDiffers(std::int32_t expected, std::int32_t actual) {
    const std::int64_t difference =
            static_cast<std::int64_t>(expected) - actual;
    return difference < -ReplayValidationRaceTimeToleranceMs ||
           difference > ReplayValidationRaceTimeToleranceMs;
}

}  // namespace

ReplayValidationComparison ReplayValidationCompareResult(
        const ReplayValidationExpectations &expected,
        const ReplayValidationObservation &observed) {
    ReplayValidationComparison comparison;

    if (observed.deviationDistance >= ReplayValidationDeviationThreshold) {
        comparison.messageFormat = "Deviates : time=%d, dist=%.3g";
        comparison.firstInteger = observed.deviationTimeMs.value_or(-1);
        comparison.reportDistance = observed.deviationReportDistance;
        return comparison;
    }

    if (!observed.raceCompleted) {
        SetInvalidComparison(comparison, "Expecting completed race");
        return comparison;
    }

    comparison.result = ReplayValidationComparisonResult::Valid;

    if (expected.raceTimeMs.has_value()) {
        if (!observed.actualFinishTimeMs.has_value()) {
            SetInvalidComparison(
                    comparison,
                    "Expecting %d RaceTime instead of unfinished race");
            comparison.firstInteger = *expected.raceTimeMs;
            return comparison;
        }
        if (RaceTimeDiffers(*expected.raceTimeMs,
                            *observed.actualFinishTimeMs)) {
            SetInvalidComparison(
                    comparison,
                    "Expecting %d RaceTime instead of %d");
            comparison.firstInteger = *expected.raceTimeMs;
            comparison.secondInteger = *observed.actualFinishTimeMs;
        }
    }

    if (expected.stuntsScore.has_value() &&
        expected.stuntsScore != observed.actualStuntsScore) {
        SetInvalidComparison(
                comparison,
                "Expecting %d StuntsScore instead of %d");
        comparison.firstInteger = *expected.stuntsScore;
        comparison.secondInteger = observed.actualStuntsScore.value_or(-1);
    }

    if (expected.respawns.has_value() &&
        *expected.respawns != static_cast<std::int32_t>(
                                       observed.respawnCount)) {
        SetInvalidComparison(
                comparison,
                "Expecting %d Respawns instead of %d");
        comparison.firstInteger = *expected.respawns;
        comparison.secondInteger =
                static_cast<std::int32_t>(observed.respawnCount);
    }

    return comparison;
}
