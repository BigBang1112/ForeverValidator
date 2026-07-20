#pragma once

#include "simulation/replay/replay_challenge_construction.h"
#include "simulation/replay/replay_scene_definition.h"
struct ReplayChallengeBuildReport {
    u32 mapBlockCount = 0u;
    u32 resolvedBlockCount = 0u;
    u32 fieldUnitCount = 0u;
    u32 constructedBlockCount = 0u;
    u32 automaticBaseBlockCount = 0u;
    u32 terrainModifierBlockCount = 0u;
    u32 removedInitialBlockCount = 0u;
    u32 missingBlockCount = 0u;
    bool complete = false;
};

bool BuildReplayChallenge(
        const CGameCtnReplayMapInput &mapInput,
        const ReplaySceneDefinition &scene,
        CGameCtnChallengeConstruction &construction,
        ReplayChallengeBuildReport &report);
