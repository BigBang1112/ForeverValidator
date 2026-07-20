#pragma once

#include "engine/core/engine_types.h"
#include <memory>
#include <vector>

#include "engine/game/game_ctn_challenge.h"
#include "engine/scene/replay_scene_placements.h"
class CGameCtnChallengeConstruction {
public:
    CGameCtnChallengeConstruction(void) = default;
    CGameCtnChallengeConstruction(const CGameCtnChallengeConstruction &) = delete;
    CGameCtnChallengeConstruction &operator=(
            const CGameCtnChallengeConstruction &) = delete;
    CGameCtnChallengeConstruction(CGameCtnChallengeConstruction &&other) noexcept;
    CGameCtnChallengeConstruction &operator=(
            CGameCtnChallengeConstruction &&other) noexcept;

    void SetChallenge(std::unique_ptr<CGameCtnChallenge> challenge);
    std::unique_ptr<CGameCtnChallenge> ReleaseChallenge(void);
    CGameCtnChallenge *Challenge(void);
    const CGameCtnChallenge *Challenge(void) const;
    bool Ready(void) const;

    void SetPreparedBlockPlacements(ReplaySceneBlockPlacements placements);
    bool MovePreparedSceneTo(
            std::unique_ptr<CGameCtnChallenge> &challenge,
            ReplaySceneBlockPlacements &placements);
    ReplaySceneBlockPlacements *PreparedBlockPlacements(void);
    const ReplaySceneBlockPlacements *PreparedBlockPlacements(void) const;
    bool HasPreparedBlockPlacements(void) const;

    void SetRetainedLogicalStartPlacements(
            std::vector<ReplaySceneLogicalStartPlacement> placements);
    const std::vector<ReplaySceneLogicalStartPlacement> &
            RetainedLogicalStartPlacements(void) const;

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
    ReplaySceneBlockPlacements preparedBlockPlacements_;
    std::vector<ReplaySceneLogicalStartPlacement>
            retainedLogicalStartPlacements_;
    bool preparedBlockPlacementsReady_ = false;
    u32 authoredBlockCount_ = 0u;
    u32 automaticBaseBlockCount_ = 0u;
    u32 terrainModifierBlockCount_ = 0u;
    u32 missingBlockCount_ = 0u;
};
