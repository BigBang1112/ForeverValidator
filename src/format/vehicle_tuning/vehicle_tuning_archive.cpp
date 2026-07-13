#include "format/vehicle_tuning/vehicle_tuning_archive.h"
#include "format/vehicle_tuning/archive_decoder.h"
#include "format/vehicle_tuning/curve_definition.h"
#include <new>
std::optional<ReplayVehicleTuningDefinition> VehicleTuningArchive::Decode(
        const std::vector<std::uint8_t> &bytes) {
    if (bytes.empty()) {
        return std::nullopt;
    }

    try {
        VehicleTuningArchiveDocument archive;
        if (archive.Load(bytes) != 0) {
            return std::nullopt;
        }

        std::vector<VehicleTuningScalarChunk> scalarChunks;
        std::vector<TransmissionArray> transmissionArrays;
        std::vector<VehicleTuningCurveRecord> curves;
        if (ReadVehicleTuningScalars(archive, scalarChunks) != 0 ||
            ReadVehicleTransmissionArrays(archive, transmissionArrays) != 0 ||
            ReadVehicleTuningCurves(archive, curves) != 0) {
            return std::nullopt;
        }

        ReplayVehicleTuningDefinition definition;
        if (ApplyVehicleTuningScalars(scalarChunks, definition) != 0 ||
            ApplyVehicleTransmissionArrays(
                    transmissionArrays, definition) != 0 ||
            ApplyVehicleTuningCurves(curves, definition) != 0) {
            return std::nullopt;
        }
        return definition;
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
}
