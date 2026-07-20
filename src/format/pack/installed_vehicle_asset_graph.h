#pragma once

#include <optional>
#include <string>

#include "engine/core/engine_types.h"

class CPlugFilePack;

struct InstalledVehicleAssetReference {
    std::string logicalPath;
    std::string selectedPath;
    u32 classId = 0u;

    bool IsValid() const {
        return !logicalPath.empty() && !selectedPath.empty() && classId != 0u;
    }
};

struct InstalledVehicleAssetGraph {
    InstalledVehicleAssetReference collector;
    InstalledVehicleAssetReference mobil;
    InstalledVehicleAssetReference solid;
    InstalledVehicleAssetReference tuning;
    InstalledVehicleAssetReference materials;
    InstalledVehicleAssetReference vehicleStruct;
    InstalledVehicleAssetReference fakeContactBitmap;
    InstalledVehicleAssetReference fakeContactTexture;

    static std::optional<InstalledVehicleAssetGraph> ResolveFromPack(
            CPlugFilePack &pack);
    bool IsComplete() const;
};
