#include "engine/game/trackmania_race.h"

#include <algorithm>
#include <cmath>
#include <new>

#include "engine/core/binary32_math.h"

float CTrackManiaRace::s_MasterJumpFactor = 0.25f;
unsigned long CTrackManiaRace::s_ReverseBonus = 10ul;
unsigned long CTrackManiaRace::s_TimeBonus = 20ul;
unsigned long CTrackManiaRace::s_InterComboDelay = 2000ul;
unsigned long CTrackManiaRace::s_MinStuntTime = 5ul;
unsigned long CTrackManiaRace::s_MinGrindTime = 200ul;
unsigned long CTrackManiaRace::s_MaxGrindInterval = 50ul;
float CTrackManiaRace::s_MaxGrindRotation = 0.1f;
float CTrackManiaRace::s_FigureRepeatMalus = 0.2f;
unsigned long CTrackManiaRace::s_FigureRepeatCompensation = 5ul;
float CTrackManiaRace::s_ChainBonus1 = 0.2f;
float CTrackManiaRace::s_ChainBonus2 = 0.15000001f;
float CTrackManiaRace::s_ChainBonus3 = 0.1f;
float CTrackManiaRace::s_ChainBonus4 = 0.050000001f;
float CTrackManiaRace::s_ChainBonus5 = 0.02f;
unsigned long CTrackManiaRace::s_StuntRespawnPenalty = 50ul;

namespace {

constexpr float Pi = 3.1415927410125732f;
constexpr float HalfPi = 1.5707963705062866f;
constexpr float StraightLandingLimit = 0.39269909262657166f;
constexpr float ReverseLandingLimit = 2.7488937377929688f;
constexpr float LandingSpeedEpsilon = 0.000009999999747378752f;
u32 RotationCount(float rotation, bool badLanding) {
    const float magnitude = std::fabs(rotation);
    return static_cast<u32>(
            badLanding ? magnitude / Pi : (magnitude + HalfPi) / Pi);
}

GmVec3 PointInTakeoffFrame(
        const GmIso4 &takeoff,
        const GmVec3 &worldPoint) {
    GmVec3 relative = {
            worldPoint.x - takeoff.translation.x,
            worldPoint.y - takeoff.translation.y,
            worldPoint.z - takeoff.translation.z,
    };
    relative.MultTranspose(takeoff.rotation);
    return relative;
}

}  // namespace

void CTrackManiaRace::ConfigureReplayStuntsSimulation(
        bool enabled,
        u32 timeLimitMs) {
    replayStuntsEnabled_ = enabled;
    replayStuntStateAvailable_ = false;
    replayStuntsTimeLimitMs_ = timeLimitMs;
    replayStuntsRaceStartTimeMs_ = UINT32_MAX;
    replayStuntState_ = {};
    replayStuntInputHistorySize_ = 0u;
    replayStuntLocationHistorySize_ = 0u;
    replayStuntPreviousLocation_.SetIdentity();
    replayStuntTakeoffLocation_.SetIdentity();
    replayStuntPreviousLandingTick_ = UINT32_MAX;
    replayStuntChain_ = 0u;
    replayStuntComboWindowMs_ = 0u;
    replayStuntScoreAtTimeLimit_.reset();
    replayStuntFigureScores_.fill(0u);
    stuntsScore_ = 0u;
    stuntEvents_.clear();
    ResetStunts();
}

void CTrackManiaRace::SetReplayStuntSimulationState(
        const ReplayStuntSimulationState &state) {
    replayStuntState_ = state;
    replayStuntStateAvailable_ = true;
    if (state.raceStart) {
        replayStuntsRaceStartTimeMs_ = state.tickTimeMs;
        replayStuntScoreAtTimeLimit_.reset();
        ResetStunts();
    }
}

