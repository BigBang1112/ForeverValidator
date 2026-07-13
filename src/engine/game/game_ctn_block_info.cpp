#include "engine/game/game_ctn_block_info.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>

#include "engine/game/game_ctn_utils.h"
#include "engine/core/gm_func.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/scene/plug_solid.h"
#include "engine/rendering/plug_tree.h"
CMwId CGameCtnBlockUnitInfo::GetReplacementId(void) {
    return replacementId;
}



CMwId CGameCtnBlockUnitInfo::GetTerrainModifierId(void) {
    return terrainModifierId;
}

int CGameCtnBlockUnitInfo::HasTerrainModifierId(void) const {
    return !terrainModifierId.IsInvalid();
}


CMwId CGameCtnBlockUnitInfo::GetSurface(void) {
    return surfaceId;
}


void CGameCtnBlockInfoPylon::UpdateGroundMobilArray(unsigned long count) {
    ResetMobilVariants(CGameCtnBlockInfo::GroundMobilFamily, count);
}



CGameCtnBlockInfo::MobilStorage::MobilVariants &
CGameCtnBlockInfo::MobilStorage::Family(int family) {
    return family != 0 ? ground_ : air_;
}

const CGameCtnBlockInfo::MobilStorage::MobilVariants &
CGameCtnBlockInfo::MobilStorage::Family(int family) const {
    return family != 0 ? ground_ : air_;
}

CGameCtnBlockInfo::MobilStorage::MobilVariant *
CGameCtnBlockInfo::MobilStorage::Find(
        int family,
        unsigned long variantIndex) {
    MobilVariants &variants = Family(family);
    if (variantIndex >= variants.size()) {
        return nullptr;
    }
    return &variants[static_cast<std::size_t>(variantIndex)];
}

const CGameCtnBlockInfo::MobilStorage::MobilVariant *
CGameCtnBlockInfo::MobilStorage::Find(
        int family,
        unsigned long variantIndex) const {
    const MobilVariants &variants = Family(family);
    if (variantIndex >= variants.size()) {
        return nullptr;
    }
    return &variants[static_cast<std::size_t>(variantIndex)];
}

CGameCtnBlockInfo::MobilStorage::MobilVariant &
CGameCtnBlockInfo::MobilStorage::Require(
        int family,
        unsigned long variantIndex) {
    return Family(family)[static_cast<std::size_t>(variantIndex)];
}

bool CGameCtnBlockInfo::MobilStorage::HasAny(int family) const {
    for (const MobilVariant &variant : Family(family)) {
        if (!variant.empty()) {
            return true;
        }
    }
    return false;
}

void CGameCtnBlockInfo::MobilStorage::Reset(
        int family,
        unsigned long variantCount) {
    MobilVariants &variants = Family(family);
    ClearFamily(variants);
    variants.resize(static_cast<std::size_t>(variantCount));
}

void CGameCtnBlockInfo::ConfigureDefaultMobilItem(CHmsItem &item) const {
    item.SetLightEmitter(1);
    item.SetIsCollisionStatic(1);
    item.SetIsVisionStatic(1);
    item.SetCollisionGroup(static_cast<CHmsItem::ECollisionGroup>(
            DefaultMobilCollisionGroup));
    item.SetCountShadowTexCasted(0u, 1);
    item.SetShadowCasterGroupMask(DefaultMobilShadowCasterGroupMask);
}

void CGameCtnBlockInfo::ConfigureDefaultMobil(CSceneMobil &mobil) const {
    ConfigureDefaultMobilItem(*mobil.HmsItem());
}

CFastBuffer<CSceneMobil *> *CGameCtnBlockInfo::GetMobilBuffer(
        int family,
        unsigned long variantIndex) {
    const MobilVariant *variant = mobilVariants_.Find(family, variantIndex);
    if (variant == nullptr) {
        return nullptr;
    }
    auto &familyViews = mobilBufferViews_[family != 0 ? 1u : 0u];
    if (familyViews.size() <= variantIndex) {
        familyViews.resize(static_cast<std::size_t>(variantIndex + 1u));
    }
    CFastBuffer<CSceneMobil *> &view =
            familyViews[static_cast<std::size_t>(variantIndex)];
    view.clear();
    view.reserve(variant->size());
    for (const CMwNodRef<CSceneMobil> &mobil : *variant) {
        view.push_back(mobil.Get());
    }
    return &view;
}

void CGameCtnBlockInfo::ResetMobilVariants(
        int family,
        unsigned long variantCount) {
    mobilVariants_.Reset(family, variantCount);
    auto &familyViews = mobilBufferViews_[family != 0 ? 1u : 0u];
    familyViews.clear();
    familyViews.resize(static_cast<std::size_t>(variantCount));
}





