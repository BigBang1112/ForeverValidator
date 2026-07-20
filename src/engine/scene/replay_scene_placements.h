#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

#include "engine/core/engine_types.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/game/replay_checkpoint_trigger.h"
#include "engine/resources/static_solid_asset.h"
#include "engine/scene/scene_mobil.h"
class CGameCtnBlock;
class CGameCtnChallenge;

using ReplaySceneInstallationOrdinal = std::uint64_t;

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
            const GmIso4 &worldIso,
            ReplaySceneInstallationOrdinal installationOrdinal = 0u);

    const CGameCtnBlock &Block() const;
    const GmIso4 &WorldIso() const;
    ReplaySceneInstallationOrdinal InstallationOrdinal() const;
    bool ContainsMobilGeneration(const CSceneMobil *mobil) const;
    bool RemoveMobilFromGeneration(const CSceneMobil *mobil);
    bool HasSceneProducingMobils() const;
    bool HasPublicSceneRepresentation() const;
    bool HasDedicatedCollisionRepresentation() const;

    bool IsSuppressed() const;
    bool IsClip() const;
    bool IsStartLine() const;
    StaticSolidMaterialVariant MaterialVariant() const;

    std::optional<ReplaySceneAssetPlacement> MainAssetPlacement() const;
    std::vector<ReplaySceneAssetPlacement> SubMobilAssetPlacements() const;
    std::optional<ReplaySceneAssetPlacement>
            DedicatedCollisionAssetPlacement() const;
    std::optional<CHmsItem::Properties> DedicatedCollisionProperties() const;
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
    ReplaySceneInstallationOrdinal installationOrdinal_ = 0u;
    CMwNodRef<CSceneMobil> mainMobil_;
    CMwNodRef<CSceneMobil> helperMobil_;
    CMwNodRef<CSceneMobil> triggerMobil_;
    std::vector<CMwNodRef<CSceneMobil>> subMobils_;
    CMwNodRef<CSceneMobil> familyHelperMobilSource_;
    CMwNodRef<CSceneMobil> commonHelperMobilSource_;
    std::array<CMwNodRef<CSceneMobil>, 4> clipSourceMobils_;
    StaticSolidMaterialVariant materialVariant_ =
            StaticSolidMaterialVariant::BlockMobil;
    bool suppressed_ = false;
    bool clip_ = false;
    bool startLine_ = false;
    float collectionSquareSize_ = 32.0f;
    CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity
            checkpointIdentity_{};
    GmIso4 blockSpawnIso_{};
};

class ReplayScenePylonPlacement {
public:
    ReplayScenePylonPlacement(
            CSceneMobil &mobil,
            const GmIso4 &worldIso,
            ReplaySceneInstallationOrdinal installationOrdinal = 0u);

    CSceneMobil &Mobil() const;
    const GmIso4 &WorldIso() const;
    ReplaySceneInstallationOrdinal InstallationOrdinal() const;
    bool ContainsMobil(const CSceneMobil *mobil) const;
    std::optional<ReplaySceneAssetPlacement> AssetPlacement() const;
    CHmsItem::Properties ItemProperties() const;

private:
    CMwNodRef<CSceneMobil> mobil_;
    GmIso4 worldIso_{};
    ReplaySceneInstallationOrdinal installationOrdinal_ = 0u;
};

enum class ReplaySceneInstallationTag {
    Block,
    Pylon,
};

class ReplaySceneInstallation {
public:
    explicit ReplaySceneInstallation(ReplaySceneBlockPlacement placement);
    explicit ReplaySceneInstallation(ReplayScenePylonPlacement placement);

    ReplaySceneInstallationTag Tag() const;
    ReplaySceneInstallationOrdinal InstallationOrdinal() const;
    bool IsActive() const;
    bool ContainsMobil(const CSceneMobil *mobil) const;
    bool RemoveMobil(const CSceneMobil *mobil);
    const ReplaySceneBlockPlacement *BlockPlacement() const;
    const ReplayScenePylonPlacement *PylonPlacement() const;

private:
    ReplaySceneInstallationTag tag_ = ReplaySceneInstallationTag::Block;
    ReplaySceneInstallationOrdinal installationOrdinal_ = 0u;
    std::variant<
            std::monostate,
            ReplaySceneBlockPlacement,
            ReplayScenePylonPlacement> payload_;
};

struct ReplaySceneModelCounts {
    u32 placedBlocks = 0u;
    u32 blockSubMobils = 0u;
    u32 clipAssemblies = 0u;
    u32 helperAssemblies = 0u;
    u32 checkpointTriggers = 0u;
    u32 pylons = 0u;

    u32 TotalWithEnvironment(u32 environmentModelCount) const;
};

struct ReplaySceneLogicalStartPlacement {
    u32 authoredOrdinal = 0u;
    GmIso4 spawnLocation{};
};

class ReplaySceneBlockPlacements {
public:
    void Clear();
    void PopulateFromChallenge(CGameCtnChallenge &challenge);
    void SetRetainedLogicalStartPlacements(
            std::vector<ReplaySceneLogicalStartPlacement> placements);
    void AppendBlockMobil(const CGameCtnBlock &block);
    void AppendPylonMobil(CSceneMobil &mobil, const GmIso4 &worldIso);
    void RemoveMobilFromScene(CSceneMobil &mobil);
    void RemoveRetiredLogicalBlock(const CGameCtnBlock &block);

    const std::vector<ReplaySceneBlockPlacement> &Placements() const;
    const std::vector<ReplaySceneBlockPlacement> &CollisionPlacements() const;
    const std::vector<ReplaySceneBlockPlacement> &
            DedicatedCollisionPlacements() const;
    const std::vector<ReplayScenePylonPlacement> &PylonPlacements() const;
    const std::vector<ReplaySceneInstallation> &InstallationStream() const;
    bool ContainsMobil(const CSceneMobil *mobil) const;
    bool ReferencesBlock(const CGameCtnBlock *block) const;
    bool Empty() const;
    ReplaySceneModelCounts ModelCounts() const;
    std::optional<GmIso4> FirstSurvivingStartLineSpawn() const;
    std::optional<GmIso4> FirstRetainedLogicalStartLineSpawn() const;
    std::optional<GmIso4> FirstStartLineSpawn() const;

private:
    void AppendLogicalStart(const CGameCtnBlock &block);
    void AppendBlock(const CGameCtnBlock &block,
                     ReplaySceneInstallationOrdinal ordinal);
    void RemoveBlockOrdinal(ReplaySceneInstallationOrdinal ordinal,
                            const CSceneMobil *mobil,
                            bool remainsActive);
    void RemovePylonOrdinal(ReplaySceneInstallationOrdinal ordinal);

    std::vector<ReplaySceneBlockPlacement> placements_;
    std::vector<ReplaySceneBlockPlacement> collisionPlacements_;
    std::vector<ReplaySceneBlockPlacement> dedicatedCollisionPlacements_;
    std::vector<ReplayScenePylonPlacement> pylonPlacements_;
    std::vector<ReplaySceneInstallation> installationStream_;
    std::vector<ReplaySceneLogicalStartPlacement>
            retainedLogicalStartPlacements_;
    ReplaySceneInstallationOrdinal nextInstallationOrdinal_ = 0u;
};
