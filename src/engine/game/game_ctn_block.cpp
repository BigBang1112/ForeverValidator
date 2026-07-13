#include "engine/game/game_ctn_block.h"
#include <algorithm>
#include <utility>

#include "engine/game/control.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/game/game_ctn_block_unit.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/rendering/plug_bitmap.h"
#include "engine/rendering/plug_material.h"
#include "engine/rendering/plug_shader.h"
#include "engine/rendering/plug_tree.h"
#include "engine/scene/scene_mobil.h"
#include "engine/resources/system_fid.h"
#include "engine/resources/system_fid_parameter_types.h"
#include "engine/resources/system_fid_parameters.h"
#include "engine/core/binary32_math.h"
namespace {

float NaturalAsFloat(u32 value) {
    return Binary32::FromUnsignedInteger(value);
}

}  // namespace

CGameCtnBlockInfo *CGameCtnBlock::BlockInfoRef(void) const {
    return blockInfo.Get();
}

const SGameCtnIdentifier &CGameCtnBlock::Identifier(void) const {
    return identifier;
}

CGameCtnBlockSkin *CGameCtnBlock::Skin(void) const {
    return skin.Get();
}

CSceneMobil *CGameCtnBlock::MainMobil(void) const {
    return mobil.Get();
}

CSceneMobil *CGameCtnBlock::TriggerMobil(void) const {
    return triggerMobil.Get();
}

CSceneMobil *CGameCtnBlock::HelperMobil(void) const {
    return helperMobil.Get();
}

const std::vector<CMwNodRef<CSceneMobil>> &
CGameCtnBlock::SubMobils(void) const {
    return subMobils;
}

const std::array<CMwNodRef<CSceneMobil>, 4> &
CGameCtnBlock::ClipSourceMobils(void) const {
    return clipSourceMobils;
}

const std::vector<std::unique_ptr<CGameCtnBlockUnit>> &
CGameCtnBlock::BlockUnits(void) const {
    return blockUnits;
}

CGameCtnBlockUnit *CGameCtnBlock::BlockUnitAt(u32 index) const {
    return index < blockUnits.size() ? blockUnits[index].get() : nullptr;
}

bool CGameCtnBlock::HasBlockUnits(void) const {
    return !blockUnits.empty();
}

const GmNat3 &CGameCtnBlock::BaseCoord(void) const {
    return coord;
}

ECardinalDir CGameCtnBlock::Direction(void) const {
    return direction;
}

uint32_t CGameCtnBlock::VariantIndex(void) const {
    return placement_.VariantIndex();
}

const BlockPlacementState &CGameCtnBlock::Placement(void) const {
    return placement_;
}

int CGameCtnBlock::IsType(EBlockType type) const {
    return GetType() == type;
}

int CGameCtnBlock::HasNoMainMobil(void) const {
    return MainMobil() == nullptr;
}

void CGameCtnBlock::GetMobilLoc(
        GmIso4 &out,
        const GmNat3 &coord,
        ECardinalDir direction,
        const GmNat3 &size) {
    GmMat3 rotation{};
    out.translation.x = NaturalAsFloat(coord.x) * SquareSize;
    out.translation.y = NaturalAsFloat(coord.y) * SquareHeight;
    out.translation.z = NaturalAsFloat(coord.z) * SquareSize;

    unsigned long quarterTurn = 0u;
    switch (direction) {
    case ECardinalDir::North:
        break;
    case ECardinalDir::West:
        out.translation.x += NaturalAsFloat(size.z) * SquareSize;
        quarterTurn = 3u;
        break;
    case ECardinalDir::South:
        out.translation.x += NaturalAsFloat(size.x) * SquareSize;
        out.translation.z += NaturalAsFloat(size.z) * SquareSize;
        quarterTurn = 2u;
        break;
    case ECardinalDir::East:
        out.translation.z += NaturalAsFloat(size.x) * SquareSize;
        quarterTurn = 1u;
        break;
    }
    rotation.SetRotateQuarterY(quarterTurn);
    out.rotation = rotation;
}

void CGameCtnBlock::GetMobilLoc(GmIso4 &out) {
    out = MobilLocation();
}

