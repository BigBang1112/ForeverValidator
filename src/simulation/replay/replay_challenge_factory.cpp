#include "engine/game/game_ctn_block_info.h"
#include "simulation/replay/replay_challenge_factory.h"
#include <memory>
#include <vector>

#include "simulation/replay/replay_challenge_construction.h"
#include "simulation/replay/replay_challenge_field_units.h"
namespace {

CGameCtnBlock *CreateConstructedBlock(
        CGameCtnChallenge &challenge,
        const CGameCtnReplayMapInputBlock &placement,
        const ReplaySceneBlockDefinition &definition) {
    CGameCtnBlockInfo *info = definition.BlockInfo();
    if (info == nullptr) {
        return nullptr;
    }
    auto loadedBlock = std::make_unique<CGameCtnBlock>(
            info,
            placement.Coordinate(),
            placement.Direction(),
            definition.Placement(),
            nullptr);
    CGameCtnBlock *block = loadedBlock.get();
    block->SetInstanceId(
            CGameCtnBlock::InstanceId(placement.Id().Ordinal()));
    block->SetSourceAsset(
            definition.Asset(),
            CGameCtnBlock::PlacementOrigin::Authored);
    block->SetMaterialRemaps(
            definition.UsesReplacementMaterialRemap(),
            definition.UsesDecorationSkinMaterialRemap());
    if (definition.UsesCollectionLandZoneHeight()) {
        block->SetMobilVerticalOffset(-static_cast<float>(
                *definition.CollectionLandZoneHeight()) *
                CGameCtnBlock::SquareHeight);
    }
    block->SetClipSourceMobils(definition.ClipSourceMobils());
    return challenge.AddLoadedBlock(std::move(loadedBlock));
}

CGameCtnBlock *CreateAutomaticBase(
        CGameCtnChallenge &challenge,
        const ReplaySceneBlockDefinition &definition,
        u32 x,
        u32 z) {
    CGameCtnBlockInfo *info = definition.BlockInfo();
    if (info == nullptr) {
        return nullptr;
    }
    auto loadedBlock = std::make_unique<CGameCtnBlock>(
            info,
            GmNat3{x, 0u, z},
            ECardinalDir::North,
            definition.Placement(),
            nullptr);
    CGameCtnBlock *block = loadedBlock.get();
    block->SetSourceAsset(
            definition.Asset(),
            CGameCtnBlock::PlacementOrigin::AutomaticBase);
    block->SetMaterialRemaps(
            definition.UsesReplacementMaterialRemap(),
            definition.UsesDecorationSkinMaterialRemap());
    block->SetClipSourceMobils(definition.ClipSourceMobils());
    return challenge.AddLoadedBlock(std::move(loadedBlock));
}

bool OccupiesColumn(const ChallengeFieldUnits &fieldUnits,
                    u32 x,
                    u32 z) {
    for (const auto &unit : fieldUnits.Units()) {
        if (unit.IsTerrainTopMarker() &&
            unit.position.x == static_cast<int32_t>(x) &&
            unit.position.z == static_cast<int32_t>(z)) {
            return true;
        }
    }
    return false;
}

struct TopMarker {
    int32_t x;
    int32_t y;
    int32_t z;
    CGameCtnBlock *block;
};

void ApplyFieldUnitRemovalParity(
        const ChallengeFieldUnits &fieldUnits,
        const std::vector<CGameCtnBlock *> &authored,
        const std::vector<CGameCtnBlock *> &automaticBase) {
    std::vector<TopMarker> markers;
    markers.reserve(fieldUnits.Count() + automaticBase.size());
    for (const auto &unit : fieldUnits.Units()) {
        const u32 ordinal = unit.placementId.Ordinal();
        if (!unit.IsTerrainTopMarker() || ordinal >= authored.size()) {
            continue;
        }
        markers.push_back(
                {unit.position.x,
                 unit.position.y + 1,
                 unit.position.z,
                 authored[ordinal]});
    }
    for (CGameCtnBlock *block : automaticBase) {
        markers.push_back({static_cast<int32_t>(block->BaseCoord().x),
                           static_cast<int32_t>(block->BaseCoord().y) + 1,
                           static_cast<int32_t>(block->BaseCoord().z),
                           block});
    }
    for (const auto &unit : fieldUnits.Units()) {
        const u32 ordinal = unit.placementId.Ordinal();
        if (!unit.IsPlacedMobil() || ordinal >= authored.size()) {
            continue;
        }
        CGameCtnBlock *remover = authored[ordinal];
        for (const TopMarker &marker : markers) {
            if (marker.x != unit.position.x ||
                marker.y != unit.position.y ||
                marker.z != unit.position.z) {
                continue;
            }
            if (marker.block->IsSuppressed()) {
                marker.block->ClearSuppression();
            } else {
                marker.block->SuppressBy(*remover);
            }
        }
    }
}

u32 ColumnHeight(const std::vector<CGameCtnBlock *> &blocks,
                 int32_t x,
                 int32_t z) {
    for (const CGameCtnBlock *block : blocks) {
        if (block->GetType() <= EBlockType::Frontier &&
            static_cast<int32_t>(block->BaseCoord().x) == x &&
            static_cast<int32_t>(block->BaseCoord().z) == z) {
            return block->BaseCoord().y & 0xffu;
        }
    }
    return 0u;
}

std::optional<CMwId> ColumnModifier(
        const ChallengeFieldUnits &fieldUnits,
        const std::vector<CGameCtnBlock *> &blocks,
        u32 mapHeight,
        int32_t x,
        int32_t z) {
    for (u32 y = ColumnHeight(blocks, x, z) + 2u; y < mapHeight; ++y) {
        for (const auto &unit : fieldUnits.Units()) {
            if (unit.position.x == x &&
                unit.position.y == static_cast<int32_t>(y) &&
                unit.position.z == z &&
                unit.terrainModifierId) {
                return unit.terrainModifierId;
            }
        }
    }
    return std::nullopt;
}

bool PlacementAlreadyHasModifier(
        const ChallengeFieldUnits &fieldUnits,
        const CGameCtnBlock &block) {
    if (!block.BlockInstanceId()) {
        return false;
    }
    return fieldUnits.HasTerrainModifierFor(
            CGameCtnReplayBlockPlacementId(
                    block.BlockInstanceId()->Value()));
}

bool AppendTerrainModifierBlocks(
        CGameCtnChallenge &challenge,
        const ChallengeFieldUnits &fieldUnits,
        u32 *appendedOut) {
    std::vector<CGameCtnBlock *> sourceBlocks;
    sourceBlocks.reserve(challenge.Blocks().size());
    for (const auto &block : challenge.Blocks()) {
        sourceBlocks.push_back(block.get());
    }
    u32 appended = 0u;
    for (CGameCtnBlock *source : sourceBlocks) {
        if ((!source->UsesGroundMobilSize() &&
             source->GetType() > EBlockType::Frontier) ||
            PlacementAlreadyHasModifier(fieldUnits, *source)) {
            continue;
        }
        std::optional<CMwId> modifier;
        if (source->BlockInstanceId()) {
            bool foundUnit = false;
            for (const auto &unit : fieldUnits.Units()) {
                if (unit.placementId.Ordinal() !=
                    source->BlockInstanceId()->Value()) {
                    continue;
                }
                foundUnit = true;
                const auto found = ColumnModifier(fieldUnits,
                                                  sourceBlocks,
                                                  challenge.MapHeight(),
                                                  unit.position.x,
                                                  unit.position.z);
                if (found) {
                    modifier = found;
                }
            }
            if (!foundUnit) {
                modifier = ColumnModifier(fieldUnits,
                                          sourceBlocks,
                                          challenge.MapHeight(),
                                          static_cast<int32_t>(source->BaseCoord().x),
                                          static_cast<int32_t>(source->BaseCoord().z));
            }
        } else {
            modifier = ColumnModifier(fieldUnits,
                                      sourceBlocks,
                                      challenge.MapHeight(),
                                      static_cast<int32_t>(source->BaseCoord().x),
                                      static_cast<int32_t>(source->BaseCoord().z));
        }
        if (!modifier) {
            continue;
        }
        auto loadedCopy = std::make_unique<CGameCtnBlock>(source);
        CGameCtnBlock *copy = loadedCopy.get();
        if (source->BlockInstanceId()) {
            copy->SetInstanceId(*source->BlockInstanceId());
        }
        copy->SetTerrainModifiedId(*modifier);
        copy->SetSourceAsset(
                source->SourceAsset(),
                source->Origin() == CGameCtnBlock::PlacementOrigin::AutomaticBase
                        ? CGameCtnBlock::PlacementOrigin::
                                  AutomaticBaseTerrainModifier
                        : CGameCtnBlock::PlacementOrigin::TerrainModifier);
        copy->SetMaterialRemaps(
                source->UsesReplacementMaterialRemap(),
                source->UsesReplacementMaterialRemap()
                        ? source->UsesDecorationSkinMaterialRemap()
                        : true);
        if (const CGameCtnBlock *suppressor = source->SuppressingBlock()) {
            copy->SuppressBy(*suppressor);
        }
        if (challenge.AddLoadedBlock(std::move(loadedCopy)) == nullptr) {
            return false;
        }
        appended++;
    }
    *appendedOut = appended;
    return true;
}

}  // namespace

