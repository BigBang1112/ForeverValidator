#ifndef TMNF_FORMAT_VEHICLE_TUNING_CURVE_READER_H
#define TMNF_FORMAT_VEHICLE_TUNING_CURVE_READER_H

#include <stddef.h>
#include <vector>

#include "engine/core/engine_types.h"
#include "format/vehicle_tuning/archive_reader.h"
#include "engine/game/replay_vehicle_tuning_definition.h"
constexpr u32 MaxVehicleTuningCurveCount = 64u;

struct VehicleTuningCurveRecord {
    VehicleTuningChunkWord chunk;
    u32 nodRefIndex;
    u32 interpolationMode;
    std::vector<ReplayTuningCurveKey> keys;
};

int ReadVehicleTuningCurveChunk(
        const VehicleTuningArchiveDocument &archive,
        size_t *offset,
        std::vector<VehicleTuningCurveRecord> *records,
        int *sawFacade);

#endif