GmIso4 CGameCtnBlock::MobilLocation(void) const {
    GmIso4 location{};
    if (blockInfo == nullptr) {
        location.SetIdentity();
        return location;
    }
    GetMobilLoc(location,
                coord,
                direction,
                blockInfo->SizeForMobilFamily(UsesGroundMobilSize()));
    location.translation.y += mobilVerticalOffset_;
    return location;
}

void CGameCtnBlock::GetSpawnLoc(
        GmIso4 &out,
        unsigned long spawnCount,
        unsigned long spawnIndex) {
    out = SpawnLocation(spawnCount, spawnIndex);
}

GmIso4 CGameCtnBlock::SpawnLocation(
        unsigned long spawnCount,
        unsigned long spawnIndex) const {
    GmIso4 location{};
    if (blockInfo == nullptr) {
        location.SetIdentity();
        return location;
    }
    const bool ground = UsesGroundMobilSize();
    const std::optional<GmIso4> &local =
            blockInfo->SpawnLocationForMobilFamily(ground);
    location = local.value_or(GmIso4{});
    if (!local) {
        location.SetIdentity();
    }
    location.Mult(MobilLocation());
    if (spawnCount != 0u) {
        const float centeredIndex = NaturalAsFloat(spawnIndex) -
                NaturalAsFloat(spawnCount - 1u) * 0.5f;
        const float lateral = centeredIndex * 6.0f;
        const GmVec3 lateralDirection = location.rotation.Row(GmAxis::X);
        location.translation.x += lateralDirection.x * lateral;
        location.translation.y += lateralDirection.y * lateral;
        location.translation.z += lateralDirection.z * lateral;
    }
    return location;
}

void CGameCtnBlock::CreateBlockMobil(int helperFlags, int reserved) {
    (void)reserved;
    if (blockInfo == nullptr) {
        return;
    }
    const int family = placement_.UsesGroundMobilFamily() ? 1 : 0;
    const unsigned long variant = placement_.VariantIndex();
    if (!placement_.MobilSelection()) {
        placement_.SelectMobil(blockInfo->GetVariantIndex(family, variant));
    }
    const unsigned long mobilIndex = *placement_.MobilSelection();
    CSceneMobil *mainMobil = blockInfo->BuildBlockMobil(
            family, variant, mobilIndex, helperFlags);
    CSceneMobil *helperMobil = blockInfo->BuildBlockHelperMobil(
            family, helperFlags);
    if (mainMobil == nullptr &&
        (blockInfo->Type() != EBlockType::RectAsym ||
         !blockInfo->UsesCustomSize() || variant != 0u)) {
        mainMobil = blockInfo->BuildBlockMobil(
                family == 0 ? 1 : 0,
                variant,
                mobilIndex,
                helperFlags);
    }
    SetMobilAndHelper(mainMobil, helperMobil, nullptr);
    ApplySkin();
}

int CGameCtnBlock::IsLandscape(void) const {
    return blockInfo != nullptr && blockInfo->IsLandscape();
}

void CGameCtnBlock::SetSourceAsset(
        BlockInfoAssetHandle asset,
        PlacementOrigin origin) {
    sourceAsset_ = asset;
    origin_ = origin;
}

BlockInfoAssetHandle CGameCtnBlock::SourceAsset(void) const {
    return sourceAsset_;
}

CGameCtnBlock::PlacementOrigin CGameCtnBlock::Origin(void) const {
    return origin_;
}

void CGameCtnBlock::SetMaterialRemaps(bool replacement, bool decorationSkin) {
    replacementMaterialRemap_ = replacement;
    decorationSkinMaterialRemap_ = decorationSkin;
}

bool CGameCtnBlock::UsesReplacementMaterialRemap(void) const {
    return replacementMaterialRemap_;
}

bool CGameCtnBlock::UsesDecorationSkinMaterialRemap(void) const {
    return decorationSkinMaterialRemap_;
}

bool CGameCtnBlock::UsesGroundMobilSize(void) const {
    return placement_.UsesGroundMobilFamily();
}

void CGameCtnBlock::SuppressBy(const CGameCtnBlock &block) {
    suppressingBlock_ = &block;
}

void CGameCtnBlock::ClearSuppression(void) {
    suppressingBlock_ = nullptr;
}

bool CGameCtnBlock::IsSuppressed(void) const {
    return suppressingBlock_ != nullptr;
}

const CGameCtnBlock *CGameCtnBlock::SuppressingBlock(void) const {
    return suppressingBlock_;
}

