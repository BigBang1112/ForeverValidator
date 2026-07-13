#ifndef TMNF_FORMAT_VEHICLE_TUNING_SCALAR_READER_H
#define TMNF_FORMAT_VEHICLE_TUNING_SCALAR_READER_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "format/vehicle_tuning/archive_reader.h"
constexpr uint32_t MaxVehicleTuningChunkCount = 128u;
constexpr uint32_t MaxVehicleTuningChunkRealCount = 32u;
constexpr uint32_t MaxVehicleTuningChunkNaturalCount = 8u;

struct VehicleTuningScalarChunk {
    VehicleTuningChunkWord chunk;
    std::vector<float> reals;
    std::vector<uint32_t> naturals;
};

int ReadVehicleTuningScalarChunk(
        const VehicleTuningArchiveDocument &archive,
        size_t *offset,
        VehicleTuningScalarChunk *out);

#endif
