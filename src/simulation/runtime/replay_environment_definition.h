#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "engine/core/gm_types.h"
#include "engine/game/water_occupancy.h"
struct ReplayUniformForceFieldDefinition {
    GmVec3 acceleration{};
};

struct ReplayBallForceFieldDefinition {
    GmVec3 center{};
    float radius = 0.0f;
    float strength = 0.0f;
};

using ReplayForceFieldDefinition =
        std::variant<ReplayUniformForceFieldDefinition,
                     ReplayBallForceFieldDefinition>;

class ReplayWaterDefinition {
public:
    static std::optional<ReplayWaterDefinition> Create(
            const GmVec2 &cellSize,
            const GmVec2 &origin,
            const GmNat2 &dimensions,
            WaterOccupancy outsideOccupancy,
            float surfaceHeight,
            float secondaryCullHeight);

    bool MarkWaterCell(const GmNat2 &cell);
    const WaterOccupancyGrid &OccupancyGrid(void) const {
        return occupancy_;
    }
    float SurfaceHeight(void) const { return surfaceHeight_; }
    float SecondaryCullHeight(void) const { return secondaryCullHeight_; }

private:
    WaterOccupancyGrid occupancy_;
    float surfaceHeight_ = 0.0f;
    float secondaryCullHeight_ = 0.0f;
};

struct ReplayEnvironmentDefinition {
    float zoneLinearDampingCoefficient = 1.0f;
    float zoneAngularDampingCoefficient = 1.0f;
    std::vector<ReplayForceFieldDefinition> forceFields;
    std::optional<ReplayWaterDefinition> water;
};
