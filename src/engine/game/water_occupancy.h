#pragma once

#include <cstdint>
#include <vector>

#include "engine/core/gm_types.h"
enum class WaterOccupancy : std::uint8_t {
    Dry,
    Water,
};

struct WaterOccupancyGrid {
    GmVec2 cellSize{};
    GmVec2 origin{};
    GmNat2 dimensions{};
    WaterOccupancy outside = WaterOccupancy::Dry;
    std::vector<WaterOccupancy> cells;
    std::uint8_t outsidePlaneIndex = 0u;
    std::vector<std::uint8_t> planeIndices;
    std::vector<GmVec4> planes;
};
