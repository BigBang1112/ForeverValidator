#include "engine/scene/replay_static_scene_corpuses.h"
#include <utility>

#include "engine/scene/replay_scene_composite_asset.h"
#include "engine/scene/replay_scene_placements.h"
namespace {

bool AddCompositeModel(
        ReplaySceneCompositeAsset &composite,
        const GmIso4 &worldIso,
        StaticScenePurpose purpose,
        StaticSceneModelCollection &models) {
    if (composite.Empty()) {
        return true;
    }
    StaticSolidPrototype prototype = composite.TakePrototype();
    return prototype.IsValid() &&
           models.Add(StaticSceneModel(
                   std::move(prototype), worldIso, purpose));
}

bool AddHelperModel(
        const ReplaySceneBlockPlacement &placement,
        StaticSceneModelCollection &models,
        bool &complete) {
    const std::vector<ReplaySceneAssetPlacement> helperAssets =
            placement.HelperAssetPlacements();
    if (helperAssets.empty()) {
        return true;
    }

    ReplaySceneCompositeAsset composite =
            ReplaySceneCompositeAsset::ForHelpers(helperAssets);
    if (!composite.IsComplete()) {
        complete = false;
    }
    return AddCompositeModel(composite,
                             placement.WorldIso(),
                             StaticScenePurpose::Helper,
                             models);
}

bool AddClipModel(
        const ReplaySceneBlockPlacement &placement,
        StaticSceneModelCollection &models) {
    ReplaySceneCompositeAsset composite = ReplaySceneCompositeAsset::ForClip(
            placement.ClipAssetPlacements());
    return AddCompositeModel(composite,
                             placement.WorldIso(),
                             StaticScenePurpose::Clip,
                             models);
}

bool AddCheckpointTriggerModel(
        const ReplaySceneBlockPlacement &placement,
        StaticSceneModelCollection &models,
        bool &complete) {
    const std::optional<ReplaySceneAssetPlacement> triggerAsset =
            placement.CheckpointTriggerAssetPlacement();
    if (!triggerAsset.has_value()) {
        return true;
    }

    const StaticSolidPrototype prototype = triggerAsset->Prototype();
    const std::optional<GmIso4> worldIso =
            placement.CheckpointTriggerWorldIso();
    if (!prototype.IsValid() || !worldIso.has_value()) {
        complete = false;
        return true;
    }

    StaticSceneModel model(prototype,
                           *worldIso,
                           StaticScenePurpose::CheckpointTrigger);
    model.SetItemProperties(placement.CheckpointTriggerProperties());
    model.SetCheckpoint(placement.CheckpointIdentity(),
                        placement.BlockSpawnIso());
    return models.Add(std::move(model));
}

bool AddBlockPlacement(
        const ReplaySceneBlockPlacement &placement,
        StaticSceneModelCollection &models,
        bool &complete) {
    if (placement.IsSuppressed()) {
        return true;
    }
    if (placement.IsClip()) {
        return AddClipModel(placement, models);
    }

    const std::optional<ReplaySceneAssetPlacement> mainAsset =
            placement.MainAssetPlacement();
    if (!mainAsset.has_value()) {
        return AddHelperModel(placement, models, complete);
    }

    const StaticSolidPrototype prototype = mainAsset->Prototype();
    if (!prototype.IsValid()) {
        complete = false;
        return true;
    }
    if (!models.Add(StaticSceneModel(prototype,
                                     placement.WorldIso(),
                                     StaticScenePurpose::PlacedBlock))) {
        return false;
    }
    return AddCheckpointTriggerModel(placement, models, complete) &&
           AddHelperModel(placement, models, complete);
}

}  // namespace

bool AppendBlockPlacementModels(
        const ReplaySceneBlockPlacements &placements,
        StaticSceneModelCollection &models) {
    bool complete = models.IsComplete();
    const ReplaySceneModelCounts counts = placements.ModelCounts();
    if (!models.Reserve(models.Models().size() +
                        counts.placedBlocks +
                        counts.clipAssemblies +
                        counts.helperAssemblies +
                        counts.checkpointTriggers)) {
        return false;
    }

    for (const ReplaySceneBlockPlacement &placement :
         placements.Placements()) {
        if (!AddBlockPlacement(placement, models, complete)) {
            return false;
        }
    }
    if (!complete) {
        models.MarkIncomplete();
    }
    return true;
}
