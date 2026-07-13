#ifndef TMNF_FORMAT_VEHICLE_TUNING_CURVE_DEFINITION_H
#define TMNF_FORMAT_VEHICLE_TUNING_CURVE_DEFINITION_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "format/vehicle_tuning/curve_reader.h"
int ReadVehicleTuningCurves(
        const VehicleTuningArchiveDocument &archive,
        std::vector<VehicleTuningCurveRecord> &records);
int ApplyVehicleTuningCurves(
        const std::vector<VehicleTuningCurveRecord> &records,
        ReplayVehicleTuningDefinition &definition);

#endif
