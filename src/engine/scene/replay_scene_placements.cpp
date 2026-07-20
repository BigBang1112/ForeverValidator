#include "engine/scene/replay_scene_placements.h"
#include <algorithm>
#include <utility>
#include "engine/game/game_ctn_block.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/game/game_ctn_challenge.h"
#include "engine/game/game_ctn_app.h"
#include "engine/scene/scene_mobil.h"

u32 ReplaySceneModelCounts::TotalWithEnvironment(
        u32 environmentModelCount) const {
    return placedBlocks + blockSubMobils + clipAssemblies + helperAssemblies +
           checkpointTriggers + pylons + environmentModelCount;
}

void ReplaySceneBlockPlacements::Clear() {
    placements_.clear();
    collisionPlacements_.clear();
    dedicatedCollisionPlacements_.clear();
    pylonPlacements_.clear();
    installationStream_.clear();
    retainedLogicalStartPlacements_.clear();
    nextInstallationOrdinal_ = 0u;
}

void ReplaySceneBlockPlacements::SetRetainedLogicalStartPlacements(
        std::vector<ReplaySceneLogicalStartPlacement> placements) {
    retainedLogicalStartPlacements_ = std::move(placements);
}

void ReplaySceneBlockPlacements::AppendBlockMobil(
        const CGameCtnBlock &block) {
    AppendBlock(block, nextInstallationOrdinal_++);
}

void ReplaySceneBlockPlacements::AppendPylonMobil(
        CSceneMobil &mobil,
        const GmIso4 &worldIso) {
    const ReplaySceneInstallationOrdinal ordinal =
            nextInstallationOrdinal_++;
    ReplayScenePylonPlacement placement(mobil, worldIso, ordinal);
    pylonPlacements_.push_back(placement);
    installationStream_.emplace_back(std::move(placement));
}

void ReplaySceneBlockPlacements::AppendBlock(
        const CGameCtnBlock &block,
        ReplaySceneInstallationOrdinal ordinal) {
    const CGameCtnBlockInfo *blockInfo = block.BlockInfoRef();
    if (blockInfo == nullptr) {
        return;
    }
    ReplaySceneBlockPlacement placement(
            block, block.MobilLocation(), ordinal);
    const bool hasPublic = placement.HasPublicSceneRepresentation();
    const bool hasDedicated =
            placement.HasDedicatedCollisionRepresentation();
    if (!hasPublic && !hasDedicated) {
        return;
    }

    collisionPlacements_.push_back(placement);
    if (hasPublic) {
        placements_.push_back(placement);
    }
    if (hasDedicated) {
        dedicatedCollisionPlacements_.push_back(placement);
    }
    installationStream_.emplace_back(std::move(placement));
}

void ReplaySceneBlockPlacements::AppendLogicalStart(
        const CGameCtnBlock &block) {
    const CGameCtnBlockInfo *blockInfo = block.BlockInfoRef();
    if (blockInfo == nullptr || !::IsStartLine(blockInfo->RaceRole())) {
        return;
    }
    const bool alreadyPresent = std::any_of(
            placements_.begin(),
            placements_.end(),
            [&block](const ReplaySceneBlockPlacement &placement) {
                return &placement.Block() == &block;
    });
    if (!alreadyPresent) {
        placements_.emplace_back(
                block, block.MobilLocation(), nextInstallationOrdinal_++);
    }
}

void ReplaySceneBlockPlacements::RemoveBlockOrdinal(
        ReplaySceneInstallationOrdinal ordinal,
        const CSceneMobil *mobil,
        bool remainsActive) {
    const auto update = [ordinal, mobil, remainsActive](
            std::vector<ReplaySceneBlockPlacement> &placements,
            bool publicView,
            bool dedicatedView) {
        for (auto placement = placements.begin();
             placement != placements.end();) {
            if (placement->InstallationOrdinal() != ordinal) {
                ++placement;
                continue;
            }
            (void)placement->RemoveMobilFromGeneration(mobil);
            const bool hasPublic =
                    placement->HasPublicSceneRepresentation();
            const bool hasDedicated =
                    placement->HasDedicatedCollisionRepresentation();
            const bool represented = publicView
                    ? hasPublic
                    : (dedicatedView
                               ? hasDedicated
                               : hasPublic || hasDedicated);
            if (!remainsActive || !represented) {
                placement = placements.erase(placement);
            } else {
                ++placement;
            }
        }
    };
    update(placements_, true, false);
    update(collisionPlacements_, false, false);
    update(dedicatedCollisionPlacements_, false, true);
}

void ReplaySceneBlockPlacements::RemovePylonOrdinal(
        ReplaySceneInstallationOrdinal ordinal) {
    pylonPlacements_.erase(
            std::remove_if(
                    pylonPlacements_.begin(),
                    pylonPlacements_.end(),
                    [ordinal](const ReplayScenePylonPlacement &placement) {
                        return placement.InstallationOrdinal() == ordinal;
                    }),
            pylonPlacements_.end());
}

