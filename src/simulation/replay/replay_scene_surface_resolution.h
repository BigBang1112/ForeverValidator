#pragma once

#include "engine/core/engine_types.h"
#include "engine/game/game_ctn_block_info.h"
#include <stddef.h>
#include <stdint.h>
#include <array>
#include <string>
#include <vector>

#include "engine/resources/catalog_asset_repository.h"
#include "engine/game/replay_challenge_map_input.h"
#include "simulation/replay/replay_scene_definition.h"
#include "simulation/replay/replay_terrain_top_markers.h"
class CollectionColumnSurface {
public:
    int Configure(int32_t x, int32_t z, const char *surfaceIdName);
    int MatchesColumn(int32_t x, int32_t z) const;
    int SetSurface(const char *surfaceIdName);
    const char *SurfaceIdName() const;

private:
    int32_t x = 0;
    int32_t z = 0;
    std::string surfaceIdName;
};

class CollectionColumnSurfaces {
public:
    int AddBlockSurfaces(
            const CGameCtnReplayMapInputBlock &block,
            const ReplaySceneBlockDefinition &definition,
            const CGameCtnBlockInfo *blockInfo);
    const char *Find(int32_t x, int32_t z) const;

private:
    std::vector<CollectionColumnSurface> columns;

    int Upsert(int32_t x, int32_t z, const char *surfaceIdName);
};

class ReplaySceneUnit {
public:
    int Configure(const CGameCtnReplayMapInputBlock &block,
                  const ReplaySceneBlockDefinition &definition,
                  const CGameCtnBlockUnitInfo &unitInfo,
                  const GmInt3 &rotatedOffset);

    int MatchesCoord(int32_t x, int32_t y, int32_t z) const;
    int BelongsToPlacement(CGameCtnReplayBlockPlacementId id) const;
    BlockInfoAssetHandle SourceAssetForSide(u32 side) const;

    int32_t X() const;
    int32_t Y() const;
    int32_t Z() const;
    ECardinalDir Direction() const;
    EBlockType BlockType() const;

private:
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    CGameCtnReplayBlockPlacementId placementId;
    ECardinalDir direction = ECardinalDir::North;
    EBlockType blockType = EBlockType::Flat;
    const CGameCtnBlockUnitInfo *unitInfo = nullptr;
};

struct ReplaySceneAssetResolver {
    CatalogAssetRepository *assets = nullptr;

    CSceneMobil *SelectMobil(
            BlockInfoAssetHandle sourceAsset,
            u32 selectorGroup,
            u32 variantIndex) const;
    std::optional<std::string> FirstGroundSurface(
            BlockInfoAssetHandle sourceAsset) const;
};

class ReplaySceneUnits {
public:
    int AddBlockUnitSources(
            const CGameCtnReplayMapInputBlock &block,
            const ReplaySceneBlockDefinition &definition,
            const CGameCtnBlockInfo *blockInfo,
            const TerrainTopMarkers *terrainMarkers);
    int ApplyClipNeighbourSourceSides(
            const CGameCtnReplayMapInput &mapInput,
            const ReplaySceneAssetResolver &assetResolver,
            ReplaySceneDefinition &scene) const;
    const ReplaySceneUnit *FindAtCoord(
            int32_t x,
            int32_t y,
            int32_t z) const;
    const ReplaySceneUnit *FindFirstForPlacement(
            CGameCtnReplayBlockPlacementId id) const;

private:
    std::vector<ReplaySceneUnit> units;

    int Append(const ReplaySceneUnit &unit);
};

struct ReplaySceneSurfaceResolver {
    CatalogAssetRepository *assets = nullptr;
    std::string_view collectionName;
    const ReplaySceneDefinition *scene = nullptr;
    const ReplaySceneUnits *units = nullptr;
    const CollectionColumnSurfaces *columnSurfaces = nullptr;
    const CGameCtnReplayMapInputBlock *block = nullptr;
    const ReplaySceneBlockDefinition *definition = nullptr;
    const ReplaySceneAssetResolver *assetResolver = nullptr;

    int ReplacementRemapApplies(int *okOut) const;
    const char *RealZoneSurfaceIdName(int *okOut) const;
    std::optional<std::string> SourceSurfaceIdName(int *okOut) const;
};
