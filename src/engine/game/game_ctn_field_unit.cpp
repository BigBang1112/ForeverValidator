#include "engine/game/game_ctn_field_unit.h"
#include "engine/game/game_ctn_block.h"
#include "engine/game/game_ctn_block_unit.h"
#include "engine/game/game_ctn_block_info.h"
void CGameCtnFieldUnit::ClearBlockUnitRef(void) {
    CGameCtnBlockUnit *attachedUnit = blockUnit_;
    blockUnit_ = nullptr;
    if (attachedUnit != nullptr && attachedUnit->fieldUnit_ == this) {
        attachedUnit->fieldUnit_ = nullptr;
    }
}

CGameCtnBlockUnit *CGameCtnFieldUnit::BlockUnitRef(void) const {
    return blockUnit_;
}

CGameCtnBlockUnitInfo *CGameCtnFieldUnit::BlockUnitInfoRef(void) const {
    CGameCtnBlockUnit *unit = BlockUnitRef();
    return unit != nullptr ? unit->InfoRef() : nullptr;
}

CGameCtnBlock *CGameCtnFieldUnit::BlockRef(void) const {
    CGameCtnBlockUnit *unit = BlockUnitRef();
    return unit != nullptr ? unit->BlockRef() : nullptr;
}

ETerrain CGameCtnFieldUnit::Terrain(void) const {
    return terrain_;
}

void CGameCtnFieldUnit::SetTerrain(ETerrain value) {
    terrain_ = value;
}

void CGameCtnFieldUnit::SetBlockUnitRef(CGameCtnBlockUnit *unit) {
    if (blockUnit_ == unit) {
        if (unit != nullptr && unit->fieldUnit_ != this) {
            CGameCtnFieldUnit *previousFieldUnit = unit->fieldUnit_;
            unit->fieldUnit_ = this;
            if (previousFieldUnit != nullptr &&
                previousFieldUnit->blockUnit_ == unit) {
                previousFieldUnit->blockUnit_ = nullptr;
            }
        }
        return;
    }

    ClearBlockUnitRef();
    if (unit != nullptr) {
        CGameCtnFieldUnit *previousFieldUnit = unit->fieldUnit_;
        unit->fieldUnit_ = this;
        if (previousFieldUnit != nullptr &&
            previousFieldUnit->blockUnit_ == unit) {
            previousFieldUnit->blockUnit_ = nullptr;
        }
    }
    blockUnit_ = unit;
}


CGameCtnFieldUnit::CGameCtnFieldUnit(ETerrain terrain)
        : terrain_(terrain) {
}



CGameCtnFieldUnit::~CGameCtnFieldUnit(void) {
    ClearBlockUnitRef();
}
