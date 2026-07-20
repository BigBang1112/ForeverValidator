#pragma once

#include "engine/core/engine_types.h"
#include <array>

#include "engine/core/cmw_nod.h"
#include "engine/core/gm_types.h"
class CGameCtnBlock;
class CGameCtnBlockInfoClip;
class CGameCtnBlockUnitInfo;
class CGameCtnFieldUnit;

class CGameCtnBlockUnit : public CMwNod {
public:
    CGameCtnBlockUnit(void);
    CGameCtnBlockUnit(CGameCtnBlock *block,
                      CGameCtnBlockUnitInfo *blockUnitInfo,
                      CGameCtnFieldUnit *fieldUnit,
                      GmNat3 offset,
                      unsigned long mask,
                      unsigned long helper);
    CGameCtnBlockUnit(CGameCtnBlockUnit *source, CGameCtnBlock *block);
    ~CGameCtnBlockUnit(void) override;
    GmNat3 GetCoord(void);
    static CMwNod *MwNewCGameCtnBlockUnit(void);

    CGameCtnBlock *BlockRef(void) const;
    CGameCtnBlockUnitInfo *InfoRef(void) const;
    const GmNat3 &LocalOffset(void) const;
    u32 RotatedJunctionMask(void) const;
    u32 RotatedHelperMask(void) const;
    CGameCtnBlockInfoClip *JunctionAt(ECardinalDir direction) const;

private:
    friend class CGameCtnChallenge;
    friend class CGameCtnFieldUnit;

    void ConnectToField(CGameCtnBlock *block,
                        CGameCtnBlockUnitInfo *unitInfo,
                        CGameCtnFieldUnit *fieldUnit,
                        const GmNat3 &offset);
    void DisconnectFromField(void);
    void RotateMasksForBlockDirection(void);
    void CopyRotatedJunctionsFrom(const CGameCtnBlockUnitInfo &unitInfo,
                                  ECardinalDir blockDirection);

    CGameCtnBlock *block_ = nullptr;
    CGameCtnBlockUnitInfo *blockUnitInfo_ = nullptr;
    CGameCtnFieldUnit *fieldUnit_ = nullptr;
    std::array<CGameCtnBlockInfoClip *, 4> junctions_{};
    GmNat3 localOffset_{};
    u32 junctionMask_ = 0u;
    u32 helperMask_ = 0u;
};
