#pragma once

#include <optional>
#include <vector>

#include "engine/core/gm_types.h"
#include "engine/core/mw_id.h"
#include "engine/game/replay_challenge_map_input.h"
#include "simulation/replay/replay_scene_definition.h"
class TerrainTopMarkers;

struct ChallengeFieldUnit {
    GmInt3 position{};
    EBlockType blockType = EBlockType::Classic;
    CGameCtnReplayBlockPlacementId placementId;
    std::optional<CMwId> terrainModifierId;

    bool IsTerrainTopMarker() const;
    bool IsPlacedMobil() const;
    bool BelongsTo(CGameCtnReplayBlockPlacementId id) const;
};

class ChallengeFieldUnits {
public:
    bool Build(const ReplaySceneDefinition &scene,
               const CGameCtnReplayMapInput &mapInput);

    u32 Count() const;
    const std::vector<ChallengeFieldUnit> &Units() const;
    bool HasTerrainModifierFor(
            CGameCtnReplayBlockPlacementId placementId) const;

private:
    enum class BuildPhase {
        Terrain,
        PlacedMobils,
    };

    bool BuildPhaseUnits(
            BuildPhase phase,
            const ReplaySceneDefinition &scene,
            const CGameCtnReplayMapInput &mapInput,
            TerrainTopMarkers &terrainMarkers);
    bool AddBlockUnits(
            const ReplaySceneBlockDefinition &definition,
            const CGameCtnReplayMapInput &mapInput,
            const TerrainTopMarkers &terrainMarkers);
    bool AddPlacementUnits(
            const CGameCtnReplayMapInputBlock &placement,
            const ReplaySceneBlockDefinition &definition,
            class CGameCtnBlockInfo &blockInfo,
            bool ground);
    bool Append(ChallengeFieldUnit unit);
    void Clear();

    std::vector<ChallengeFieldUnit> units_;
    u32 missingBlockInfoCount_ = 0u;
};
