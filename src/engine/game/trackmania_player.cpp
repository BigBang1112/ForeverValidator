#include "engine/game/trackmania_player.h"
void CTrackManiaPlayerInfo::SetSpawnLoc(
        const GmIso4 &spawnIso,
        int updateHistory) {
    if (updateHistory != 0) {
        previousSpawnIso_ = spawnIso;
    }
    currentSpawnIso_ = spawnIso;
}