void ReplaySceneBlockPlacements::RemoveMobilFromScene(
        CSceneMobil &mobil) {
    for (ReplaySceneInstallation &installation : installationStream_) {
        if (!installation.ContainsMobil(&mobil)) {
            continue;
        }
        const ReplaySceneInstallationTag tag = installation.Tag();
        const ReplaySceneInstallationOrdinal ordinal =
                installation.InstallationOrdinal();
        if (!installation.RemoveMobil(&mobil)) {
            continue;
        }
        if (tag == ReplaySceneInstallationTag::Block) {
            RemoveBlockOrdinal(
                    ordinal, &mobil, installation.IsActive());
        } else {
            RemovePylonOrdinal(ordinal);
        }
    }
}

void ReplaySceneBlockPlacements::RemoveRetiredLogicalBlock(
        const CGameCtnBlock &block) {
    placements_.erase(
            std::remove_if(
                    placements_.begin(),
                    placements_.end(),
                    [this, &block](
                            const ReplaySceneBlockPlacement &placement) {
                        if (&placement.Block() != &block) {
                            return false;
                        }
                        return std::none_of(
                                installationStream_.begin(),
                                installationStream_.end(),
                                [&placement](
                                        const ReplaySceneInstallation &entry) {
                                    return entry.InstallationOrdinal() ==
                                           placement.InstallationOrdinal();
                                });
                    }),
            placements_.end());
}

void ReplaySceneBlockPlacements::PopulateFromChallenge(
        CGameCtnChallenge &challenge) {
    Clear();
    placements_.reserve(challenge.Blocks().size());
    collisionPlacements_.reserve(challenge.Blocks().size());
    dedicatedCollisionPlacements_.reserve(challenge.Blocks().size());

    CGameCtnApp app(*this,
                    challenge,
                    challenge.CollectionSquareSize(),
                    challenge.CollectionSquareHeight());
    app.UpdateBlockMobils();
    app.AddClipsToScene();

    for (const auto &owned : challenge.Blocks()) {
        if (owned != nullptr) {
            AppendLogicalStart(*owned);
        }
    }
}

const std::vector<ReplaySceneBlockPlacement> &
ReplaySceneBlockPlacements::Placements() const {
    return placements_;
}

const std::vector<ReplaySceneBlockPlacement> &
ReplaySceneBlockPlacements::CollisionPlacements() const {
    return collisionPlacements_;
}

const std::vector<ReplaySceneBlockPlacement> &
ReplaySceneBlockPlacements::DedicatedCollisionPlacements() const {
    return dedicatedCollisionPlacements_;
}

const std::vector<ReplayScenePylonPlacement> &
ReplaySceneBlockPlacements::PylonPlacements() const {
    return pylonPlacements_;
}

const std::vector<ReplaySceneInstallation> &
ReplaySceneBlockPlacements::InstallationStream() const {
    return installationStream_;
}

bool ReplaySceneBlockPlacements::ContainsMobil(
        const CSceneMobil *mobil) const {
    return mobil != nullptr && std::any_of(
            installationStream_.begin(),
            installationStream_.end(),
            [mobil](const ReplaySceneInstallation &installation) {
                return installation.IsActive() &&
                       installation.ContainsMobil(mobil);
            });
}

bool ReplaySceneBlockPlacements::ReferencesBlock(
        const CGameCtnBlock *block) const {
    if (block == nullptr) {
        return false;
    }
    const auto references = [block](
            const ReplaySceneBlockPlacement &placement) {
        return &placement.Block() == block;
    };
    if (std::any_of(placements_.begin(), placements_.end(), references) ||
        std::any_of(collisionPlacements_.begin(),
                    collisionPlacements_.end(),
                    references) ||
        std::any_of(dedicatedCollisionPlacements_.begin(),
                    dedicatedCollisionPlacements_.end(),
                    references)) {
        return true;
    }
    return std::any_of(
            installationStream_.begin(),
            installationStream_.end(),
            [block](const ReplaySceneInstallation &installation) {
                const ReplaySceneBlockPlacement *placement =
                        installation.BlockPlacement();
                return placement != nullptr &&
                       &placement->Block() == block;
            });
}

bool ReplaySceneBlockPlacements::Empty() const {
    return collisionPlacements_.empty() && pylonPlacements_.empty();
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
        counts.blockSubMobils += static_cast<u32>(
                placement.SubMobilAssetPlacements().size());
        counts.helperAssemblies +=
                placement.HelperAssetPlacements().empty() ? 0u : 1u;
        counts.checkpointTriggers +=
                placement.CheckpointTriggerAssetPlacement().has_value()
                ? 1u
                : 0u;
    }
    counts.pylons = static_cast<u32>(pylonPlacements_.size());
    return counts;
}

std::optional<GmIso4>
ReplaySceneBlockPlacements::FirstSurvivingStartLineSpawn()
        const {
    for (const ReplaySceneBlockPlacement &placement : placements_) {
        if (placement.IsStartLine()) {
            return placement.BlockSpawnIso();
        }
    }
    return std::nullopt;
}

std::optional<GmIso4>
ReplaySceneBlockPlacements::FirstRetainedLogicalStartLineSpawn() const {
    if (retainedLogicalStartPlacements_.empty()) {
        return std::nullopt;
    }
    return retainedLogicalStartPlacements_.front().spawnLocation;
}

std::optional<GmIso4> ReplaySceneBlockPlacements::FirstStartLineSpawn()
        const {
    const std::optional<GmIso4> surviving =
            FirstSurvivingStartLineSpawn();
    return surviving.has_value()
            ? surviving
            : FirstRetainedLogicalStartLineSpawn();
}