void CTrackManiaRace::PushReplayStuntInputSnapshot() {
    ReplayStuntInputSnapshot snapshot;
    snapshot.tickTimeMs = replayStuntState_.tickTimeMs;
    snapshot.lastChangeTimeMs = replayStuntState_.inputLastChangeTimeMs;
    if (replayStuntInputHistorySize_ ==
        replayStuntInputHistory_.size()) {
        for (std::size_t index = 1u;
             index < replayStuntInputHistory_.size();
             ++index) {
            replayStuntInputHistory_[index - 1u] =
                    replayStuntInputHistory_[index];
        }
        replayStuntInputHistory_.back() = snapshot;
    } else {
        replayStuntInputHistory_[replayStuntInputHistorySize_++] = snapshot;
    }
}

void CTrackManiaRace::PushReplayStuntVehicleLocation() {
    if (replayStuntLocationHistorySize_ ==
        replayStuntLocationHistory_.size()) {
        for (std::size_t index = 1u;
             index < replayStuntLocationHistory_.size();
             ++index) {
            replayStuntLocationHistory_[index - 1u] =
                    replayStuntLocationHistory_[index];
        }
        replayStuntLocationHistory_.back() =
                replayStuntState_.vehicleLocation;
    } else {
        replayStuntLocationHistory_[replayStuntLocationHistorySize_++] =
                replayStuntState_.vehicleLocation;
    }
}

int CTrackManiaRace::IsReverseLanding(void) {
    return std::fabs(replayStuntLandingDirection_) >= ReverseLandingLimit;
}

int CTrackManiaRace::IsStraightLanding(void) {
    return std::fabs(replayStuntLandingDirection_) <= StraightLandingLimit;
}

unsigned long CTrackManiaRace::GetTimePenalty(unsigned long timeMs) {
    return timeMs / 100ul;
}

void CTrackManiaRace::UpdateStuntTime(void) {
    if (!replayStuntStateAvailable_) {
        return;
    }
    GmMat3 relative = replayStuntState_.vehicleLocation.rotation;
    relative.MultTranspose(replayStuntPreviousLocation_.rotation);
    GmQuat rotation;
    rotation.Set(relative);
    float angle = 0.0f;
    GmVec3 axis{};
    rotation.GetRotation(angle, axis);
    replayStuntRotation_.x += axis.x * angle;
    replayStuntRotation_.y += axis.y * angle;
    replayStuntRotation_.z += axis.z * angle;
    replayStuntPreviousLocation_ = replayStuntState_.vehicleLocation;
}

void CTrackManiaRace::DisplayStuntMessages(
        EFigures figure,
        unsigned long degree,
        unsigned long score,
        float bonus,
        int straightLanding,
        int reverseLanding,
        int masterJump,
        unsigned long chain) {
    ReplayStuntEvent event;
    event.figure = figure;
    event.degree = static_cast<u32>(degree);
    event.score = static_cast<u32>(score);
    event.bonus = bonus;
    event.straightLanding = straightLanding != 0;
    event.reverseLanding = reverseLanding != 0;
    event.masterJump = masterJump != 0;
    event.chain = static_cast<u32>(chain);
    try {
        stuntEvents_.push_back(event);
    } catch (const std::bad_alloc &) {
        // Diagnostics must not make replay validation fail.
    }
}

int CTrackManiaRace::IsStuntTimeOver(unsigned long tickTimeMs) {
    const u32 tick = static_cast<u32>(tickTimeMs);
    if (replayStuntsRaceStartTimeMs_ == UINT32_MAX ||
        replayStuntsRaceStartTimeMs_ >= tick) {
        return 0;
    }
    return replayStuntsTimeLimitMs_ <
           tick - replayStuntsRaceStartTimeMs_;
}

