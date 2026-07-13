#pragma once

#include "engine/core/engine_types.h"

struct StaticSolidArchiveTreeStateFlags {
    static u32 FromCPlugTreeChunk0904f010(u32 rawFlags);
    static u32 Low17Or2800(u32 rawFlags);
    static u32 FullOr2800(u32 rawFlags);
    static u32 FullOr2000(u32 rawFlags);
};
