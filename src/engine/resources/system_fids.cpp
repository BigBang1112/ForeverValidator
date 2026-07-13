#include "engine/resources/system_fids.h"
#include <algorithm>

#include "engine/resources/system_fid.h"
#include "engine/resources/system_fid_file.h"
#include "engine/resources/system_file_name.h"
#include "engine/resources/system_manager_file.h"
int CSystemFids::IsOneBaseOf(const CSystemFids *other) const {
    for (const CSystemFids *cursor = other;
         cursor != nullptr;
         cursor = cursor->parent) {
        if (cursor == this) {
            return 1;
        }
    }
    return 0;
}

void CSystemFids::UpdateTree(int updateMode) {
    Browse(updateMode);
    UpdateFidProps(0, updateMode);
}

CSystemFids *CSystemFids::GetRoot(void) {
    CSystemFids *root = this;
    while (root->parent != nullptr) {
        root = root->parent;
    }
    return root;
}

int CSystemFids::CreateSub(CSystemFid *fid, int mode) {
    (void)fid;
    (void)mode;
    return 0;
}

int CSystemFids::TruncLocal(CSystemFid *fid) {
    (void)fid;
    return 1;
}

void CSystemFids::NewFidsAdded(CSystemFids *tree) { (void)tree; }
void CSystemFids::NewFidAdded(CSystemFid *fid) { (void)fid; }
void CSystemFids::Browse(int mode) { (void)mode; }
void CSystemFids::UpdateNameDown(void) {
    for (const FidEntry &entry : leaves) {
        if (CSystemFid *fid = entry.Get()) {
            fid->UpdateNameDown();
        }
    }
    for (const ChildEntry &entry : childTrees) {
        if (CSystemFids *child = entry.Get()) {
            child->UpdateNameDown();
        }
    }
}

void CSystemFids::UpdateFidProps(int recursive, int updateMode) {
    (void)updateMode;
    for (const FidEntry &entry : leaves) {
        if (CSystemFid *fid = entry.Get()) {
            fid->UpdateFidProps();
        }
    }
    if (recursive == 0) {
        return;
    }
    for (const ChildEntry &entry : childTrees) {
        if (CSystemFids *child = entry.Get()) {
            child->UpdateFidProps(1, updateMode);
        }
    }
}

int CSystemFids::LeavesHaveCustomLoader(void) const {
    return std::any_of(
            leaves.begin(), leaves.end(),
            [](const FidEntry &entry) {
                const CSystemFid *fid = entry.Get();
                return fid != nullptr &&
                       fid->loader != &CSystemFid::s_DefaultLoaderFile;
            });
}

int CSystemFids::HasCustomLoaderBelow(u32 incoming) const {
    return incoming != 0u ||
           (std::any_of(
            childTrees.begin(), childTrees.end(),
            [](const ChildEntry &entry) {
                const CSystemFids *child = entry.Get();
                return child != nullptr && child->HasVirtualRefresh();
            })) ||
           LeavesHaveCustomLoader();
}

void CSystemFids::ConnectLeave(CSystemFid *fid) {
    if (fid != nullptr) {
        fid->location = this;
    }
}

void CSystemFids::ConnectTree(CSystemFids *tree) {
    if (tree != nullptr) {
        tree->parent = this;
    }
}

void CSystemFids::AddLeave(CSystemFid *fid) {
    if (fid == nullptr) {
        return;
    }
    const auto existing = std::find_if(
            leaves.begin(), leaves.end(),
            [fid](const FidEntry &entry) { return entry.Get() == fid; });
    if (existing != leaves.end()) {
        return;
    }
    if (fid->location != nullptr) {
        fid->location->RemoveLeaveSafe(fid);
    }
    ConnectLeave(fid);
    leaves.emplace_back(*fid);
    NewFidAdded(fid);
}

void CSystemFids::AddLeaveIfNot(CSystemFid *fid) {
    const auto found = std::find_if(
            leaves.begin(), leaves.end(),
            [fid](const FidEntry &entry) { return entry.Get() == fid; });
    if (found == leaves.end()) {
        AddLeave(fid);
    }
}

void CSystemFids::RemoveLeaveSafe(CSystemFid *fid) {
    const auto found = std::find_if(
            leaves.begin(), leaves.end(),
            [fid](const FidEntry &entry) { return entry.Get() == fid; });
    if (found == leaves.end()) {
        return;
    }
    if (fid != nullptr && fid->location == this) {
        fid->location = nullptr;
    }
    std::unique_ptr<CSystemFid> ownership = found->Release();
    leaves.erase(found);
    if (ownership != nullptr) {
        (void)ownership.release();
    }
}

