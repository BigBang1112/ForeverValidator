#include "simulation/runtime/replay_map_environment.h"
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "engine/core/finite_values.h"
#include "engine/game/game_ctn_challenge.h"
#include "engine/game/game_ctn_zone.h"
#include "engine/game/replay_challenge_map_input.h"
#include "engine/physics/world/hms_zone.h"
#include "engine/scene/replay_scene_placements.h"
#include "engine/scene/replay_static_scene_corpuses.h"
#include "engine/scene/static_scene_model.h"
#include "simulation/runtime/replay_water_definition_defaults.h"
#include <new>
namespace {

bool IsWaterBlock(const CGameCtnReplayMapInputBlock &block) {
    return block.Identifier().NameEquals("StadiumPool") ||
           block.Identifier().NameEquals("StadiumWater") ||
           block.Identifier().NameEquals("StadiumWaterClip");
}

bool IsCoastDryBlock(const CGameCtnReplayMapInputBlock &block) {
    return block.Identifier().NameEquals("CoastDirt") ||
           block.Identifier().NameEquals("CoastDirtCliff");
}

bool MarkWaterOccupancy(
        ReplayWaterDefinition &water,
        const GmNat2 &cell,
        bool hasWater) {
    return hasWater ? water.MarkWaterCell(cell)
                    : water.MarkDryCell(cell);
}

bool HasValidMapSize(const GmNat3 &size) {
    return size.x != 0u && size.z != 0u &&
           size.x <= std::numeric_limits<std::size_t>::max() / size.z;
}

bool DecorationMatchesMap(
        const CatalogDecorationSizeDefinition &decoration,
        const GmNat3 &size) {
    return decoration.mapSize.x == size.x &&
           decoration.mapSize.y == size.y &&
           decoration.mapSize.z == size.z &&
           decoration.defaultZoneHeight != UINT32_MAX;
}

bool CollectionHasValidWaterGrid(
        const CatalogCollectionDefinition &collection) {
    return FiniteValues::All(collection.squareSize,
                             collection.squareHeight) &&
           collection.squareSize > 0.0f &&
           collection.squareHeight > 0.0f;
}

int BuildSceneWaterPlaneTable(
        const StaticSceneModelCollection &sceneModels,
        std::vector<GmVec4> *planes) {
    if (planes == nullptr) {
        return 1;
    }
    planes->clear();
    CHmsZone planeZone;
    for (const StaticSceneModel &model : sceneModels.Models()) {
        ReplayStaticCorpusBody body;
        if (!body.ConstructStaticMobil(
                    model.Prototype(), model.WorldIso())) {
            return 6;
        }
        planeZone.AddCorpus(&body.Corpus());
        planeZone.RemoveCorpus(&body.Corpus());
    }
    if (planeZone.WaterPlaneEqCount() >
        std::numeric_limits<std::uint8_t>::max()) {
        return 7;
    }
    try {
        planes->reserve(planeZone.WaterPlaneEqCount());
        for (std::size_t index = 0u;
             index < planeZone.WaterPlaneEqCount();
             ++index) {
            const GmVec4 *plane = planeZone.WaterPlaneEqAt(index);
            if (plane == nullptr) {
                planes->clear();
                return 7;
            }
            planes->push_back(*plane);
        }
    } catch (const std::bad_alloc &) {
        planes->clear();
        return 3;
    }
    return 0;
}

int PlacementWaterPlane(
        const ReplaySceneBlockPlacement &placement,
        std::optional<GmVec4> *plane) {
    if (plane == nullptr) {
        return 1;
    }
    plane->reset();
    std::optional<ReplaySceneAssetPlacement> asset =
            placement.DedicatedCollisionAssetPlacement();
    if (!asset.has_value()) {
        asset = placement.MainAssetPlacement();
    }
    if (!asset.has_value()) {
        return 0;
    }
    const StaticSolidPrototype prototype = asset->Prototype();
    if (!prototype.IsValid()) {
        return 0;
    }
    ReplayStaticCorpusBody body;
    if (!body.ConstructStaticMobil(prototype, placement.WorldIso())) {
        return 6;
    }
    GmVec4 candidate{};
    if (body.Corpus().WaterGetPlaneEqInZone(candidate)) {
        *plane = candidate;
    }
    return 0;
}

std::uint8_t WaterPlaneIndex(
        const std::vector<GmVec4> &planes,
        const GmVec4 &plane) {
    for (std::size_t index = 0u; index < planes.size(); ++index) {
        if (planes[index].PlaneEqIsNearlyEqual(plane, 0.0f, 0.0f)) {
            return static_cast<std::uint8_t>(index + 1u);
        }
    }
    return 0u;
}

int BuildLegacyCoastWaterDefinition(
        const CGameCtnReplayMapInput &map,
        std::optional<ReplayWaterDefinition> *out) {
    const GmNat3 &size = map.Size();
    std::optional<ReplayWaterDefinition> water =
            ReplayWaterDefinition::Create(
                    {CoastWaterCellSize, CoastWaterCellSize},
                    {},
                    {size.x, size.z},
                    WaterOccupancy::Water,
                    CoastWaterSurfaceHeight,
                    CoastWaterSecondaryCullHeight);
    if (!water.has_value()) {
        return 3;
    }
    for (const CGameCtnReplayMapInputBlock &block : map.Blocks()) {
        if (!IsCoastDryBlock(block)) {
            continue;
        }
        const GmNat3 &coord = block.Coordinate();
        if (!water->MarkDryCell({coord.x, coord.z})) {
            return 4;
        }
    }
    *out = std::move(*water);
    return 0;
}

}  // namespace

