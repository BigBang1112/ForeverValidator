#include "engine/scene/replay_scene_placements.h"
#include <array>
#include <utility>

#include "engine/game/game_ctn_block.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/scene/scene_mobil.h"
namespace {

StaticSolidAssetHandle Asset(const CSceneMobil *mobil) {
    return mobil != nullptr
            ? mobil->StaticSolidAsset().Asset()
            : StaticSolidAssetHandle{};
}

CSceneMobil *FamilyHelper(const CGameCtnBlock &block) {
    CGameCtnBlockInfo *blockInfo = block.BlockInfoRef();
    if (blockInfo == nullptr) {
        return nullptr;
    }
    return blockInfo->FamilyHelperMobilSource(block.UsesGroundMobilSize());
}

CSceneMobil *CommonHelper(const CGameCtnBlock &block) {
    CGameCtnBlockInfo *blockInfo = block.BlockInfoRef();
    return blockInfo != nullptr
            ? blockInfo->CommonHelperMobilSource()
            : nullptr;
}

std::optional<GmIso4> ClipSideTransform(u32 side) {
    if (side >= 4u) {
        return std::nullopt;
    }

    GmIso4 transform{};
    transform.translation.y = 0.0f;
    switch (side) {
    case 0u:
        transform.translation.x = 0.0f;
        transform.translation.z = 0.0f;
        transform.rotation.SetRotateQuarterY(0u);
        break;
    case 1u:
        transform.translation.x = 32.0f;
        transform.translation.z = 0.0f;
        transform.rotation.SetRotateQuarterY(3u);
        break;
    case 2u:
        transform.translation.x = 32.0f;
        transform.translation.z = 32.0f;
        transform.rotation.SetRotateQuarterY(2u);
        break;
    case 3u:
        transform.translation.x = 0.0f;
        transform.translation.z = 32.0f;
        transform.rotation.SetRotateQuarterY(1u);
        break;
    }
    return transform;
}

}  // namespace

ReplaySceneAssetPlacement::ReplaySceneAssetPlacement(
        StaticSolidAssetHandle asset,
        StaticSolidMaterialVariant materialVariant,
        std::optional<GmIso4> relativeTransform)
        : asset_(std::move(asset)),
          materialVariant_(materialVariant),
          relativeTransform_(std::move(relativeTransform)) {}

const StaticSolidAssetHandle &ReplaySceneAssetPlacement::Asset() const {
    return asset_;
}

StaticSolidMaterialVariant
ReplaySceneAssetPlacement::MaterialVariant() const {
    return materialVariant_;
}

const std::optional<GmIso4> &
ReplaySceneAssetPlacement::RelativeTransform() const {
    return relativeTransform_;
}

StaticSolidPrototype ReplaySceneAssetPlacement::Prototype() const {
    return asset_ != nullptr
            ? asset_->Prototype(materialVariant_)
            : StaticSolidPrototype{};
}

ReplaySceneBlockPlacement::ReplaySceneBlockPlacement(
        const CGameCtnBlock &block,
        const GmIso4 &worldIso)
        : block_(&block), worldIso_(worldIso) {}

const CGameCtnBlock &ReplaySceneBlockPlacement::Block() const {
    return *block_;
}

const GmIso4 &ReplaySceneBlockPlacement::WorldIso() const {
    return worldIso_;
}

bool ReplaySceneBlockPlacement::IsSuppressed() const {
    return block_->IsSuppressed();
}

bool ReplaySceneBlockPlacement::IsClip() const {
    return block_->GetType() == EBlockType::Clip;
}

bool ReplaySceneBlockPlacement::IsStartLine() const {
    const CGameCtnBlockInfo *blockInfo = block_->BlockInfoRef();
    if (blockInfo == nullptr) {
        return false;
    }
    return ::IsStartLine(blockInfo->RaceRole());
}

StaticSolidMaterialVariant ReplaySceneBlockPlacement::MaterialVariant() const {
    if (block_->UsesReplacementMaterialRemap()) {
        return StaticSolidMaterialVariant::Replacement;
    }
    if (block_->UsesDecorationSkinMaterialRemap()) {
        return StaticSolidMaterialVariant::DecorationSkin;
    }
    return StaticSolidMaterialVariant::BlockMobil;
}

