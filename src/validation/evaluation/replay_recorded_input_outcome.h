#ifndef TMNF_REPLAY_RECORDED_INPUT_OUTCOME_H
#define TMNF_REPLAY_RECORDED_INPUT_OUTCOME_H

#include <cstdint>
#include <optional>

class ReplayInputTimeline;
struct ReplaySimulationTimelineResult;
struct ReplayValidationPlan;

struct ReplayRecordedInputOutcome {
    std::optional<std::int32_t> finishRaceTimeMs;
    std::uint32_t respawnCount = 0u;
};

ReplayRecordedInputOutcome ObserveReplayRecordedInputOutcome(
        const ReplayInputTimeline &timeline,
        std::uint32_t inputTimeBaseMs);

bool ApplyReplayRecordedInputOutcome(
        const ReplayValidationPlan &plan,
        const ReplayRecordedInputOutcome &recorded,
        ReplaySimulationTimelineResult *simulationResult);

#endif
