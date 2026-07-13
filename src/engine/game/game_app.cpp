#include "engine/game/game_app.h"
#include <utility>

#include "engine/resources/system_pack_desc.h"
#include "engine/resources/system_pack_manager.h"
#include "engine/game/game_ctn_catalog.h"
CGameApp *CGameApp::s_TheGame = nullptr;

CGameProcess::CGameProcess(void) = default;
CGameProcess::~CGameProcess(void) = default;

CSystemConfig::CSystemConfig(void) = default;
CSystemConfig::~CSystemConfig(void) = default;

int CSystemConfig::ParentalLock_ComputeIsLocked(void) const {
    return parentalLock ? 1 : 0;
}

void CSystemConfig::SetParentalLock(bool locked) {
    parentalLock = locked;
}

CGameApp::CGameApp(void) {
    if (s_TheGame == nullptr) {
        s_TheGame = this;
    }
}

CGameApp::~CGameApp(void) {
    if (s_TheGame == this) {
        s_TheGame = nullptr;
    }
}

int CGameApp::Profile_IsPackDescParentalLocked(
        const CSystemPackDesc *packDesc) const {
    if (packDesc == nullptr || !profileActive || !config ||
        config->ParentalLock_ComputeIsLocked() == 0) {
        return 0;
    }
    if (!packDesc->HasSourceFid()) {
        return 1;
    }
    return packManager != nullptr &&
           packManager->IsPackDescInCache(packDesc) != 0;
}

void CGameApp::OnCacheCleaned(void) {
    if (cacheCleanedListener) {
        cacheCleanedListener();
    }
}

void CGameApp::SetSystemConfig(CSystemConfig *systemConfig) {
    config.MwSetNod(systemConfig);
}

void CGameApp::SetPackManager(CSystemPackManager *manager) {
    packManager = manager;
}

void CGameApp::SetProfileActive(bool active) {
    profileActive = active;
}

void CGameApp::SetCacheCleanedListener(
        std::function<void(void)> listener) {
    cacheCleanedListener = std::move(listener);
}

void CGameApp::SetCatalog(CGameCtnCatalog *newCatalog) {
    catalog.MwSetNod(newCatalog);
}

CGameCtnCatalog *CGameApp::Catalog(void) const {
    return catalog.Get();
}
