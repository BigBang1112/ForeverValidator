#include "engine/game/trackmania_race.h"
#include <algorithm>
#include <limits>
#include <new>

#include "engine/game/game_ctn_block_info.h"
#include "engine/core/mw_cmd_buffer_core.h"
#include "engine/scene/replay_static_scene_corpuses.h"
void CTrackManiaRace::ResetValidationSession() {
    player = {};
    checkpointSlotsPassed_.clear();
    playerSpawnLocation_.reset();
    lastAcceptedSpawnLocation_.reset();
    currentSpawnLocationInitialized_ = false;
    preparedEventTimeMs_ = 0u;
    progress_ = {};
    progress_.requiredLapCount = replayNbLaps_;
    vehicle = nullptr;
    checkpointCourse = nullptr;
    ConfigureReplayStuntsSimulation(false, 0u);
}

void CTrackManiaRace::SetReplayChallengePlayMode(
        EChallengePlayMode playMode) {
    replayPlayMode_ = playMode;
}

void CTrackManiaRace::InitNbLapsAndCheckpoints(unsigned long nbLaps) {
    replayNbLaps_ = nbLaps <= std::numeric_limits<u32>::max()
            ? static_cast<u32>(nbLaps)
            : std::numeric_limits<u32>::max();
    progress_.requiredLapCount = replayNbLaps_;
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

void CTrackManiaRace::ResetCheckpointSlots() {
    std::fill(checkpointSlotsPassed_.begin(),
              checkpointSlotsPassed_.end(), 0u);
    progress_.currentLapCheckpointCount = 0u;
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
    const std::optional<u32> checkpointSlot =
            checkpointCourse != nullptr
                    ? checkpointCourse->CheckpointSlotForBlock(checkpointBlock)
                    : std::nullopt;
    if (!checkpointSlot.has_value()) {
        return;
    }

    const GmIso4 *spawnIso = checkpointCourse != nullptr
                                     ? checkpointCourse->SpawnIsoForCheckpointBlock(checkpointBlock)
                                     : nullptr;
    if (blockInfo != nullptr && blockInfo->RespawnUsesCurrentTransform()) {
        spawnIso = &player.Info().CurrentSpawnLocation();
    }
    if (InternalOnCheckpoint(
                preparedEventTimeMs_, 0u,
                progress_.currentLapCheckpointCount,
                *checkpointSlot, &player.Info(), spawnIso,
                checkpointBlock != nullptr
                        ? checkpointBlock->TriggerMobil()
                        : nullptr,
                1) == 0) {
        return;
    }
    progress_.lastAcceptedBlockId = progress_.lastContactBlockId;
}

void CTrackManiaRace::OnFinishLine(CTrackManiaPlayer *finishPlayer,
                                   CGameCtnBlock *finishBlock) {
    (void)finishPlayer;
    const CGameCtnBlockInfo *blockInfo =
            finishBlock != nullptr ? finishBlock->BlockInfoRef() : nullptr;
    progress_.lastBlockRole = blockInfo != nullptr
            ? static_cast<u32>(blockInfo->RaceRole())
            : 0u;
    if (progress_.raceCompleted ||
        progress_.currentLapCheckpointCount <
                progress_.requiredCheckpointCount) {
        return;
    }

    if (replayPlayMode_ == EChallengePlayMode::Shortcut) {
        ++progress_.finishCount;
        progress_.completedLapCount = 1u;
        progress_.raceCompleted = true;
        return;
    }

    const u32 finishSlot = progress_.requiredCheckpointCount;
    if (InternalOnCheckpoint(
                preparedEventTimeMs_, 0u, finishSlot, finishSlot,
                &player.Info(), nullptr,
                finishBlock != nullptr ? finishBlock->TriggerMobil() : nullptr,
                0) == 0) {
        return;
    }

    ++progress_.finishCount;
    ++progress_.completedLapCount;
    if (progress_.requiredLapCount != 0u &&
        progress_.completedLapCount >= progress_.requiredLapCount) {
        progress_.raceCompleted = true;
        return;
    }
    ResetCheckpointSlots();
}

int CTrackManiaRace::InternalOnCheckpoint(
        unsigned long raceTime,
        unsigned long score,
        unsigned long checkpointIndex,
        unsigned long checkpointSlot,
        CTrackManiaPlayerInfo *playerInfo,
        const GmIso4 *spawnIso,
        const CSceneMobil *triggerMobil,
        int playSound) {
    (void)raceTime;
    (void)score;
    (void)triggerMobil;
    (void)playSound;
    if (playerInfo == nullptr ||
        checkpointIndex >= checkpointSlotsPassed_.size() ||
        checkpointSlot >= checkpointSlotsPassed_.size()) {
        return 0;
    }
    const std::size_t slot = static_cast<std::size_t>(checkpointSlot);
    if (checkpointSlotsPassed_[slot] != 0u) {
        return 0;
    }
    checkpointSlotsPassed_[slot] = 1u;
    if (checkpointIndex != progress_.requiredCheckpointCount) {
        ++progress_.currentLapCheckpointCount;
        ++progress_.checkpointCount;
    }
    ++progress_.totalCheckpointEventCount;
    if (spawnIso != nullptr) {
        EnsurePreviousSpawnLocationInitialized(*spawnIso);
        StoreSpawnLocation(*spawnIso);
    } else {
        const GmIso4 &previousSpawn = playerInfo->previousSpawnIso_;
        playerInfo->SetSpawnLoc(previousSpawn, 0);
        if (currentSpawnLocationInitialized_) {
            playerSpawnLocation_ = previousSpawn;
        }
    }
    ClearVehicleFreewheelState();
    return 1;
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
    progress_.requiredCheckpointCount = checkpointCourse != nullptr
            ? checkpointCourse->CheckpointCount()
            : 0u;
    try {
        checkpointSlotsPassed_.assign(
                static_cast<std::size_t>(
                        progress_.requiredCheckpointCount) + 1u,
                0u);
    } catch (const std::bad_alloc &) {
        checkpointSlotsPassed_.clear();
    }
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
