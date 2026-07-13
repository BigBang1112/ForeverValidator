#pragma once

#include <memory>
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

    bool Configure(const StaticSceneModel &model);
    void InstallIntoZone(CHmsCollisionManager::SZone &zone);
    void DetachFromWorld();
    u32 InstallRaceTriggerHook(ReplayCheckpointContactObserver &observer);
    const GmIso4 *SpawnIsoForCheckpointBlock(
            const CGameCtnBlock *block) const;
    u32 RaceBlockIdForCheckpointBlock(const CGameCtnBlock *block) const;
    bool OwnsTreeInAncestry(const CPlugTree *tree) const;
    bool IsRaceTriggerMobil() const;

private:
    ReplayStaticCorpusBody body_;
    CGameCtnReplayCheckpointTrigger checkpointTrigger;
    StaticScenePurpose purpose = StaticScenePurpose::PlacedBlock;
    std::optional<
            CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity>
            checkpointIdentity;
    std::optional<GmIso4> checkpointSpawnIso;
};

class ReplayStaticCorpusCollection {
public:
    void Clear();
    bool BuildFromModels(const StaticSceneModelCollection &models);
    bool InstallIntoZone(
            CHmsCollisionManager::SZone &zone);

    u32 Count() const;
    const GmIso4 *SpawnIsoForCheckpointBlock(
            const CGameCtnBlock *block) const;
    u32 RaceBlockIdForCheckpointBlock(const CGameCtnBlock *block) const;
    u32 InstallRaceTriggerHooks(ReplayCheckpointContactObserver &observer);

private:
    std::vector<std::unique_ptr<ReplayStaticCorpus>> corpuses;
};

bool AppendBlockPlacementModels(
        const ReplaySceneBlockPlacements &placements,
        StaticSceneModelCollection &models);
