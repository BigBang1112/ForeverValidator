#pragma once

#include "engine/core/engine_types.h"
#include <memory>

#include "engine/game/game_ctn_challenge.h"
class CGameCtnChallengeConstruction {
public:
    CGameCtnChallengeConstruction(void) = default;
    CGameCtnChallengeConstruction(const CGameCtnChallengeConstruction &) = delete;
    CGameCtnChallengeConstruction &operator=(
            const CGameCtnChallengeConstruction &) = delete;
    CGameCtnChallengeConstruction(CGameCtnChallengeConstruction &&) noexcept =
            default;
    CGameCtnChallengeConstruction &operator=(
            CGameCtnChallengeConstruction &&) noexcept = default;

    void SetChallenge(std::unique_ptr<CGameCtnChallenge> challenge);
    std::unique_ptr<CGameCtnChallenge> ReleaseChallenge(void);
    CGameCtnChallenge *Challenge(void);
    const CGameCtnChallenge *Challenge(void) const;
    bool Ready(void) const;

    void SetCounts(u32 authored,
                   u32 automaticBase,
                   u32 terrainModifiers,
                   u32 missingBlocks);
    u32 AuthoredBlockCount(void) const;
    u32 AutomaticBaseBlockCount(void) const;
    u32 TerrainModifierBlockCount(void) const;
    u32 MissingBlockCount(void) const;
    u32 BlockCount(void) const;

private:
    std::unique_ptr<CGameCtnChallenge> challenge_;
    u32 authoredBlockCount_ = 0u;
    u32 automaticBaseBlockCount_ = 0u;
    u32 terrainModifierBlockCount_ = 0u;
    u32 missingBlockCount_ = 0u;
};
