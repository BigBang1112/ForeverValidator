#include "engine/resources/system_fid.h"
#include <algorithm>

#include "engine/resources/system_fids.h"
CSystemFid::CSystemFid(void)
        : parameters(CSystemFidParameters::None),
          loader(&s_DefaultLoaderFile) {}

CSystemFid::~CSystemFid(void) {
    for (const std::unique_ptr<CSystemFid> &child : parametrizedFids) {
        child->loadableFid = nullptr;
    }
    parametrizedFids.clear();
    SetVirtualLoader(nullptr, 0u);
    DetachFromLoadableParent();
    if (nod != nullptr) {
        DetachNod(nod);
    }
    if (location != nullptr) {
        location->RemoveLeaveSafe(this);
    }
    CSystemFids::SetTravelInfo(this, 0u);
}

int CSystemFid::IsEqualFid(CSystemFid *other) {
    return other != nullptr && location == other->location;
}

void CSystemFid::UpdateNameDown(void) {
    for (const std::unique_ptr<CSystemFid> &child : parametrizedFids) {
        child->UpdateNameDown();
    }
}

int CSystemFid::RequestWrite(void) {
    metadataNeedsRefresh = true;
    return 1;
}

void CSystemFid::DetachNod(CMwNod *detachedNod) {
    if (detachedNod == nullptr || detachedNod != nod) {
        return;
    }
    nod = nullptr;
    if (detachedNod->fid == this) {
        detachedNod->fid = nullptr;
    }
}

void CSystemFid::SetNod(CMwNod *newNod) {
    if (newNod == nod) {
        return;
    }
    if (nod != nullptr) {
        DetachNod(nod);
    }
    nod = newNod;
    if (newNod != nullptr) {
        newNod->fid = this;
    }
}

CSystemFid *CSystemFid::ParametrizedGetLoadableFid(void) const {
    return loadableFid != nullptr
            ? loadableFid
            : const_cast<CSystemFid *>(this);
}

CSystemFids *CSystemFid::Location(void) const {
    return location;
}

CMwNod *CSystemFid::LoadedNode(void) const {
    return nod;
}

AssetType CSystemFid::GetAssetType(void) {
    if (loadableFid != nullptr) {
        return loadableFid->GetAssetType();
    }
    if (!assetType.has_value()) {
        LoadHeaderUserData(nullptr);
    }
    return assetType.value_or(AssetType::Unknown);
}

void CSystemFid::ClearAssetType(void) {
    assetType.reset();
}

void CSystemFid::SetAssetType(AssetType type) {
    assetType = type;
}

bool CSystemFid::MatchesAudience(const AssetAudience &audience) {
    return audience.Accepts(*this);
}

CSystemFidParameters &CSystemFid::Parameters(void) {
    return parameters;
}

const CSystemFidParameters &CSystemFid::Parameters(void) const {
    return parameters;
}

bool CSystemFid::UsesDefaultFileLoader(void) const {
    return loader == &s_DefaultLoaderFile;
}