bool BuildReplayChallenge(
        const CGameCtnReplayMapInput &mapInput,
        const ReplaySceneDefinition &scene,
        CGameCtnChallengeConstruction &construction,
        ReplayChallengeBuildReport &report) {
    report = ReplayChallengeBuildReport{};
    report.mapBlockCount = mapInput.BlockCount();

    ChallengeFieldUnits fieldUnits;
    if (!fieldUnits.Build(scene, mapInput)) {
        return false;
    }

    auto challenge = std::make_unique<CGameCtnChallenge>();
    challenge->SetMapSize(mapInput.Size());

    std::vector<CGameCtnBlock *> authored;
    authored.reserve(mapInput.BlockCount());
    u32 missingBlocks = 0u;
    for (const auto &placement : mapInput.Blocks()) {
        const ReplaySceneBlockDefinition *definition =
                scene.FindAuthoredBlock(placement.Id());
        if (definition == nullptr) {
            missingBlocks++;
            continue;
        }
        CGameCtnBlock *block = CreateConstructedBlock(
                *challenge, placement, *definition);
        if (block == nullptr) {
            return false;
        }
        authored.push_back(block);
    }

    std::vector<CGameCtnBlock *> automaticBase;
    const ReplaySceneBlockDefinition *base = scene.AutomaticBaseBlock();
    if (fieldUnits.Count() != 0u && base != nullptr) {
        for (u32 x = 0u; x < mapInput.Size().x; ++x) {
            for (u32 z = 0u; z < mapInput.Size().z; ++z) {
                if (OccupiesColumn(fieldUnits, x, z)) {
                    continue;
                }
                CGameCtnBlock *block = CreateAutomaticBase(
                        *challenge, *base, x, z);
                if (block == nullptr) {
                    return false;
                }
                automaticBase.push_back(block);
            }
        }
    }

    ApplyFieldUnitRemovalParity(fieldUnits, authored, automaticBase);
    u32 terrainModifierCount = 0u;
    if (!AppendTerrainModifierBlocks(*challenge,
                                     fieldUnits,
                                     &terrainModifierCount)) {
        return false;
    }

    construction.SetCounts(static_cast<u32>(authored.size()),
                           static_cast<u32>(automaticBase.size()),
                           terrainModifierCount,
                           missingBlocks);
    construction.SetChallenge(std::move(challenge));
    report.resolvedBlockCount = static_cast<u32>(scene.BlockCount());
    report.fieldUnitCount = fieldUnits.Count();
    report.constructedBlockCount = construction.BlockCount();
    report.automaticBaseBlockCount = construction.AutomaticBaseBlockCount();
    report.terrainModifierBlockCount =
            construction.TerrainModifierBlockCount();
    report.missingBlockCount = construction.MissingBlockCount();
    report.complete = missingBlocks == 0u &&
            construction.BlockCount() == authored.size() +
                    automaticBase.size() + terrainModifierCount;
    return report.complete;
}
