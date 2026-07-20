#include "simulation/replay/replay_challenge_field_units.h"
#include <utility>

#include "engine/game/game_ctn_block_info.h"
#include "simulation/replay/replay_terrain_top_markers.h"
#include <new>
namespace {

GmInt3 RotatedOffset(CGameCtnBlockInfo &blockInfo,
                     const CGameCtnBlockUnitInfo &unit,
                     ECardinalDir direction,
                     bool ground) {
    const GmNat3 offset = blockInfo.RotatedUnitOffset(
            unit, direction, ground ? 1 : 0);
    return {static_cast<int32_t>(offset.x),
            static_cast<int32_t>(offset.y),
            static_cast<int32_t>(offset.z)};
}

std::optional<CMwId> TerrainModifier(
        const CGameCtnBlockUnitInfo &unit) {
    return unit.TerrainModifierId().IsInvalid()
            ? std::nullopt
            : std::optional<CMwId>(unit.TerrainModifierId());
}

bool IsBlockOnGround(CGameCtnBlockInfo &blockInfo,
                     const TerrainTopMarkers &terrainMarkers,
                     const CGameCtnReplayMapInputBlock &block,
                     EBlockType blockType) {
    if (blockType <= EBlockType::Frontier) {
        return true;
    }
    const auto &groundUnits = blockInfo.BlockUnitInfos(1);
    if (groundUnits.empty()) {
        return false;
    }

    bool hasUndergroundUnit = false;
    for (const auto &unit : groundUnits) {
        hasUndergroundUnit = hasUndergroundUnit ||
                unit->IsUndergroundUnit();
    }

    u32 checkedLayer = 0u;
    if (hasUndergroundUnit) {
        bool foundAboveGroundLayer = false;
        for (const auto &unit : groundUnits) {
            if (unit->IsUndergroundUnit()) {
                continue;
            }
            if (!foundAboveGroundLayer || unit->UnitLayer() < checkedLayer) {
                checkedLayer = unit->UnitLayer();
                foundAboveGroundLayer = true;
            }
        }
    }

    const GmNat3 &coordinate = block.Coordinate();
    for (const auto &unit : groundUnits) {
        if (unit->IsUndergroundUnit() ||
            unit->UnitLayer() != checkedLayer) {
            continue;
        }
        const GmInt3 offset = RotatedOffset(
                blockInfo, *unit, block.Direction(), true);
        if (!terrainMarkers.Contains(
                    static_cast<int32_t>(coordinate.x) + offset.x,
                    static_cast<int32_t>(coordinate.y) + offset.y,
                    static_cast<int32_t>(coordinate.z) + offset.z)) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool ChallengeFieldUnit::IsTerrainTopMarker() const {
    return blockType <= EBlockType::Frontier && position.y >= 0;
}

bool ChallengeFieldUnit::IsPlacedMobil() const {
    return blockType > EBlockType::Frontier;
}

bool ChallengeFieldUnit::BelongsTo(
        CGameCtnReplayBlockPlacementId id) const {
    return placementId == id;
}

void ChallengeFieldUnits::Clear() {
    units_.clear();
    missingBlockInfoCount_ = 0u;
}

bool ChallengeFieldUnits::Append(ChallengeFieldUnit unit) {
    try {
        units_.push_back(std::move(unit));
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

u32 ChallengeFieldUnits::Count() const {
    return static_cast<u32>(units_.size());
}

const std::vector<ChallengeFieldUnit> &ChallengeFieldUnits::Units() const {
    return units_;
}

bool ChallengeFieldUnits::HasTerrainModifierFor(
        CGameCtnReplayBlockPlacementId placementId) const {
    for (const ChallengeFieldUnit &unit : units_) {
        if (unit.BelongsTo(placementId) && unit.terrainModifierId) {
            return true;
        }
    }
    return false;
}

bool ChallengeFieldUnits::AddPlacementUnits(
        const CGameCtnReplayMapInputBlock &placement,
        const ReplaySceneBlockDefinition &definition,
        CGameCtnBlockInfo &blockInfo,
        bool ground) {
    const auto &unitInfos = blockInfo.BlockUnitInfos(ground ? 1 : 0);
    if (unitInfos.empty()) {
        return false;
    }

    const GmNat3 &coordinate = placement.Coordinate();
    if (definition.IsTerrainBlock()) {
        return Append({{static_cast<int32_t>(coordinate.x),
                        static_cast<int32_t>(coordinate.y),
                        static_cast<int32_t>(coordinate.z)},
                       definition.Type(),
                       placement.Id(),
                       TerrainModifier(*unitInfos.front())});
    }

    for (const auto &unitInfo : unitInfos) {
        const GmInt3 offset = RotatedOffset(
                blockInfo, *unitInfo, placement.Direction(), ground);
        if (!Append({{static_cast<int32_t>(coordinate.x) + offset.x,
                     static_cast<int32_t>(coordinate.y) + offset.y,
                     static_cast<int32_t>(coordinate.z) + offset.z},
                    definition.Type(),
                    placement.Id(),
                    TerrainModifier(*unitInfo)})) {
            return false;
        }
    }
    return true;
}

bool ChallengeFieldUnits::AddBlockUnits(
        const ReplaySceneBlockDefinition &definition,
        const CGameCtnReplayMapInput &mapInput,
        const TerrainTopMarkers &terrainMarkers) {
    if (!definition.IsAuthored()) {
        return true;
    }
    const CGameCtnReplayMapInputBlock *placement =
            mapInput.FindBlock(*definition.PlacementId());
    if (placement == nullptr) {
        ++missingBlockInfoCount_;
        return true;
    }

    CGameCtnBlockInfo *blockInfo = definition.BlockInfo();
    if (blockInfo == nullptr) {
        ++missingBlockInfoCount_;
        return true;
    }
    blockInfo->ConfigurePlacement(definition.Type(),
                                  blockInfo->UsesCustomSize(),
                                  blockInfo->DefaultPlacementSource());

    if (definition.NeedsDefaultUnitGeometry()) {
        if (!blockInfo->HasBlockUnitInfos(true)) {
            blockInfo->AddBlock({0u, 0u, 0u}, 0, 0u, 1);
        }
        if (!blockInfo->HasBlockUnitInfos(false)) {
            blockInfo->AddBlock({0u, 0u, 0u}, 0, 0u, 0);
        }
    }

    const bool ground = definition.IsTerrainBlock() ||
            IsBlockOnGround(*blockInfo,
                            terrainMarkers,
                            *placement,
                            definition.Type());
    const auto &selectedUnits = blockInfo->BlockUnitInfos(ground ? 1 : 0);
    return selectedUnits.empty() ||
           AddPlacementUnits(*placement, definition, *blockInfo, ground);
}

bool ChallengeFieldUnits::BuildPhaseUnits(
        BuildPhase phase,
        const ReplaySceneDefinition &scene,
        const CGameCtnReplayMapInput &mapInput,
        TerrainTopMarkers &terrainMarkers) {
    if (phase == BuildPhase::PlacedMobils &&
        !terrainMarkers.BuildFromFieldUnits(scene, mapInput, *this)) {
        return false;
    }

    for (const ReplaySceneBlockDefinition &definition : scene.Blocks()) {
        const bool selected = phase == BuildPhase::Terrain
                ? definition.IsTerrainBlock()
                : definition.IsPlacedMobilBlock();
        if (selected && !AddBlockUnits(definition,
                                       mapInput,
                                       terrainMarkers)) {
            return false;
        }
    }
    return true;
}

bool ChallengeFieldUnits::Build(
        const ReplaySceneDefinition &scene,
        const CGameCtnReplayMapInput &mapInput) {
    Clear();
    TerrainTopMarkers terrainMarkers;
    if (!BuildPhaseUnits(BuildPhase::Terrain,
                         scene,
                         mapInput,
                         terrainMarkers) ||
        !BuildPhaseUnits(BuildPhase::PlacedMobils,
                         scene,
                         mapInput,
                         terrainMarkers) ||
        missingBlockInfoCount_ != 0u) {
        Clear();
        return false;
    }
    return true;
}
