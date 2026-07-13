#include "engine/resources/system_manager_file.h"
#include "engine/resources/system_file_operations.h"
#include "engine/resources/system_fid_file.h"
#include "engine/resources/system_fid_memory.h"
#include "engine/resources/system_fids.h"
CSystemManagerFile::CSystemManagerFile(void) = default;

CSystemManagerFile::~CSystemManagerFile(void) = default;

CSystemFidsFolder *CSystemManagerFile::CreateFidsFolder(void) const {
    return DoCreateFidsFolder();
}

CSystemFidFile *CSystemManagerFile::CreateFidFile(void) const {
    return DoCreateFidFile();
}

CSystemFidFile *CSystemManagerFile::CreateFidResourceFile(void) const {
    CSystemFidFile *fid = DoCreateFidFile();
    if (fid != nullptr) {
        fid->resourceFile = true;
    }
    return fid;
}

CSystemFidMemory *CSystemManagerFile::CreateFidMemory(void) const {
    return new CSystemFidMemory();
}

CSystemFid *CSystemManagerFile::CreateFidFromType(
        CSystemFid::EFidType type) const {
    switch (type) {
        case CSystemFid::FidType_File:
            return CreateFidFile();
        case CSystemFid::FidType_Memory:
            return CreateFidMemory();
        case CSystemFid::FidType_ResourceFile:
            return CreateFidResourceFile();
    }
    return nullptr;
}

CSystemFidsFolder *CSystemManagerFile::DoCreateFidsFolder(void) const {
    return new CSystemFidsFolder();
}

CSystemFidFile *CSystemManagerFile::DoCreateFidFile(void) const {
    return new CSystemFidFile();
}

int CSystemManagerFile::UpdateFidProps(
        CSystemFidFile *fid,
        const CFastStringInt &fullName) {
    if (fid == nullptr) {
        return 1;
    }
    const tmnf::platform::SystemFileOperations *operations =
            tmnf::platform::GetSystemFileOperations();
    const tmnf::platform::SystemFileMetadata metadata = operations != nullptr
            ? operations->ReadMetadata(fullName)
            : tmnf::platform::SystemFileMetadata{};
    fid->StoreFileMetadata(
            metadata.exists, metadata.readOnly, metadata.byteSize);
    return 1;
}
