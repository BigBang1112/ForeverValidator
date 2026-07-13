#pragma once

#include "engine/core/engine_types.h"
#include <memory>
#include <vector>

#include "engine/core/cfast_buffer.h"
#include "engine/core/cmw_nod.h"
#include "engine/core/fast_string.h"
#include "engine/core/owned_or_borrowed.h"
#include "engine/resources/system_fid.h"
struct CSystemManagerFile;
class AssetAudience;

struct CSystemFids : CMwNod {
    using FolderChain = CFastBuffer<CSystemFids *>;

    enum EFindWay : u32 {
        FindWay_ChildrenOnly = 0u,
        FindWay_AllowParent = 1u,
    };

    CSystemFids(void);
    ~CSystemFids(void) override;
    virtual int CreateSub(CSystemFid *fid, int mode);
    virtual int TruncLocal(CSystemFid *fid);
    virtual CSystemFids *GetRoot(void);
    virtual void GetFullName(CFastStringInt &outName, int fullName) const;
    virtual int GetFullNameUpTo(
            CFastStringInt &outName,
            const CSystemFids *upToFids) const;
    virtual void NewFidsAdded(CSystemFids *tree);
    virtual void NewFidAdded(CSystemFid *fid);
    virtual void Browse(int mode);
    virtual void UpdateTree(int updateMode);
    virtual void UpdateNameDown(void);
    virtual void UpdateFidProps(int recursive, int updateMode);
    void ConnectLeave(CSystemFid *fid);
    void ConnectTree(CSystemFids *tree);
    void AddLeave(CSystemFid *fid);
    void AddLeaveIfNot(CSystemFid *fid);
    void RemoveLeaveSafe(CSystemFid *fid);
    void RemoveTreeSafe(CSystemFids *tree);
    void AddTree(CSystemFids *tree);
    int TruncLocals(CSystemFid *fid, int mode);
    int IsOneBaseOf(const CSystemFids *other) const;
    int ComputeFolderChainToBase(CSystemFids *base, FolderChain &outChain);
    void RefreshVirtualRecursive(int hasCustomLoader);
    CSystemFid *FindFidFromBaseName(
            const CFastStringInt &filename,
            const AssetAudience &audience);
    CSystemFids *FindLocationDown(
            const CFastStringInt &directory,
            int browseMissing,
            EFindWay findWay);
    CSystemFid *FindFid(
            const CFastStringInt &filename,
            int browseMissing,
            EFindWay findWay);
    static unsigned long GetTravelInfo(CMwNod *nod);
    static void SetTravelInfo(CMwNod *nod, unsigned long travelInfo);

    void AdoptLeave(std::unique_ptr<CSystemFid> fid);
    void AdoptTree(std::unique_ptr<CSystemFids> tree);
    CSystemFids *Parent(void) const;

private:
    friend struct CSystemEngine;

    using FidEntry = OwnedOrBorrowed<CSystemFid>;
    using ChildEntry = OwnedOrBorrowed<CSystemFids>;

    std::vector<FidEntry> leaves;
    std::vector<ChildEntry> childTrees;
    CSystemFids *parent = nullptr;
    bool virtualRefresh = false;

    int HasVirtualRefresh(void) const {
        return virtualRefresh ? 1 : 0;
    }
    int LeavesHaveCustomLoader(void) const;
    int HasCustomLoaderBelow(u32 incoming) const;
    int FolderNameMatchesDirectory(const CFastStringInt &directory) const;
    static int IsParentDirectorySegment(const CFastStringInt &directory);
    CSystemFids *FindChildTreeByDirectory(const CFastStringInt &directory);
    CSystemFids *FindOneLocationDown(
            const CFastStringInt &directory,
            int browseMissing,
            EFindWay findWay);
    std::unique_ptr<CSystemFid> ReleaseOwnedLeave(CSystemFid *fid);
    std::unique_ptr<CSystemFids> ReleaseOwnedTree(CSystemFids *tree);
};

struct CSystemFidsFolder : CSystemFids {
    CSystemFidsFolder(void);
    ~CSystemFidsFolder(void) override;
    void GetFullName(CFastStringInt &outName, int fullName) const override;
    int GetFullNameUpTo(
            CFastStringInt &outName,
            const CSystemFids *upToFids) const override;
    void UpdateNameDown(void) override;
    void SetDirName(const CFastStringInt &name);
    virtual void UpdatedDirName(void);
    const CFastStringInt &DirName(void) const;

private:
    CFastStringInt dirName;
};

struct CSystemFidsDrive : CSystemFidsFolder {
    CSystemFidsDrive(void);
    ~CSystemFidsDrive(void) override;
    void GetFullName(CFastStringInt &outName, int fullName) const override;
    int GetFullNameUpTo(
            CFastStringInt &outName,
            const CSystemFids *upToFids) const override;
    void SetDriveName(const CFastStringInt &name);
    void SetLogicalPrefix(const char *prefix);

private:
    CFastStringInt logicalPrefix;
};
