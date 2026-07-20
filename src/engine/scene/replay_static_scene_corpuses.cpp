#include "engine/scene/replay_static_scene_corpuses.h"
#include "engine/game/game_ctn_block.h"
#include "engine/scene/scene_mobil.h"
#include <new>
bool ReplayStaticCorpusBody::ConstructStaticMobil(
        const StaticSolidPrototype &prototype,
        const GmIso4 &worldIso) {
    CMwNodRef<CPlugSolid> solid = prototype.CreateInstance();
    if (solid.Get() == nullptr || solid->CollisionTree() == nullptr) {
        return false;
    }
    item.SetSolid(solid.Get());
    corpus.SetDynamics(nullptr);
    corpus.SetItem(&item);
    corpus.SetLocation(worldIso);
    return true;
}
bool ReplayStaticCorpusBody::OwnsTreeInAncestry(
        const CPlugTree *tree) const {
    return tree != nullptr && tree == CollisionTree();
}

void ReplayStaticCorpusBody::MarkCollisionTreeAsTrigger() {
    if (CPlugTree *tree = CollisionTree()) {
        tree->SetCollisionEnabled(true);
    }
}

CHmsItem &ReplayStaticCorpusBody::Item() {
    return item;
}

const CHmsItem &ReplayStaticCorpusBody::Item() const {
    return item;
}

CHmsCorpus &ReplayStaticCorpusBody::Corpus() {
    return corpus;
}

const CHmsCorpus &ReplayStaticCorpusBody::Corpus() const {
    return corpus;
}

CPlugTree *ReplayStaticCorpusBody::CollisionTree() const {
    return item.Solid() != nullptr ? item.Solid()->CollisionTree() : nullptr;
}

ReplayStaticCorpus::~ReplayStaticCorpus() {
    DetachFromWorld();
}

bool ReplayStaticCorpus::Configure(
        const StaticSceneModel &model,
        u32 installationOrder) {
    if (!body_.ConstructStaticMobil(model.Prototype(), model.WorldIso())) {
        return false;
    }
    purpose = model.Purpose();
    installationOrder_ = installationOrder;
    if (model.ItemProperties().has_value()) {
        body_.Item().SetProperties(*model.ItemProperties());
        if (purpose == StaticScenePurpose::CheckpointTrigger) {
            body_.Item().SetIsCollisionStatic(1);
        }
    } else {
        body_.Item().ApplyBlockMobilDefaults();
    }
    checkpointIdentity = model.CheckpointIdentity();
    checkpointSpawnIso = model.CheckpointSpawnIso();
    return true;
}

void ReplayStaticCorpus::InstallIntoZone(
        CHmsCollisionManager::SZone &zone) {
    zone.AddCorpus(&body_.Corpus());
}

void ReplayStaticCorpus::DetachFromWorld() {
    body_.Corpus().DetachFromWorld();
    body_.Item().SetSolid(nullptr);
}

bool ReplayStaticCorpus::IsRaceTriggerMobil() const {
    return purpose == StaticScenePurpose::CheckpointTrigger;
}

u32 ReplayStaticCorpus::InstallationOrder() const {
    return installationOrder_;
}

StaticScenePurpose ReplayStaticCorpus::Purpose() const {
    return purpose;
}

const CHmsItem &ReplayStaticCorpus::Item() const {
    return body_.Item();
}

const CHmsCorpus &ReplayStaticCorpus::Corpus() const {
    return body_.Corpus();
}

u32 ReplayStaticCorpus::InstallRaceTriggerHook(
        ReplayCheckpointContactObserver &observer) {
    if (!IsRaceTriggerMobil() || !checkpointIdentity.has_value()) {
        return 0u;
    }
    checkpointTrigger.PrepareOwner(*checkpointIdentity, observer);
    CHmsItem &item = body_.Item();
    CSceneMobil *owner = checkpointTrigger.MobilOwner();
    const bool alreadyInstalled = item.SceneMobilOwner() == owner;
    if (owner != nullptr) {
        owner->AttachTriggerPhysicsItem(item);
    }
    body_.MarkCollisionTreeAsTrigger();
    return alreadyInstalled ? 0u : 1u;
}

const GmIso4 *
ReplayStaticCorpus::SpawnIsoForCheckpointBlock(
        const CGameCtnBlock *block) const {
    return checkpointSpawnIso.has_value() &&
                   checkpointTrigger.OwnsBlock(block)
            ? &*checkpointSpawnIso
            : nullptr;
}

u32 ReplayStaticCorpus::RaceBlockIdForCheckpointBlock(
        const CGameCtnBlock *block) const {
    return checkpointTrigger.OwnsBlock(block)
            ? checkpointTrigger.Identity().raceBlockId
            : 0u;
}

bool ReplayStaticCorpus::IsCheckpoint() const {
    return checkpointIdentity.has_value() &&
           checkpointIdentity->raceRole == BlockRaceRole::Checkpoint;
}

bool ReplayStaticCorpus::OwnsCheckpointBlock(
        const CGameCtnBlock *block) const {
    return IsCheckpoint() && checkpointTrigger.OwnsBlock(block);
}

bool ReplayStaticCorpus::OwnsTreeInAncestry(
        const CPlugTree *tree) const {
    return body_.OwnsTreeInAncestry(tree);
}

void ReplayStaticCorpusCollection::Clear() {
    corpuses.clear();
}

