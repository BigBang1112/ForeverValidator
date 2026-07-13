#pragma once

#include "engine/core/engine_types.h"
constexpr u32 CHmsCollisionManager_GroupCount = 5u;

enum class CHmsReplayStaticCollisionGroupRole : u32 {
    StaticSource = 2u,
    MapBaseGroundTarget = 3u,
};