void CGameCtnBlock::SetInstanceId(InstanceId id) {
    instanceId_ = id;
}

const std::optional<CGameCtnBlock::InstanceId> &
CGameCtnBlock::BlockInstanceId(void) const {
    return instanceId_;
}

void CGameCtnBlock::SetMobilVerticalOffset(float offset) {
    mobilVerticalOffset_ = offset;
}

void CGameCtnBlock::SetTriggerMobil(CSceneMobil *newTriggerMobil) {
    if (triggerMobil != nullptr && triggerMobil->OwningBlock() == this) {
        triggerMobil->SetOwningBlock(nullptr);
    }
    triggerMobil = newTriggerMobil;
    if (triggerMobil != nullptr) {
        triggerMobil->SetOwningBlock(this);
    }
}

void CGameCtnBlock::SetClipSourceMobils(
        const std::array<CMwNodRef<CSceneMobil>, 4> &mobils) {
    clipSourceMobils = mobils;
}

void CGameCtnBlock::SetTerrainModifiedId(const CMwId &id) {
    terrainModifiedId = id;
}

CMwId CGameCtnBlock::GetTerrainModifiedId(void) {
    return terrainModifiedId;
}


CGameCtnBlock::CGameCtnBlock(void) = default;

CGameCtnBlock::CGameCtnBlock(
        CGameCtnBlockInfo *blockInfo,
        GmNat3 blockCoord,
        ECardinalDir direction,
        unsigned long variantOrSource,
        int isGround,
        int createMobilFlag,
        CGameCtnBlockSkin *skin)
        : CGameCtnBlock(
                  blockInfo,
                  blockCoord,
                  direction,
                  BlockPlacementState::Create(
                          static_cast<u32>(variantOrSource) % 64u,
                          std::nullopt,
                          isGround != 0 ? BlockMobilFamily::Ground
                                        : BlockMobilFamily::Air,
                          createMobilFlag != 0,
                          blockInfo != nullptr && blockInfo->UsesCustomSize(),
                          blockInfo != nullptr
                                  ? blockInfo->DefaultPlacementSource()
                                  : BlockPlacementSource::Primary),
                  skin) {}

CGameCtnBlock::CGameCtnBlock(
        CGameCtnBlockInfo *blockInfo,
        GmNat3 blockCoord,
        ECardinalDir direction,
        BlockPlacementState placement,
        CGameCtnBlockSkin *skin)
        : CGameCtnBlock() {
    // A placed block always has a block definition; the default constructor
    // represents the separate empty state.
    SetBlockInfo(blockInfo);
    coord = blockCoord;
    this->direction = direction;
    placement_ = std::move(placement);
    SetSkin(skin);
}

CGameCtnBlock::CGameCtnBlock(CGameCtnBlock *source)
        : CGameCtnBlock() {
    SetBlockInfo(source->blockInfo);
    identifier = source->identifier;
    coord = source->coord;
    direction = source->direction;
    placement_ = source->placement_;
    clipSourceMobils = source->clipSourceMobils;
    sourceAsset_ = source->sourceAsset_;
    origin_ = source->origin_;
    replacementMaterialRemap_ = source->replacementMaterialRemap_;
    decorationSkinMaterialRemap_ = source->decorationSkinMaterialRemap_;
    suppressingBlock_ = source->suppressingBlock_;
    instanceId_ = source->instanceId_;
    mobilVerticalOffset_ = source->mobilVerticalOffset_;

    // Terrain-modifier and mobil state are generated again for the copy.
    terrainModifiedId.SetInvalid();

    blockUnits.reserve(source->blockUnits.size());
    for (const std::unique_ptr<CGameCtnBlockUnit> &sourceUnit :
         source->blockUnits) {
        AddBlockUnit(sourceUnit != nullptr
                             ? new CGameCtnBlockUnit(sourceUnit.get(), this)
                             : nullptr);
    }

    if (source->skin != nullptr) {
        CGameCtnBlockSkin *skinCopy = new CGameCtnBlockSkin();
        skinCopy->label = source->skin->label;
        SetSkin(skinCopy);
    }
}

