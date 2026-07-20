#ifndef TMNF_TRACKMANIA_PLAYER_H
#define TMNF_TRACKMANIA_PLAYER_H

#include "engine/core/gm_types.h"
class CTrackManiaPlayerInfo {
public:
    void SetSpawnLoc(const GmIso4 &spawnIso, int updateHistory);
    const GmIso4 &CurrentSpawnLocation() const { return currentSpawnIso_; }
    void SetPreviousSpawnLocation(const GmIso4 &spawnIso) {
        previousSpawnIso_ = spawnIso;
    }
    void MarkEventPrepared() { preparedEvent_ = true; }
    bool IsEventPrepared() const { return preparedEvent_; }

private:
    friend class CTrackManiaRace;

    GmIso4 previousSpawnIso_{};
    GmIso4 currentSpawnIso_{};
    bool preparedEvent_ = false;
};

class CTrackManiaPlayer {
public:
    CTrackManiaPlayerInfo &Info() { return info_; }
    const CTrackManiaPlayerInfo &Info() const { return info_; }

private:
    CTrackManiaPlayerInfo info_{};
};

#endif
