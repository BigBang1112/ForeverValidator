#include "validation/planning/replay_challenge_map_preload.h"
#include <new>

#include "simulation/replay/replay_scene_definition_factory.h"
#include "simulation/replay/replay_challenge_factory.h"
#include "format/assets/replay_asset_repository.h"
#include "simulation/runtime/replay_map_environment.h"
#include "simulation/runtime/replay_simulation_session.h"
ReplayChallengePreloadResult
CGameCtnReplayChallengeMapPreload::PreloadMapInput(
        CGameCtnChallenge &challenge) {
    const auto &collection = sceneDefinition.Collection();
    const auto &decoration = sceneDefinition.DecorationSize();
    if (!collection.has_value() || !decoration.has_value() ||
        BuildReplayWaterDefinition(
                challenge,
                *collection,
                *decoration,
                &waterDefinition) != 0) {
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
    if (mapAssets == nullptr || decorationAssets == nullptr ||
        !BuildReplaySceneDefinition(
                mapInput,
                *mapAssets,
                *decorationAssets,
                sceneDefinition) ||
        !sceneDefinition.AppendAutomaticBase(mapInput,
                                             *mapAssets)) {
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
    if (challengeBuilt && load.construction.Challenge() != nullptr &&
        simulationSession != nullptr) {
        CGameCtnChallenge *challenge = load.construction.Challenge();
        ReplaySceneBlockPlacements placements;
        placements.PopulateFromChallenge(*challenge);
        placements.SetRetainedLogicalStartPlacements(
                load.construction.RetainedLogicalStartPlacements());
        const ReplayChallengePreloadResult waterResult =
                PreloadMapInput(*challenge);
        if (waterResult != ReplayChallengePreloadResult::Success) {
            return waterResult;
        }
        load.construction.SetPreparedBlockPlacements(
                std::move(placements));
        return PreloadStaticSolidSources(load.construction);
    }
    return ReplayChallengePreloadResult::ChallengeConstructionFailed;
}

ReplayChallengePreloadResult CGameCtnReplayChallengeMapPreload::Preload(
        const CGameCtnReplayMapInput &input,
        ReplayAssetRepository &assetRepository,
        ReplaySimulationSession &session) {
    return Preload(input,
                   assetRepository,
                   assetRepository,
                   session);
}

ReplayChallengePreloadResult CGameCtnReplayChallengeMapPreload::Preload(
        const CGameCtnReplayMapInput &input,
        ReplayAssetRepository &mapAssetRepository,
        ReplayAssetRepository &decorationAssetRepository,
        ReplaySimulationSession &session) {
    try {
        mapInput = input;
    } catch (const std::bad_alloc &) {
        return ReplayChallengePreloadResult::AllocationFailed;
    }
    mapAssets = &mapAssetRepository;
    decorationAssets = &decorationAssetRepository;
    simulationSession = &session;
    ReplayChallengePreloadResult result = PreloadSceneDefinition();
    if (result != ReplayChallengePreloadResult::Success) {
        return result;
    }
    result = PreloadChallengeConstruction();
    return result;
}

ReplayChallengePreloadResult
CGameCtnReplayChallengeMapPreload::PreloadStaticSolidSources(
        CGameCtnChallengeConstruction &construction) {
    StaticSceneModelCollection sceneModels;
    CGameCtnChallenge *challenge = construction.Challenge();
    const ReplaySceneBlockPlacements *placements =
            construction.PreparedBlockPlacements();
    if (mapAssets == nullptr || decorationAssets == nullptr ||
        simulationSession == nullptr || challenge == nullptr ||
        placements == nullptr ||
        !mapAssets->BuildStaticSceneWithDecorationAssets(
                *decorationAssets,
                mapInput,
                *placements,
                &sceneModels)) {
        return ReplayChallengePreloadResult::StaticSceneFailed;
    }
    const auto &collection = sceneDefinition.Collection();
    const auto &decoration = sceneDefinition.DecorationSize();
    if (collection.has_value() && decoration.has_value() &&
        collection->water.has_value() &&
        collection->water->geometryWaterPlanes &&
        BuildReplayGeometryWaterDefinition(
                *challenge,
                *collection,
                *decoration,
                *placements,
                sceneModels,
                &waterDefinition) != 0) {
        return ReplayChallengePreloadResult::WaterDefinitionFailed;
    }
    if (!simulationSession->PreloadChallenge(construction)) {
        return ReplayChallengePreloadResult::ChallengeConstructionFailed;
    }
    if (simulationSession->InstallStaticScene(std::move(sceneModels))) {
        return ReplayChallengePreloadResult::Success;
    }
    return ReplayChallengePreloadResult::StaticSceneFailed;
}
