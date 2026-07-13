#include "engine/game/game_ctn_catalog.h"
#include <algorithm>
#include <utility>

#include "engine/game/game_app.h"
#include "engine/game/game_ctn_challenge.h"
#include "engine/game/game_ctn_collection.h"
#include "engine/resources/system_pack_desc.h"
namespace {

bool SameIdAndCollection(const SGameCtnIdentifier &lhs,
                         const SGameCtnIdentifier &rhs) {
    return lhs.id == rhs.id && lhs.collection == rhs.collection;
}

}  // namespace

CGameCtnCatalog::CGameCtnCatalog(void) = default;

CGameCtnCatalog::~CGameCtnCatalog(void) {
    PurgeLoadedArticles();
    for (CMwNodRef<CGameCtnChapter> &chapter : chapters_) {
        if (chapter.Get() != nullptr) {
            chapter->catalog_ = nullptr;
            chapter->ClearAll();
        }
    }
    chapters_.clear();
}

void CGameCtnCatalog::AddChapter(CGameCtnChapter *chapter) {
    if (chapter == nullptr || GetChapter(chapter->CollectionId()) != nullptr) {
        return;
    }
    chapter->catalog_ = this;
    chapters_.emplace_back(chapter);
}

void CGameCtnCatalog::AddArticle(CGameCtnArticle *article) {
    if (article == nullptr) {
        return;
    }
    CGameCtnChapter *chapter = GetChapter(article->Identifier().collection);
    if (chapter != nullptr) {
        chapter->AddArticle(article);
    }
}

CGameCtnChapter *CGameCtnCatalog::GetChapter(const CMwId &id) {
    if (id.IsInvalid()) {
        return nullptr;
    }
    for (const CMwNodRef<CGameCtnChapter> &chapter : chapters_) {
        if (chapter->CollectionId() == id) {
            return chapter.Get();
        }
    }
    return nullptr;
}

CGameCtnArticle *CGameCtnCatalog::GetArticleFromIdent(
        const SGameCtnIdentifier &ident) {
    CGameCtnChapter *chapter = GetChapter(ident.collection);
    return chapter != nullptr ? chapter->GetArticle(ident) : nullptr;
}

void CGameCtnCatalog::TrackLoaded(CGameCtnArticle &article) {
    if (std::find(loadedArticles_.begin(), loadedArticles_.end(), &article) ==
        loadedArticles_.end()) {
        loadedArticles_.push_back(&article);
    }
}

void CGameCtnCatalog::ForgetLoaded(CGameCtnArticle &article) {
    loadedArticles_.erase(
            std::remove(loadedArticles_.begin(),
                        loadedArticles_.end(),
                        &article),
            loadedArticles_.end());
}

void CGameCtnCatalog::PurgeLoadedArticles() {
    const std::vector<CGameCtnArticle *> loaded = loadedArticles_;
    for (CGameCtnArticle *article : loaded) {
        if (article != nullptr) {
            article->Purge();
        }
    }
    loadedArticles_.clear();
    decorationCacheIdentifier_ = SGameCtnIdentifier{};
    decorationCacheSkin_.MwSetNod(nullptr);
}

void CGameCtnCatalog::PrepareDecorationLoad(
        const SGameCtnIdentifier &identifier,
        CSystemPackDesc *packDesc) {
    bool purgeSelectively = false;
    if (decorationCacheSkin_.Get() == packDesc) {
        if (SameIdAndCollection(decorationCacheIdentifier_, identifier)) {
            decorationCacheIdentifier_ = identifier;
            return;
        }
        purgeSelectively = decorationCacheIdentifier_.collection ==
                           identifier.collection;
    }

    if (!purgeSelectively &&
        decorationCacheIdentifier_.author == identifier.author) {
        PurgeLoadedArticles();
    } else {
        const std::vector<CGameCtnArticle *> loaded = loadedArticles_;
        for (CGameCtnArticle *article : loaded) {
            if (article != nullptr &&
                (article->Identifier().collection != identifier.collection ||
                 article->IsDecorationAsset())) {
                article->Purge();
            }
        }
    }

    if (CGameApp::s_TheGame != nullptr) {
        CGameApp::s_TheGame->OnCacheCleaned();
    }
    decorationCacheIdentifier_ = identifier;
    decorationCacheSkin_.MwSetNod(packDesc);
}

CGameCtnChapter::CGameCtnChapter(void) = default;

CGameCtnChapter::~CGameCtnChapter(void) {
    ClearAll();
}

void CGameCtnChapter::ClearAll(void) {
    for (CMwNodRef<CGameCtnArticle> &article : articles_) {
        if (article.Get() != nullptr) {
            article->catalog_ = nullptr;
        }
    }
    articles_.clear();
}

void CGameCtnChapter::AddArticle(CGameCtnArticle *article) {
    if (article == nullptr || GetArticle(article->Identifier()) != nullptr) {
        return;
    }
    const auto insertionPoint = std::find_if(
            articles_.begin(),
            articles_.end(),
            [article](const CMwNodRef<CGameCtnArticle> &existing) {
                return existing->SortIndex() >= article->SortIndex();
            });
    article->catalog_ = catalog_;
    articles_.emplace(insertionPoint, article);
}