void CSystemFids::RemoveTreeSafe(CSystemFids *tree) {
    const auto found = std::find_if(
            childTrees.begin(), childTrees.end(),
            [tree](const ChildEntry &entry) { return entry.Get() == tree; });
    if (found == childTrees.end()) {
        return;
    }
    if (tree != nullptr && tree->parent == this) {
        tree->parent = nullptr;
    }
    std::unique_ptr<CSystemFids> ownership = found->Release();
    childTrees.erase(found);
    if (ownership != nullptr) {
        (void)ownership.release();
    }
}

void CSystemFids::AddTree(CSystemFids *tree) {
    if (tree == nullptr || tree == this) {
        return;
    }
    if (tree->parent == this) {
        return;
    }
    if (tree->parent != nullptr) {
        CSystemFids *oldParent = tree->parent;
        std::unique_ptr<CSystemFids> ownership =
                oldParent->ReleaseOwnedTree(tree);
        if (ownership != nullptr) {
            ConnectTree(tree);
            childTrees.emplace_back(std::move(ownership));
            NewFidsAdded(tree);
            return;
        }
        oldParent->RemoveTreeSafe(tree);
    }
    ConnectTree(tree);
    childTrees.emplace_back(*tree);
    NewFidsAdded(tree);
}

void CSystemFids::AdoptLeave(std::unique_ptr<CSystemFid> fid) {
    if (fid == nullptr) {
        return;
    }
    CSystemFid *child = fid.get();
    if (child->location != nullptr) {
        child->location->RemoveLeaveSafe(child);
    }
    ConnectLeave(child);
    leaves.emplace_back(std::move(fid));
    NewFidAdded(child);
}

void CSystemFids::AdoptTree(std::unique_ptr<CSystemFids> tree) {
    if (tree == nullptr || tree.get() == this) {
        return;
    }
    CSystemFids *child = tree.get();
    if (child->parent != nullptr) {
        child->parent->RemoveTreeSafe(child);
    }
    ConnectTree(child);
    childTrees.emplace_back(std::move(tree));
    NewFidsAdded(child);
}

CSystemFids *CSystemFids::Parent(void) const {
    return parent;
}

std::unique_ptr<CSystemFid> CSystemFids::ReleaseOwnedLeave(
        CSystemFid *fid) {
    const auto found = std::find_if(
            leaves.begin(), leaves.end(),
            [fid](const FidEntry &entry) { return entry.Owns(fid); });
    if (found == leaves.end()) {
        return nullptr;
    }
    if (fid != nullptr && fid->location == this) {
        fid->location = nullptr;
    }
    std::unique_ptr<CSystemFid> released = found->Release();
    leaves.erase(found);
    return released;
}

std::unique_ptr<CSystemFids> CSystemFids::ReleaseOwnedTree(
        CSystemFids *tree) {
    const auto found = std::find_if(
            childTrees.begin(), childTrees.end(),
            [tree](const ChildEntry &entry) { return entry.Owns(tree); });
    if (found == childTrees.end()) {
        return nullptr;
    }
    if (tree != nullptr && tree->parent == this) {
        tree->parent = nullptr;
    }
    std::unique_ptr<CSystemFids> released = found->Release();
    childTrees.erase(found);
    return released;
}

int CSystemFids::ComputeFolderChainToBase(
        CSystemFids *base,
        FolderChain &outChain) {
    outChain.clear();
    if (base == nullptr || base->GetRoot() != GetRoot()) {
        return 0;
    }
    for (CSystemFids *cursor = this; cursor != base; cursor = cursor->parent) {
        if (cursor == nullptr) {
            outChain.clear();
            return 0;
        }
        outChain.push_back(cursor);
    }
    return 1;
}

int CSystemFids::TruncLocals(CSystemFid *fid, int mode) {
    if (TruncLocal(fid) == 0) {
        return 0;
    }
    for (const ChildEntry &entry : childTrees) {
        CSystemFids *child = entry.Get();
        if (child != nullptr && child->TruncLocals(fid, mode)) {
            return 1;
        }
    }
    return CreateSub(fid, mode);
}

void CSystemFids::RefreshVirtualRecursive(int hasCustomLoader) {
    for (CSystemFids *folder = this; folder != nullptr;
         folder = folder->parent) {
        const bool next = folder->HasCustomLoaderBelow(hasCustomLoader) != 0;
        if (folder->virtualRefresh == next) {
            return;
        }
        folder->virtualRefresh = next;
        hasCustomLoader = next ? 1 : 0;
    }
}

unsigned long CSystemFids::GetTravelInfo(CMwNod *nod) {
    return nod != nullptr ? nod->fidsTravelInfo : 0ul;
}

void CSystemFids::SetTravelInfo(CMwNod *nod, unsigned long travelInfo) {
    if (nod != nullptr) {
        nod->fidsTravelInfo = travelInfo;
    }
}

