#include "simulation/replay/replay_challenge_construction.h"
#include <utility>

void CGameCtnChallengeConstruction::SetChallenge(
        std::unique_ptr<CGameCtnChallenge> challenge) {
    challenge_ = std::move(challenge);
}

std::unique_ptr<CGameCtnChallenge>
CGameCtnChallengeConstruction::ReleaseChallenge(void) {
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
