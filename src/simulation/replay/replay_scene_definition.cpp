#include "simulation/replay/replay_scene_definition.h"
#include <utility>

#include "engine/resources/block_info_catalog.h"
#include "engine/game/game_ctn_block_info.h"
#include <new>
namespace {

}  // namespace

std::optional<ReplaySceneBlockDefinition>
ReplaySceneBlockDefinition::Authored(
        const CGameCtnReplayMapInputBlock &placement,
        const BlockInfoCatalogEntry &catalogEntry,
        std::string_view sceneCollection,
        CGameCtnBlockInfo &blockInfo) {
    if (!catalogEntry.asset.IsValid()) {
        return std::nullopt;
    }
    ReplaySceneBlockDefinition definition;
    definition.placementId_ = placement.Id();
    definition.origin_ = ReplaySceneBlockOrigin::Authored;
    definition.type_ = catalogEntry.blockType;
    definition.placement_ = placement.Placement();
    definition.asset_ = catalogEntry.asset;
    definition.blockInfo_.MwSetNod(&blockInfo);
    if (catalogEntry.collection == sceneCollection) {
        definition.collectionLandZoneHeight_ =
                catalogEntry.collectionLandZoneHeight;
    }
    return definition;
}

std::optional<ReplaySceneBlockDefinition>
ReplaySceneBlockDefinition::AutomaticBase(
        const BlockInfoCatalogEntry &catalogEntry,
        CGameCtnBlockInfo &blockInfo) {
    if (!catalogEntry.asset.IsValid()) {
        return std::nullopt;
    }
    ReplaySceneBlockDefinition definition;
    definition.origin_ = ReplaySceneBlockOrigin::AutomaticBase;
    definition.type_ = catalogEntry.blockType;
    definition.placement_ = BlockPlacementState::AutomaticBase();
    definition.asset_ = catalogEntry.asset;
    definition.blockInfo_.MwSetNod(&blockInfo);
    return definition;
}

const std::optional<CGameCtnReplayBlockPlacementId> &
ReplaySceneBlockDefinition::PlacementId() const {
    return placementId_;
}

ReplaySceneBlockOrigin ReplaySceneBlockDefinition::Origin() const {
    return origin_;
}

EBlockType ReplaySceneBlockDefinition::Type() const {
    return type_;
}

const BlockPlacementState &ReplaySceneBlockDefinition::Placement() const {
    return placement_;
}

BlockInfoAssetHandle ReplaySceneBlockDefinition::Asset() const {
    return asset_;
}

CGameCtnBlockInfo *ReplaySceneBlockDefinition::BlockInfo() const {
    return blockInfo_.Get();
}

const std::optional<u32> &
ReplaySceneBlockDefinition::CollectionLandZoneHeight() const {
    return collectionLandZoneHeight_;
}

bool ReplaySceneBlockDefinition::IsAuthored() const {
    return origin_ == ReplaySceneBlockOrigin::Authored &&
           placementId_.has_value();
}

bool ReplaySceneBlockDefinition::IsAutomaticBase() const {
    return origin_ == ReplaySceneBlockOrigin::AutomaticBase;
}

bool ReplaySceneBlockDefinition::IsTerrainBlock() const {
    return type_ <= EBlockType::Frontier;
}

bool ReplaySceneBlockDefinition::IsPlacedMobilBlock() const {
    return !IsTerrainBlock();
}

bool ReplaySceneBlockDefinition::NeedsDefaultUnitGeometry() const {
    return type_ == EBlockType::RectAsym;
}

bool ReplaySceneBlockDefinition::UsesCollectionLandZoneHeight() const {
    return type_ == EBlockType::Frontier &&
           collectionLandZoneHeight_.has_value();
}

bool ReplaySceneBlockDefinition::IsClip() const {
    return type_ == EBlockType::Clip;
}

bool ReplaySceneBlockDefinition::UsesReplacementMaterialRemap() const {
    return replacementMaterialRemap_;
}

bool ReplaySceneBlockDefinition::UsesDecorationSkinMaterialRemap() const {
    return decorationSkinMaterialRemap_;
}

void ReplaySceneBlockDefinition::SetMaterialRemaps(
        bool replacement,
        bool decorationSkin) {
    replacementMaterialRemap_ = replacement;
    decorationSkinMaterialRemap_ = decorationSkin;
}

const std::array<CMwNodRef<CSceneMobil>, 4> &
ReplaySceneBlockDefinition::ClipSourceMobils() const {
    return clipSourceMobils_;
}

std::array<CMwNodRef<CSceneMobil>, 4> &
ReplaySceneBlockDefinition::MutableClipSourceMobils() {
    return clipSourceMobils_;
}

bool ReplaySceneBlockDefinition::InstallDefaultClipSourceSides(
        CSceneMobil *mobil) {
    if (!((mobil) != nullptr && (mobil)->StaticSolidAsset().IsSpecified())) {
        return true;
    }
    for (CMwNodRef<CSceneMobil> &side : clipSourceMobils_) {
        side.MwSetNod(mobil);
    }
    return true;
}

void ReplaySceneDefinition::Clear() {
    blocks_.clear();
    collection_.reset();
}

bool ReplaySceneDefinition::Reserve(std::size_t blockCount) {
    try {
        blocks_.reserve(blockCount);
        return true;
    } catch (const std::bad_alloc &) {
        Clear();
        return false;
    }
}

bool ReplaySceneDefinition::Add(ReplaySceneBlockDefinition block) {
    try {
        blocks_.push_back(std::move(block));
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

std::size_t ReplaySceneDefinition::BlockCount() const {
    return blocks_.size();
}

const ReplaySceneBlockDefinition *ReplaySceneDefinition::BlockAt(
        std::size_t index) const {
    return index < blocks_.size() ? &blocks_[index] : nullptr;
}

ReplaySceneBlockDefinition *ReplaySceneDefinition::MutableBlockAt(
        std::size_t index) {
    return index < blocks_.size() ? &blocks_[index] : nullptr;
}

const ReplaySceneBlockDefinition *ReplaySceneDefinition::FindAuthoredBlock(
        CGameCtnReplayBlockPlacementId id) const {
    for (const ReplaySceneBlockDefinition &block : blocks_) {
        if (block.PlacementId() && *block.PlacementId() == id) {
            return &block;
        }
    }
    return nullptr;
}

const ReplaySceneBlockDefinition *
ReplaySceneDefinition::AutomaticBaseBlock() const {
    for (const ReplaySceneBlockDefinition &block : blocks_) {
        if (block.IsAutomaticBase()) {
            return &block;
        }
    }
    return nullptr;
}

const std::vector<ReplaySceneBlockDefinition> &
ReplaySceneDefinition::Blocks() const {
    return blocks_;
}

std::vector<ReplaySceneBlockDefinition> &
ReplaySceneDefinition::MutableBlocks() {
    return blocks_;
}

void ReplaySceneDefinition::SetCollection(
        CatalogCollectionDefinition definition) {
    collection_ = std::move(definition);
}

const std::optional<CatalogCollectionDefinition> &
ReplaySceneDefinition::Collection() const {
    return collection_;
}

std::optional<std::string> ReplaySceneDefinition::DefaultBaseIdentifier()
        const {
    return collection_ ? collection_->defaultBaseIdentifier : std::nullopt;
}

CatalogConstructionZoneKind ReplaySceneDefinition::DefaultZoneKind() const {
    return collection_ ? collection_->defaultZoneKind
                       : CatalogConstructionZoneKind::None;
}
