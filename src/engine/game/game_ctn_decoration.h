#pragma once

#include <memory>

#include "engine/core/cmw_nod.h"
#include "engine/core/engine_types.h"
#include "engine/game/game_ctn_collector.h"
struct CSystemPackDesc;

class CGameCtnDecorationSize : public CMwNod {
public:
    CGameCtnDecorationSize(void);
    ~CGameCtnDecorationSize(void) override;

    u32 mapWidth = 0u;
    u32 mapHeight = 0u;
    u32 mapDepth = 0u;
    u32 defaultZoneHeight = 0u;
};

class CGameCtnDecoration : public CGameCtnCollector {
public:
    CGameCtnDecoration(void);
    ~CGameCtnDecoration(void) override;

    void LoadScene3d(void);
    void Init(void);
    void InitWithNoSkin(void);
    void Reset(void);
    int IsInit(void) const;
    static CGameCtnDecoration *CreateAndInitSkinnedDecorationFromIdent(
            const SGameCtnIdentifier &identifier,
            CSystemPackDesc *packDesc);

    CGameCtnDecorationSize &Size();
    const CGameCtnDecorationSize &Size() const;

private:
    enum class Lifecycle {
        Constructed,
        SceneLoaded,
        Initialized,
    };

    std::unique_ptr<CGameCtnDecorationSize> size_;
    Lifecycle lifecycle_ = Lifecycle::Constructed;
};
