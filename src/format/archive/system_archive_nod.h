#pragma once

#include "engine/core/engine_types.h"
#include <vector>

#include "format/archive/classic_archive.h"
#include "engine/core/cmw_nod.h"
struct CSystemFid;

struct CSystemArchiveNod : CClassicArchive {
    enum EArchive : u32 {
        Archive_Header = 1u,
        Archive_Default = 7u,
    };

    enum EVersion : u32 {
        Version_6 = 6u,
    };

    CSystemArchiveNod(void);
    ~CSystemArchiveNod(void) override;
    void DoNodPtr(CMwNod *&nod) override;
    int DoLoadAllRef(void);
    int DoFidLoadFile(CMwNod *&outNod);
    int AddInternalRef(
            CMwNod *nod,
            unsigned long &outRefIndex,
            const char *contextName);
    int DoFidLoadRefs(EArchive loadMode, CClassicBuffer *buffer);
    int DoLoadFromFid(CMwNod *&outNod);
    int DoLoadResource(unsigned long resourceId, CMwNod *&outNod);
    static int LoadFromFid(
            CMwNod *&outNod,
            CSystemFid *fid,
            EArchive archiveMode);
    static int Compare(CMwNod *leftNod, CMwNod *rightNod, int &isEqualOut);
    static int Duplicate(CMwNod *&target, int branchSelector);
    static int LoadResource(unsigned long resourceId, CMwNod *&outNod);

private:
    friend struct CSystemFid;

    std::vector<CMwNodRef<CMwNod>> internalRefs;
    CSystemFid *fid = nullptr;
    EArchive archiveMode = Archive_Default;
};
