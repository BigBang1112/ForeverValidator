#pragma once

#include <optional>

#include "simulation/runtime/replay_environment_definition.h"
struct CGameCtnReplayMapInput;

int BuildReplayWaterDefinition(
        const CGameCtnReplayMapInput &map,
        std::optional<ReplayWaterDefinition> *out);
