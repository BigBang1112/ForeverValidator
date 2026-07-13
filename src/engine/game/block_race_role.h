#pragma once

#include "engine/core/engine_types.h"
enum class BlockRaceRole : u32 {
    StartLine = 0u,
    FinishLine = 1u,
    Checkpoint = 2u,
    None = 3u,
    StartFinishLine = 4u,
};

inline bool IsStartLine(BlockRaceRole role) {
    return role == BlockRaceRole::StartLine ||
           role == BlockRaceRole::StartFinishLine;
}