const std::vector<std::unique_ptr<CGameCtnBlockUnitInfo>> &
CGameCtnBlockInfo::BlockUnitInfos(int isGround) const {
    return isGround != 0 ? groundBlockUnitInfos_ : airBlockUnitInfos_;
}

unsigned long CGameCtnBlockInfo::BlockUnitInfoCount(int isGround) const {
    return BlockUnitInfos(isGround).size();
}

CGameCtnBlockUnitInfo *CGameCtnBlockInfo::BlockUnitInfoAt(
        unsigned long index,
        int isGround) const {
    return BlockUnitInfos(isGround)[index].get();
}

CGameCtnBlockUnitInfo *CGameCtnBlockInfo::FirstBlockUnitInfo(int isGround) const {
    return BlockUnitInfoAt(0u, isGround);
}

CGameCtnBlockUnitInfo *CGameCtnBlockInfo::TerrainModifierUnitAt(
        int isGround,
        unsigned long index) const {
    return BlockUnitInfoAt(index, isGround);
}

CGameCtnBlockUnitInfo *CGameCtnBlockInfo::ReplacementCandidateAt(
        int isGround,
        unsigned long index) const {
    return BlockUnitInfoAt(index, isGround);
}

unsigned long CGameCtnBlockInfo::UnitMaxLayer(int isGround) const {
    unsigned long maxOffsetY = 0u;
    for (const std::unique_ptr<CGameCtnBlockUnitInfo> &unit :
         BlockUnitInfos(isGround)) {
        const unsigned long offsetY = unit->UnitLayer();
        if (offsetY > maxOffsetY) {
            maxOffsetY = offsetY;
        }
    }
    return maxOffsetY;
}

int CGameCtnBlockInfo::UnitFamilyHasAnyUnderground(int isGround) const {
    for (const std::unique_ptr<CGameCtnBlockUnitInfo> &unit :
         BlockUnitInfos(isGround)) {
        if (unit->IsUndergroundUnit()) {
            return 1;
        }
    }
    return 0;
}

int CGameCtnBlockInfo::UnitFamilyAllUnderground(int isGround) const {
    const std::vector<std::unique_ptr<CGameCtnBlockUnitInfo>> &units =
            BlockUnitInfos(isGround);
    if (units.empty()) {
        return 0;
    }
    for (const std::unique_ptr<CGameCtnBlockUnitInfo> &unit : units) {
        if (!unit->IsUndergroundUnit()) {
            return 0;
        }
    }
    return 1;
}

const GmNat3 *CGameCtnBlockInfo::UnitFamilySize(int isGround) const {
    return isGround != 0 ? &groundSize_ : &airSize_;
}

GmNat3 CGameCtnBlockInfo::RotatedUnitOffset(
        const CGameCtnBlockUnitInfo &unit,
        ECardinalDir direction,
        int isGround) const {
    const uint32_t offX = unit.UnitOffsetX();
    const uint32_t offZ = unit.UnitOffsetZ();
    GmNat3 offset{offX, unit.UnitYForBlockType(blockType_), offZ};

    const GmNat3 *dims = UnitFamilySize(isGround);
    uint32_t dimX = dims->x;
    uint32_t dimZ = dims->z;
    if (usesCustomSize_ && customSize_.x != 0u && customSize_.z != 0u) {
        dimX = customSize_.x;
        dimZ = customSize_.z;
    }

    switch (direction) {
    case ECardinalDir::North:
        break;
    case ECardinalDir::West:
        offset.z = offX;
        offset.x = dimZ - offZ - 1u;
        break;
    case ECardinalDir::South:
        offset.x = dimX - offX - 1u;
        offset.z = dimZ - offZ - 1u;
        break;
    case ECardinalDir::East:
        offset.x = offZ;
        offset.z = dimX - offX - 1u;
        break;
    default:
        break;
    }
    return offset;
}


void CGameCtnBlockInfo::AssignSurfaceToBlockUnits(int isGround) {
    for (const std::unique_ptr<CGameCtnBlockUnitInfo> &unit :
         BlockUnitInfos(isGround)) {
        unit->CopySurfaceIdFrom(blockUnitSurfaceId_);
    }
}

void CGameCtnBlockInfo::ConfigurePlacement(
        EBlockType type,
        bool usesCustomSize,
        BlockPlacementSource source) {
    blockType_ = type;
    usesCustomSize_ = usesCustomSize;
    defaultPlacementSource_ = source;
}

EBlockType CGameCtnBlockInfo::Type(void) const {
    return blockType_;
}

