#ifndef REPLAY_GHOST_TRAJECTORY_H
#define REPLAY_GHOST_TRAJECTORY_H

#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <utility>
#include <vector>

#include "engine/core/finite_values.h"
enum class ReplayGhostTrajectoryCreateResult {
    Success,
    MissingOutput,
    InvalidFixedStep,
    TimestampOutOfRange,
    NonFinitePosition,
};

class ReplayGhostTrajectory {
public:
    ReplayGhostTrajectory() = default;

    static ReplayGhostTrajectoryCreateResult CreateFixedStep(
            uint32_t fixedStepMs,
            std::vector<GmVec3> positions,
            ReplayGhostTrajectory *out) {
        if (out == nullptr) {
            return ReplayGhostTrajectoryCreateResult::MissingOutput;
        }
        if (!positions.empty()) {
            if (fixedStepMs == 0u ||
                fixedStepMs > static_cast<uint32_t>(
                        std::numeric_limits<int32_t>::max())) {
                return ReplayGhostTrajectoryCreateResult::InvalidFixedStep;
            }
            const size_t lastSampleIndex = positions.size() - 1u;
            const size_t lastTimeLimit = static_cast<size_t>(
                    std::numeric_limits<int32_t>::max());
            if (lastSampleIndex > lastTimeLimit / fixedStepMs) {
                return ReplayGhostTrajectoryCreateResult::TimestampOutOfRange;
            }
            for (const GmVec3 &position : positions) {
                if (!FiniteValues::IsFinite(position)) {
                    return ReplayGhostTrajectoryCreateResult::NonFinitePosition;
                }
            }
        }

        ReplayGhostTrajectory trajectory;
        trajectory.fixedStepMs_ = fixedStepMs;
        trajectory.positions_ = std::move(positions);
        *out = std::move(trajectory);
        return ReplayGhostTrajectoryCreateResult::Success;
    }

    uint32_t FixedStepMs() const {
        return fixedStepMs_;
    }

    size_t SampleCount() const {
        return positions_.size();
    }

    bool Empty() const {
        return positions_.empty();
    }

    size_t SampleTimeMs(size_t sampleIndex) const {
        return sampleIndex * static_cast<size_t>(fixedStepMs_);
    }

    const GmVec3 &PositionAt(size_t sampleIndex) const {
        return positions_[sampleIndex];
    }

    size_t CountThrough(int32_t inclusiveTimeMs) const {
        if (inclusiveTimeMs < 0) {
            return SampleCount();
        }
        if (positions_.empty()) {
            return 0u;
        }
        const size_t samplesThroughTime =
                static_cast<size_t>(inclusiveTimeMs) / fixedStepMs_ + 1u;
        return samplesThroughTime < positions_.size()
                       ? samplesThroughTime
                       : positions_.size();
    }

private:
    uint32_t fixedStepMs_ = 0u;
    std::vector<GmVec3> positions_;
};

#endif
