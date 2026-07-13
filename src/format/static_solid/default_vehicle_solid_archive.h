#pragma once

#include <optional>

#include "engine/game/replay_vehicle_solid_definition.h"
class CPlugFilePack;
class DefaultVehicleSolidArchive {
public:
    static std::optional<ReplayVehicleSolidDefinition> LoadFromPack(
            CPlugFilePack &pack);
};
