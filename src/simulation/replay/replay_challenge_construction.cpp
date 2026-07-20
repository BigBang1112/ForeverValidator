#include "simulation/replay/replay_challenge_construction.h"
#include <utility>

CGameCtnChallengeConstruction::CGameCtnChallengeConstruction(
        CGameCtnChallengeConstruction &&other) noexcept
        : challenge_(std::move(other.challenge_)),
          preparedBlockPlacements_(
                  std::move(other.preparedBlockPlacements_)),
          retainedLogicalStartPlacements_(
                  std::move(other.retainedLogicalStartPlacements_)),
          preparedBlockPlacementsReady_(
                  other.preparedBlockPlacementsReady_),
          authoredBlockCount_(other.authoredBlockCount_),
          automaticBaseBlockCount_(other.automaticBaseBlockCount_),
          terrainModifierBlockCount_(other.terrainModifierBlockCount_),
          missingBlockCount_(other.missingBlockCount_) {
    other.preparedBlockPlacementsReady_ = false;
}

CGameCtnChallengeConstruction &
CGameCtnChallengeConstruction::operator=(
        CGameCtnChallengeConstruction &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    preparedBlockPlacements_.Clear();
    preparedBlockPlacementsReady_ = false;
    challenge_.reset();
    challenge_ = std::move(other.challenge_);
    preparedBlockPlacements_ =
            std::move(other.preparedBlockPlacements_);
    retainedLogicalStartPlacements_ =
            std::move(other.retainedLogicalStartPlacements_);
    preparedBlockPlacementsReady_ =
            other.preparedBlockPlacementsReady_;
    authoredBlockCount_ = other.authoredBlockCount_;
    automaticBaseBlockCount_ = other.automaticBaseBlockCount_;
    terrainModifierBlockCount_ = other.terrainModifierBlockCount_;
    missingBlockCount_ = other.missingBlockCount_;
    other.preparedBlockPlacementsReady_ = false;
    return *this;
}

void CGameCtnChallengeConstruction::SetChallenge(
        std::unique_ptr<CGameCtnChallenge> challenge) {
    preparedBlockPlacements_.Clear();
    retainedLogicalStartPlacements_.clear();
    preparedBlockPlacementsReady_ = false;
    challenge_ = std::move(challenge);
}

std::unique_ptr<CGameCtnChallenge>
CGameCtnChallengeConstruction::ReleaseChallenge(void) {
    preparedBlockPlacements_.Clear();
    retainedLogicalStartPlacements_.clear();
    preparedBlockPlacementsReady_ = false;
    return std::move(challenge_);
}

CGameCtnChallenge *CGameCtnChallengeConstruction::Challenge(void) {
    return challenge_.get();
}

const CGameCtnChallenge *CGameCtnChallengeConstruction::Challenge(void) const {
    return challenge_.get();
}

bool CGameCtnChallengeConstruction::Ready(void) const {
    return challenge_ != nullptr;
}

void CGameCtnChallengeConstruction::SetPreparedBlockPlacements(
        ReplaySceneBlockPlacements placements) {
    preparedBlockPlacements_ = std::move(placements);
    preparedBlockPlacementsReady_ = true;
}

bool CGameCtnChallengeConstruction::MovePreparedSceneTo(
        std::unique_ptr<CGameCtnChallenge> &challenge,
        ReplaySceneBlockPlacements &placements) {
    if (!Ready() || !preparedBlockPlacementsReady_) {
        return false;
    }

    placements.Clear();
    challenge.reset();
    challenge = std::move(challenge_);
    placements = std::move(preparedBlockPlacements_);
    preparedBlockPlacementsReady_ = false;
    return challenge != nullptr;
}

ReplaySceneBlockPlacements *
CGameCtnChallengeConstruction::PreparedBlockPlacements(void) {
    return preparedBlockPlacementsReady_ ? &preparedBlockPlacements_ : nullptr;
}

const ReplaySceneBlockPlacements *
CGameCtnChallengeConstruction::PreparedBlockPlacements(void) const {
    return preparedBlockPlacementsReady_ ? &preparedBlockPlacements_ : nullptr;
}

bool CGameCtnChallengeConstruction::HasPreparedBlockPlacements(void) const {
    return preparedBlockPlacementsReady_;
}

void CGameCtnChallengeConstruction::SetRetainedLogicalStartPlacements(
        std::vector<ReplaySceneLogicalStartPlacement> placements) {
    retainedLogicalStartPlacements_ = std::move(placements);
}

const std::vector<ReplaySceneLogicalStartPlacement> &
CGameCtnChallengeConstruction::RetainedLogicalStartPlacements(void) const {
    return retainedLogicalStartPlacements_;
}

void CGameCtnChallengeConstruction::SetCounts(
        u32 authored,
        u32 automaticBase,
        u32 terrainModifiers,
        u32 missingBlocks) {
    authoredBlockCount_ = authored;
    automaticBaseBlockCount_ = automaticBase;
    terrainModifierBlockCount_ = terrainModifiers;
    missingBlockCount_ = missingBlocks;
}

u32 CGameCtnChallengeConstruction::AuthoredBlockCount(void) const {
    return authoredBlockCount_;
}

u32 CGameCtnChallengeConstruction::AutomaticBaseBlockCount(void) const {
    return automaticBaseBlockCount_;
}

u32 CGameCtnChallengeConstruction::TerrainModifierBlockCount(void) const {
    return terrainModifierBlockCount_;
}

u32 CGameCtnChallengeConstruction::MissingBlockCount(void) const {
    return missingBlockCount_;
}

u32 CGameCtnChallengeConstruction::BlockCount(void) const {
    return challenge_ != nullptr && challenge_->Blocks().size() <= UINT32_MAX
            ? static_cast<u32>(challenge_->Blocks().size())
            : UINT32_MAX;
}