bool CGameCtnBlockInfo::UsesCustomSize(void) const {
    return usesCustomSize_;
}

BlockPlacementSource CGameCtnBlockInfo::DefaultPlacementSource(void) const {
    return defaultPlacementSource_;
}

BlockInterfaceKind CGameCtnBlockInfo::InterfaceKind(void) const {
    return interfaceKind_;
}

const GmNat3 &CGameCtnBlockInfo::SizeForMobilFamily(bool ground) const {
    return ground ? groundSize_ : airSize_;
}

const std::optional<GmIso4> &
CGameCtnBlockInfo::SpawnLocationForMobilFamily(bool ground) const {
    return ground ? groundMobilLocation_ : airMobilLocation_;
}

void CGameCtnBlockInfo::SetMobilFamilySize(
        bool ground,
        const GmNat3 &size) {
    (ground ? groundSize_ : airSize_) = size;
}

void CGameCtnBlockInfo::SetSpawnLocationForMobilFamily(
        bool ground,
        std::optional<GmIso4> location) {
    (ground ? groundMobilLocation_ : airMobilLocation_) =
            std::move(location);
}

void CGameCtnBlockInfo::AddBlockUnitInfo(
        bool ground,
        std::unique_ptr<CGameCtnBlockUnitInfo> unit) {
    MutableBlockUnitInfos(ground ? 1 : 0).push_back(std::move(unit));
}

bool CGameCtnBlockInfo::HasBlockUnitInfos(bool ground) const {
    return !BlockUnitInfos(ground ? 1 : 0).empty();
}

CSceneMobil *CGameCtnBlockInfo::FamilyHelperMobilSource(bool ground) const {
    return ground ? groundHelperMobilSource_.Get()
                  : airHelperMobilSource_.Get();
}

CSceneMobil *CGameCtnBlockInfo::CommonHelperMobilSource(void) const {
    return commonHelperMobilSource_.Get();
}

void CGameCtnBlockInfo::SetFamilyHelperMobilSource(
        bool ground,
        CSceneMobil *mobil) {
    (ground ? groundHelperMobilSource_ : airHelperMobilSource_) = mobil;
}

void CGameCtnBlockInfo::SetCommonHelperMobilSource(CSceneMobil *mobil) {
    commonHelperMobilSource_ = mobil;
}

void CGameCtnBlockInfo::AddSourceMobil(CSceneMobil *mobil) {
    sourceMobils_.emplace_back(mobil);
}

const std::vector<CMwNodRef<CSceneMobil>> &
CGameCtnBlockInfo::SourceMobils(void) const {
    return sourceMobils_;
}

BlockRaceRole CGameCtnBlockInfo::RaceRole(void) const {
    return raceRole_;
}

bool CGameCtnBlockInfo::RespawnUsesCurrentTransform(void) const {
    return respawnUsesCurrentTransform_;
}

void CGameCtnBlockInfo::SetRaceRole(BlockRaceRole role) {
    raceRole_ = role;
}

void CGameCtnBlockInfo::SetRespawnUsesCurrentTransform(bool enabled) {
    respawnUsesCurrentTransform_ = enabled;
}



int CGameCtnBlockInfo::IsPartUnderground(
        int isGround) {
    return UnitFamilyHasAnyUnderground(isGround);
}




int CGameCtnBlockInfo::IsAllUnderground(
        int isGround) {
    return UnitFamilyAllUnderground(isGround);
}





void CGameCtnBlockUnitInfo::InitializeUnitFields(
        const GmNat3 &unitOffset,
        uint32_t helperMaskValue,
        uint32_t junctionMaskValue,
        CGameCtnBlockInfo *owner) {
    offset = unitOffset;
    helperMask = helperMaskValue;
    junctionMask = junctionMaskValue;
    underground = false;
    blockInfo = owner;
    MarkTerrainModifierInvalid();
}

void CGameCtnBlockUnitInfo::InitializeDefaultUnitFields(void) {
    InitializeUnitFields({0u, 0u, 0u}, 0u, 0u, nullptr);
}

const GmNat3 &CGameCtnBlockUnitInfo::UnitOffset(void) const {
    return offset;
}

uint32_t CGameCtnBlockUnitInfo::UnitOffsetX(void) const {
    return UnitOffset().x;
}

uint32_t CGameCtnBlockUnitInfo::UnitOffsetY(void) const {
    return UnitOffset().y;
}

uint32_t CGameCtnBlockUnitInfo::UnitOffsetZ(void) const {
    return UnitOffset().z;
}

uint32_t CGameCtnBlockUnitInfo::UnitLayer(void) const {
    return UnitOffsetY();
}

