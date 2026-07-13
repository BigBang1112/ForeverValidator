// Source-owned block-info mobil families.


#include "engine/game/game_ctn_block_info.h"
#include "engine/rendering/plug_tree.h"
#include "engine/game/game_ctn_utils.h"
#include <algorithm>
#include <cstring>

namespace {

const CMwId CGameCtnBlockInfoHelperTreeLocalId =
        CMwId::CreateFromLocalName("CHALLENGEHELPERTREE");
const CMwId CGameCtnBlockInfoHelperConstructionModeTreeLocalId =
        CMwId::CreateFromLocalName("CHALLENGEHELPERTREECONSTRUCTION");

}  // namespace

CGameCtnBlockInfo::CGameCtnBlockInfo(void) = default;

CGameCtnBlockInfo::~CGameCtnBlockInfo(void) = default;

void CGameCtnBlockInfo::UpdateSize(void) {
    const auto updateFamily = [](const auto &units, GmNat3 &size) {
        size = {1u, 1u, 1u};
        for (const auto &unit : units) {
            if (unit == nullptr) {
                continue;
            }
            const GmNat3 &offset = unit->UnitOffset();
            size.x = std::max(size.x, offset.x + 1u);
            size.y = std::max(size.y, offset.y + 1u);
            size.z = std::max(size.z, offset.z + 1u);
        }
    };
    updateFamily(groundBlockUnitInfos_, groundSize_);
    updateFamily(airBlockUnitInfos_, airSize_);
}

int CGameCtnBlockInfo::IsLandscape(void) const {
    return blockType_ < EBlockType::Classic;
}

void CGameCtnBlockInfo::OnNodLoaded(void) {
    for (int family : {GroundMobilFamily, AirMobilFamily}) {
        for (unsigned long variant = 0u;; ++variant) {
            CFastBuffer<CSceneMobil *> *mobils =
                    GetMobilBuffer(family, variant);
            if (mobils == nullptr) {
                break;
            }
            for (CSceneMobil *mobil : *mobils) {
                if (mobil != nullptr) {
                    SetDefaultFlagsOnMobil(mobil);
                }
            }
        }
    }
    UpdateSize();

    const char *name = identifier.id.GetString();
    if (name == nullptr || usesCustomSize_ ||
        blockType_ < EBlockType::Classic || blockType_ == EBlockType::Clip) {
        raceRole_ = BlockRaceRole::None;
    } else if (std::strstr(name, "StartFinishLine") != nullptr) {
        raceRole_ = BlockRaceRole::StartFinishLine;
    } else if (std::strstr(name, "Finish") != nullptr) {
        raceRole_ = BlockRaceRole::FinishLine;
    } else if (std::strstr(name, "StartLine") != nullptr) {
        raceRole_ = BlockRaceRole::StartLine;
    } else if (std::strstr(name, "Checkpoint") != nullptr ||
               std::strstr(name, "CheckPoint") != nullptr) {
        raceRole_ = BlockRaceRole::Checkpoint;
    } else {
        raceRole_ = BlockRaceRole::None;
    }
}

void CGameCtnBlockInfoPylon::OnNodLoaded(void) {
    CGameCtnBlockInfo::OnNodLoaded();
}

CSceneMobil *CGameCtnBlockInfoPylon::FirstSourceMobil(void) const {
    return firstSourceMobil_.Get();
}

CSceneMobil *CGameCtnBlockInfoPylon::MiddleSourceMobil(void) const {
    return middleSourceMobil_.Get();
}

CSceneMobil *CGameCtnBlockInfoPylon::LastSourceMobil(void) const {
    return lastSourceMobil_.Get();
}

void CGameCtnBlockInfoPylon::SetSourceMobils(
        CSceneMobil *first,
        CSceneMobil *middle,
        CSceneMobil *last) {
    firstSourceMobil_ = first;
    middleSourceMobil_ = middle;
    lastSourceMobil_ = last;
}

