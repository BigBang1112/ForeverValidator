#pragma once

#include <vector>

#include "engine/core/engine_types.h"
#include "engine/core/mw_id.h"
struct CSystemPackDesc;

struct SCtnForcedMods {
    struct SEnvMod {
        CMwId environmentId;
        CSystemPackDesc *packDesc = nullptr;

        SEnvMod(void);
        ~SEnvMod(void);
    };

    u32 overrideExistingMods = 0u;
    std::vector<SEnvMod> envMods;

    ~SCtnForcedMods(void);
};
