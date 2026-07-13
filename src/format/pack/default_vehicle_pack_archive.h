#ifndef TMNF_DEFAULT_VEHICLE_PACK_ARCHIVE_H
#define TMNF_DEFAULT_VEHICLE_PACK_ARCHIVE_H

#include <optional>

#include "engine/game/replay_vehicle_source_definition.h"
#include "engine/game/replay_vehicle_tuning_definition.h"
class CPlugFilePack;
struct DefaultVehiclePackData {
    ReplayVehicleTuningDefinition tuning;
    ReplayVehicleSourceDefinition vehicle;
};

class DefaultVehiclePackArchive {
public:
    static std::optional<DefaultVehiclePackData> LoadFromPack(
            CPlugFilePack &pack);
};

#endif
