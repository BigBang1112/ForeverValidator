#include "simulation/replay/replay_terrain_top_markers.h"
#include <cstdint>

#include "simulation/replay/replay_challenge_field_units.h"
#include "simulation/replay/replay_scene_definition.h"
#include <new>
void TerrainTopMarkers::Clear() {
    markers_.clear();
    occupiedColumns_.clear();
    sizeX_ = 0u;
    sizeZ_ = 0u;
}

bool TerrainTopMarkers::BeginMap(
        const CGameCtnReplayMapInput &mapInput) {
    const GmNat3 &mapSize = mapInput.Size();
    if (mapSize.x > UINT32_MAX / (mapSize.z != 0u ? mapSize.z : 1u)) {
        return false;
    }
    Clear();
    sizeX_ = mapSize.x;
    sizeZ_ = mapSize.z;
    const u32 cellCount = sizeX_ * sizeZ_;
    try {
        occupiedColumns_.resize(cellCount != 0u ? cellCount : 1u);
        return true;
    } catch (const std::bad_alloc &) {
        Clear();
        return false;
    }
}

bool TerrainTopMarkers::AddTopMarker(
        int32_t x,
        int32_t y,
        int32_t z) {
    if (markers_.size() >= UINT32_MAX) {
        return false;
    }
    try {
        markers_.push_back({x, y, z});
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

void TerrainTopMarkers::MarkOccupiedColumn(int32_t x, int32_t z) {
    if (x < 0 || z < 0) {
        return;
    }
    const u32 columnX = static_cast<u32>(x);
    const u32 columnZ = static_cast<u32>(z);
    if (columnX < sizeX_ && columnZ < sizeZ_) {
        occupiedColumns_[columnX * sizeZ_ + columnZ] = 1u;
    }
}

bool TerrainTopMarkers::AddBaseMarkersForUnoccupiedColumns() {
    for (u32 x = 0u; x < sizeX_; ++x) {
        for (u32 z = 0u; z < sizeZ_; ++z) {
            if (occupiedColumns_[x * sizeZ_ + z] != 0u) {
                continue;
            }
            if (!AddTopMarker(static_cast<int32_t>(x),
                              1,
                              static_cast<int32_t>(z))) {
                return false;
            }
        }
    }
    return true;
}

bool TerrainTopMarkers::Contains(
        int32_t x,
        int32_t y,
        int32_t z) const {
    for (const GmInt3 &marker : markers_) {
        if (marker.x == x && marker.y == y && marker.z == z) {
            return true;
        }
    }
    return false;
}

bool TerrainTopMarkers::BuildFromSceneDefinition(
        const ReplaySceneDefinition &scene,
        const CGameCtnReplayMapInput &mapInput) {
    if (!BeginMap(mapInput)) {
        return false;
    }
    for (const ReplaySceneBlockDefinition &definition : scene.Blocks()) {
        if (!definition.IsAuthored() || !definition.IsTerrainBlock()) {
            continue;
        }
        const CGameCtnReplayMapInputBlock *block =
                mapInput.FindBlock(*definition.PlacementId());
        if (block == nullptr) {
            Clear();
            return false;
        }
        const GmNat3 &coordinate = block->Coordinate();
        if (!AddTopMarker(static_cast<int32_t>(coordinate.x),
                          static_cast<int32_t>(coordinate.y) + 1,
                          static_cast<int32_t>(coordinate.z))) {
            Clear();
            return false;
        }
        MarkOccupiedColumn(static_cast<int32_t>(coordinate.x),
                           static_cast<int32_t>(coordinate.z));
    }
    if (!AddBaseMarkersForUnoccupiedColumns()) {
        Clear();
        return false;
    }
    return true;
}

bool TerrainTopMarkers::BuildFromFieldUnits(
        const CGameCtnReplayMapInput &mapInput,
        const ChallengeFieldUnits &fieldUnits) {
    if (!BeginMap(mapInput)) {
        return false;
    }
    for (const ChallengeFieldUnit &unit : fieldUnits.Units()) {
        if (!unit.IsTerrainTopMarker()) {
            continue;
        }
        if (!AddTopMarker(unit.position.x,
                          unit.position.y + 1,
                          unit.position.z)) {
            Clear();
            return false;
        }
        MarkOccupiedColumn(unit.position.x, unit.position.z);
    }
    if (!AddBaseMarkersForUnoccupiedColumns()) {
        Clear();
        return false;
    }
    return true;
}
