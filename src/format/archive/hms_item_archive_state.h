#pragma once

#include "engine/core/engine_types.h"
#include "engine/physics/dynamics/hms_item.h"
struct HmsItemArchiveWords {
    u32 physics = 0u;
    u32 rendering = 0u;
    u32 visibility = 0u;
};

CHmsItem::Properties DecodeHmsItemArchiveState(
        const HmsItemArchiveWords &words);
HmsItemArchiveWords EncodeHmsItemArchiveState(
        const CHmsItem::Properties &properties);