int CSystemFids::FolderNameMatchesDirectory(
        const CFastStringInt &directory) const {
    const auto *folder = dynamic_cast<const CSystemFidsFolder *>(this);
    if (folder == nullptr) {
        return 0;
    }
    const CFastStringInt &childName = folder->DirName();
    const SStringParamInt directoryParam = directory.Param();
    return childName.Length() + 1u == directory.Length() &&
           (!childName.Empty() || directory.Empty()) &&
           childName.CompareNoCase(directoryParam, childName.Length()) == 0;
}

int CSystemFids::IsParentDirectorySegment(
        const CFastStringInt &directory) {
    const SStringParamInt parentDirectory = {u"..\\", 3u, 1u};
    return directory.Length() == 3u &&
           directory.CompareNoCase(parentDirectory, 0u) == 0;
}

CSystemFids *CSystemFids::FindChildTreeByDirectory(
        const CFastStringInt &directory) {
    const auto found = std::find_if(
            childTrees.begin(), childTrees.end(),
            [&directory](const ChildEntry &entry) {
                CSystemFids *child = entry.Get();
                return child != nullptr &&
                       child->FolderNameMatchesDirectory(directory);
            });
    return found != childTrees.end() ? found->Get() : nullptr;
}

CSystemFids *CSystemFids::FindOneLocationDown(
        const CFastStringInt &directory,
        int browseMissing,
        EFindWay findWay) {
    (void)browseMissing;
    if (IsParentDirectorySegment(directory)) {
        return findWay == FindWay_AllowParent ? parent : nullptr;
    }
    return FindChildTreeByDirectory(directory);
}

CSystemFids *CSystemFids::FindLocationDown(
        const CFastStringInt &directory,
        int browseMissing,
        EFindWay findWay) {
    CSystemFids *location = this;
    unsigned long offset = 0ul;
    CFastStringInt segment;
    while (location != nullptr &&
           CSystemFileName::SplitFirstDirectory(
                   directory, segment, offset) != 0) {
        location = location->FindOneLocationDown(
                segment, browseMissing, findWay);
    }
    return location;
}

CSystemFid *CSystemFids::FindFidFromBaseName(
        const CFastStringInt &filename,
        const AssetAudience &audience) {
    CSystemFids *location = this;
    unsigned long offset = 0ul;
    CFastStringInt segment;
    while (CSystemFileName::SplitFirstDirectory(
                   filename, segment, offset) != 0) {
        location = location->FindOneLocationDown(
                segment, 0, FindWay_ChildrenOnly);
        if (location == nullptr) {
            return nullptr;
        }
    }
    if (segment.Empty()) {
        return nullptr;
    }

    const SStringParamInt segmentParam = segment.Param();
    for (const FidEntry &entry : location->leaves) {
        CSystemFid *candidate = entry.Get();
        auto *candidateFile = dynamic_cast<CSystemFidFile *>(candidate);
        if (candidateFile == nullptr) {
            continue;
        }
        const CFastStringInt &candidateName = candidateFile->FileName();
        if (candidateName.CompareNoCase(segmentParam, segment.Length()) != 0) {
            continue;
        }
        CFastStringInt shortBaseName;
        CSystemFileName::ExtractShortBaseName(candidateName, shortBaseName);
        const bool exactNameMatches =
                candidateName.Length() == segment.Length() &&
                candidateName.CompareNoCase(segmentParam, 0u) == 0;
        const bool shortNameMatches =
                segment.Length() == shortBaseName.Length() &&
                shortBaseName.CompareNoCase(segmentParam, 0u) == 0;
        if ((exactNameMatches || shortNameMatches) &&
            candidate->MatchesAudience(audience)) {
            return candidate;
        }
    }
    return nullptr;
}

CSystemFid *CSystemFids::FindFid(
        const CFastStringInt &filename,
        int browseMissing,
        EFindWay findWay) {
    CSystemFids *location = this;
    unsigned long offset = 0ul;
    CFastStringInt segment;
    while (CSystemFileName::SplitFirstDirectory(
                   filename, segment, offset) != 0) {
        location = location->FindOneLocationDown(
                segment, browseMissing, findWay);
        if (location == nullptr) {
            return nullptr;
        }
    }
    const SStringParamInt segmentParam = segment.Param();
    const auto found = std::find_if(
            location->leaves.begin(), location->leaves.end(),
            [&segmentParam](const FidEntry &entry) {
                CSystemFid *candidate = entry.Get();
                auto *file = dynamic_cast<CSystemFidFile *>(candidate);
                return file != nullptr && file->FileNameEquals(segmentParam);
            });
    return found != location->leaves.end() ? found->Get() : nullptr;
}