std::optional<ReplayWaterDefinition> ReplayWaterDefinition::Create(
        const GmVec2 &cellSize,
        const GmVec2 &origin,
        const GmNat2 &dimensions,
        WaterOccupancy outsideOccupancy,
        float surfaceHeight,
        float secondaryCullHeight) {
    if (dimensions.x == 0u || dimensions.y == 0u ||
        dimensions.x >
                std::numeric_limits<std::size_t>::max() / dimensions.y ||
        !FiniteValues::IsFinite(cellSize) ||
        cellSize.x <= 0.0f || cellSize.y <= 0.0f ||
        !FiniteValues::IsFinite(origin) ||
        !FiniteValues::All(surfaceHeight, secondaryCullHeight)) {
        return std::nullopt;
    }

    ReplayWaterDefinition water;
    water.occupancy_.cellSize = cellSize;
    water.occupancy_.origin = origin;
    water.occupancy_.dimensions = dimensions;
    water.occupancy_.outside = outsideOccupancy;
    water.surfaceHeight_ = surfaceHeight;
    water.secondaryCullHeight_ = secondaryCullHeight;
    try {
        const std::size_t cellCount =
                static_cast<std::size_t>(dimensions.x) * dimensions.y;
        water.occupancy_.cells.assign(
                cellCount,
                outsideOccupancy);
        water.occupancy_.planes.push_back(
                {0.0f, 1.0f, 0.0f, -surfaceHeight});
        const std::uint8_t outsidePlaneIndex =
                outsideOccupancy == WaterOccupancy::Water ? 1u : 0u;
        water.occupancy_.outsidePlaneIndex = outsidePlaneIndex;
        water.occupancy_.planeIndices.assign(
                cellCount, outsidePlaneIndex);
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    } catch (const std::length_error &) {
        return std::nullopt;
    }
    return water;
}

