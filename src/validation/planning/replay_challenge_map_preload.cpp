#include "validation/planning/replay_challenge_map_preload.h"
#include <new>

#include "simulation/replay/replay_scene_definition_factory.h"
#include "simulation/replay/replay_challenge_factory.h"
#include "format/assets/replay_asset_repository.h"
#include "simulation/runtime/replay_map_environment.h"
#include "simulation/runtime/replay_simulation_session.h"
ReplayChallengePreloadResult
CGameCtnReplayChallengeMapPreload::PreloadMapInput() {
    if (BuildReplayWaterDefinition(mapInput, &waterDefinition) != 0) {
        return ReplayChallengePreloadResult::WaterDefinitionFailed;
    }
    return ReplayChallengePreloadResult::Success;
}

const std::optional<ReplayWaterDefinition> &
CGameCtnReplayChallengeMapPreload::WaterDefinition(void) const {
    return waterDefinition;
}

ReplayChallengePreloadResult
CGameCtnReplayChallengeMapPreload::PreloadSceneDefinition() {
    if (assets == nullptr ||
        !BuildReplaySceneDefinition(
                mapInput, *assets, sceneDefinition) ||
        !sceneDefinition.AppendAutomaticBase(mapInput,
                                             *assets)) {
        return ReplayChallengePreloadResult::SceneDefinitionFailed;
    }
    sceneDefinitionReady = true;
    return ReplayChallengePreloadResult::Success;
}

ReplayChallengePreloadResult
CGameCtnReplayChallengeMapPreload::PreloadChallengeConstruction() {
    struct ChallengeConstructionLoad {
        CGameCtnChallengeConstruction construction{};
        ReplayChallengeBuildReport report{};
    } load;

    const int challengeBuilt =
            sceneDefinitionReady &&
            BuildReplayChallenge(
                    mapInput,
                    sceneDefinition,
                    load.construction,
                    load.report);
    int challengePreloaded = 0;
    if (challengeBuilt && load.construction.Challenge() != nullptr &&
        simulationSession != nullptr) {
        constructedBlockPlacements.PopulateFromChallenge(
                *load.construction.Challenge());
        challengePreloaded =
                simulationSession->PreloadChallenge(load.construction);
    }
    if (!challengePreloaded) {
        return ReplayChallengePreloadResult::ChallengeConstructionFailed;
    }
    return ReplayChallengePreloadResult::Success;
}

ReplayChallengePreloadResult CGameCtnReplayChallengeMapPreload::Preload(
        const CGameCtnReplayMapInput &input,
        ReplayAssetRepository &assetRepository,
        ReplaySimulationSession &session) {
    try {
        mapInput = input;
    } catch (const std::bad_alloc &) {
        return ReplayChallengePreloadResult::AllocationFailed;
    }
    assets = &assetRepository;
    simulationSession = &session;
    ReplayChallengePreloadResult result = PreloadMapInput();
    if (result != ReplayChallengePreloadResult::Success) {
        return result;
    }
    result = PreloadSceneDefinition();
    if (result != ReplayChallengePreloadResult::Success) {
        return result;
    }
    result = PreloadChallengeConstruction();
    if (result != ReplayChallengePreloadResult::Success) {
        return result;
    }
    return PreloadStaticSolidSources();
}

ReplayChallengePreloadResult
CGameCtnReplayChallengeMapPreload::PreloadStaticSolidSources() {
    StaticSceneModelCollection sceneModels;
    if (assets != nullptr && simulationSession != nullptr &&
        assets->BuildStaticScene(mapInput,
                                 constructedBlockPlacements,
                                 &sceneModels) &&
        simulationSession->InstallStaticScene(std::move(sceneModels))) {
        return ReplayChallengePreloadResult::Success;
    }
    return ReplayChallengePreloadResult::StaticSceneFailed;
}
