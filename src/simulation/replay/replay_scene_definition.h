#pragma once

#include <array>
#include <optional>
#include <vector>

#include "engine/resources/catalog_asset_repository.h"
#include "engine/game/game_ctn_block.h"
#include "engine/game/replay_challenge_map_input.h"
struct BlockInfoCatalogEntry;

enum class ReplaySceneBlockOrigin {
    Authored,
    AutomaticBase,
};

class ReplaySceneBlockDefinition {
public:
    static std::optional<ReplaySceneBlockDefinition> Authored(
            const CGameCtnReplayMapInputBlock &placement,
            const BlockInfoCatalogEntry &catalogEntry,
            std::string_view sceneCollection,
            CGameCtnBlockInfo &blockInfo);
    static std::optional<ReplaySceneBlockDefinition> AutomaticBase(
            const BlockInfoCatalogEntry &catalogEntry,
            CGameCtnBlockInfo &blockInfo);

    const std::optional<CGameCtnReplayBlockPlacementId> &PlacementId() const;
    ReplaySceneBlockOrigin Origin() const;
    EBlockType Type() const;
    const BlockPlacementState &Placement() const;
    BlockInfoAssetHandle Asset() const;
    CGameCtnBlockInfo *BlockInfo() const;
    const std::optional<u32> &CollectionLandZoneHeight() const;

    bool IsAuthored() const;
    bool IsAutomaticBase() const;
    bool IsTerrainBlock() const;
    bool IsPlacedMobilBlock() const;
    bool NeedsDefaultUnitGeometry() const;
    bool UsesCollectionLandZoneHeight() const;
    bool IsClip() const;

    bool UsesReplacementMaterialRemap() const;
    bool UsesDecorationSkinMaterialRemap() const;
    void SetMaterialRemaps(bool replacement, bool decorationSkin);

    const std::array<CMwNodRef<CSceneMobil>, 4> &ClipSourceMobils() const;
    std::array<CMwNodRef<CSceneMobil>, 4> &MutableClipSourceMobils();
    bool InstallDefaultClipSourceSides(CSceneMobil *mobil);

private:
    std::optional<CGameCtnReplayBlockPlacementId> placementId_;
    ReplaySceneBlockOrigin origin_ = ReplaySceneBlockOrigin::Authored;
    EBlockType type_ = EBlockType::Classic;
    BlockPlacementState placement_;
    BlockInfoAssetHandle asset_;
    CMwNodRef<CGameCtnBlockInfo> blockInfo_;
    std::optional<u32> collectionLandZoneHeight_;
    bool replacementMaterialRemap_ = false;
    bool decorationSkinMaterialRemap_ = false;
    std::array<CMwNodRef<CSceneMobil>, 4> clipSourceMobils_;
};

class ReplaySceneDefinition {
public:
    void Clear();
    bool Reserve(std::size_t blockCount);
    bool Add(ReplaySceneBlockDefinition block);

    std::size_t BlockCount() const;
    const ReplaySceneBlockDefinition *BlockAt(std::size_t index) const;
    ReplaySceneBlockDefinition *MutableBlockAt(std::size_t index);
    const ReplaySceneBlockDefinition *FindAuthoredBlock(
            CGameCtnReplayBlockPlacementId id) const;
    const ReplaySceneBlockDefinition *AutomaticBaseBlock() const;

    const std::vector<ReplaySceneBlockDefinition> &Blocks() const;
    std::vector<ReplaySceneBlockDefinition> &MutableBlocks();

    void SetCollection(CatalogCollectionDefinition definition);
    const std::optional<CatalogCollectionDefinition> &Collection() const;
    std::optional<std::string> DefaultBaseIdentifier() const;
    CatalogConstructionZoneKind DefaultZoneKind() const;

    bool AppendAutomaticBase(
            const CGameCtnReplayMapInput &mapInput,
            CatalogAssetRepository &assets);

private:
    std::vector<ReplaySceneBlockDefinition> blocks_;
    std::optional<CatalogCollectionDefinition> collection_;
};
