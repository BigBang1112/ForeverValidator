#pragma once

#include <functional>
#include <optional>

#include "engine/core/cmw_nod.h"
#include "engine/core/engine_types.h"
#include "engine/core/fast_string.h"
#include "engine/resources/plug_file_zip.h"
#include "engine/resources/system_fid_file.h"
struct CSystemFid;
struct CSystemFids;
struct CSystemPackManager;
struct SSystemTime;

struct CSystemPackDesc : CMwNod {
    int IsChecksumSpecial(void) const;
    void TouchLastTimeOfUse(void);
    void GetContents(CSystemFids *&outFids, CSystemFid *&outFid);

    bool HasSourceFid(void) const;
    bool ReferencesFid(const CSystemFid *fid) const;

private:
    friend struct CSystemPackManager;

    CSystemPackDesc(
            const SNat128 &checksum,
            const CFastStringInt &name,
            CSystemFidFile *fid,
            SSystemTime *time);
    ~CSystemPackDesc(void) override;

    SNat128 checksum;
    CFastStringInt name;
    std::optional<std::reference_wrapper<CSystemFidFile>> sourceFid;
    CMwNodRef<CPlugFileZip> contents;
    std::uint64_t lastUseSequence = 0u;
    CSystemPackManager *manager = nullptr;

    CSystemFids *CacheBase(void) const;
    CSystemFidFile *SourceFid(void) const {
        return sourceFid.has_value() ? &sourceFid->get() : nullptr;
    }
    int PackFidIsZip(void) const;
};