uint32_t CGameCtnBlockUnitInfo::UnitYForBlockType(EBlockType blockType) const {
    return blockType <= EBlockType::Frontier ? 0u : UnitOffsetY();
}

int CGameCtnBlockUnitInfo::IsUndergroundUnit(void) const {
    return underground;
}

uint32_t CGameCtnBlockUnitInfo::JunctionMask(void) const {
    return junctionMask;
}

uint32_t CGameCtnBlockUnitInfo::HelperMask(void) const {
    return helperMask;
}

CGameCtnBlockInfo *CGameCtnBlockUnitInfo::OwnerBlockInfo(void) const {
    return blockInfo;
}

void CGameCtnBlockUnitInfo::MarkTerrainModifierInvalid(void) {
    terrainModifierId.SetInvalid();
}

void CGameCtnBlockUnitInfo::CopySurfaceIdFrom(const CMwId &id) {
    surfaceId = id;
}

CGameCtnBlockInfoClip *CGameCtnBlockUnitInfo::JunctionAt(
        ECardinalDir direction) const {
    return junctions[static_cast<size_t>(direction)].Get();
}

const CMwId &CGameCtnBlockUnitInfo::SurfaceId(void) const {
    return surfaceId;
}

const CMwId &CGameCtnBlockUnitInfo::TerrainModifierId(void) const {
    return terrainModifierId;
}

const char *CGameCtnBlockUnitInfo::SurfaceIdName(void) const {
    return surfaceId.GetString();
}

void CGameCtnBlockUnitInfo::SetUnderground(bool value) {
    underground = value;
}

void CGameCtnBlockUnitInfo::SetSurfaceIdName(const char *name) {
    surfaceId.SetLocalName(name);
}

void CGameCtnBlockUnitInfo::SetTerrainModifierId(const CMwId &id) {
    terrainModifierId = id;
}

void CGameCtnBlockUnitInfo::SetJunction(
        ECardinalDir direction,
        CGameCtnBlockInfoClip *junction) {
    junctions[static_cast<std::size_t>(direction)] = junction;
}


CGameCtnBlockUnitInfo::CGameCtnBlockUnitInfo(void) {
    InitializeDefaultUnitFields();
}




CGameCtnBlockUnitInfo::~CGameCtnBlockUnitInfo(void) = default;




CMwNod * CGameCtnBlockUnitInfo::MwNewCGameCtnBlockUnitInfo(void) {
    return new CGameCtnBlockUnitInfo();
}


CGameCtnBlockInfoClip * CGameCtnBlockUnitInfo::GetRotatedJunction(
        ECardinalDir direction,
        ECardinalDir rotation) {
    return JunctionAt(CGameCtnUtils::SubDirs(direction, rotation));
}



CGameCtnBlockUnitInfo * CGameCtnBlockUnitInfo::GetReplacementBlockUnitInfo(int isGround) {
    CGameCtnBlockInfo *owner = OwnerBlockInfo();
    if (owner == nullptr) {
        return nullptr;
    }

    const uint32_t count = owner->GetNbBlockUnitInfos(isGround);
    if (count == 0u) {
        return nullptr;
    }

    for (uint32_t index = 0u; index < count; ++index) {
        CGameCtnBlockUnitInfo *candidate = owner->ReplacementCandidateAt(isGround, index);
        if (candidate->replacementId == replacementId) {
            return candidate;
        }
    }
    return nullptr;
}



CGameCtnBlockUnitInfo::CGameCtnBlockUnitInfo(
        GmNat3 offset,
        unsigned long helperMask,
        unsigned long junctionMask,
        CGameCtnBlockInfoClip *northJunction,
        CGameCtnBlockInfoClip *westJunction,
        CGameCtnBlockInfoClip *southJunction,
        CGameCtnBlockInfoClip *eastJunction,
        CGameCtnBlockInfo *blockInfo) {
    InitializeUnitFields(offset, helperMask, junctionMask, blockInfo);
    SetJunction(ECardinalDir::North, northJunction);
    SetJunction(ECardinalDir::West, westJunction);
    SetJunction(ECardinalDir::South, southJunction);
    SetJunction(ECardinalDir::East, eastJunction);
}




void CGameCtnBlockInfo::AddBlock(
        GmNat3 offset,
        int helperMask,
        unsigned long junctionMask,
        int isGround) {
    MutableBlockUnitInfos(isGround).push_back(
            std::make_unique<CGameCtnBlockUnitInfo>(
                    offset,
                    static_cast<unsigned long>(helperMask),
                    junctionMask,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr));
}



