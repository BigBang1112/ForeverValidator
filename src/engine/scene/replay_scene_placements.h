#pragma once

#include <optional>
#include <vector>

#include "engine/core/engine_types.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/game/replay_checkpoint_trigger.h"
#include "engine/resources/static_solid_asset.h"
class CGameCtnBlock;
class CGameCtnChallenge;

class ReplaySceneAssetPlacement {
public:
    ReplaySceneAssetPlacement(
            StaticSolidAssetHandle asset,
            StaticSolidMaterialVariant materialVariant,
            std::optional<GmIso4> relativeTransform = std::nullopt);

    const StaticSolidAssetHandle &Asset() const;
    StaticSolidMaterialVariant MaterialVariant() const;
    const std::optional<GmIso4> &RelativeTransform() const;
    StaticSolidPrototype Prototype() const;

private:
    StaticSolidAssetHandle asset_;
    StaticSolidMaterialVariant materialVariant_ =
            StaticSolidMaterialVariant::Base;
    std::optional<GmIso4> relativeTransform_;
};

class ReplaySceneBlockPlacement {
public:
    ReplaySceneBlockPlacement(
            const CGameCtnBlock &block,
            const GmIso4 &worldIso);

    const CGameCtnBlock &Block() const;
    const GmIso4 &WorldIso() const;

    bool IsSuppressed() const;
    bool IsClip() const;
    bool IsStartLine() const;
    StaticSolidMaterialVariant MaterialVariant() const;

    std::optional<ReplaySceneAssetPlacement> MainAssetPlacement() const;
    std::vector<ReplaySceneAssetPlacement> HelperAssetPlacements() const;
    std::vector<ReplaySceneAssetPlacement> ClipAssetPlacements() const;
    std::optional<ReplaySceneAssetPlacement>
            CheckpointTriggerAssetPlacement() const;

    CHmsItem::Properties CheckpointTriggerProperties() const;
    CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity
            CheckpointIdentity() const;
    GmIso4 BlockSpawnIso() const;
    std::optional<GmIso4> CheckpointTriggerWorldIso() const;

private:
    const CGameCtnBlock *block_;
    GmIso4 worldIso_{};
};

struct ReplaySceneModelCounts {
    u32 placedBlocks = 0u;
    u32 clipAssemblies = 0u;
    u32 helperAssemblies = 0u;
    u32 checkpointTriggers = 0u;

    u32 TotalWithEnvironment(u32 environmentModelCount) const;
};

class ReplaySceneBlockPlacements {
public:
    void Clear();
    void PopulateFromChallenge(const CGameCtnChallenge &challenge);

    const std::vector<ReplaySceneBlockPlacement> &Placements() const;
    bool Empty() const;
    ReplaySceneModelCounts ModelCounts() const;
    std::optional<GmIso4> FirstStartLineSpawn() const;

private:
    void AppendBlock(const CGameCtnBlock &block);

    std::vector<ReplaySceneBlockPlacement> placements_;
};
