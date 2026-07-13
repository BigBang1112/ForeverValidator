#ifndef TMNF_TRACKMANIA_RACE_H
#define TMNF_TRACKMANIA_RACE_H

#include "engine/core/engine_types.h"
#include <optional>
#include <unordered_set>

#include "engine/game/game_ctn_block.h"
#include "engine/physics/collision/hms_collision.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/game/replay_checkpoint_trigger.h"
#include "engine/game/trackmania_player.h"
#include "engine/scene/scene_mobil.h"
#include "engine/scene/scene_vehicle_car.h"
class ReplayStaticCorpusCollection;

struct ReplayRaceProgress {
    u32 installedTriggerCount = 0u;
    u32 preparedEventCount = 0u;
    u32 checkpointCount = 0u;
    u32 finishCount = 0u;
    u32 freewheelClearCount = 0u;
    u32 lastBlockRole = 0u;
    u32 lastPrepareTimeMs = 0u;
    u32 lastAcceptedBlockId = 0u;
    u32 lastContactBlockId = 0u;
};

class CTrackManiaRace : public ReplayCheckpointContactObserver {
public:
    void ResetValidationSession();
    void BindVehicle(CSceneVehicleCar *vehicle);
    void BindCheckpointCourse(
            ReplayStaticCorpusCollection *course);
    void SetInitialSpawnLocation(const GmIso4 &spawnIso);
    bool HasRespawnLocation() const;
    const GmIso4 &RespawnLocation() const;
    const ReplayRaceProgress &Progress() const { return progress_; }

    CTrackManiaPlayer *GetPlayerFromMobil(CSceneMobil *mobil);
    CTrackManiaPlayer *GetPlayingPlayer(void);
    CGameCtnBlock *GetBlockFromCheckpointMobil(CSceneMobil *mobil);
    void InternalPrepareEvent(CTrackManiaPlayer *player);
    virtual void OnCheckpoint(CTrackManiaPlayer *player,
                              CGameCtnBlock *block);
    virtual void OnFinishLine(CTrackManiaPlayer *player,
                              CGameCtnBlock *block);
    void PrepareCheckpoints();

    void OnCheckpointContact(CHmsItem &item,
                             CHmsPhysicalContact &contact) override;

private:
    void StoreSpawnLocation(const GmIso4 &spawnIso);
    void EnsurePreviousSpawnLocationInitialized(const GmIso4 &spawnIso);
    bool AcceptCheckpointOnce(const CGameCtnBlock *block);
    void ClearVehicleFreewheelState();

    CTrackManiaPlayer player{};
    CSceneVehicleCar *vehicle = nullptr;
    ReplayStaticCorpusCollection *checkpointCourse = nullptr;
    std::unordered_set<u32> acceptedCheckpointIds{};
    std::optional<GmIso4> playerSpawnLocation_;
    std::optional<GmIso4> lastAcceptedSpawnLocation_;
    bool currentSpawnLocationInitialized_ = false;
    u32 preparedEventTimeMs_ = 0u;
    ReplayRaceProgress progress_{};
};

#endif
