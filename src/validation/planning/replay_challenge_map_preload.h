#ifndef TMNF_REPLAY_CHALLENGE_MAP_PRELOAD_H
#define TMNF_REPLAY_CHALLENGE_MAP_PRELOAD_H

#include "simulation/replay/replay_challenge_construction.h"
#include "engine/game/replay_challenge_map_input.h"
#include "simulation/replay/replay_scene_definition.h"
#include "engine/scene/replay_scene_placements.h"
#include <optional>

#include "simulation/runtime/replay_environment_definition.h"
class ReplayAssetRepository;
class ReplaySimulationSession;

enum class ReplayChallengePreloadResult {
    Success,
    InvalidRequest,
    AllocationFailed,
    WaterDefinitionFailed,
    SceneDefinitionFailed,
    ChallengeConstructionFailed,
    StaticSceneFailed,
};

class CGameCtnReplayChallengeMapPreload {
public:
    CGameCtnReplayChallengeMapPreload() = default;
    CGameCtnReplayChallengeMapPreload(
            const CGameCtnReplayChallengeMapPreload &) = delete;
    CGameCtnReplayChallengeMapPreload &operator=(
            const CGameCtnReplayChallengeMapPreload &) = delete;

    ReplayChallengePreloadResult Preload(
            const CGameCtnReplayMapInput &input,
            ReplayAssetRepository &assets,
            ReplaySimulationSession &simulationSession);
    ReplayChallengePreloadResult Preload(
            const CGameCtnReplayMapInput &input,
            ReplayAssetRepository &mapAssets,
            ReplayAssetRepository &decorationAssets,
            ReplaySimulationSession &simulationSession);
    const std::optional<ReplayWaterDefinition> &WaterDefinition(void) const;

private:
    ReplayAssetRepository *mapAssets = nullptr;
    ReplayAssetRepository *decorationAssets = nullptr;
    ReplaySimulationSession *simulationSession = nullptr;
    CGameCtnReplayMapInput mapInput{};
    ReplaySceneDefinition sceneDefinition{};
    bool sceneDefinitionReady = false;
    std::optional<ReplayWaterDefinition> waterDefinition;

    ReplayChallengePreloadResult PreloadMapInput(
            CGameCtnChallenge &challenge);
    ReplayChallengePreloadResult PreloadSceneDefinition();
    ReplayChallengePreloadResult PreloadChallengeConstruction();
    ReplayChallengePreloadResult PreloadStaticSolidSources(
            CGameCtnChallengeConstruction &construction);
};

#endif
