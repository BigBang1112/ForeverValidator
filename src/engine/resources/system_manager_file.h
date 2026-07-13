#pragma once

#include "engine/core/cmw_nod.h"
#include "engine/resources/system_fid.h"
struct CFastStringInt;
struct CSystemFidFile;
struct CSystemFidMemory;
struct CSystemFidsFolder;

struct CSystemManagerFile : CMwNod {
    CSystemManagerFile(void);
    ~CSystemManagerFile(void) override;
    CSystemFidsFolder *CreateFidsFolder(void) const;
    CSystemFidFile *CreateFidFile(void) const;
    CSystemFidFile *CreateFidResourceFile(void) const;
    CSystemFidMemory *CreateFidMemory(void) const;
    CSystemFid *CreateFidFromType(CSystemFid::EFidType type) const;
    static int UpdateFidProps(
            CSystemFidFile *fid,
            const CFastStringInt &fullName);

protected:
    virtual CSystemFidsFolder *DoCreateFidsFolder(void) const;
    virtual CSystemFidFile *DoCreateFidFile(void) const;
};
