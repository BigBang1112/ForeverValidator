#include "engine/game/game_ctn_decoration.h"
#include "engine/game/game_app.h"
#include "engine/game/game_ctn_catalog.h"
CGameCtnDecorationSize::CGameCtnDecorationSize(void) = default;

CGameCtnDecorationSize::~CGameCtnDecorationSize(void) = default;

CGameCtnDecoration::CGameCtnDecoration(void)
        : size_(std::make_unique<CGameCtnDecorationSize>()) {}

CGameCtnDecoration::~CGameCtnDecoration(void) = default;

void CGameCtnDecoration::LoadScene3d(void) {
    if (lifecycle_ == Lifecycle::Constructed) {
        lifecycle_ = Lifecycle::SceneLoaded;
    }
}

void CGameCtnDecoration::Init(void) {
    if (lifecycle_ == Lifecycle::Initialized) {
        return;
    }
    LoadScene3d();
    lifecycle_ = Lifecycle::Initialized;
}

void CGameCtnDecoration::InitWithNoSkin(void) {
    Init();
}

void CGameCtnDecoration::Reset(void) {
    lifecycle_ = Lifecycle::Constructed;
}

int CGameCtnDecoration::IsInit(void) const {
    return lifecycle_ == Lifecycle::Initialized ? 1 : 0;
}

CGameCtnDecorationSize &CGameCtnDecoration::Size() {
    return *size_;
}

const CGameCtnDecorationSize &CGameCtnDecoration::Size() const {
    return *size_;
}

CGameCtnDecoration *CGameCtnDecoration::CreateAndInitSkinnedDecorationFromIdent(
        const SGameCtnIdentifier &identifier,
        CSystemPackDesc *packDesc) {
    CGameCtnCatalog *catalog = CGameApp::s_TheGame != nullptr
            ? CGameApp::s_TheGame->Catalog()
            : nullptr;
    if (catalog == nullptr || identifier.id.IsInvalid()) {
        return nullptr;
    }

    CGameCtnArticle::PrepareCacheForLoading(identifier, packDesc);
    CGameCtnArticle *article = catalog->GetArticleFromIdent(identifier);
    if (article == nullptr ||
        article->AssetKind() != GameCatalogAssetKind::Decoration) {
        return nullptr;
    }
    auto *decoration = dynamic_cast<CGameCtnDecoration *>(
            article->GetNod(0));
    if (decoration == nullptr) {
        return nullptr;
    }
    decoration->identifier = identifier;
    decoration->Reset();
    if (packDesc == nullptr) {
        decoration->InitWithNoSkin();
    } else {
        decoration->Init();
    }
    return decoration;
}