void CGameCtnBlock::ClearMobilOwners(void) {
    const auto clearOwner = [this](CSceneMobil *mobil) {
        if (mobil != nullptr && mobil->OwningBlock() == this) {
            mobil->SetOwningBlock(nullptr);
        }
    };
    clearOwner(mobil.Get());
    clearOwner(helperMobil.Get());
    clearOwner(triggerMobil.Get());
    for (const CMwNodRef<CSceneMobil> &subMobil : subMobils) {
        clearOwner(subMobil.Get());
    }
}

CGameCtnBlock::~CGameCtnBlock(void) {
    ClearMobilOwners();
}


const CFastStringInt &CGameCtnBlockSkin::InterfaceLabel(void) const {
    return label;
}

namespace {

const CFastStringInt *NonEmptyInterfaceLabel(
        const CGameCtnBlockSkin &skin) {
    const CFastStringInt &label = skin.InterfaceLabel();
    return !label.Empty() ? &label : nullptr;
}

void AddTemporaryInterfaceLabelParameter(
        CSystemFidParameters *fidParameters,
        CSystemFid *fid,
        const CFastString *paramName,
        const CFastStringInt *value) {
    CSystemFidParameters::SParam_Set setParam(
            fid,
            *paramName,
            *value,
            InterfaceLabelParameterAudience());
    fidParameters->AddParam(setParam);
}

void AddBlockInterfaceLabelParameter(
        CGameCtnBlock *ctnBlock,
        CSystemFidParameters *fidParameters,
        CSystemFid *bitmapFid) {
    CGameCtnBlockSkin *interfaceRoot = ctnBlock->Skin();
    if (interfaceRoot == nullptr) {
        return;
    }

    const CFastStringInt *label = NonEmptyInterfaceLabel(*interfaceRoot);
    if (label == nullptr) {
        return;
    }

    CFastString paramName("Mobils[##InterfaceRoot].Label");

    AddTemporaryInterfaceLabelParameter(
            fidParameters,
            bitmapFid,
            &paramName,
            label);
}

}  // namespace


void CGameCtnBlock::ApplySkin(void) {
    CGameCtnBlockSkin *interfaceRoot = skin;
    CSceneMobil *mobil = this->mobil;
    if (interfaceRoot == nullptr || mobil == nullptr) {
        return;
    }

    for (const CMwNodRef<CSceneObjectLink> &linkRef : mobil->Links()) {
        CSceneObjectLink *link = linkRef.Get();
        CSceneObject *linkedObject =
                link != nullptr ? link->Object() : nullptr;
        CControlBase *control =
                dynamic_cast<CControlBase *>(linkedObject);
        if (control != nullptr) {
            CControlBase::RecursiveConnect(control, interfaceRoot, 1);
        }
    }

    const BlockInterfaceKind interfaceKind = blockInfo != nullptr
            ? blockInfo->InterfaceKind()
            : BlockInterfaceKind::None;
    if (interfaceKind == BlockInterfaceKind::None) {
        return;
    }

    CMwId interfaceRootId =
            CMwId::CreateFromLocalName("InterfaceRoot");
    CMwId connectControlInterfaceRootId =
            CMwId::CreateFromLocalName("ConnectControlInterfaceRoot");

    CPlugTree::CIteratorShader shaderIterator(
            mobil->GetTree(),
            CPlugTree::CIteratorTree::Mode_IncludeRoot);

    while (shaderIterator.HasNext()) {
        CPlugTree *shaderTree = nullptr;
        CPlugShader *shader =
                shaderIterator.GetNextShader(&shaderTree);
        CPlugBitmapRenderScene3d *sceneRender =
                shader->FindScene3dBitmapRender(nullptr, nullptr);
        if (sceneRender == nullptr) {
            continue;
        }
        CSystemFid *bitmapFid = sceneRender->SourceFid();
        if (bitmapFid == nullptr) {
            continue;
        }

        CSystemFid *shaderFid = shader->fid;
        if (shaderFid == nullptr) {
            continue;
        }

        CSystemFidParameters fidParameters(shaderFid->Parameters());

        if (interfaceKind == BlockInterfaceKind::Label) {
            AddBlockInterfaceLabelParameter(
                    this,
                    &fidParameters,
                    bitmapFid);
        }

        CPlugShader *loadedShader = nullptr;
        {
            const CSystemFidParameters::Scope parameterScope(fidParameters, 1);
            CMwNod *loadedNod = nullptr;
            int loaded = shaderFid->LoadNode(loadedNod);
            loadedShader = dynamic_cast<CPlugShader *>(loadedNod);
            if (loaded != 0 &&
                loadedNod != nullptr &&
                loadedShader == nullptr) {
                loadedNod->MwAddRef();
                loadedNod->MwRelease();
                loaded = 0;
            }
            if (loadedShader != nullptr) {
                loadedShader->MwAddRef();
            }
            if (loaded == 0 && loadedShader != nullptr) {
                loadedShader->MwRelease();
                loadedShader = nullptr;
            }
        }

        if (loadedShader != nullptr) {
            shaderTree->SetShader(loadedShader);
            loadedShader->MwRelease();
        }

    }
}

