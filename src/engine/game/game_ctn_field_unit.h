#pragma once

#include "engine/core/gm_types.h"
class CGameCtnBlock;
class CGameCtnBlockUnit;
class CGameCtnBlockUnitInfo;

class CGameCtnFieldUnit {
public:
    explicit CGameCtnFieldUnit(ETerrain terrain);
    virtual ~CGameCtnFieldUnit(void);

    CGameCtnBlockUnit *BlockUnitRef(void) const;
    CGameCtnBlockUnitInfo *BlockUnitInfoRef(void) const;
    CGameCtnBlock *BlockRef(void) const;
    ETerrain Terrain(void) const;
    void SetTerrain(ETerrain terrain);

private:
    friend class CGameCtnBlockUnit;

    void SetBlockUnitRef(CGameCtnBlockUnit *unit);
    void ClearBlockUnitRef(void);

    CGameCtnBlockUnit *blockUnit_ = nullptr;
    ETerrain terrain_ = ETerrain::Air;
};