int CTrackManiaRace::IsMasterJump(
        unsigned long startTimeMs,
        unsigned long endTimeMs) {
    const u32 queryStartTimeMs =
            static_cast<u32>(startTimeMs) +
            replayStuntState_.inputQueryTimeOffsetMs;
    const u32 queryEndTimeMs =
            static_cast<u32>(endTimeMs) +
            replayStuntState_.inputQueryTimeOffsetMs;
    const ReplayStuntInputSnapshot *selected = nullptr;
    for (std::size_t index = replayStuntInputHistorySize_;
         index != 0u;
         --index) {
        const ReplayStuntInputSnapshot &candidate =
                replayStuntInputHistory_[index - 1u];
        if (candidate.tickTimeMs <= queryEndTimeMs) {
            selected = &candidate;
            break;
        }
    }
    if (selected == nullptr) {
        return 1;
    }
    return std::all_of(
            selected->lastChangeTimeMs.begin(),
            selected->lastChangeTimeMs.end(),
            [queryStartTimeMs](u32 lastChangeTimeMs) {
                return lastChangeTimeMs <= queryStartTimeMs;
            });
}

void CTrackManiaRace::ResetStunts(void) {
    replayStuntRotation_ = {};
    replayStuntTakeoffTick_ = UINT32_MAX;
    replayStuntLandingTick_ = UINT32_MAX;
    replayStuntInProgress_ = false;
    replayStuntMasterJump_ = false;
    replayStuntBadLanding_ = false;
    replayStuntLandingDirection_ = 0.0f;
    if (replayStuntLocationHistorySize_ != 0u) {
        replayStuntPreviousLocation_ = replayStuntLocationHistory_.front();
    } else if (replayStuntStateAvailable_) {
        replayStuntPreviousLocation_ = replayStuntState_.vehicleLocation;
    } else {
        replayStuntPreviousLocation_.SetIdentity();
    }
}

void CTrackManiaRace::UpdateStunts(void) {
    if (!replayStuntsEnabled_ || !replayStuntStateAvailable_) {
        return;
    }
    const u32 tick = replayStuntState_.tickTimeMs;
    const auto latchTimeLimitScore = [this, tick] {
        if (!replayStuntScoreAtTimeLimit_.has_value() &&
            IsStuntTimeOver(tick)) {
            replayStuntScoreAtTimeLimit_ = stuntsScore_;
        }
    };
    if (replayStuntState_.finishRace) {
        latchTimeLimitScore();
        return;
    }
    PushReplayStuntInputSnapshot();
    PushReplayStuntVehicleLocation();

    const bool hasGroundContact = replayStuntState_.hasWheelContact ||
                                  replayStuntState_.hasBodyContact;
    if (replayStuntInProgress_) {
        if (replayStuntState_.noGroundFrictionGuard) {
            ResetStunts();
        }
        if (hasGroundContact) {
            if (replayStuntState_.hasBodyContact &&
                (replayStuntState_.bodyContactHorizontalAngle > 0.5f ||
                 replayStuntState_.bodyContactVerticalAngle > 0.5f)) {
                replayStuntBadLanding_ = true;
            }
            replayStuntLandingTick_ = tick;
            const float speedMagnitude = CIsqrt(
                    replayStuntState_.forwardSpeed *
                            replayStuntState_.forwardSpeed +
                    replayStuntState_.sideSpeed *
                            replayStuntState_.sideSpeed);
            replayStuntLandingDirection_ =
                    speedMagnitude <= LandingSpeedEpsilon
                            ? 0.0f
                            : CIatan2(replayStuntState_.sideSpeed /
                                              speedMagnitude,
                                      replayStuntState_.forwardSpeed /
                                              speedMagnitude);
            ComputeStunt();
            ResetStunts();
        } else {
            replayStuntLandingTick_ = UINT32_MAX;
            const u32 masterStart = replayStuntTakeoffTick_ + 100u;
            if (tick <= replayStuntTakeoffTick_ + 200u) {
                replayStuntMasterJump_ = true;
            } else if (replayStuntMasterJump_) {
                replayStuntMasterJump_ =
                        IsMasterJump(masterStart, tick - 100u) != 0;
            }
            UpdateStuntTime();
        }
    }

    if (!replayStuntInProgress_ && !hasGroundContact &&
        !replayStuntState_.noGroundFrictionGuard &&
        replayStuntsRaceStartTimeMs_ != UINT32_MAX &&
        tick > replayStuntsRaceStartTimeMs_) {
        ResetStunts();
        replayStuntTakeoffTick_ = tick;
        replayStuntInProgress_ = true;
        replayStuntTakeoffLocation_ = replayStuntState_.vehicleLocation;
    }
    latchTimeLimitScore();
}

