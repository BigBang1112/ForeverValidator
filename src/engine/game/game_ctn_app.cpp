#include "engine/game/game_ctn_app.h"

#include <cstddef>

#include "engine/core/binary32_math.h"
#include "engine/core/gm_types.h"
#include "engine/game/game_ctn_block.h"
#include "engine/game/game_ctn_challenge.h"
#include "engine/scene/replay_scene_placements.h"

namespace {

struct PylonSideTransform {
    float xFromNearSide;
    float zFromNearSide;
    bool xFromFarSide;
    bool zFromFarSide;
    unsigned long quarterY;
};

constexpr float PylonInset = 4.0f;
constexpr PylonSideTransform PylonTransforms[8] = {
        {PylonInset, 0.0f, true, true, 2u},
        {PylonInset, 0.0f, false, true, 0u},
        {0.0f, PylonInset, false, true, 1u},
        {0.0f, PylonInset, false, false, 3u},
        {PylonInset, 0.0f, false, false, 0u},
        {PylonInset, 0.0f, true, false, 2u},
        {0.0f, PylonInset, true, false, 3u},
        {0.0f, PylonInset, true, true, 1u},
};

float NaturalAsFloat(u32 value) {
    return Binary32::FromUnsignedInteger(value);
}

}  // namespace

CGameCtnApp::CGameCtnApp(
        ReplaySceneBlockPlacements &placements,
        float squareSize,
        float squareHeight)
        : placements_(&placements),
          squareSize_(squareSize),
          squareHeight_(squareHeight) {}

CGameCtnApp::CGameCtnApp(
        ReplaySceneBlockPlacements &placements,
        CGameCtnChallenge &challenge,
        float squareSize,
        float squareHeight)
        : placements_(&placements),
          challenge_(&challenge),
          squareSize_(squareSize),
          squareHeight_(squareHeight) {}

void CGameCtnApp::UpdateBlockMobils(void) {
    if (challenge_ == nullptr) {
        return;
    }
    while (CGameCtnBlock *block = challenge_->GetBlockFromAddList()) {
        AddBlockMobilToScene(block);
        challenge_->RemoveBlockFromAddList();
    }
    while (SPylonMobil *pylon = challenge_->GetPylonFromAddList()) {
        AddPylonMobilToScene(pylon);
        challenge_->RemovePylonFromAddList();
    }
    while (CSceneMobil *mobil = challenge_->GetMobilFromRemoveList()) {
        RemoveMobilFromScene(mobil);
        challenge_->RemoveMobilFromRemoveList();
    }
    challenge_->UpdateBlockMobils();
    challenge_->ReleaseRetiredBlockOwnersAfterMobilRemovalIf(
            [this](const CGameCtnBlock &block) {
                if (placements_ == nullptr) {
                    return true;
                }
                placements_->RemoveRetiredLogicalBlock(block);
                return !placements_->ReferencesBlock(&block);
            });
}

void CGameCtnApp::AddClipsToScene(void) {
    if (challenge_ == nullptr) {
        return;
    }

    std::size_t index = 0u;
    std::size_t remaining = challenge_->Blocks().size();
    bool sawClip = false;
    while (index < remaining) {
        const auto &blocks = challenge_->Blocks();
        CGameCtnBlock *block = index < blocks.size()
                ? blocks[index].get()
                : nullptr;
        if (block == nullptr || block->GetType() != EBlockType::Clip) {
            ++index;
            continue;
        }

        const GmNat3 coord = block->BaseCoord();
        (void)challenge_->UpdateClip(coord);
        --remaining;
        sawClip = true;
    }

    if (sawClip) {
        UpdateBlockMobils();
    }
}

void CGameCtnApp::AddBlockMobilToScene(CGameCtnBlock *block) {
    CSceneMobil *mainMobil = block != nullptr ? block->MainMobil() : nullptr;
    if (placements_ == nullptr || mainMobil == nullptr) {
        return;
    }
    if (placements_->ContainsMobil(mainMobil)) {
        return;
    }
    placements_->AppendBlockMobil(*block);
}

void CGameCtnApp::AddPylonPart(
        CSceneMobil &mobil,
        const GmIso4 &worldIso) {
    if (placements_ != nullptr) {
        placements_->AppendPylonMobil(mobil, worldIso);
    }
}

void CGameCtnApp::AddPylonMobilToScene(SPylonMobil *pylonMobil) {
    if (pylonMobil == nullptr || pylonMobil->pylonIndex >= 8u) {
        return;
    }
    CSceneMobil &first = pylonMobil->IsSingleMobil()
            ? pylonMobil->SingleMobil()
            : pylonMobil->TripleMobils().Lower();
    if (placements_ == nullptr || placements_->ContainsMobil(&first)) {
        return;
    }

    const PylonSideTransform &side =
            PylonTransforms[pylonMobil->pylonIndex];
    GmIso4 worldIso{};
    worldIso.rotation.SetRotateQuarterY(side.quarterY);
    worldIso.translation = {
            NaturalAsFloat(pylonMobil->coord.x) * squareSize_ +
                    (side.xFromFarSide ? squareSize_ - side.xFromNearSide
                                       : side.xFromNearSide),
            NaturalAsFloat(pylonMobil->coord.y) * squareHeight_,
            NaturalAsFloat(pylonMobil->coord.z) * squareSize_ +
                    (side.zFromFarSide ? squareSize_ - side.zFromNearSide
                                       : side.zFromNearSide),
    };

    AddPylonPart(first, worldIso);
    if (pylonMobil->IsSingleMobil()) {
        return;
    }
    const SPylonMobil::STripleMobils &triple =
            pylonMobil->TripleMobils();
    AddPylonPart(triple.Middle(), worldIso);
    worldIso.translation.y +=
            NaturalAsFloat(static_cast<u32>(pylonMobil->pylonHeight)) *
            squareHeight_;
    AddPylonPart(triple.Upper(), worldIso);
}

void CGameCtnApp::RemoveMobilFromScene(CSceneMobil *mobil) {
    if (placements_ != nullptr && mobil != nullptr) {
        placements_->RemoveMobilFromScene(*mobil);
    }
}
