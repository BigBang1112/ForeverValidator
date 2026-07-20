#include "engine/scene/replay_scene_placements.h"
#include <algorithm>
#include <array>
#include <utility>

#include "engine/game/game_ctn_block.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/scene/scene_mobil.h"
namespace {

StaticSolidAssetHandle Asset(CSceneMobil *mobil) {
    return mobil != nullptr
            ? mobil->StaticSolidAsset().Asset()
            : StaticSolidAssetHandle{};
}

StaticSolidAssetHandle DedicatedCollisionAsset(CSceneMobil *mobil) {
    return mobil != nullptr && mobil->DoesMotionChangeLocations() &&
                    mobil->ReplayCollisionUsesInitialTransform() &&
                    mobil->InitialItemProperties().has_value()
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

StaticSolidMaterialVariant BlockMaterialVariant(
        const CGameCtnBlock &block) {
    if (block.UsesReplacementMaterialRemap()) {
        return StaticSolidMaterialVariant::Replacement;
    }
    if (block.UsesDecorationSkinMaterialRemap()) {
        return StaticSolidMaterialVariant::DecorationSkin;
    }
    return StaticSolidMaterialVariant::BlockMobil;
}

CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity
CheckpointIdentityForBlock(const CGameCtnBlock &block) {
    const CGameCtnBlockInfo *blockInfo = block.BlockInfoRef();
    if (blockInfo == nullptr) {
        return {};
    }
    const u32 blockId = block.BlockInstanceId()
            ? block.BlockInstanceId()->Value() + 1u
            : 0u;
    return CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity::
            FromRaceBlock(blockInfo->RaceRole(),
                          blockId,
                          blockInfo->RespawnUsesCurrentTransform());
}

std::optional<GmIso4> ClipSideTransform(u32 side, float squareSize) {
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
        transform.translation.x = squareSize;
        transform.translation.z = 0.0f;
        transform.rotation.SetRotateQuarterY(3u);
        break;
    case 2u:
        transform.translation.x = squareSize;
        transform.translation.z = squareSize;
        transform.rotation.SetRotateQuarterY(2u);
        break;
    case 3u:
        transform.translation.x = 0.0f;
        transform.translation.z = squareSize;
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
        const GmIso4 &worldIso,
        ReplaySceneInstallationOrdinal installationOrdinal)
        : block_(&block),
          worldIso_(worldIso),
          installationOrdinal_(installationOrdinal),
          mainMobil_(block.MainMobil()),
          helperMobil_(block.HelperMobil()),
          triggerMobil_(block.TriggerMobil()),
          subMobils_(block.SubMobils()),
          familyHelperMobilSource_(FamilyHelper(block)),
          commonHelperMobilSource_(CommonHelper(block)),
          clipSourceMobils_(block.ClipSourceMobils()),
          materialVariant_(BlockMaterialVariant(block)),
          suppressed_(block.IsSuppressed()),
          clip_(block.GetType() == EBlockType::Clip),
          startLine_(block.BlockInfoRef() != nullptr &&
                     ::IsStartLine(block.BlockInfoRef()->RaceRole())),
          collectionSquareSize_(block.CollectionSquareSize()),
          checkpointIdentity_(CheckpointIdentityForBlock(block)),
          blockSpawnIso_(block.SpawnLocation(0u, 0u)) {}

ReplayScenePylonPlacement::ReplayScenePylonPlacement(
        CSceneMobil &mobil,
        const GmIso4 &worldIso,
        ReplaySceneInstallationOrdinal installationOrdinal)
        : mobil_(&mobil),
          worldIso_(worldIso),
          installationOrdinal_(installationOrdinal) {}

CSceneMobil &ReplayScenePylonPlacement::Mobil() const {
    return *mobil_;
}

const GmIso4 &ReplayScenePylonPlacement::WorldIso() const {
    return worldIso_;
}

ReplaySceneInstallationOrdinal
ReplayScenePylonPlacement::InstallationOrdinal() const {
    return installationOrdinal_;
}

bool ReplayScenePylonPlacement::ContainsMobil(
        const CSceneMobil *mobil) const {
    return mobil != nullptr && mobil_.Get() == mobil;
}

std::optional<ReplaySceneAssetPlacement>
ReplayScenePylonPlacement::AssetPlacement() const {
    StaticSolidAssetHandle asset = Asset(mobil_.Get());
    if (asset == nullptr) {
        return std::nullopt;
    }
    return ReplaySceneAssetPlacement(
            std::move(asset), StaticSolidMaterialVariant::BlockMobil);
}

CHmsItem::Properties ReplayScenePylonPlacement::ItemProperties() const {
    return mobil_->HmsItem()->GetProperties();
}

const CGameCtnBlock &ReplaySceneBlockPlacement::Block() const {
    return *block_;
}

const GmIso4 &ReplaySceneBlockPlacement::WorldIso() const {
    return worldIso_;
}

ReplaySceneInstallationOrdinal
ReplaySceneBlockPlacement::InstallationOrdinal() const {
    return installationOrdinal_;
}

bool ReplaySceneBlockPlacement::ContainsMobilGeneration(
        const CSceneMobil *mobil) const {
    if (mobil == nullptr) {
        return false;
    }
    if (mainMobil_.Get() == mobil || helperMobil_.Get() == mobil ||
        triggerMobil_.Get() == mobil) {
        return true;
    }
    return std::any_of(
            subMobils_.begin(),
            subMobils_.end(),
            [mobil](const CMwNodRef<CSceneMobil> &candidate) {
                return candidate.Get() == mobil;
            });
}

bool ReplaySceneBlockPlacement::RemoveMobilFromGeneration(
        const CSceneMobil *mobil) {
    if (mobil == nullptr) {
        return false;
    }
    bool removed = false;
    if (mainMobil_.Get() == mobil) {
        mainMobil_ = nullptr;
        removed = true;
    }
    if (helperMobil_.Get() == mobil) {
        helperMobil_ = nullptr;
        familyHelperMobilSource_ = nullptr;
        commonHelperMobilSource_ = nullptr;
        removed = true;
    }
    if (triggerMobil_.Get() == mobil) {
        triggerMobil_ = nullptr;
        removed = true;
    }
    const auto oldSize = subMobils_.size();
    subMobils_.erase(
            std::remove_if(
                    subMobils_.begin(),
                    subMobils_.end(),
                    [mobil](const CMwNodRef<CSceneMobil> &candidate) {
                        return candidate.Get() == mobil;
                    }),
            subMobils_.end());
    return removed || subMobils_.size() != oldSize;
}

bool ReplaySceneBlockPlacement::HasSceneProducingMobils() const {
    if (mainMobil_ != nullptr || triggerMobil_ != nullptr) {
        return true;
    }
    return std::any_of(
            subMobils_.begin(),
            subMobils_.end(),
            [](const CMwNodRef<CSceneMobil> &mobil) {
                return mobil.Get() != nullptr;
            });
}

bool ReplaySceneBlockPlacement::HasPublicSceneRepresentation() const {
    const bool mainStatic = mainMobil_ != nullptr &&
            !(mainMobil_->DoesMotionChangeLocations() &&
              mainMobil_->ReplayCollisionUsesInitialTransform() &&
              mainMobil_->InitialItemProperties().has_value()) &&
            mainMobil_->StaticSolidAsset().IsSpecified();
    const bool helperStatic = helperMobil_ != nullptr &&
            ((familyHelperMobilSource_ != nullptr &&
              familyHelperMobilSource_->StaticSolidAsset().IsSpecified()) ||
             (commonHelperMobilSource_ != nullptr &&
              commonHelperMobilSource_->StaticSolidAsset().IsSpecified()));
    const bool clipStatic = std::any_of(
            clipSourceMobils_.begin(),
            clipSourceMobils_.end(),
            [](const CMwNodRef<CSceneMobil> &mobil) {
                return mobil != nullptr &&
                       mobil->StaticSolidAsset().IsSpecified();
            });
    const bool subMobilStatic = std::any_of(
            subMobils_.begin(),
            subMobils_.end(),
            [](const CMwNodRef<CSceneMobil> &mobil) {
                return mobil != nullptr &&
                       mobil->StaticSolidAsset().IsSpecified();
            });
    return mainStatic || subMobilStatic || helperStatic || clipStatic ||
           startLine_;
}

bool ReplaySceneBlockPlacement::HasDedicatedCollisionRepresentation() const {
    return mainMobil_ != nullptr && mainMobil_->DoesMotionChangeLocations() &&
           mainMobil_->ReplayCollisionUsesInitialTransform() &&
           mainMobil_->StaticSolidAsset().IsSpecified() &&
           mainMobil_->InitialItemProperties().has_value();
}

bool ReplaySceneBlockPlacement::IsSuppressed() const {
    return suppressed_;
}

bool ReplaySceneBlockPlacement::IsClip() const {
    return clip_;
}

bool ReplaySceneBlockPlacement::IsStartLine() const {
    return startLine_;
}

StaticSolidMaterialVariant ReplaySceneBlockPlacement::MaterialVariant() const {
    return materialVariant_;
}

std::optional<ReplaySceneAssetPlacement>
ReplaySceneBlockPlacement::MainAssetPlacement() const {
    StaticSolidAssetHandle asset = Asset(mainMobil_.Get());
    if (asset == nullptr) {
        return std::nullopt;
    }
    return ReplaySceneAssetPlacement(std::move(asset), MaterialVariant());
}

std::vector<ReplaySceneAssetPlacement>
ReplaySceneBlockPlacement::SubMobilAssetPlacements() const {
    std::vector<ReplaySceneAssetPlacement> placements;
    placements.reserve(subMobils_.size());
    for (const CMwNodRef<CSceneMobil> &mobil : subMobils_) {
        StaticSolidAssetHandle asset = Asset(mobil.Get());
        if (asset != nullptr) {
            placements.emplace_back(std::move(asset), MaterialVariant());
        }
    }
    return placements;
}

std::optional<ReplaySceneAssetPlacement>
ReplaySceneBlockPlacement::DedicatedCollisionAssetPlacement() const {
    StaticSolidAssetHandle asset = DedicatedCollisionAsset(mainMobil_.Get());
    if (asset == nullptr) {
        return std::nullopt;
    }
    return ReplaySceneAssetPlacement(std::move(asset), MaterialVariant());
}

std::optional<CHmsItem::Properties>
ReplaySceneBlockPlacement::DedicatedCollisionProperties() const {
    CSceneMobil *mobil = mainMobil_.Get();
    if (mobil == nullptr || !mobil->DoesMotionChangeLocations() ||
        !mobil->ReplayCollisionUsesInitialTransform()) {
        return std::nullopt;
    }
    return mobil->InitialItemProperties();
}

std::vector<ReplaySceneAssetPlacement>
ReplaySceneBlockPlacement::HelperAssetPlacements() const {
    std::vector<ReplaySceneAssetPlacement> placements;
    if (helperMobil_ == nullptr) {
        return placements;
    }
    placements.reserve(2u);
    const StaticSolidMaterialVariant variant = materialVariant_ !=
                    StaticSolidMaterialVariant::BlockMobil
            ? materialVariant_
            : StaticSolidMaterialVariant::Base;
    for (CSceneMobil *source : std::array<CSceneMobil *, 2>{
                 familyHelperMobilSource_.Get(),
                 commonHelperMobilSource_.Get()}) {
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
    const auto &sources = clipSourceMobils_;
    placements.reserve(sources.size());
    const StaticSolidMaterialVariant variant = materialVariant_ !=
                    StaticSolidMaterialVariant::BlockMobil
            ? materialVariant_
            : StaticSolidMaterialVariant::Base;
    for (u32 side = 0u; side < sources.size(); ++side) {
        StaticSolidAssetHandle asset = Asset(sources[side].Get());
        if (asset != nullptr) {
            placements.emplace_back(
                    std::move(asset),
                    variant,
                    ClipSideTransform(side, collectionSquareSize_));
        }
    }
    return placements;
}

std::optional<ReplaySceneAssetPlacement>
ReplaySceneBlockPlacement::CheckpointTriggerAssetPlacement() const {
    CSceneMobil *trigger = triggerMobil_.Get();
    StaticSolidAssetHandle asset = Asset(trigger);
    if (trigger == nullptr || asset == nullptr ||
        !trigger->InitialItemProperties().has_value()) {
        return std::nullopt;
    }
    return ReplaySceneAssetPlacement(std::move(asset), MaterialVariant());
}

CHmsItem::Properties
ReplaySceneBlockPlacement::CheckpointTriggerProperties() const {
    CSceneMobil *trigger = triggerMobil_.Get();
    return trigger != nullptr
            ? trigger->InitialItemProperties().value_or(
                      trigger->HmsItem()->GetProperties())
            : CHmsItem::Properties{};
}

CGameCtnReplayCheckpointTrigger::RaceCheckpointIdentity
ReplaySceneBlockPlacement::CheckpointIdentity() const {
    return checkpointIdentity_;
}

GmIso4 ReplaySceneBlockPlacement::BlockSpawnIso() const {
    return blockSpawnIso_;
}

std::optional<GmIso4>
ReplaySceneBlockPlacement::CheckpointTriggerWorldIso() const {
    CSceneMobil *main = mainMobil_.Get();
    CSceneMobil *trigger = triggerMobil_.Get();
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

ReplaySceneInstallation::ReplaySceneInstallation(
        ReplaySceneBlockPlacement placement)
        : tag_(ReplaySceneInstallationTag::Block),
          installationOrdinal_(placement.InstallationOrdinal()),
          payload_(std::move(placement)) {}

ReplaySceneInstallation::ReplaySceneInstallation(
        ReplayScenePylonPlacement placement)
        : tag_(ReplaySceneInstallationTag::Pylon),
          installationOrdinal_(placement.InstallationOrdinal()),
          payload_(std::move(placement)) {}

ReplaySceneInstallationTag ReplaySceneInstallation::Tag() const {
    return tag_;
}

ReplaySceneInstallationOrdinal
ReplaySceneInstallation::InstallationOrdinal() const {
    return installationOrdinal_;
}

bool ReplaySceneInstallation::IsActive() const {
    return !std::holds_alternative<std::monostate>(payload_);
}

bool ReplaySceneInstallation::ContainsMobil(
        const CSceneMobil *mobil) const {
    if (const auto *block = std::get_if<ReplaySceneBlockPlacement>(&payload_)) {
        return block->ContainsMobilGeneration(mobil);
    }
    if (const auto *pylon = std::get_if<ReplayScenePylonPlacement>(&payload_)) {
        return pylon->ContainsMobil(mobil);
    }
    return false;
}

bool ReplaySceneInstallation::RemoveMobil(const CSceneMobil *mobil) {
    if (auto *block = std::get_if<ReplaySceneBlockPlacement>(&payload_)) {
        if (!block->RemoveMobilFromGeneration(mobil)) {
            return false;
        }
        if (!block->HasSceneProducingMobils()) {
            payload_ = std::monostate{};
        }
        return true;
    }
    if (const auto *pylon =
            std::get_if<ReplayScenePylonPlacement>(&payload_)) {
        if (!pylon->ContainsMobil(mobil)) {
            return false;
        }
        payload_ = std::monostate{};
        return true;
    }
    return false;
}

const ReplaySceneBlockPlacement *
ReplaySceneInstallation::BlockPlacement() const {
    return std::get_if<ReplaySceneBlockPlacement>(&payload_);
}

const ReplayScenePylonPlacement *
ReplaySceneInstallation::PylonPlacement() const {
    return std::get_if<ReplayScenePylonPlacement>(&payload_);
}
