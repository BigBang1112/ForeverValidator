#pragma once

#include "engine/core/engine_types.h"
#include "engine/core/cmw_nod.h"
#include "engine/game/game_identifier.h"
#include <string>

class CGameCtnCollector : public CMwNod {
public:
    struct SHeaderDesc {
        SGameCtnIdentifier identifier;
        std::string displayName;

        SHeaderDesc(void);
        ~SHeaderDesc(void);
    };

    SGameCtnIdentifier identifier;
};

class CGameCtnCollectorList : public CMwNod {
public:
    CGameCtnCollectorList(void);
    ~CGameCtnCollectorList(void) override;
};
