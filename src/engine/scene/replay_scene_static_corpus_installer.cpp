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

bool AddSubMobilModels(
        const ReplaySceneBlockPlacement &placement,
        StaticSceneModelCollection &models,
        bool &complete) {
    for (const ReplaySceneAssetPlacement &asset :
         placement.SubMobilAssetPlacements()) {
        const StaticSolidPrototype prototype = asset.Prototype();
        if (!prototype.IsValid()) {
            complete = false;
            continue;
        }
        if (!models.Add(StaticSceneModel(
                    prototype,
                    placement.WorldIso(),
                    StaticScenePurpose::PlacedBlock))) {
            return false;
        }
    }
    return true;
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
    const std::optional<ReplaySceneAssetPlacement> dedicatedCollisionAsset =
            placement.DedicatedCollisionAssetPlacement();
    if (mainAsset.has_value() || dedicatedCollisionAsset.has_value()) {
        const bool isDedicatedCollision =
                dedicatedCollisionAsset.has_value();
        const ReplaySceneAssetPlacement &selectedAsset =
                isDedicatedCollision ? *dedicatedCollisionAsset : *mainAsset;
        const StaticSolidPrototype prototype = selectedAsset.Prototype();
        if (!prototype.IsValid()) {
            complete = false;
        } else {
            StaticSceneModel model(
                    prototype,
                    placement.WorldIso(),
                    isDedicatedCollision
                            ? StaticScenePurpose::DedicatedInitialCollision
                            : StaticScenePurpose::PlacedBlock);
            if (isDedicatedCollision) {
                const std::optional<CHmsItem::Properties> properties =
                        placement.DedicatedCollisionProperties();
                if (!properties.has_value()) {
                    complete = false;
                } else {
                    model.SetItemProperties(*properties);
                }
            }
            if ((!isDedicatedCollision || model.ItemProperties().has_value()) &&
                !models.Add(std::move(model))) {
                return false;
            }
        }
    }
    return AddCheckpointTriggerModel(placement, models, complete) &&
           AddSubMobilModels(placement, models, complete) &&
           AddHelperModel(placement, models, complete);
}

bool AddPylonPlacement(
        const ReplayScenePylonPlacement &placement,
        StaticSceneModelCollection &models,
        bool &complete) {
    const std::optional<ReplaySceneAssetPlacement> asset =
            placement.AssetPlacement();
    if (!asset.has_value() || !asset->Prototype().IsValid()) {
        complete = false;
        return true;
    }
    StaticSceneModel model(asset->Prototype(),
                           placement.WorldIso(),
                           StaticScenePurpose::Pylon);
    model.SetItemProperties(placement.ItemProperties());
    return models.Add(std::move(model));
}

}  // namespace

bool AppendBlockPlacementModels(
        const ReplaySceneBlockPlacements &placements,
        StaticSceneModelCollection &models) {
    bool complete = models.IsComplete();
    const ReplaySceneModelCounts counts = placements.ModelCounts();
    if (!models.Reserve(models.Models().size() +
                        counts.placedBlocks +
                        counts.blockSubMobils +
                        counts.clipAssemblies +
                        counts.helperAssemblies +
                        counts.checkpointTriggers +
                        counts.pylons +
                        placements.DedicatedCollisionPlacements().size())) {
        return false;
    }

    for (const ReplaySceneInstallation &installation :
         placements.InstallationStream()) {
        if (const ReplaySceneBlockPlacement *block =
                    installation.BlockPlacement()) {
            if (!AddBlockPlacement(*block, models, complete)) {
                return false;
            }
        } else if (const ReplayScenePylonPlacement *pylon =
                           installation.PylonPlacement()) {
            if (!AddPylonPlacement(*pylon, models, complete)) {
                return false;
            }
        }
    }
    if (!complete) {
        models.MarkIncomplete();
    }
    return true;
}
