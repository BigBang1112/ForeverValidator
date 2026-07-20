#pragma once

#include <optional>
#include <vector>

#include "engine/core/gm_types.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/game/replay_checkpoint_trigger.h"
#include "engine/resources/static_solid_asset.h"
enum class StaticScenePurpose {
    Environment,
    PlacedBlock,
    Clip,
    Helper,
    CheckpointTrigger,
    DedicatedInitialCollision,
    Pylon,
};

class StaticSceneModel {
public:
    StaticSceneModel(StaticSolidPrototype prototype,
                     const GmIso4 &worldIso,
                     StaticScenePurpose purpose);

    const StaticSolidPrototype &Prototype() const;
    const GmIso4 &WorldIso() const;
    StaticScenePurpose Purpose() const;

    void SetItemProperties(const CHmsItem::Properties &properties);
    const std::optional<CHmsItem::Properties> &ItemProperties() const;

    void SetCheckpoint(
            CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity identity,
            std::optional<GmIso4> spawnIso);
    const std::optional<
            CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity> &
    CheckpointIdentity() const;
    const std::optional<GmIso4> &CheckpointSpawnIso() const;

private:
    StaticSolidPrototype prototype_;
    GmIso4 worldIso_{};
    StaticScenePurpose purpose_ = StaticScenePurpose::PlacedBlock;
    std::optional<CHmsItem::Properties> itemProperties_;
    std::optional<CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity>
            checkpointIdentity_;
    std::optional<GmIso4> checkpointSpawnIso_;
};

class StaticSceneModelCollection {
public:
    void Clear();
    bool Reserve(std::size_t count);
    bool Add(StaticSceneModel model);
    const std::vector<StaticSceneModel> &Models() const;
    bool Empty() const;
    void MarkIncomplete();
    bool IsComplete() const;

private:
    std::vector<StaticSceneModel> models_;
    bool complete_ = true;
};
