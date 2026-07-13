#include "engine/resources/system_pack_manager.h"
#include <cstring>
#include <utility>

#include "engine/resources/system_fid_file.h"
#include "engine/resources/system_fids.h"
CSystemPackManager::SQueueElem::SQueueElem(void) = default;
CSystemPackManager::SQueueElem::~SQueueElem(void) = default;

CSystemPackManager::CSystemPackManager(void) = default;

CSystemPackManager::~CSystemPackManager(void) {
    for (CMwNodRef<CSystemPackDesc> &descriptor : packDescs) {
        if (descriptor) {
            descriptor->manager = nullptr;
        }
    }
}

int CSystemPackManager::IsPackDescInCache(
        const CSystemPackDesc *packDesc) const {
    return packDesc != nullptr && CacheContainsBase(packDesc->CacheBase());
}

CSystemFidFile *CSystemPackManager::GetPackElem(
        CSystemPackDesc *packDesc,
        const CFastString &packElemName,
        unsigned long packElemIndex,
        CSystemFid *sourceFid,
        CMwNod *owner) {
    const bool special =
            packDesc != nullptr && packDesc->IsChecksumSpecial() != 0;
    const AssetType sourceType = sourceFid != nullptr
            ? sourceFid->GetAssetType()
            : AssetType::Unknown;
    const AssetAudience audience = sourceType == AssetType::Unknown
            ? AnyAssetAudience()
            : AssetAudience(sourceType);
    int missing = 0;
    CSystemFidFile *packElem = SimpleGetPackElem(
            packDesc, packElemName, audience, missing, 1);

    if (special && packDesc != nullptr && specialPackElementHandler) {
        packElem = specialPackElementHandler(*packDesc, owner, packElem);
    }

    CSystemFid *queuedFid = sourceFid;
    CSystemFidFile *retryResolvedFid = nullptr;
    if (packElem != nullptr) {
        return packElem;
    }
    if (sourceFid == nullptr) {
        queuedFid = nullptr;
    } else {
        int retryMissing = 0;
        retryResolvedFid = SimpleGetPackElem(
                packDesc, packElemName, audience,
                retryMissing, 0);
        if (retryResolvedFid != nullptr) {
            queuedFid = retryResolvedFid;
        }
    }

    if (missing == 0 && owner != nullptr && queuedFid != nullptr) {
        expectedPackElemsQueue.emplace();
        SQueueElem &entry = expectedPackElemsQueue.back();
        entry.packDesc.MwSetNod(packDesc);
        entry.packElemName = packElemName;
        entry.packElemIndex = packElemIndex;
        entry.fid.MwSetNod(queuedFid);
        entry.owner.MwSetNod(owner);
        entry.retryResolved = retryResolvedFid != nullptr;
    }
    return dynamic_cast<CSystemFidFile *>(queuedFid);
}

CSystemFidFile *CSystemPackManager::SimpleGetPackElem(
        CSystemPackDesc *packDesc,
        const CFastString &packElemName,
        const AssetAudience &audience,
        int &outMissing,
        int loadNow) {
    const char *rawName = packElemName.Data();
    const bool starName = rawName != nullptr && rawName[0] == '*';
    const SStringParam dot = {".", 1u};
    const bool dotName =
            packElemName.Length() == 1u &&
            packElemName.Compare(dot, 0u) == 0;
    const char *lookupName = starName ? rawName + 1 : rawName;
    CFastStringInt lookupWide;
    lookupWide.SetString({
        lookupName,
        lookupName != nullptr ? static_cast<u32>(std::strlen(lookupName)) : 0u,
    });

    if (packDesc == nullptr) {
        if (!looseFileResolver) {
            outMissing = 1;
            return nullptr;
        }
        CSystemFidFile *resolved = looseFileResolver(lookupWide, audience);
        outMissing = resolved == nullptr;
        return resolved;
    }

    (void)loadNow;

    CSystemFids *contentsFids = nullptr;
    CSystemFid *contentsFid = nullptr;
    packDesc->GetContents(contentsFids, contentsFid);
    if (contentsFids != nullptr) {
        auto *resolved = dynamic_cast<CSystemFidFile *>(
                contentsFids->FindFidFromBaseName(
                        lookupWide, audience));
        outMissing = resolved == nullptr;
        if (resolved != nullptr) {
            packDesc->TouchLastTimeOfUse();
        }
        return resolved;
    }
    if (contentsFid == nullptr) {
        outMissing = 1;
        return nullptr;
    }
    if ((!starName && !dotName) ||
        !contentsFid->MatchesAudience(audience)) {
        outMissing = 1;
        return nullptr;
    }
    outMissing = 0;
    packDesc->TouchLastTimeOfUse();
    return dynamic_cast<CSystemFidFile *>(contentsFid);
}

CSystemPackDesc *CSystemPackManager::AddPackDesc(
        const SNat128 &checksum,
        const CFastStringInt &name,
        CSystemFidFile *fid,
        SSystemTime *time,
        int flags) {
    (void)flags;
    CMwNodRef<CSystemPackDesc> descriptor(
            new CSystemPackDesc(checksum, name, fid, time));
    CSystemPackDesc *result = descriptor.Get();
    result->manager = this;
    packDescs.push_back(std::move(descriptor));
    return result;
}

CSystemPackDesc *CSystemPackManager::FindOrAddPackDesc(
        const SNat128 &checksum,
        const CFastStringInt &name) {
    for (const CMwNodRef<CSystemPackDesc> &descriptor : packDescs) {
        if (descriptor->checksum == checksum &&
            descriptor->name.CompareNoCase(name.Param(), 0u) == 0) {
            return descriptor.Get();
        }
    }
    return AddPackDesc(checksum, name, nullptr, nullptr, 0);
}

void CSystemPackManager::SetCacheDir(CSystemFidsFolder *directory) {
    if (directory == nullptr) {
        cacheDir.reset();
    } else {
        cacheDir = *directory;
    }
}

void CSystemPackManager::SetSpecialPackElementHandler(
        std::function<CSystemFidFile *(
                CSystemPackDesc &, CMwNod *, CSystemFidFile *)> handler) {
    specialPackElementHandler = std::move(handler);
}

void CSystemPackManager::SetLooseFileResolver(
        std::function<CSystemFidFile *(
                const CFastStringInt &,
                const AssetAudience &)> resolver) {
    looseFileResolver = std::move(resolver);
}

bool CSystemPackManager::IsDirty(void) const {
    return dirty;
}

std::uint64_t CSystemPackManager::UseSequence(void) const {
    return useSequence;
}
