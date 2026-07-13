#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <queue>
#include <vector>

#include "engine/core/cmw_nod.h"
#include "engine/core/engine_types.h"
#include "engine/core/fast_string.h"
#include "engine/resources/system_fid.h"
#include "engine/resources/system_fids.h"
#include "engine/resources/system_pack_desc.h"
struct CSystemFid;
struct CSystemFidFile;
struct CSystemPackDesc;
struct SSystemTime;

struct CSystemPackManager : CMwNod {
    struct SQueueElem {
        CMwNodRef<CSystemPackDesc> packDesc;
        CFastString packElemName;
        unsigned long packElemIndex = 0ul;
        CMwNodRef<CSystemFid> fid;
        CMwNodRef<CMwNod> owner;
        bool retryResolved = false;

        SQueueElem(void);
        ~SQueueElem(void);
    };

    CSystemPackManager(void);
    ~CSystemPackManager(void) override;
    int IsPackDescInCache(const CSystemPackDesc *packDesc) const;
    CSystemFidFile *SimpleGetPackElem(
            CSystemPackDesc *packDesc,
            const CFastString &packElemName,
            const AssetAudience &audience,
            int &outMissing,
            int loadNow);
    CSystemFidFile *GetPackElem(
            CSystemPackDesc *packDesc,
            const CFastString &packElemName,
            unsigned long packElemIndex,
            CSystemFid *sourceFid,
            CMwNod *owner);
    CSystemPackDesc *AddPackDesc(
            const SNat128 &checksum,
            const CFastStringInt &name,
            CSystemFidFile *fid,
            SSystemTime *time,
            int flags);
    CSystemPackDesc *FindOrAddPackDesc(
            const SNat128 &checksum,
            const CFastStringInt &name);
    void SetCacheDir(CSystemFidsFolder *directory);

    void SetSpecialPackElementHandler(
            std::function<CSystemFidFile *(
                    CSystemPackDesc &,
                    CMwNod *,
                    CSystemFidFile *)> handler);
    void SetLooseFileResolver(
            std::function<CSystemFidFile *(
                    const CFastStringInt &,
                    const AssetAudience &)> resolver);
    bool IsDirty(void) const;
    std::uint64_t UseSequence(void) const;

private:
    friend struct CSystemPackDesc;

    std::queue<SQueueElem> expectedPackElemsQueue;
    std::function<CSystemFidFile *(
            CSystemPackDesc &,
            CMwNod *,
            CSystemFidFile *)> specialPackElementHandler;
    std::function<CSystemFidFile *(
            const CFastStringInt &,
            const AssetAudience &)> looseFileResolver;
    std::vector<CMwNodRef<CSystemPackDesc>> packDescs;
    std::optional<std::reference_wrapper<CSystemFidsFolder>> cacheDir;
    bool dirty = false;
    std::uint64_t useSequence = 0u;

    void MarkDirty(void) {
        dirty = true;
    }
    std::uint64_t NotePackUse(void) {
        return ++useSequence;
    }
    int CacheContainsBase(const CSystemFids *packBase) const {
        return packBase != nullptr && cacheDir.has_value() &&
               cacheDir->get().IsOneBaseOf(packBase);
    }
};
