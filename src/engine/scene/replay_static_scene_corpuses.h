#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "engine/physics/collision/hms_collision_manager.h"
#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/game/replay_checkpoint_trigger.h"
#include "engine/scene/static_scene_model.h"
class CGameCtnBlock;
class ReplaySceneBlockPlacements;

class ReplayStaticCorpusBody {
public:
    bool ConstructStaticMobil(const StaticSolidPrototype &prototype,
                              const GmIso4 &worldIso);
    bool OwnsTreeInAncestry(const CPlugTree *tree) const;
    void MarkCollisionTreeAsTrigger();

    CHmsItem &Item();
    const CHmsItem &Item() const;
    CHmsCorpus &Corpus();
    const CHmsCorpus &Corpus() const;
    CPlugTree *CollisionTree() const;

private:
    CHmsItem item;
    CHmsCorpus corpus;
};

class ReplayStaticCorpus {
public:
    ReplayStaticCorpus() = default;
    ~ReplayStaticCorpus();

    bool Configure(const StaticSceneModel &model,
                   u32 installationOrder = 0u);
    void InstallIntoZone(CHmsCollisionManager::SZone &zone);
    void DetachFromWorld();
    u32 InstallRaceTriggerHook(ReplayCheckpointContactObserver &observer);
    const GmIso4 *SpawnIsoForCheckpointBlock(
            const CGameCtnBlock *block) const;
    u32 RaceBlockIdForCheckpointBlock(const CGameCtnBlock *block) const;
    bool IsCheckpoint() const;
    bool OwnsCheckpointBlock(const CGameCtnBlock *block) const;
    bool OwnsTreeInAncestry(const CPlugTree *tree) const;
    bool IsRaceTriggerMobil() const;
    u32 InstallationOrder() const;
    StaticScenePurpose Purpose() const;
    const CHmsItem &Item() const;
    const CHmsCorpus &Corpus() const;

private:
    ReplayStaticCorpusBody body_;
    CGameCtnReplayCheckpointTrigger checkpointTrigger;
    StaticScenePurpose purpose = StaticScenePurpose::PlacedBlock;
    std::optional<
            CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity>
            checkpointIdentity;
    std::optional<GmIso4> checkpointSpawnIso;
    u32 installationOrder_ = 0u;
};

enum class ReplayStaticCorpusModelSelection {
    All,
    ExcludeDedicatedInitialCollision,
    DedicatedInitialCollisionOnly,
};

class ReplayStaticCorpusCollection {
public:
    void Clear();
    bool BuildFromModels(
            const StaticSceneModelCollection &models,
            ReplayStaticCorpusModelSelection selection =
                    ReplayStaticCorpusModelSelection::All);
    bool InstallIntoZone(
            CHmsCollisionManager::SZone &zone);

    u32 Count() const;
    const GmIso4 *SpawnIsoForCheckpointBlock(
            const CGameCtnBlock *block) const;
    u32 RaceBlockIdForCheckpointBlock(const CGameCtnBlock *block) const;
    u32 CheckpointCount() const;
    std::optional<u32> CheckpointSlotForBlock(
            const CGameCtnBlock *block) const;
    u32 InstallRaceTriggerHooks(ReplayCheckpointContactObserver &observer);
    ReplayStaticCorpus *CorpusAt(u32 index);
    const ReplayStaticCorpus *CorpusAt(u32 index) const;

private:
    std::vector<std::unique_ptr<ReplayStaticCorpus>> corpuses;
};

class ReplayDedicatedCollisionCorpusCollection {
public:
    void Clear();
    bool BuildFromModels(const StaticSceneModelCollection &models);
    u32 Count() const;
    ReplayStaticCorpus *CorpusAt(u32 index);
    const ReplayStaticCorpus *CorpusAt(u32 index) const;

private:
    ReplayStaticCorpusCollection corpuses_;
};

bool InstallReplaySceneCollisionCorpuses(
        ReplayStaticCorpusCollection &staticCorpuses,
        ReplayDedicatedCollisionCorpusCollection &dedicatedCorpuses,
        CHmsCollisionManager::SZone &zone);

bool AppendBlockPlacementModels(
        const ReplaySceneBlockPlacements &placements,
        StaticSceneModelCollection &models);