void CTrackManiaRace::ComputeStunt(void) {
    if (!replayStuntsEnabled_ || replayStuntLandingTick_ == UINT32_MAX) {
        return;
    }
    u32 duration = 0u;
    if (replayStuntTakeoffTick_ != UINT32_MAX) {
        duration = (replayStuntLandingTick_ - replayStuntTakeoffTick_) / 100u;
        if (duration < s_MinStuntTime) {
            return;
        }
    }

    const float absX = std::fabs(replayStuntRotation_.x);
    const float absY = std::fabs(replayStuntRotation_.y);
    const float absZ = std::fabs(replayStuntRotation_.z);
    const u32 countX = RotationCount(
            replayStuntRotation_.x, replayStuntBadLanding_);
    const u32 countY = RotationCount(
            replayStuntRotation_.y, replayStuntBadLanding_);
    const u32 countZ = RotationCount(
            replayStuntRotation_.z, replayStuntBadLanding_);

    u32 figure = 1u;
    u32 primaryCount = 0u;
    if (countZ == 0u) {
        if (countX == 0u) {
            if (countY != 0u) {
                if (std::fabs(
                            replayStuntTakeoffLocation_.rotation.basisY.y) >
                    0.20000000298023224f) {
                    figure = 4u;
                } else {
                    const GmVec3 relativeLanding = PointInTakeoffFrame(
                            replayStuntTakeoffLocation_,
                            replayStuntState_.vehicleLocation.translation);
                    figure = (relativeLanding.x < 0.0f &&
                              replayStuntRotation_.y > 0.0f) ||
                                     (relativeLanding.x > 0.0f &&
                                      replayStuntRotation_.y < 0.0f)
                            ? 6u
                            : 5u;
                }
                primaryCount = countY;
            }
        } else if (countY == 0u) {
            figure = replayStuntRotation_.x > 0.0f ? 2u : 3u;
            primaryCount = countX;
        } else if (absX > absY) {
            figure = 10u;
            primaryCount = countX;
        } else {
            figure = 8u;
            primaryCount = countY;
        }
    } else if (countX == 0u) {
        if (countY == 0u) {
            figure = 7u;
            primaryCount = countZ;
        } else if (absZ > absY) {
            figure = 12u;
            primaryCount = countZ;
        } else {
            figure = 9u;
            primaryCount = countY;
        }
    } else if (countY == 0u) {
        if (absZ <= absX) {
            figure = 11u;
            primaryCount = countX;
        } else {
            figure = 13u;
            primaryCount = countZ;
        }
    } else if (absZ > absX && absZ > absY) {
        figure = 16u;
        primaryCount = countZ;
    } else if (absX > absZ && absX > absY) {
        figure = 15u;
        primaryCount = countX;
    } else {
        figure = 14u;
        primaryCount = countY;
    }

    if (replayStuntBadLanding_) {
        figure += 17u;
    }
    int reverseLanding = IsReverseLanding();
    int straightLanding = IsStraightLanding();
    int masterJump = 0;
    if (figure >= 2u && figure <= 16u &&
        replayStuntLandingTick_ >= 100u) {
        masterJump = IsMasterJump(
                replayStuntTakeoffTick_ + 100u,
                replayStuntLandingTick_ - 100u);
        if (!straightLanding && !reverseLanding) {
            masterJump = 0;
        }
    }

    const u32 previousLanding = replayStuntPreviousLandingTick_;
    const u32 landingGap = previousLanding == UINT32_MAX
            ? 0u
            : replayStuntTakeoffTick_ - previousLanding;
    if (previousLanding == UINT32_MAX ||
        landingGap > replayStuntComboWindowMs_) {
        replayStuntChain_ = 0u;
    } else {
        ++replayStuntChain_;
    }
    replayStuntPreviousLandingTick_ = replayStuntLandingTick_;

    if (replayStuntBadLanding_) {
        masterJump = 0;
        straightLanding = 0;
        reverseLanding = 0;
    }
    u32 baseScore = duration + 15u * countX +
                    10u * (countY + 2u * countZ);
    if (figure == 6u) {
        baseScore += 5u;
    } else if (figure == 1u) {
        baseScore >>= 1u;
    }
    if (replayStuntBadLanding_) {
        baseScore >>= 1u;
    }

    float bonus = 1.0f;
    if (reverseLanding || straightLanding) {
        bonus = 1.25f;
    }
    if (masterJump) {
        bonus += 0.25f;
    }
    if (replayStuntChain_ != 0u) {
        bonus += s_ChainBonus1;
    }
    if (replayStuntChain_ > 1u) {
        bonus += s_ChainBonus2;
    }
    if (replayStuntChain_ > 2u) {
        bonus += s_ChainBonus3;
    }
    if (replayStuntChain_ > 3u) {
        bonus += s_ChainBonus4;
    }
    if (replayStuntChain_ > 4u) {
        bonus += static_cast<float>(replayStuntChain_ - 4u) *
                 s_ChainBonus5;
    }

    const float repeatMalus = std::min(
            static_cast<float>(replayStuntFigureScores_[figure]) *
                    s_FigureRepeatMalus / 100.0f,
            0.75f);
    u32 repeatedScore = static_cast<u32>(
            (1.0f - repeatMalus) * static_cast<float>(baseScore));
    if (repeatedScore == 0u) {
        repeatedScore = 1u;
    }
    const u32 score = static_cast<u32>(
            static_cast<float>(repeatedScore) * bonus);
    if (IsStuntTimeOver(
                replayStuntState_.tickTimeMs +
                replayStuntState_.inputQueryTimeOffsetMs)) {
        return;
    }

    stuntsScore_ += score;
    u32 interComboDelay = static_cast<u32>(s_InterComboDelay);
    if (replayStuntComboWindowMs_ != 0u &&
        replayStuntComboWindowMs_ > s_InterComboDelay + landingGap) {
        interComboDelay = replayStuntComboWindowMs_ - landingGap;
    }
    replayStuntComboWindowMs_ = interComboDelay + 20u * score;
    DisplayStuntMessages(
            static_cast<EFigures>(figure),
            180u * primaryCount,
            score,
            bonus,
            straightLanding,
            reverseLanding,
            masterJump,
            replayStuntChain_);
    replayStuntFigureScores_[figure] += score;
}