bool ReplayStaticCorpusCollection::BuildFromModels(
        const StaticSceneModelCollection &models,
        ReplayStaticCorpusModelSelection selection) {
    Clear();
    try {
        corpuses.reserve(models.Models().size());
        for (u32 modelIndex = 0u;
             modelIndex < models.Models().size();
             ++modelIndex) {
            const StaticSceneModel &model = models.Models()[modelIndex];
            const bool dedicated = model.Purpose() ==
                    StaticScenePurpose::DedicatedInitialCollision;
            if ((selection == ReplayStaticCorpusModelSelection::
                                      ExcludeDedicatedInitialCollision &&
                 dedicated) ||
                (selection == ReplayStaticCorpusModelSelection::
                                      DedicatedInitialCollisionOnly &&
                 !dedicated)) {
                continue;
            }
            auto corpus =
                    std::make_unique<ReplayStaticCorpus>();
            if (!corpus->Configure(model, modelIndex)) {
                Clear();
                return false;
            }
            corpuses.push_back(std::move(corpus));
        }
    } catch (const std::bad_alloc &) {
        Clear();
        return false;
    }
    return true;
}

bool ReplayStaticCorpusCollection::
        InstallIntoZone(CHmsCollisionManager::SZone &zone) {
    for (const std::unique_ptr<ReplayStaticCorpus> &corpus :
         corpuses) {
        corpus->InstallIntoZone(zone);
    }
    zone.UpdateStaticCollisionTrees();
    return true;
}

u32 ReplayStaticCorpusCollection::Count() const {
    return static_cast<u32>(corpuses.size());
}

const GmIso4 *
ReplayStaticCorpusCollection::SpawnIsoForCheckpointBlock(
        const CGameCtnBlock *block) const {
    for (const std::unique_ptr<ReplayStaticCorpus> &corpus :
         corpuses) {
        if (const GmIso4 *spawn = corpus->SpawnIsoForCheckpointBlock(block)) {
            return spawn;
        }
    }
    return nullptr;
}

u32 ReplayStaticCorpusCollection::RaceBlockIdForCheckpointBlock(
        const CGameCtnBlock *block) const {
    for (const std::unique_ptr<ReplayStaticCorpus> &corpus :
         corpuses) {
        const u32 id = corpus->RaceBlockIdForCheckpointBlock(block);
        if (id != 0u) {
            return id;
        }
    }
    return 0u;
}

u32 ReplayStaticCorpusCollection::CheckpointCount() const {
    u32 count = 0u;
    for (const std::unique_ptr<ReplayStaticCorpus> &corpus : corpuses) {
        count += corpus->IsCheckpoint() ? 1u : 0u;
    }
    return count;
}

std::optional<u32>
ReplayStaticCorpusCollection::CheckpointSlotForBlock(
        const CGameCtnBlock *block) const {
    u32 slot = 0u;
    for (const std::unique_ptr<ReplayStaticCorpus> &corpus : corpuses) {
        if (!corpus->IsCheckpoint()) {
            continue;
        }
        if (corpus->OwnsCheckpointBlock(block)) {
            return slot;
        }
        ++slot;
    }
    return std::nullopt;
}

u32 ReplayStaticCorpusCollection::InstallRaceTriggerHooks(
        ReplayCheckpointContactObserver &observer) {
    u32 installed = 0u;
    for (const std::unique_ptr<ReplayStaticCorpus> &corpus :
         corpuses) {
        installed += corpus->InstallRaceTriggerHook(observer);
    }
    return installed;
}

ReplayStaticCorpus *ReplayStaticCorpusCollection::CorpusAt(u32 index) {
    return index < corpuses.size() ? corpuses[index].get() : nullptr;
}

const ReplayStaticCorpus *ReplayStaticCorpusCollection::CorpusAt(
        u32 index) const {
    return index < corpuses.size() ? corpuses[index].get() : nullptr;
}

void ReplayDedicatedCollisionCorpusCollection::Clear() {
    corpuses_.Clear();
}

bool ReplayDedicatedCollisionCorpusCollection::BuildFromModels(
        const StaticSceneModelCollection &models) {
    return corpuses_.BuildFromModels(
            models,
            ReplayStaticCorpusModelSelection::
                    DedicatedInitialCollisionOnly);
}

u32 ReplayDedicatedCollisionCorpusCollection::Count() const {
    return corpuses_.Count();
}

ReplayStaticCorpus *ReplayDedicatedCollisionCorpusCollection::CorpusAt(
        u32 index) {
    return corpuses_.CorpusAt(index);
}

const ReplayStaticCorpus *
ReplayDedicatedCollisionCorpusCollection::CorpusAt(u32 index) const {
    return corpuses_.CorpusAt(index);
}

bool InstallReplaySceneCollisionCorpuses(
        ReplayStaticCorpusCollection &staticCorpuses,
        ReplayDedicatedCollisionCorpusCollection &dedicatedCorpuses,
        CHmsCollisionManager::SZone &zone) {
    u32 staticIndex = 0u;
    u32 dedicatedIndex = 0u;
    while (staticIndex < staticCorpuses.Count() ||
           dedicatedIndex < dedicatedCorpuses.Count()) {
        ReplayStaticCorpus *stationary =
                staticCorpuses.CorpusAt(staticIndex);
        ReplayStaticCorpus *dedicated =
                dedicatedCorpuses.CorpusAt(dedicatedIndex);
        if (dedicated == nullptr ||
            (stationary != nullptr &&
             stationary->InstallationOrder() <
                     dedicated->InstallationOrder())) {
            stationary->InstallIntoZone(zone);
            ++staticIndex;
        } else {
            dedicated->InstallIntoZone(zone);
            ++dedicatedIndex;
        }
    }
    zone.UpdateStaticCollisionTrees();
    return true;
}
