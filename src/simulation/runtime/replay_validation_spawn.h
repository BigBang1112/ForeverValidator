#ifndef TMNF_REPLAY_VALIDATION_SPAWN_H
#define TMNF_REPLAY_VALIDATION_SPAWN_H

#include <cstdint>

#include "engine/core/gm_types.h"

GmIso4 BuildReplayValidationSpawnLocation(
        const GmIso4 &location,
        std::uint32_t validationSeed);

#endif