void CTrackManiaRace::ApplyReplayStuntTimePenalty(
        unsigned long overtimeMs) {
    if (!replayStuntsEnabled_) {
        return;
    }
    const u32 penalty = static_cast<u32>(GetTimePenalty(overtimeMs));
    const u32 scoreAtTimeLimit =
            replayStuntScoreAtTimeLimit_.value_or(stuntsScore_);
    stuntsScore_ = penalty < scoreAtTimeLimit
            ? scoreAtTimeLimit - penalty
            : 0u;
    if (penalty == 0u) {
        return;
    }
    replayStuntComboWindowMs_ = static_cast<u32>(s_InterComboDelay);
    DisplayStuntMessages(
            static_cast<EFigures>(34u),
            0ul,
            penalty,
            0.0f,
            0,
            0,
            0,
            0ul);
}

void CTrackManiaRace::ApplyReplayStuntRespawnPenalty() {
    ApplyReplayStuntRespawnPenalty(replayStuntState_.tickTimeMs);
}

void CTrackManiaRace::ApplyReplayStuntRespawnPenalty(
        unsigned long) {
    if (!replayStuntsEnabled_) {
        return;
    }
    const u32 penalty = std::min(
            stuntsScore_, static_cast<u32>(s_StuntRespawnPenalty));
    stuntsScore_ -= penalty;
    ResetStunts();
}
