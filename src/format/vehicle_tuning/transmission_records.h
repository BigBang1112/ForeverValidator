#ifndef TMNF_FORMAT_VEHICLE_TUNING_TRANSMISSION_RECORDS_H
#define TMNF_FORMAT_VEHICLE_TUNING_TRANSMISSION_RECORDS_H

#include "engine/core/engine_types.h"
#include <stdint.h>
#include <vector>

#include "format/vehicle_tuning/archive_ids.h"
#include "format/vehicle_tuning/scalar_reader.h"
typedef uint32_t u32;

enum class TransmissionArrayKind : u32 {
    GearSpeedRatio,
    UpshiftThreshold,
    DownshiftThreshold,
    RpmWanted,
};

struct TransmissionArray {
    TransmissionArrayKind source;
    std::vector<float> values;
};

#endif