int CGameCtnBlockInfoClip::IsClipCompatible(
        CGameCtnBlockInfoClip *other) {
    return other != nullptr &&
           compatibleClipId_ == other->compatibleClipId_;
}

BlockInfoAssetHandle CGameCtnBlockInfoClip::SourceAsset(void) const {
    return sourceAsset_;
}

void CGameCtnBlockInfoClip::SetSourceAsset(BlockInfoAssetHandle asset) {
    sourceAsset_ = asset;
}

void CGameCtnBlockInfoClip::SetCompatibleClipId(const CMwId &id) {
    compatibleClipId_ = id;
}

CSceneMobil *CGameCtnBlockInfo::BuildBlockMobil(
        int isGround,
        unsigned long variantIndex,
        unsigned long mobilIndex,
        int finalArg) {
    (void)finalArg;
    CSceneMobil *sourceMobil = GetMobil(isGround, variantIndex, mobilIndex);
    if (sourceMobil == nullptr) {
        return nullptr;
    }
    CSceneMobil *cloneMobil = sourceMobil->CreateModelInstance();
    CPlugTree *tree = cloneMobil->GetTree();
    if (tree != nullptr) {
        tree->SetModelInstance(true);
    }
    return cloneMobil;
}

void CGameCtnBlockInfo::SetHelperTreeFlags(
        CPlugTree *tree,
        int finalArg) {
    tree->SetShadowCaster(finalArg != 0);
    tree->SetIsVisible(tree->State().pickableVisual);
}

void CGameCtnBlockInfo::InstallHelperTree(
        CSceneMobil *targetMobil,
        CSceneMobil *sourceMobil,
        const CMwId *helperTreeId,
        int finalArg) {
    CPlugTree *sourceTree = sourceMobil->GetTree();
    CPlugTree *duplicate = sourceTree->DuplicateRecursive();
    CPlugTree *targetTree = targetMobil->FindOrAddNewTreeWithId(*helperTreeId);
    targetTree->AddChild(duplicate);
    SetHelperTreeFlags(targetTree, finalArg);
}

void CGameCtnBlockInfo::UpdateBlockHelperMobil(
        CSceneMobil *targetMobil,
        int isGround,
        int finalArg) {
    CSceneMobil *familySource = ((isGround) != 0
            ? groundHelperMobilSource_.Get()
            : airHelperMobilSource_.Get());
    if (familySource != nullptr) {
        InstallHelperTree(
                targetMobil,
                familySource,
                ((0)
            ? &CGameCtnBlockInfoHelperConstructionModeTreeLocalId
            : &CGameCtnBlockInfoHelperTreeLocalId),
                finalArg);
    }

    CSceneMobil *commonSource = commonHelperMobilSource_.Get();
    if (commonSource != nullptr) {
        InstallHelperTree(
                targetMobil,
                commonSource,
                ((1)
            ? &CGameCtnBlockInfoHelperConstructionModeTreeLocalId
            : &CGameCtnBlockInfoHelperTreeLocalId),
                finalArg);
    }

    CPlugTree *targetRoot = targetMobil->GetTree();
    if (targetRoot != nullptr) {
        targetRoot->UpdateBoundingBox(0);
    }
}


CSceneMobil *CGameCtnBlockInfo::BuildBlockHelperMobil(
        int isGround,
        int finalArg) {
    CSceneMobil *helperMobil = nullptr;
    CSceneMobil *familyHelperSource = isGround != 0
            ? groundHelperMobilSource_.Get()
            : airHelperMobilSource_.Get();
    CSceneMobil *commonHelperSource = commonHelperMobilSource_.Get();
    if (familyHelperSource == nullptr && commonHelperSource == nullptr) {
        return nullptr;
    }

    helperMobil = new CSceneMobil();
    UpdateBlockHelperMobil(helperMobil, isGround, finalArg);
    return helperMobil;
}