std::optional<ReplayWaterDefinition> ReplayWaterDefinition::CreateGeometry(
        const GmVec2 &cellSize,
        const GmVec2 &origin,
        const GmNat2 &dimensions,
        float surfaceHeight,
        float secondaryCullHeight,
        const std::vector<GmVec4> &planes) {
    if (planes.size() > std::numeric_limits<std::uint8_t>::max()) {
        return std::nullopt;
    }
    for (const GmVec4 &plane : planes) {
        if (!FiniteValues::IsFinite(plane)) {
            return std::nullopt;
        }
    }
    std::optional<ReplayWaterDefinition> water = Create(
            cellSize,
            origin,
            dimensions,
            WaterOccupancy::Dry,
            surfaceHeight,
            secondaryCullHeight);
    if (!water.has_value()) {
        return std::nullopt;
    }
    try {
        water->occupancy_.planes = planes;
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
    return water;
}

bool ReplayWaterDefinition::MarkWaterCell(const GmNat2 &cell) {
    if (cell.x >= occupancy_.dimensions.x ||
        cell.y >= occupancy_.dimensions.y ||
        occupancy_.planes.empty()) {
        return false;
    }
    const std::size_t index =
            static_cast<std::size_t>(cell.y) * occupancy_.dimensions.x +
            cell.x;
    if (index >= occupancy_.cells.size() ||
        index >= occupancy_.planeIndices.size()) {
        return false;
    }
    occupancy_.cells[index] = WaterOccupancy::Water;
    occupancy_.planeIndices[index] = 1u;
    return true;
}

bool ReplayWaterDefinition::MarkWaterCell(
        const GmNat2 &cell,
        std::uint8_t planeIndex) {
    if (cell.x >= occupancy_.dimensions.x ||
        cell.y >= occupancy_.dimensions.y ||
        planeIndex == 0u ||
        planeIndex > occupancy_.planes.size()) {
        return false;
    }
    const std::size_t index =
            static_cast<std::size_t>(cell.y) * occupancy_.dimensions.x +
            cell.x;
    if (index >= occupancy_.cells.size() ||
        index >= occupancy_.planeIndices.size()) {
        return false;
    }
    occupancy_.cells[index] = WaterOccupancy::Water;
    occupancy_.planeIndices[index] = planeIndex;
    return true;
}

bool ReplayWaterDefinition::MarkDryCell(const GmNat2 &cell) {
    if (cell.x >= occupancy_.dimensions.x ||
        cell.y >= occupancy_.dimensions.y) {
        return false;
    }
    const std::size_t index =
            static_cast<std::size_t>(cell.y) * occupancy_.dimensions.x +
            cell.x;
    if (index >= occupancy_.cells.size() ||
        index >= occupancy_.planeIndices.size()) {
        return false;
    }
    occupancy_.cells[index] = WaterOccupancy::Dry;
    occupancy_.planeIndices[index] = 0u;
    return true;
}

int BuildReplayWaterDefinition(
        const CGameCtnReplayMapInput &map,
        std::optional<ReplayWaterDefinition> *out) {
    if (out == nullptr) {
        return 1;
    }
    out->reset();
    const GmNat3 &size = map.Size();
    if (!HasValidMapSize(size)) {
        return 2;
    }

    if (map.DefaultCollectionName() == "Coast") {
        return BuildLegacyCoastWaterDefinition(map, out);
    }

    const float surfaceHeight =
            (StadiumWaterDefaultZoneHeight + 1) *
            StadiumWaterSquareHeight;
    std::optional<ReplayWaterDefinition> water =
            ReplayWaterDefinition::Create(
                    {ReplayWaterCellSize, ReplayWaterCellSize},
                    {},
                    {size.x, size.z},
                    WaterOccupancy::Dry,
                    surfaceHeight,
                    surfaceHeight + StadiumWaterSecondaryCullYOffset);
    if (!water.has_value()) {
        return 3;
    }

    bool hasWater = false;
    for (const CGameCtnReplayMapInputBlock &block : map.Blocks()) {
        if (!IsWaterBlock(block)) {
            continue;
        }
        const GmNat3 &coord = block.Coordinate();
        if (!water->MarkWaterCell({coord.x, coord.z})) {
            return 4;
        }
        hasWater = true;
    }
    if (hasWater) {
        *out = std::move(*water);
    }
    return 0;
}

int BuildReplayGeometryWaterDefinition(
        CGameCtnChallenge &challenge,
        const CatalogCollectionDefinition &collection,
        const CatalogDecorationSizeDefinition &decoration,
        const ReplaySceneBlockPlacements &placements,
        const StaticSceneModelCollection &sceneModels,
        std::optional<ReplayWaterDefinition> *out) {
    if (out == nullptr) {
        return 1;
    }
    out->reset();
    const GmNat3 size = challenge.MapSize();
    if (!HasValidMapSize(size)) {
        return 2;
    }
    if (!collection.water.has_value() ||
        !collection.water->geometryWaterPlanes) {
        return 0;
    }
    if (!DecorationMatchesMap(decoration, size) ||
        !CollectionHasValidWaterGrid(collection)) {
        return 5;
    }

    std::vector<GmVec4> waterPlanes;
    const int planeTableResult =
            BuildSceneWaterPlaneTable(sceneModels, &waterPlanes);
    if (planeTableResult != 0) {
        return planeTableResult;
    }

    const CatalogCollectionWaterDefinition &metadata = *collection.water;
    const float baseHeight =
            static_cast<float>(decoration.defaultZoneHeight + 1u) *
            collection.squareHeight;
    std::optional<ReplayWaterDefinition> water =
            ReplayWaterDefinition::CreateGeometry(
                    {collection.squareSize, collection.squareSize},
                    {},
                    {size.x, size.z},
                    baseHeight + metadata.surfaceOffset,
                    baseHeight + metadata.secondaryOffset,
                    waterPlanes);
    if (!water.has_value()) {
        return 3;
    }

    std::unordered_map<const CGameCtnBlock *,
                       const ReplaySceneBlockPlacement *> placementByBlock;
    std::unordered_map<const CGameCtnBlock *, std::optional<GmVec4>>
            planeByBlock;
    try {
        placementByBlock.reserve(placements.CollisionPlacements().size());
        planeByBlock.reserve(placements.CollisionPlacements().size());
        for (const ReplaySceneBlockPlacement &placement :
             placements.CollisionPlacements()) {
            placementByBlock.emplace(&placement.Block(), &placement);
        }
    } catch (const std::bad_alloc &) {
        return 3;
    }

    bool hasWetZone = false;
    for (u32 z = 0u; z < size.z; ++z) {
        for (u32 x = 0u; x < size.x; ++x) {
            CGameCtnZone *zone = challenge.GetRealZone({x, 0u, z});
            if (zone == nullptr || !zone->hasWater) {
                continue;
            }
            hasWetZone = true;
            CGameCtnBlock *block = challenge.GetGroundBlock({x, 0u, z});
            if (block == nullptr) {
                return 4;
            }
            auto cachedPlane = planeByBlock.find(block);
            if (cachedPlane == planeByBlock.end()) {
                std::optional<GmVec4> plane;
                const auto placement = placementByBlock.find(block);
                if (placement != placementByBlock.end()) {
                    const int planeResult =
                            PlacementWaterPlane(*placement->second, &plane);
                    if (planeResult != 0) {
                        return planeResult;
                    }
                }
                try {
                    cachedPlane = planeByBlock.emplace(block, plane).first;
                } catch (const std::bad_alloc &) {
                    return 3;
                }
            }
            if (!cachedPlane->second.has_value()) {
                continue;
            }
            const std::uint8_t planeIndex =
                    WaterPlaneIndex(waterPlanes, *cachedPlane->second);
            if (planeIndex != 0u &&
                !water->MarkWaterCell({x, z}, planeIndex)) {
                return 4;
            }
        }
    }
    if (!hasWetZone) {
        return 0;
    }
    *out = std::move(*water);
    return 0;
}

int BuildReplayWaterDefinition(
        CGameCtnChallenge &challenge,
        const CatalogCollectionDefinition &collection,
        const CatalogDecorationSizeDefinition &decoration,
        std::optional<ReplayWaterDefinition> *out) {
    if (out == nullptr) {
        return 1;
    }
    out->reset();
    const GmNat3 size = challenge.MapSize();
    if (!HasValidMapSize(size)) {
        return 2;
    }
    if (!collection.water.has_value()) {
        return 0;
    }
    if (!DecorationMatchesMap(decoration, size) ||
        !CollectionHasValidWaterGrid(collection)) {
        return 5;
    }

    const CatalogCollectionWaterDefinition &metadata = *collection.water;
    if (metadata.geometryWaterPlanes) {
        // Exact occupancy requires the collection's per-block water planes.
        // A wet construction zone is only the game's broad-phase filter.
        return 0;
    }
    bool hasPlayFieldBlock = false;
    for (u32 z = 0u; z < size.z && !hasPlayFieldBlock; ++z) {
        for (u32 x = 0u; x < size.x; ++x) {
            if (challenge.GetBlockFromPlayField({x, 0u, z}) != nullptr) {
                hasPlayFieldBlock = true;
                break;
            }
        }
    }
    const float baseHeight =
            static_cast<float>(decoration.defaultZoneHeight + 1u) *
            collection.squareHeight;
    std::optional<ReplayWaterDefinition> water =
            ReplayWaterDefinition::Create(
                    {collection.squareSize, collection.squareSize},
                    {},
                    {size.x, size.z},
                    metadata.defaultWater
                            ? WaterOccupancy::Water
                            : WaterOccupancy::Dry,
                    baseHeight + metadata.surfaceOffset,
                    baseHeight + metadata.secondaryOffset);
    if (!water.has_value()) {
        return 3;
    }

    bool hasWaterCell = false;
    for (u32 z = 0u; z < size.z; ++z) {
        for (u32 x = 0u; x < size.x; ++x) {
            CGameCtnBlock *block =
                    challenge.GetBlockFromPlayField({x, 0u, z});
            if (block == nullptr && hasPlayFieldBlock) {
                return 4;
            }
            CGameCtnZone *zone = block != nullptr
                    ? challenge.GetRealZone({x, 0u, z})
                    : challenge.GetZone({x, 0u, z});
            if (zone == nullptr) {
                return 4;
            }
            if (!MarkWaterOccupancy(*water, {x, z}, zone->hasWater)) {
                return 4;
            }
            hasWaterCell = hasWaterCell || zone->hasWater;
        }
    }
    if (hasWaterCell) {
        *out = std::move(*water);
    }
    return 0;
}
