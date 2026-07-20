#ifndef TMNF_REPLAY_WATER_DEFINITION_DEFAULTS_H
#define TMNF_REPLAY_WATER_DEFINITION_DEFAULTS_H

#include <cstdint>

constexpr float ReplayWaterCellSize = 32.0f;
constexpr float StadiumWaterSquareHeight = 8.0f;
constexpr float StadiumWaterSecondaryCullYOffset = -1000.0f;
constexpr std::int32_t StadiumWaterDefaultZoneHeight = 0;

constexpr float CoastWaterCellSize = 16.0f;
constexpr float CoastWaterSurfaceHeight = 17.0f;
constexpr float CoastWaterSecondaryCullHeight = 12.0f;

#endif
