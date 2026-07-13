#include "engine/resources/system_fid_file.h"

#include "engine/resources/system_file_name.h"
#include "engine/resources/system_fids.h"
#include "engine/resources/system_manager_file.h"

CSystemFidFile::CSystemFidFile(void)
        : fileName(),
          resourceIndex(std::nullopt),
          byteSize(0u),
          readOnly(true),
          exists(false) {
    metadataNeedsRefresh = true;
}

CSystemFidFile::~CSystemFidFile(void) = default;

int CSystemFidFile::IsEqualFid(CSystemFid *other) {
    CSystemFidFile *otherFile = dynamic_cast<CSystemFidFile *>(other);
    if (otherFile == nullptr || !CSystemFid::IsEqualFid(other)) {
        return 0;
    }
    const CFastStringInt &otherName = otherFile->FileName();
    return fileName.Length() == otherName.Length() &&
           fileName.CompareNoCase(otherName.Param(), 0u) == 0;
}

void CSystemFidFile::CopyFromFid(CSystemFid *other) {
    CSystemFid::CopyFromFid(other);
    CSystemFidFile *otherFile = dynamic_cast<CSystemFidFile *>(other);
    if (otherFile != nullptr) {
        fileName = otherFile->fileName;
    }
    UpdatedFileName();
}

void CSystemFidFile::UpdateNameDown(void) {
    UpdatedFileName();
    CSystemFid::UpdateNameDown();
}

void CSystemFidFile::UpdateFidPropsFromFile(
        const CFastStringInt &fullName) {
    if (!metadataNeedsRefresh) {
        return;
    }
    CSystemManagerFile::UpdateFidProps(this, fullName);
    metadataNeedsRefresh = false;
}

void CSystemFidFile::UpdatedFileName(void) {}

int CSystemFidFile::RequestWrite(void) {
    return CSystemFid::RequestWrite();
}

void CSystemFidFile::ForceUpdateFidProps(void) {
    metadataNeedsRefresh = true;
    UpdateFidProps();
}

void CSystemFidFile::SetFileName(const CFastStringInt &name) {
    fileName = name;
    CSystemFileName::Normalize(fileName, 0);
    metadataNeedsRefresh = true;
    UpdateNameDown();
}

void CSystemFidFile::GetFullName(
        CFastStringInt &outName,
        int fullName,
        int unused) const {
    (void)unused;
    if (Location() != nullptr) {
        Location()->GetFullName(outName, fullName);
    }
    AppendFileNameTo(outName);

    CSystemFid *loadable = ParametrizedGetLoadableFid();
    if (!loadable->UsesDefaultFileLoader()) {
        outName.ConcatBefore({u"<virtual>", 9u, 1u});
    }
}

const CFastStringInt &CSystemFidFile::FileName(void) const {
    return fileName;
}

int CSystemFidFile::FileNameEquals(const SStringParamInt &name) const {
    return fileName.Length() == name.Length() &&
           fileName.CompareNoCase(name, 0u) == 0;
}

void CSystemFidFile::AppendFileNameTo(CFastStringInt &outName) const {
    outName.Concat(fileName.Param());
}

int CSystemFidFile::GetFullNameUpTo(
        CFastStringInt &outName,
        const CSystemFids *upToFids) const {
    if (Location() == nullptr ||
        Location()->GetFullNameUpTo(outName, upToFids) == 0) {
        return 0;
    }
    AppendFileNameTo(outName);
    return 1;
}

uint64_t CSystemFidFile::FileByteSize(void) const {
    return byteSize;
}

int CSystemFidFile::IsReadOnly(void) const {
    return readOnly ? 1 : 0;
}

bool CSystemFidFile::FileExists(void) const {
    return exists;
}

int CSystemFidFile::OSCheckIfExists(void) {
    ForceUpdateFidProps();
    return exists ? 1 : 0;
}

void CSystemFidFile::StoreFileMetadata(
        bool fileExists,
        bool fileReadOnly,
        uint64_t fileByteSize) {
    exists = fileExists;
    readOnly = fileReadOnly;
    byteSize = fileByteSize;
}
