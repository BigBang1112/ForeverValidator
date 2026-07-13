#include "engine/game/game_ctn_block_unit.h"

#include "engine/game/game_ctn_block.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/game/game_ctn_field_unit.h"
#include "engine/game/game_ctn_utils.h"
void CGameCtnBlockUnit::ConnectToField(
        CGameCtnBlock *blockValue,
        CGameCtnBlockUnitInfo *unitInfoValue,
        CGameCtnFieldUnit *fieldUnitValue,
        const GmNat3 &offsetValue) {
    if (fieldUnit_ != fieldUnitValue) {
        CGameCtnFieldUnit *previousFieldUnit = fieldUnit_;
        fieldUnit_ = nullptr;
        if (previousFieldUnit != nullptr &&
            previousFieldUnit->blockUnit_ == this) {
            previousFieldUnit->blockUnit_ = nullptr;
        }
    }
    block_ = blockValue;
    blockUnitInfo_ = unitInfoValue;
    fieldUnit_ = fieldUnitValue;
    localOffset_ = offsetValue;
    if (fieldUnit_ != nullptr && fieldUnit_->blockUnit_ != this) {
        fieldUnit_->SetBlockUnitRef(this);
    }
}

void CGameCtnBlockUnit::DisconnectFromField(void) {
    CGameCtnFieldUnit *attachedFieldUnit = fieldUnit_;
    fieldUnit_ = nullptr;
    if (attachedFieldUnit != nullptr &&
        attachedFieldUnit->blockUnit_ == this) {
        attachedFieldUnit->blockUnit_ = nullptr;
    }
    block_ = nullptr;
    blockUnitInfo_ = nullptr;
}

CGameCtnBlock *CGameCtnBlockUnit::BlockRef(void) const {
    return block_;
}

CGameCtnBlockUnitInfo *CGameCtnBlockUnit::InfoRef(void) const {
    return blockUnitInfo_;
}

const GmNat3 &CGameCtnBlockUnit::LocalOffset(void) const {
    return localOffset_;
}

uint32_t CGameCtnBlockUnit::RotatedJunctionMask(void) const {
    return junctionMask_;
}

uint32_t CGameCtnBlockUnit::RotatedHelperMask(void) const {
    return helperMask_;
}

void CGameCtnBlockUnit::RotateMasksForBlockDirection(void) {
    const uint32_t direction =
            static_cast<uint32_t>(BlockRef()->Direction());
    for (uint32_t index = 0u; index < 2u * direction; index++) {
        junctionMask_ = CGameCtnUtils::CycleLeft8Bits(junctionMask_);
        helperMask_ = CGameCtnUtils::CycleLeft8Bits(helperMask_);
    }
}
CGameCtnBlockInfoClip *CGameCtnBlockUnit::JunctionAt(
        ECardinalDir direction) const {
    return junctions_[static_cast<size_t>(direction)];
}

void CGameCtnBlockUnit::CopyRotatedJunctionsFrom(
        const CGameCtnBlockUnitInfo &unitInfo,
        ECardinalDir blockDirection) {
    for (size_t index = 0u; index < junctions_.size(); ++index) {
        const ECardinalDir sourceDirection = static_cast<ECardinalDir>(index);
        const ECardinalDir rotatedDirection = CGameCtnUtils::AddDirs(
                sourceDirection,
                blockDirection);
        junctions_[static_cast<size_t>(rotatedDirection)] =
                unitInfo.JunctionAt(sourceDirection);
    }
}


CGameCtnBlockUnit::CGameCtnBlockUnit(void) = default;





CGameCtnBlockUnit::CGameCtnBlockUnit(
        CGameCtnBlock *block,
        CGameCtnBlockUnitInfo *blockUnitInfo,
        CGameCtnFieldUnit *fieldUnit,
        GmNat3 offset,
        unsigned long mask,
        unsigned long helper) {
    ConnectToField(block, blockUnitInfo, fieldUnit, offset);

    CopyRotatedJunctionsFrom(
            *blockUnitInfo,
            block->Direction());

    junctionMask_ = mask;
    helperMask_ = helper;
    RotateMasksForBlockDirection();
}




CGameCtnBlockUnit::CGameCtnBlockUnit(
        CGameCtnBlockUnit *source,
        CGameCtnBlock *block) {
    ConnectToField(block, source->InfoRef(), nullptr, source->LocalOffset());
    junctionMask_ = source->RotatedJunctionMask();
    helperMask_ = source->RotatedHelperMask();
    junctions_ = source->junctions_;
}



CGameCtnBlockUnit::~CGameCtnBlockUnit(void) {
    DisconnectFromField();
}

GmNat3 CGameCtnBlockUnit::GetCoord(void) {
    CGameCtnBlock *block = BlockRef();
    const GmNat3 &offset = LocalOffset();
    const GmNat3 &base = block->BaseCoord();
    GmNat3 coord{
            offset.x + base.x,
            offset.y + base.y,
            offset.z + base.z,
    };

    const EBlockType blockType = block->GetType();
    if (blockType == EBlockType::Flat ||
        blockType == EBlockType::Frontier) {
        coord.y -= base.y;
    }
    return coord;
}


CMwNod * CGameCtnBlockUnit::MwNewCGameCtnBlockUnit(void) {
    return new CGameCtnBlockUnit();
}