std::optional<ReplaySceneAssetPlacement>
ReplaySceneBlockPlacement::MainAssetPlacement() const {
    StaticSolidAssetHandle asset = Asset(block_->MainMobil());
    if (asset == nullptr) {
        return std::nullopt;
    }
    return ReplaySceneAssetPlacement(std::move(asset), MaterialVariant());
}

std::vector<ReplaySceneAssetPlacement>
ReplaySceneBlockPlacement::HelperAssetPlacements() const {
    std::vector<ReplaySceneAssetPlacement> placements;
    placements.reserve(2u);
    const StaticSolidMaterialVariant variant =
            block_->UsesReplacementMaterialRemap() ||
                    block_->UsesDecorationSkinMaterialRemap()
            ? MaterialVariant()
            : StaticSolidMaterialVariant::Base;
    for (CSceneMobil *source :
         std::array<CSceneMobil *, 2>{FamilyHelper(*block_),
                                     CommonHelper(*block_)}) {
        StaticSolidAssetHandle asset = Asset(source);
        if (asset != nullptr) {
            placements.emplace_back(std::move(asset), variant);
        }
    }
    return placements;
}

std::vector<ReplaySceneAssetPlacement>
ReplaySceneBlockPlacement::ClipAssetPlacements() const {
    std::vector<ReplaySceneAssetPlacement> placements;
    const auto &sources = block_->ClipSourceMobils();
    placements.reserve(sources.size());
    const StaticSolidMaterialVariant variant =
            block_->UsesReplacementMaterialRemap() ||
                    block_->UsesDecorationSkinMaterialRemap()
            ? MaterialVariant()
            : StaticSolidMaterialVariant::Base;
    for (u32 side = 0u; side < sources.size(); ++side) {
        StaticSolidAssetHandle asset = Asset(sources[side].Get());
        if (asset != nullptr) {
            placements.emplace_back(
                    std::move(asset), variant, ClipSideTransform(side));
        }
    }
    return placements;
}

std::optional<ReplaySceneAssetPlacement>
ReplaySceneBlockPlacement::CheckpointTriggerAssetPlacement() const {
    CSceneMobil *trigger = block_->TriggerMobil();
    StaticSolidAssetHandle asset = Asset(trigger);
    if (trigger == nullptr || asset == nullptr ||
        !trigger->InitialItemProperties().has_value()) {
        return std::nullopt;
    }
    return ReplaySceneAssetPlacement(std::move(asset), MaterialVariant());
}

CHmsItem::Properties
ReplaySceneBlockPlacement::CheckpointTriggerProperties() const {
    CSceneMobil *trigger = block_->TriggerMobil();
    return trigger != nullptr
            ? trigger->InitialItemProperties().value_or(
                      trigger->HmsItem()->GetProperties())
            : CHmsItem::Properties{};
}

CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity
ReplaySceneBlockPlacement::CheckpointIdentity() const {
    const CGameCtnBlockInfo *blockInfo = block_->BlockInfoRef();
    if (blockInfo == nullptr) {
        return {};
    }
    const u32 blockId = block_->BlockInstanceId()
            ? block_->BlockInstanceId()->Value() + 1u
            : 0u;
    return CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity::
            FromRaceBlock(blockInfo->RaceRole(),
                          blockId,
                          blockInfo->RespawnUsesCurrentTransform());
}

GmIso4 ReplaySceneBlockPlacement::BlockSpawnIso() const {
    return block_->SpawnLocation(0u, 0u);
}

std::optional<GmIso4>
ReplaySceneBlockPlacement::CheckpointTriggerWorldIso() const {
    CSceneMobil *main = block_->MainMobil();
    CSceneMobil *trigger = block_->TriggerMobil();
    if (main == nullptr || trigger == nullptr) {
        return std::nullopt;
    }
    for (const CMwNodRef<CSceneObjectLink> &link : main->Links()) {
        if (link != nullptr && link->Object() == trigger) {
            GmIso4 worldIso = link->RelativeLocation();
            worldIso.Mult(worldIso_);
            return worldIso;
        }
    }
    return worldIso_;
}
