#include "engine/game/game_ctn_block_info.h"
#include "engine/game/game_ctn_zone.h"
CGameCtnZone::CGameCtnZone(void) = default;

CGameCtnZone::~CGameCtnZone(void) = default;

CGameCtnBlockInfo *CGameCtnZone::GetZoneInfo(void) {
    return nullptr;
}

CMwNod *CGameCtnZone::MwNewCGameCtnZone(void) {
    return new CGameCtnZone();
}

CGameCtnZoneFlat::CGameCtnZoneFlat(void) = default;

CGameCtnZoneFlat::~CGameCtnZoneFlat(void) = default;

CGameCtnBlockInfo *CGameCtnZoneFlat::GetZoneInfo(void) {
    return blockInfoFlat.Get();
}

CMwNod *CGameCtnZoneFlat::MwNewCGameCtnZoneFlat(void) {
    return new CGameCtnZoneFlat();
}

CGameCtnZoneFrontier::CGameCtnZoneFrontier(void) = default;

CGameCtnZoneFrontier::~CGameCtnZoneFrontier(void) = default;

CGameCtnBlockInfo *CGameCtnZoneFrontier::GetZoneInfo(void) {
    return blockInfoFrontier.Get();
}

CMwNod *CGameCtnZoneFrontier::MwNewCGameCtnZoneFrontier(void) {
    return new CGameCtnZoneFrontier();
}
