#include "engine/scene/replay_scene_placements.h"
#include "engine/game/game_ctn_block.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/game/game_ctn_challenge.h"
#include "engine/scene/scene_mobil.h"
namespace {

bool HasAssetReference(const CSceneMobil *mobil) {
    return mobil != nullptr && mobil->StaticSolidAsset().IsSpecified();
}

bool HasHelperAssetReference(const CGameCtnBlock &block) {
    const CGameCtnBlockInfo *blockInfo = block.BlockInfoRef();
    if (blockInfo == nullptr) {
        return false;
    }
    const CSceneMobil *family =
            blockInfo->FamilyHelperMobilSource(block.UsesGroundMobilSize());
    return HasAssetReference(family) ||
           HasAssetReference(blockInfo->CommonHelperMobilSource());
}

bool HasClipAssetReference(const CGameCtnBlock &block) {
    for (const CMwNodRef<CSceneMobil> &source : block.ClipSourceMobils()) {
        if (HasAssetReference(source.Get())) {
            return true;
        }
    }
    return false;
}

bool HasSceneAssetReference(const CGameCtnBlock &block) {
    return HasAssetReference(block.MainMobil()) ||
           HasHelperAssetReference(block) ||
           HasClipAssetReference(block);
}

const CGameCtnBlock &EffectiveAuthoredBlock(
        const CGameCtnChallenge &challenge,
        const CGameCtnBlock &authored) {
    const CGameCtnBlock *effective = &authored;
    if (!authored.BlockInstanceId()) {
        return *effective;
    }
    for (const auto &candidate : challenge.Blocks()) {
        if (candidate != nullptr &&
            candidate->Origin() ==
                    CGameCtnBlock::PlacementOrigin::TerrainModifier &&
            candidate->BlockInstanceId() &&
            *candidate->BlockInstanceId() == *authored.BlockInstanceId()) {
            effective = candidate.get();
        }
    }
    return *effective;
}

bool HasTerrainModifierReplacementForAutomaticBase(
        const CGameCtnChallenge &challenge,
        std::size_t index) {
    const auto &blocks = challenge.Blocks();
    const CGameCtnBlock &block = *blocks[index];
    if (block.Origin() != CGameCtnBlock::PlacementOrigin::AutomaticBase) {
        return false;
    }
    for (std::size_t later = index + 1u; later < blocks.size(); ++later) {
        const CGameCtnBlock &candidate = *blocks[later];
        if (candidate.Origin() ==
                    CGameCtnBlock::PlacementOrigin::
                            AutomaticBaseTerrainModifier &&
            block.BaseCoord().x == candidate.BaseCoord().x &&
            block.BaseCoord().y == candidate.BaseCoord().y &&
            block.BaseCoord().z == candidate.BaseCoord().z &&
            block.SourceAsset() == candidate.SourceAsset()) {
            return true;
        }
    }
    return false;
}

}  // namespace

u32 ReplaySceneModelCounts::TotalWithEnvironment(
        u32 environmentModelCount) const {
    return placedBlocks + clipAssemblies + helperAssemblies +
           checkpointTriggers + environmentModelCount;
}

void ReplaySceneBlockPlacements::Clear() {
    placements_.clear();
}

void ReplaySceneBlockPlacements::AppendBlock(const CGameCtnBlock &block) {
    if (block.BlockInfoRef() == nullptr || !HasSceneAssetReference(block)) {
        return;
    }
    placements_.emplace_back(block, block.MobilLocation());
}

void ReplaySceneBlockPlacements::PopulateFromChallenge(
        const CGameCtnChallenge &challenge) {
    Clear();
    const auto &blocks = challenge.Blocks();
    placements_.reserve(blocks.size());

    // An authored placement keeps its collision-order slot when a terrain
    // modifier replaces it.
    for (const auto &owned : blocks) {
        if (owned != nullptr &&
            owned->Origin() == CGameCtnBlock::PlacementOrigin::Authored) {
            AppendBlock(EffectiveAuthoredBlock(challenge, *owned));
        }
    }

    for (std::size_t index = 0u; index < blocks.size(); ++index) {
        const CGameCtnBlock *block = blocks[index].get();
        if (block != nullptr &&
            block->Origin() ==
                    CGameCtnBlock::PlacementOrigin::AutomaticBase &&
            !HasTerrainModifierReplacementForAutomaticBase(
                    challenge, index)) {
            AppendBlock(*block);
        }
    }

    // Automatic terrain replacements follow unaffected automatic base cells.
    for (const auto &owned : blocks) {
        if (owned != nullptr &&
            owned->Origin() ==
                    CGameCtnBlock::PlacementOrigin::
                            AutomaticBaseTerrainModifier) {
            AppendBlock(*owned);
        }
    }
}

const std::vector<ReplaySceneBlockPlacement> &
ReplaySceneBlockPlacements::Placements() const {
    return placements_;
}

bool ReplaySceneBlockPlacements::Empty() const {
    return placements_.empty();
}

ReplaySceneModelCounts ReplaySceneBlockPlacements::ModelCounts() const {
    ReplaySceneModelCounts counts;
    for (const ReplaySceneBlockPlacement &placement : placements_) {
        if (placement.IsSuppressed()) {
            continue;
        }
        if (placement.IsClip()) {
            counts.clipAssemblies +=
                    placement.ClipAssetPlacements().empty() ? 0u : 1u;
            continue;
        }
        counts.placedBlocks +=
                placement.MainAssetPlacement().has_value() ? 1u : 0u;
        counts.helperAssemblies +=
                placement.HelperAssetPlacements().empty() ? 0u : 1u;
        counts.checkpointTriggers +=
                placement.CheckpointTriggerAssetPlacement().has_value()
                ? 1u
                : 0u;
    }
    return counts;
}

std::optional<GmIso4> ReplaySceneBlockPlacements::FirstStartLineSpawn()
        const {
    for (const ReplaySceneBlockPlacement &placement : placements_) {
        if (placement.IsStartLine()) {
            return placement.BlockSpawnIso();
        }
    }
    return std::nullopt;
}