unsigned long CGameCtnBlockInfo::GetHeight(int isGround) {
    return UnitMaxLayer(isGround) + 1u;
}




void CGameCtnBlockInfo::RemoveGroundBlocks(void) {
    groundBlockUnitInfos_.clear();
}


void CGameCtnBlockInfo::RemoveAirBlocks(void) {
    airBlockUnitInfos_.clear();
}


unsigned long CGameCtnBlockInfo::GetNbBlockUnitInfos(int isGround) {
    return BlockUnitInfoCount(isGround);
}



CGameCtnBlockUnitInfo * CGameCtnBlockInfo::GetBlockUnitInfo(
        unsigned long blockUnitIndex,
        int isGround) {
    return BlockUnitInfoAt(blockUnitIndex, isGround);
}



int CGameCtnBlockInfo::IsTerrainModifier(
        int isGround) {
    const unsigned long count = BlockUnitInfoCount(isGround);
    for (unsigned long index = 0u; index < count; ++index) {
        if (TerrainModifierUnitAt(isGround, index)->HasTerrainModifierId()) {
            return 1u;
        }
    }
    return 0u;
}





GmNat3 CGameCtnBlockInfo::GetRotatedOffset(
        unsigned long blockUnitIndex,
        ECardinalDir direction,
        int isGround) {
    return RotatedUnitOffset(
            *GetBlockUnitInfo(blockUnitIndex, isGround),
            direction,
            isGround);
}



int CGameCtnBlockInfo::CanPlaceOnGround(void) {
    return mobilVariants_.HasAny(GroundMobilFamily) ? 1 : 0;
}



int CGameCtnBlockInfo::CanPlaceInAir(void) {
    return mobilVariants_.HasAny(AirMobilFamily) ? 1 : 0;
}



void CGameCtnBlockInfo::SetAllBlockUnitSurfaces(void) {
    AssignSurfaceToBlockUnits(1);
    AssignSurfaceToBlockUnits(0);
}



void CGameCtnBlockInfo::AddMobil(
        int family,
        unsigned long variantIndex,
        CSceneMobil *mobil) {
    MobilVariant &variant = mobilVariants_.Require(family, variantIndex);
    CMwNodRef<CSceneMobil> retainedMobil(mobil);
    variant.push_back(std::move(retainedMobil));
}




void CGameCtnBlockInfo::SetDefaultFlagsOnMobil(CSceneMobil *mobil) {
    ConfigureDefaultMobil(*mobil);
}



void CGameCtnBlockInfo::SetMobil(
        unsigned long mobilIndex,
        int family,
        unsigned long variantIndex,
        CSceneMobil *mobil) {
    MobilVariant &variant = mobilVariants_.Require(family, variantIndex);
    CMwNodRef<CSceneMobil> retainedMobil(mobil);
    variant[static_cast<std::size_t>(mobilIndex)] =
            std::move(retainedMobil);
}





CSceneMobil * CGameCtnBlockInfo::CreateMobilFromSolid(CPlugSolid *solid) {
    CSceneMobil *mobil = new CSceneMobil();
    mobil->SetSolid(solid->CreateModelInstance());
    SetDefaultFlagsOnMobil(mobil);
    return mobil;
}





unsigned long CGameCtnBlockInfo::GetVariantIndex(
        int family,
        unsigned long variantIndex) {
    const MobilVariant *variant = mobilVariants_.Find(family, variantIndex);
    if (variant == nullptr || variant->empty() ||
        variant->size() <= 1u) {
        return 0u;
    }
    return GmFunc::RandNat(
            0u,
            static_cast<unsigned long>(variant->size() - 1u));
}





CSceneMobil * CGameCtnBlockInfo::GetMobil(
        int family,
        unsigned long variantIndex,
        unsigned long mobilIndex) {
    const MobilVariant *variant = mobilVariants_.Find(family, variantIndex);
    if (variant == nullptr || variant->empty()) {
        return nullptr;
    }
    const std::size_t selectedIndex = mobilIndex < variant->size()
            ? static_cast<std::size_t>(mobilIndex)
            : 0u;
    return (*variant)[selectedIndex].Get();
}




void CGameCtnBlockInfo::RemoveMobil(
        int family,
        unsigned long variantIndex,
        unsigned long mobilIndex) {
    MobilVariant &variant = mobilVariants_.Require(family, variantIndex);
    const auto position =
            variant.begin() +
            static_cast<MobilVariant::difference_type>(mobilIndex);
    CMwNodRef<CSceneMobil> removedMobil(std::move(*position));
    variant.erase(position);
    removedMobil.MwSetNod(nullptr);
}
