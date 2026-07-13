#pragma once

#include <cstdint>
#include <vector>

#include "engine/core/gm_types.h"
#include "engine/game/replay_challenge_map_input.h"
class ChallengeFieldUnits;
class ReplaySceneDefinition;

class TerrainTopMarkers {
public:
    bool BuildFromSceneDefinition(
            const ReplaySceneDefinition &scene,
            const CGameCtnReplayMapInput &mapInput);
    bool BuildFromFieldUnits(
            const CGameCtnReplayMapInput &mapInput,
            const ChallengeFieldUnits &fieldUnits);
    bool Contains(int32_t x, int32_t y, int32_t z) const;

private:
    bool BeginMap(const CGameCtnReplayMapInput &mapInput);
    bool AddTopMarker(int32_t x, int32_t y, int32_t z);
    bool AddBaseMarkersForUnoccupiedColumns();
    void MarkOccupiedColumn(int32_t x, int32_t z);
    void Clear();

    std::vector<GmInt3> markers_;
    std::vector<uint8_t> occupiedColumns_;
    u32 sizeX_ = 0u;
    u32 sizeZ_ = 0u;
};
