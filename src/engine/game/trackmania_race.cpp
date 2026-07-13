#include "engine/game/trackmania_race.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/core/mw_cmd_buffer_core.h"
#include "engine/scene/replay_static_scene_corpuses.h"
void CTrackManiaRace::ResetValidationSession() {
    player = {};
    acceptedCheckpointIds.clear();
    playerSpawnLocation_.reset();
    lastAcceptedSpawnLocation_.reset();
    currentSpawnLocationInitialized_ = false;
    preparedEventTimeMs_ = 0u;
    progress_ = {};
    vehicle = nullptr;
    checkpointCourse = nullptr;
}

void CTrackManiaRace::BindVehicle(CSceneVehicleCar *vehicleForRaceEvents) {
    vehicle = vehicleForRaceEvents;
}

void CTrackManiaRace::BindCheckpointCourse(
        ReplayStaticCorpusCollection *course) {
    checkpointCourse = course;
}

void CTrackManiaRace::SetInitialSpawnLocation(const GmIso4 &spawnIso) {
    player.Info().SetSpawnLoc(spawnIso, 1);
    playerSpawnLocation_ = spawnIso;
    currentSpawnLocationInitialized_ = true;
}

bool CTrackManiaRace::HasRespawnLocation() const {
    return playerSpawnLocation_.has_value();
}

const GmIso4 &CTrackManiaRace::RespawnLocation() const {
    return player.Info().CurrentSpawnLocation();
}

CTrackManiaPlayer *CTrackManiaRace::GetPlayerFromMobil(CSceneMobil *mobil) {
    return vehicle != nullptr && mobil == static_cast<CSceneMobil *>(vehicle)
                   ? &player
                   : nullptr;
}

CTrackManiaPlayer *CTrackManiaRace::GetPlayingPlayer(void) {
    return &player;
}

CGameCtnBlock *CTrackManiaRace::GetBlockFromCheckpointMobil(CSceneMobil *mobil) {
    return mobil != nullptr ? mobil->OwningBlock() : nullptr;
}

void CTrackManiaRace::InternalPrepareEvent(CTrackManiaPlayer *eventPlayer) {
    if (eventPlayer == nullptr) {
        return;
    }

    const CMwCmdBufferCore *commandBuffer = CMwCmdBufferCore::Current();
    preparedEventTimeMs_ =
            commandBuffer != nullptr ? commandBuffer->Timer().GetTickTime() : 0ul;
    eventPlayer->Info().MarkEventPrepared();
    progress_.lastPrepareTimeMs = preparedEventTimeMs_;
    ++progress_.preparedEventCount;
}

void CTrackManiaRace::StoreSpawnLocation(const GmIso4 &spawnIso) {
    lastAcceptedSpawnLocation_ = spawnIso;
    playerSpawnLocation_ = spawnIso;
    player.Info().SetSpawnLoc(spawnIso, 0);
}

void CTrackManiaRace::EnsurePreviousSpawnLocationInitialized(
        const GmIso4 &spawnIso) {
    if (currentSpawnLocationInitialized_) {
        return;
    }
    player.Info().SetPreviousSpawnLocation(spawnIso);
    currentSpawnLocationInitialized_ = true;
}

bool CTrackManiaRace::AcceptCheckpointOnce(const CGameCtnBlock *block) {
    const u32 checkpointId = checkpointCourse != nullptr
                                     ? checkpointCourse->RaceBlockIdForCheckpointBlock(block)
                                     : 0u;
    return checkpointId != 0u && acceptedCheckpointIds.insert(checkpointId).second;
}

void CTrackManiaRace::OnCheckpoint(CTrackManiaPlayer *checkpointPlayer,
                                   CGameCtnBlock *checkpointBlock) {
    (void)checkpointPlayer;
    const CGameCtnBlockInfo *blockInfo =
            checkpointBlock != nullptr ? checkpointBlock->BlockInfoRef() : nullptr;
    progress_.lastBlockRole = blockInfo != nullptr
            ? static_cast<u32>(blockInfo->RaceRole())
            : 0u;
    progress_.lastContactBlockId =
            checkpointCourse != nullptr
                    ? checkpointCourse->RaceBlockIdForCheckpointBlock(checkpointBlock)
                    : 0u;
    if (!AcceptCheckpointOnce(checkpointBlock)) {
        return;
    }

    ++progress_.checkpointCount;
    progress_.lastAcceptedBlockId = progress_.lastContactBlockId;
    const GmIso4 *spawnIso = checkpointCourse != nullptr
                                     ? checkpointCourse->SpawnIsoForCheckpointBlock(checkpointBlock)
                                     : nullptr;
    if (blockInfo != nullptr && blockInfo->RespawnUsesCurrentTransform()) {
        spawnIso = &player.Info().CurrentSpawnLocation();
    }
    if (spawnIso != nullptr) {
        EnsurePreviousSpawnLocationInitialized(*spawnIso);
        StoreSpawnLocation(*spawnIso);
    }
    ClearVehicleFreewheelState();
}

void CTrackManiaRace::OnFinishLine(CTrackManiaPlayer *finishPlayer,
                                   CGameCtnBlock *finishBlock) {
    (void)finishPlayer;
    const CGameCtnBlockInfo *blockInfo =
            finishBlock != nullptr ? finishBlock->BlockInfoRef() : nullptr;
    progress_.lastBlockRole = blockInfo != nullptr
            ? static_cast<u32>(blockInfo->RaceRole())
            : 0u;
    ++progress_.finishCount;
    acceptedCheckpointIds.clear();
    if (currentSpawnLocationInitialized_) {
        StoreSpawnLocation(player.Info().CurrentSpawnLocation());
    }
    ClearVehicleFreewheelState();
}

void CTrackManiaRace::ClearVehicleFreewheelState() {
    if (vehicle != nullptr) {
        vehicle->VehicleFreeWheelingSet(0);
    }
    ++progress_.freewheelClearCount;
}

void CTrackManiaRace::PrepareCheckpoints() {
    progress_.installedTriggerCount = checkpointCourse != nullptr
                                              ? checkpointCourse->InstallRaceTriggerHooks(*this)
                                              : 0u;
}

void CTrackManiaRace::OnCheckpointContact(
        CHmsItem &item,
        CHmsPhysicalContact &contact) {
    if (contact.peer == nullptr || contact.peer->Item() == nullptr) {
        return;
    }

    CSceneMobil *itemMobil = item.SceneMobilOwner();
    if (itemMobil == nullptr) {
        return;
    }

    CGameCtnBlock *block = GetBlockFromCheckpointMobil(itemMobil);
    if (block == nullptr || block->BlockInfoRef() == nullptr ||
        block->BlockInfoRef()->RaceRole() == BlockRaceRole::None) {
        return;
    }

    CTrackManiaPlayer *contactPlayer =
            GetPlayerFromMobil(contact.peer->Item()->SceneMobilOwner());
    if (contactPlayer == nullptr || contactPlayer != GetPlayingPlayer()) {
        return;
    }

    InternalPrepareEvent(contactPlayer);
    switch (block->BlockInfoRef()->RaceRole()) {
    case BlockRaceRole::FinishLine:
    case BlockRaceRole::StartFinishLine:
        OnFinishLine(contactPlayer, block);
        return;
    case BlockRaceRole::Checkpoint:
        OnCheckpoint(contactPlayer, block);
        return;
    default:
        return;
    }
}
