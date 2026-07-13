#ifndef TMNF_FORMAT_VEHICLE_TUNING_ARCHIVE_DECODER_H
#define TMNF_FORMAT_VEHICLE_TUNING_ARCHIVE_DECODER_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "format/vehicle_tuning/transmission_records.h"
#include "engine/game/replay_vehicle_tuning_definition.h"
int ReadVehicleTuningScalars(
        const VehicleTuningArchiveDocument &archive,
        std::vector<VehicleTuningScalarChunk> &chunks);
int ReadVehicleTransmissionArrays(
        const VehicleTuningArchiveDocument &archive,
        std::vector<TransmissionArray> &arrays);
int ApplyVehicleTuningScalars(
        const std::vector<VehicleTuningScalarChunk> &chunks,
        ReplayVehicleTuningDefinition &definition);
int ApplyVehicleTransmissionArrays(
        const std::vector<TransmissionArray> &arrays,
        ReplayVehicleTuningDefinition &definition);

#endif
