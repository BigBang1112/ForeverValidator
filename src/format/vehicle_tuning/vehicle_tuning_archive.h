#ifndef TMNF_FORMAT_VEHICLE_TUNING_ARCHIVE_H
#define TMNF_FORMAT_VEHICLE_TUNING_ARCHIVE_H

#include <cstdint>
#include <optional>
#include <vector>

#include "engine/game/replay_vehicle_tuning_definition.h"
class VehicleTuningArchive {
public:
    static std::optional<ReplayVehicleTuningDefinition> Decode(
            const std::vector<std::uint8_t> &bytes);
};

#endif
