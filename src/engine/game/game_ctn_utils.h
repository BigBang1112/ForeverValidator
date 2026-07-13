#pragma once

#include "engine/core/gm_types.h"
struct CGameCtnUtils {
    static ECardinalDir GetOpposedDir(ECardinalDir direction);
    static ECardinalDir AddDirs(ECardinalDir lhs, ECardinalDir rhs);
    static ECardinalDir SubDirs(ECardinalDir lhs, ECardinalDir rhs);
    static GmNat3 GetNeighbourCoord(GmNat3 coord, ECardinalDir direction);
    static unsigned long CycleLeft8Bits(unsigned long value);
};
