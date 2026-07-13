#include "engine/game/game_ctn_collection.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/game/game_ctn_zone.h"
#include <limits>
#include <utility>

void CGameCtnCollection::GetDefaultDecoration(
        SGameCtnIdentifier &outIdentifier) {
    if (defaultDecoration_) {
        outIdentifier = *defaultDecoration_;
    }
}

void CGameCtnCollection::SetDefaultDecoration(
        SGameCtnIdentifier identifier) {
    defaultDecoration_ = std::move(identifier);
}

CGameCtnCollection::CGameCtnCollection(void) = default;

CGameCtnCollection::~CGameCtnCollection(void) = default;

void CGameCtnCollection::AddZone(CGameCtnZone *zone) {
    if (zone != nullptr) {
        zones_.emplace_back(zone);
    }
}

void CGameCtnCollection::AddObsoleteZone(CGameCtnZone *zone) {
    if (zone != nullptr) {
        obsoleteZones_.emplace_back(zone);
    }
}

void CGameCtnCollection::SetDefaultZone(CGameCtnZoneFlat *zone) {
    defaultZone_.MwSetNod(zone);
}

CGameCtnZoneFlat *CGameCtnCollection::DefaultZone(void) const {
    return defaultZone_.Get();
}

CGameCtnZone *CGameCtnCollection::CurrentZone(void) const {
    return currentZone_;
}

CGameCtnZone *CGameCtnCollection::GetZone(const CMwId &id) {
    for (const CMwNodRef<CGameCtnZone> &zoneRef : zones_) {
        CGameCtnZone *zone = zoneRef.Get();
        if (zone->zoneId == id) {
            return zone;
        }
    }
    return nullptr;
}

CGameCtnZone *CGameCtnCollection::GetObsoleteZone(const CMwId &id) {
    for (const CMwNodRef<CGameCtnZone> &zoneRef : obsoleteZones_) {
        CGameCtnZone *zone = zoneRef.Get();
        if (zone->zoneId == id) {
            return zone;
        }
    }
    return nullptr;
}

unsigned long CGameCtnCollection::GetZoneIndex(const CMwId &id) {
    for (size_t index = 0u; index < zones_.size(); ++index) {
        if (zones_[index]->zoneId == id) {
            return static_cast<unsigned long>(index);
        }
    }
    return 0u;
}

CGameCtnZoneFlat *CGameCtnCollection::GetZoneFlat(const CMwId &id) {
    for (const CMwNodRef<CGameCtnZone> &zoneRef : zones_) {
        CGameCtnZone *zone = zoneRef.Get();
        CGameCtnZoneFlat *flat = dynamic_cast<CGameCtnZoneFlat *>(zone);
        if (flat != nullptr && zone->zoneId == id) {
            return flat;
        }
    }
    return nullptr;
}

CGameCtnZoneFrontier *CGameCtnCollection::GetZoneFrontier(
        const CMwId &parentZoneId,
        const CMwId &childZoneId) {
    for (const CMwNodRef<CGameCtnZone> &zoneRef : zones_) {
        CGameCtnZoneFrontier *frontier =
                dynamic_cast<CGameCtnZoneFrontier *>(zoneRef.Get());
        if (frontier != nullptr &&
            frontier->parentZoneId == parentZoneId &&
            frontier->childZoneId == childZoneId) {
            return frontier;
        }
    }
    return nullptr;
}

CGameCtnZoneFlat *CGameCtnCollection::GetBasicZone(const CMwId &id) {
    CGameCtnZone *zone = GetZone(id);
    CGameCtnZoneFrontier *frontier =
            dynamic_cast<CGameCtnZoneFrontier *>(zone);
    if (frontier != nullptr) {
        return GetZoneFlat(frontier->childZoneId);
    }
    return dynamic_cast<CGameCtnZoneFlat *>(zone);
}

CGameCtnZone *CGameCtnCollection::GetZoneFromLandBlockInfo(
        const CGameCtnBlockInfo *blockInfo) const {
    const auto findByBlockInfo =
            [blockInfo](const std::vector<CMwNodRef<CGameCtnZone>> &zones) {
                for (const CMwNodRef<CGameCtnZone> &zoneRef : zones) {
                    CGameCtnZone *zone = zoneRef.Get();
                    if (zone->GetZoneInfo() == blockInfo) {
                        return zone;
                    }
                }
                return static_cast<CGameCtnZone *>(nullptr);
            };

    CGameCtnZone *zone = findByBlockInfo(zones_);
    return zone != nullptr ? zone : findByBlockInfo(obsoleteZones_);
}

