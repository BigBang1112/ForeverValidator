#include "engine/game/replay_checkpoint_trigger.h"
#include <memory>

#include "engine/game/game_ctn_block.h"
#include "engine/physics/collision/hms_collision.h"
#include "engine/physics/dynamics/hms_corpus.h"
void CGameCtnReplayCheckpointMobil::SetContactObserver(
        ReplayCheckpointContactObserver *observer) {
    contactObserver = observer;
}

void CGameCtnReplayCheckpointMobil::AbsorbContact(
        CHmsPhysicalContact &contact) {
    CHmsItem *item = contact.self != nullptr ? contact.self->Item() : nullptr;
    if (item != nullptr && contactObserver != nullptr) {
        contactObserver->OnCheckpointContact(*item, contact);
    }
}

CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity
CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity::FromRaceBlock(
        BlockRaceRole raceRole,
        u32 raceBlockId,
        bool respawnUsesCurrentTransform) {
    RaceCheckpointIdentity identity;
    identity.raceRole = raceRole;
    identity.raceBlockId = raceBlockId;
    identity.respawnUsesCurrentTransform = respawnUsesCurrentTransform;
    return identity;
}

void CGameCtnReplayCheckpointTrigger::Reset() {
    if (mobil != nullptr) {
        mobil->SetOwningBlock(nullptr);
        mobil->SetContactObserver(nullptr);
    }
    block.reset();
    blockInfo = nullptr;
    mobil = nullptr;
    raceIdentity = RaceCheckpointIdentity{};
}

void CGameCtnReplayCheckpointTrigger::PrepareOwner(
        const RaceCheckpointIdentity &identity,
        ReplayCheckpointContactObserver &observer) {
    Reset();

    raceIdentity = identity;
    mobil = MakeMwNod<CGameCtnReplayCheckpointMobil>();
    mobil->SetContactObserver(&observer);
    mobil->EnableAbsorbContactCallback(1);
    blockInfo = MakeMwNod<CGameCtnBlockInfo>();
    blockInfo->SetRaceRole(identity.raceRole);
    blockInfo->SetRespawnUsesCurrentTransform(
            identity.respawnUsesCurrentTransform);
    block = std::make_unique<CGameCtnBlock>(
            blockInfo.Get(),
            GmNat3(),
            ECardinalDir::North,
            0u,
            0,
            0,
            nullptr);
    mobil->SetOwningBlock(block.get());
    block->SetTriggerMobil(mobil.Get());
}

CSceneMobil *
CGameCtnReplayCheckpointTrigger::MobilOwner() {
    return mobil.Get();
}

const CSceneMobil *
CGameCtnReplayCheckpointTrigger::MobilOwner() const {
    return mobil.Get();
}

CGameCtnBlock *
CGameCtnReplayCheckpointTrigger::BlockOwner() {
    return block.get();
}

const CGameCtnBlock *
CGameCtnReplayCheckpointTrigger::BlockOwner() const {
    return block.get();
}

int CGameCtnReplayCheckpointTrigger::OwnsBlock(
        const CGameCtnBlock *candidate) const {
    return candidate == block.get();
}

const CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity &
CGameCtnReplayCheckpointTrigger::Identity() const {
    return raceIdentity;
}