EBlockType CGameCtnBlock::GetType(void) const {
    CGameCtnBlockInfo *blockInfo = this->blockInfo;
    if (blockInfo == nullptr) {
        return EBlockType::Flat;
    }
    return blockInfo->Type();
}


void CGameCtnBlock::SetSkin(CGameCtnBlockSkin *skin) {
    this->skin = skin;
}


void CGameCtnBlock::SetBlockInfo(
        CGameCtnBlockInfo *blockInfo) {
    if (blockInfo != nullptr) {
        identifier = blockInfo->identifier;
    }
    this->blockInfo = blockInfo;
}


void CGameCtnBlock::SetOldBlockInfo(
        CGameCtnBlockInfo *blockInfo) {
    identifier = blockInfo->identifier;
    this->blockInfo = blockInfo;
}


void CGameCtnBlock::ReleaseMobils(void) {
    ClearMobilOwners();
    mobil = nullptr;
    helperMobil = nullptr;
    triggerMobil = nullptr;
    blockUnits.clear();
    subMobils.clear();
    skin = nullptr;
    blockInfo = nullptr;
}


void CGameCtnBlock::SetMobilAndHelper(
        CSceneMobil *mobil,
        CSceneMobil *helperMobil,
        const CFastBuffer<CSceneMobil *> *additionalMobils) {
    ClearMobilOwners();
    this->mobil = mobil;
    this->helperMobil = helperMobil;
    triggerMobil = nullptr;
    subMobils.clear();
    if (additionalMobils != nullptr) {
        subMobils.reserve(additionalMobils->size());
        for (CSceneMobil *additionalMobil : *additionalMobils) {
            subMobils.emplace_back(additionalMobil);
        }
    }

    if (this->mobil != nullptr) {
        this->mobil->SetOwningBlock(this);

        static const CMwId checkpointId =
                CMwId::CreateFromLocalName("TriggerCheckpoint");
        static const CMwId finishId =
                CMwId::CreateFromLocalName("TriggerFinishLine");
        for (const CMwNodRef<CSceneObjectLink> &linkRef :
             this->mobil->Links()) {
            CSceneObjectLink *link = linkRef.Get();
            CSceneMobil *child = dynamic_cast<CSceneMobil *>(
                    link != nullptr ? link->Object() : nullptr);
            if (child != nullptr &&
                (child->id == checkpointId || child->id == finishId)) {
                triggerMobil = child;
            }
        }
    }

    if (triggerMobil != nullptr) {
        triggerMobil->SetOwningBlock(this);
    }
    if (this->helperMobil != nullptr) {
        this->helperMobil->SetOwningBlock(this);
    }
    for (CMwNodRef<CSceneMobil> &subMobil : subMobils) {
        if (subMobil.Get() != nullptr) {
            subMobil->SetOwningBlock(this);
        }
    }
}

void CGameCtnBlock::AllMobilsSetIsVisible(int isVisible) {
    const auto setVisible = [isVisible](CSceneMobil *mobil) {
        if (mobil != nullptr) {
            mobil->SetIsVisible(isVisible);
        }
    };

    setVisible(mobil);
    if (helperMobil != nullptr) {
        setVisible(helperMobil);
    }
    for (const CMwNodRef<CSceneMobil> &subMobil : subMobils) {
        setVisible(subMobil.Get());
    }
}


void CGameCtnBlock::AddBlockUnit(CGameCtnBlockUnit *blockUnit) {
    std::unique_ptr<CGameCtnBlockUnit> ownedBlockUnit(blockUnit);
    blockUnits.push_back(std::move(ownedBlockUnit));
}
