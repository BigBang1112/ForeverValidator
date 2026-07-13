#include "simulation/runtime/replay_map_environment.h"
#include <limits>
#include <utility>

#include "engine/core/finite_values.h"
#include "engine/game/replay_challenge_map_input.h"
#include "simulation/runtime/replay_water_definition_defaults.h"
#include <new>
namespace {

bool IsWaterBlock(const CGameCtnReplayMapInputBlock &block) {
    return block.Identifier().NameEquals("StadiumPool") ||
           block.Identifier().NameEquals("StadiumWater") ||
           block.Identifier().NameEquals("StadiumWaterClip");
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
        water.occupancy_.cells.assign(
                static_cast<std::size_t>(dimensions.x) * dimensions.y,
                WaterOccupancy::Dry);
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
    return water;
}

bool ReplayWaterDefinition::MarkWaterCell(const GmNat2 &cell) {
    if (cell.x >= occupancy_.dimensions.x ||
        cell.y >= occupancy_.dimensions.y) {
        return false;
    }
    occupancy_.cells[static_cast<std::size_t>(cell.y) *
                             occupancy_.dimensions.x +
                     cell.x] = WaterOccupancy::Water;
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
    if (size.x == 0u || size.z == 0u ||
        size.x > std::numeric_limits<std::size_t>::max() / size.z) {
        return 2;
    }

    const float surfaceHeight =
            (StadiumWaterDefaultZoneHeight + 1) * StadiumWaterSquareHeight;
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
