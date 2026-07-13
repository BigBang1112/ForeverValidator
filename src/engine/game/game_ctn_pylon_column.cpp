#include "engine/game/game_ctn_pylon_column.h"
#include <utility>

CGameCtnPylonColumn::CGameCtnPylonColumn(void) = default;


CGameCtnPylonColumn::~CGameCtnPylonColumn(void) = default;





void CGameCtnPylonColumn::RemovePylon(unsigned long y) {
    (void)y;
    span.reset();
}


void CGameCtnPylonColumn::AddPylon(
        unsigned long firstY,
        unsigned long lastY) {
    span = Span{firstY, lastY};
}


void CGameCtnPylonColumn::SetMobil(CSceneMobil *mobil) {
    mobils = SingleMobilRef(mobil);
}


void CGameCtnPylonColumn::SetMobils(
        CSceneMobil *lowerMobil,
        CSceneMobil *middleMobil,
        CSceneMobil *upperMobil) {
    TripleMobils triple;
    triple[0] = lowerMobil;
    triple[1] = middleMobil;
    triple[2] = upperMobil;
    mobils = std::move(triple);
}


void CGameCtnPylonColumn::DeleteMobils(void) {
    mobils = std::monostate{};
}

CMwNod *CGameCtnPylonColumn::MwNewCGameCtnPylonColumn(void) {
    return new CGameCtnPylonColumn();
}

const std::optional<CGameCtnPylonColumn::Span> &
CGameCtnPylonColumn::PylonSpan(void) const {
    return span;
}

bool CGameCtnPylonColumn::HasPylon(void) const {
    return span.has_value();
}

CSceneMobil *CGameCtnPylonColumn::SingleMobil(void) const {
    const SingleMobilRef *single = std::get_if<SingleMobilRef>(&mobils);
    return single != nullptr ? single->Get() : nullptr;
}

CSceneMobil *CGameCtnPylonColumn::LowerMobil(void) const {
    const TripleMobils *triple = std::get_if<TripleMobils>(&mobils);
    return triple != nullptr ? (*triple)[0].Get() : nullptr;
}

CSceneMobil *CGameCtnPylonColumn::MiddleMobil(void) const {
    const TripleMobils *triple = std::get_if<TripleMobils>(&mobils);
    return triple != nullptr ? (*triple)[1].Get() : nullptr;
}

CSceneMobil *CGameCtnPylonColumn::UpperMobil(void) const {
    const TripleMobils *triple = std::get_if<TripleMobils>(&mobils);
    return triple != nullptr ? (*triple)[2].Get() : nullptr;
}
