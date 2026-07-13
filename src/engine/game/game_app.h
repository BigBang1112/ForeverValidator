#pragma once

#include <functional>

#include "engine/game/game_process.h"
struct CSystemPackDesc;
struct CSystemPackManager;
class CGameCtnCatalog;

struct CSystemConfig : CMwNod {
    CSystemConfig(void);
    ~CSystemConfig(void) override;
    int ParentalLock_ComputeIsLocked(void) const;

    void SetParentalLock(bool locked);

private:
    bool parentalLock = false;
};

struct CGameApp : CGameProcess {
    static CGameApp *s_TheGame;

    CGameApp(void);
    ~CGameApp(void) override;
    int Profile_IsPackDescParentalLocked(
            const CSystemPackDesc *packDesc) const;
    void OnCacheCleaned(void);

    void SetSystemConfig(CSystemConfig *config);
    void SetPackManager(CSystemPackManager *manager);
    void SetProfileActive(bool active);
    void SetCacheCleanedListener(std::function<void(void)> listener);
    void SetCatalog(CGameCtnCatalog *catalog);
    CGameCtnCatalog *Catalog(void) const;

private:
    CMwNodRef<CSystemConfig> config;
    CSystemPackManager *packManager = nullptr;
    bool profileActive = false;
    std::function<void(void)> cacheCleanedListener;
    CMwNodRef<CGameCtnCatalog> catalog;
};