void CGameCtnCollection::SetCurrentZone(const CMwId &id) {
    currentZone_ = GetZone(id);
}

CGameCtnZoneFrontier *CGameCtnCollection::GetFrontier(const CMwId &id) {
    for (const CMwNodRef<CGameCtnZone> &zoneRef : zones_) {
        CGameCtnZoneFrontier *frontier =
                dynamic_cast<CGameCtnZoneFrontier *>(zoneRef.Get());
        if (frontier != nullptr && frontier->childZoneId == id) {
            return frontier;
        }
    }
    return nullptr;
}

CGameCtnZone *CGameCtnCollection::GetUpToDateZone(CGameCtnZone *zone) {
    if (zone == nullptr || !zone->oldZone) {
        return zone;
    }

    const bool preserveFrontier =
            dynamic_cast<CGameCtnZoneFrontier *>(zone) != nullptr;
    CGameCtnZone *parent = zone;
    if (CGameCtnZoneFrontier *frontier =
                dynamic_cast<CGameCtnZoneFrontier *>(zone)) {
        parent = GetZone(frontier->parentZoneId);
        if (parent == nullptr) {
            parent = GetObsoleteZone(frontier->parentZoneId);
        }
    }

    const auto findParentFrontier =
            [](const std::vector<CMwNodRef<CGameCtnZone>> &zones,
               const CMwId &childZoneId) {
                for (const CMwNodRef<CGameCtnZone> &zoneRef : zones) {
                    CGameCtnZoneFrontier *frontier =
                            dynamic_cast<CGameCtnZoneFrontier *>(zoneRef.Get());
                    if (frontier != nullptr &&
                        frontier->childZoneId == childZoneId) {
                        return frontier;
                    }
                }
                return static_cast<CGameCtnZoneFrontier *>(nullptr);
            };

    CGameCtnZone *result = zone;
    const size_t maximumTransitions = zones_.size() + obsoleteZones_.size();
    for (size_t transition = 0u;
         parent != nullptr && transition <= maximumTransitions;
         ++transition) {
        CGameCtnZoneFrontier *frontier =
                findParentFrontier(zones_, parent->zoneId);
        if (frontier == nullptr) {
            frontier = findParentFrontier(obsoleteZones_, parent->zoneId);
        }
        if (frontier == nullptr) {
            return parent;
        }

        parent = GetZone(frontier->parentZoneId);
        if (parent == nullptr) {
            parent = GetObsoleteZone(frontier->parentZoneId);
        }
        result = preserveFrontier ? static_cast<CGameCtnZone *>(frontier)
                                  : parent;
        if (result == nullptr || !result->oldZone) {
            return result;
        }
    }
    return result;
}

void CGameCtnCollection::AddSurfaceReplacement(
        const CMwId &sourceSurfaceId,
        const CMwId &targetSurfaceId) {
    surfaceReplacements_.push_back(
            SSurfaceReplacement{sourceSurfaceId, targetSurfaceId});
}

unsigned long CGameCtnCollection::SurfaceReplacementIndex(
        const CMwId &sourceSurfaceId,
        const CMwId &targetSurfaceId) const {
    for (size_t index = 0u; index < surfaceReplacements_.size(); ++index) {
        const SSurfaceReplacement &replacement = surfaceReplacements_[index];
        if (replacement.sourceSurfaceId == sourceSurfaceId &&
            replacement.targetSurfaceId == targetSurfaceId) {
            return static_cast<unsigned long>(index);
        }
    }
    return static_cast<unsigned long>(
            std::numeric_limits<std::uint32_t>::max());
}

void CGameCtnCollection::AddReplacementZone(void) {
    surfaceReplacements_.emplace_back();
}

void CGameCtnCollection::RemoveReplacementZone(void) {
    if (!surfaceReplacements_.empty()) {
        surfaceReplacements_.pop_back();
    }
}