CGameCtnCollection *CGameCtnChapter::GetNod(void) {
    if (!collectionSource_) {
        return nullptr;
    }
    CMwNod *node = collectionSource_->LoadedNode();
    if (node == nullptr) {
        node = collectionSource_->LoadNode({}, *this);
    }
    return dynamic_cast<CGameCtnCollection *>(node);
}

CGameCtnArticle *CGameCtnChapter::GetArticle(
        SGameCtnIdentifier identifier) {
    if (identifier.id.IsInvalid() ||
        identifier.collection != collectionId_) {
        return nullptr;
    }
    for (const CMwNodRef<CGameCtnArticle> &article : articles_) {
        if (SameIdAndCollection(article->Identifier(), identifier)) {
            return article.Get();
        }
    }
    return nullptr;
}

CGameCtnArticle *CGameCtnChapter::GetDecorationIfUnique(void) {
    CGameCtnArticle *unique = nullptr;
    for (const CMwNodRef<CGameCtnArticle> &article : articles_) {
        if (!article->IsDecorationAsset()) {
            continue;
        }
        if (unique != nullptr) {
            return nullptr;
        }
        unique = article.Get();
    }
    return unique;
}

void CGameCtnChapter::SetCollectionId(CMwId collectionId) {
    collectionId_ = collectionId;
}

const CMwId &CGameCtnChapter::CollectionId() const {
    return collectionId_;
}

void CGameCtnChapter::SetCollectionSource(
        GameCatalogAssetSourceHandle source) {
    collectionSource_ = std::move(source);
}

bool CGameCtnChapter::HasCollectionSource() const {
    return collectionSource_ != nullptr;
}

CGameCtnArticle::CGameCtnArticle(void) = default;

CGameCtnArticle::~CGameCtnArticle(void) {
    Purge();
}

void CGameCtnArticle::Purge(void) {
    if (!cachedNode_) {
        return;
    }
    cachedNode_.MwSetNod(nullptr);
    if (catalog_ != nullptr) {
        catalog_->ForgetLoaded(*this);
    }
}

void CGameCtnArticle::PurgeAllForce(void) {
    CGameCtnCatalog *catalog = (CGameApp::s_TheGame != nullptr
            ? CGameApp::s_TheGame->Catalog()
            : nullptr);
    if (catalog != nullptr) {
        catalog->PurgeLoadedArticles();
    }
}

void CGameCtnArticle::PrepareCacheForLoading(
        const SGameCtnIdentifier &identifier,
        CSystemPackDesc *packDesc) {
    CGameCtnCatalog *catalog = (CGameApp::s_TheGame != nullptr
            ? CGameApp::s_TheGame->Catalog()
            : nullptr);
    if (catalog != nullptr) {
        catalog->PrepareDecorationLoad(identifier, packDesc);
    }
}

CMwNod *CGameCtnArticle::GetNod(int loadNow) {
    if (!source_) {
        return nullptr;
    }
    CMwNod *node = source_->LoadedNode();
    if (node != nullptr) {
        if (loadNow != 0) {
            return node;
        }
    } else if (loadNow != 0) {
        return nullptr;
    } else {
        GameCatalogLoadOptions options;
        options.includeSiblingShaders = (AssetKind() == GameCatalogAssetKind::BlockInfo);
        node = source_->LoadNode(options, *this);
    }

    if (node != cachedNode_.Get()) {
        Purge();
        cachedNode_.MwSetNod(node);
        if (node != nullptr && catalog_ != nullptr) {
            catalog_->TrackLoaded(*this);
        }
    }
    return node;
}

void CGameCtnArticle::SetIdentifier(SGameCtnIdentifier identifier) {
    identifier_ = std::move(identifier);
}

const SGameCtnIdentifier &CGameCtnArticle::Identifier() const {
    return identifier_;
}

void CGameCtnArticle::SetSortIndex(uint16_t sortIndex) {
    sortIndex_ = sortIndex;
}

uint16_t CGameCtnArticle::SortIndex() const {
    return sortIndex_;
}

void CGameCtnArticle::SetAssetSource(GameCatalogAssetSourceHandle source) {
    Purge();
    source_ = std::move(source);
}

GameCatalogAssetKind CGameCtnArticle::AssetKind() const {
    return source_ != nullptr ? source_->Kind()
                              : GameCatalogAssetKind::Other;
}

CGameCtnChapter *CGameCtnChallenge::GetChapter(void) const {
    if (catalog == nullptr) {
        return nullptr;
    }
    CGameCtnChapter *chapter = catalog->GetChapter(collectionId);
    return chapter != nullptr && chapter->HasCollectionSource()
            ? chapter
            : nullptr;
}

CSystemPackDesc *CGameCtnChallenge::GetModPackDesc(
        const SCtnForcedMods *forcedMods) {
    if (forcedMods != nullptr) {
        for (const SCtnForcedMods::SEnvMod &environmentMod :
             forcedMods->envMods) {
            if (environmentMod.environmentId != collectionId ||
                environmentMod.packDesc == nullptr) {
                continue;
            }
            if (modPackDesc != nullptr &&
                forcedMods->overrideExistingMods == 0u) {
                break;
            }
            return environmentMod.packDesc;
        }
    }
    return modPackDesc;
}
