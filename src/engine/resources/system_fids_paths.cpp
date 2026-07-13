#include "engine/resources/system_fids.h"
#include "engine/resources/system_engine.h"
#include "engine/resources/system_fid.h"
#include "engine/resources/system_file_name.h"
#include <cstring>

namespace {

constexpr const char *DriveNames[CSystemEngine::DriveSlotCount] = {
    ":resource:\\",
    ":user:\\",
    ":data:\\",
    ":shared:\\",
    ":temp:\\",
};

}

CSystemFids::CSystemFids(void)
        : parent(nullptr) {}

CSystemFids::~CSystemFids(void) {
    if (parent != nullptr) {
        parent->RemoveTreeSafe(this);
    }

    for (const ChildEntry &entry : childTrees) {
        CSystemFids *child = entry.Get();
        if (child != nullptr && child->parent == this) {
            child->parent = nullptr;
        }
    }
    for (const FidEntry &entry : leaves) {
        CSystemFid *fid = entry.Get();
        if (fid != nullptr && fid->location == this) {
            fid->location = nullptr;
        }
    }
    SetTravelInfo(this, 0u);
}

void CSystemFids::GetFullName(
        CFastStringInt &outName,
        int fullName) const {
    (void)outName;
    (void)fullName;
}

int CSystemFids::GetFullNameUpTo(
        CFastStringInt &outName,
        const CSystemFids *upToFids) const {
    (void)outName;
    (void)upToFids;
    return 1;
}

CSystemFidsFolder::CSystemFidsFolder(void) = default;

CSystemFidsFolder::~CSystemFidsFolder(void) = default;

void CSystemFidsFolder::GetFullName(
        CFastStringInt &outName,
        int fullName) const {
    CSystemFids *owner = Parent();
    if (owner != nullptr) {
        owner->GetFullName(outName, fullName);
    }
    CSystemFileName::ConcatDirectory(outName, dirName);
}

int CSystemFidsFolder::GetFullNameUpTo(
        CFastStringInt &outName,
        const CSystemFids *upToFids) const {
    if (upToFids == this) {
        return 1;
    }
    CSystemFids *owner = Parent();
    if (owner == nullptr ||
        owner->GetFullNameUpTo(outName, upToFids) == 0) {
        return 0;
    }
    CSystemFileName::ConcatDirectory(outName, dirName);
    return 1;
}

void CSystemFidsFolder::UpdateNameDown(void) {
    UpdatedDirName();
    CSystemFids::UpdateNameDown();
}

void CSystemFidsFolder::SetDirName(const CFastStringInt &name) {
    dirName = name;
    CSystemFileName::StripTrailingSlash(dirName);
    UpdateNameDown();
}

void CSystemFidsFolder::UpdatedDirName(void) {}

const CFastStringInt &CSystemFidsFolder::DirName(void) const {
    return dirName;
}

CSystemFidsDrive::CSystemFidsDrive(void) = default;

CSystemFidsDrive::~CSystemFidsDrive(void) = default;

void CSystemFidsDrive::GetFullName(
        CFastStringInt &outName,
        int fullName) const {
    if (fullName != 0 && !logicalPrefix.Empty()) {
        outName = logicalPrefix;
        return;
    }

    outName.Clear();
    CSystemFileName::ConcatDirectory(outName, DirName());
}

int CSystemFidsDrive::GetFullNameUpTo(
        CFastStringInt &outName,
        const CSystemFids *upToFids) const {
    (void)outName;
    return upToFids == this;
}

void CSystemFidsDrive::SetDriveName(const CFastStringInt &name) {
    SetDirName(name);
}

void CSystemFidsDrive::SetLogicalPrefix(const char *prefix) {
    logicalPrefix.SetString({
        prefix,
        prefix != nullptr ? static_cast<u32>(std::strlen(prefix)) : 0u,
    });
}

const char *CSystemEngine::GetDriveName(
        const CSystemFidsDrive *drive) const {
    if (drive == nullptr) {
        return ":null:\\";
    }
    for (u32 index = 0u; index < DriveSlotCount; ++index) {
        if (drives[index].get() == drive) {
            return DriveNames[index];
        }
    }
    return ":???:\\";
}

void CSystemEngine::BuildDriveName(
        u32 index,
        CSystemFidsDrive *drive,
        int hasDrivePrefix,
        CFastStringInt &outName) const {
    outName.Clear();
    if (hasDrivePrefix != 0) {
        const char *prefix = DriveNames[index];
        outName.SetString(
                {prefix, static_cast<u32>(std::strlen(prefix))});
        return;
    }
    drive->GetFullName(outName, 0);
}

int CSystemEngine::TryDriveForRelativeName(
        u32 index,
        const CFastStringInt &filename,
        int hasDrivePrefix,
        CSystemFidsDrive *&outDrive,
        CFastStringInt &outRelativeName) const {
    CSystemFidsDrive *drive = ((index) < DriveSlotCount ? drives[(index)].get() : nullptr);
    if (drive == nullptr) {
        return 0;
    }

    CFastStringInt driveName;
    BuildDriveName(index, drive, hasDrivePrefix, driveName);
    if (CSystemFileName::GetRelativeName(
                filename,
                driveName,
                outRelativeName) == 0) {
        return 0;
    }
    outDrive = drive;
    return 1;
}

int CSystemEngine::FindDriveAndRelName(
        const CFastStringInt &filename,
        CSystemFidsDrive *&outDrive,
        CFastStringInt &outRelativeName) {
    const int hasDrivePrefix =
            !filename.Empty() && filename.Data()[0] == ':';
    for (u32 index = 0u; index < DriveSlotCount; ++index) {
        if (TryDriveForRelativeName(
                    index,
                    filename,
                    hasDrivePrefix,
                    outDrive,
                    outRelativeName) != 0) {
            return 1;
        }
    }

    outDrive = nullptr;
    outRelativeName.Clear();
    return 0;
}
