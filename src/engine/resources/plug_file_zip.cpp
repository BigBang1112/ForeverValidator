#include "engine/resources/plug_file_zip.h"
#include "engine/resources/system_fids.h"
CPlugFileFidContainer::CPlugFileFidContainer(void) = default;
CPlugFileFidContainer::~CPlugFileFidContainer(void) = default;

void CPlugFileFidContainer::InstallFids(
        CSystemFids *owningFids,
        int recursive) {
    (void)recursive;
    if (contents == nullptr) {
        contents = std::make_unique<CSystemFidsFolder>();
    }
    if (owningFids != nullptr && contents->Parent() != owningFids) {
        owningFids->AddTree(contents.get());
    }
}

void CPlugFileFidContainer::UninstallFids(void) {
    contents.reset();
}

void CPlugFileFidContainer::CLoaderFidContainer::OnLoaderOverriden(
        const CSystemFid *fid,
        CSystemFid::CLoader *newLoader) {
    (void)fid;
    (void)newLoader;
}

void CPlugFileFidContainer::CLoaderFidContainer::UpdateFidProps(
        CSystemFid *fid) const {
    (void)fid;
}

CSystemFid *
CPlugFileFidContainer::CLoaderFidContainer::GetRealFileOnDisk(
        CSystemFid *fid) const {
    return fid;
}

CPlugFileZip::CPlugFileZip(void) = default;
CPlugFileZip::~CPlugFileZip(void) = default;

void CPlugFileZip::InstallFids(CSystemFids *owningFids, int recursive) {
    CPlugFileFidContainer::InstallFids(owningFids, recursive);
}

void CPlugFileZip::UninstallFids(void) {
    CPlugFileFidContainer::UninstallFids();
}

CSystemFids *CPlugFileZip::ContentsFolder(void) const {
    return contents.get();
}

int CPlugFileZip::ContentsMissing(void) const {
    return contents == nullptr;
}

CClassicBuffer *CPlugFileZip::CLoaderZip::OpenBuffer(
        const CSystemFid *fid,
        CClassicBuffer::EMode mode,
        int forRefs) const {
    (void)forRefs;
    return mode == CClassicBuffer::EMode::Read
            ? OpenBufferLoad(fid)
            : OpenBufferSave(fid);
}

void CPlugFileZip::CLoaderZip::CloseBuffer(
        const CSystemFid *fid,
        CClassicBuffer *buffer) const {
    if (buffer == nullptr) {
        return;
    }
    if (buffer->IsStoringSecured()) {
        CloseBufferSave(fid, buffer);
    } else {
        CloseBufferLoad(fid, buffer);
    }
}

CClassicBuffer *CPlugFileZip::CLoaderZip::OpenBufferLoad(
        const CSystemFid *fid) const {
    (void)fid;
    return nullptr;
}

CClassicBuffer *CPlugFileZip::CLoaderZip::OpenBufferSave(
        const CSystemFid *fid) const {
    (void)fid;
    return nullptr;
}

void CPlugFileZip::CLoaderZip::CloseBufferLoad(
        const CSystemFid *fid,
        CClassicBuffer *buffer) const {
    (void)fid;
    (void)buffer;
}

void CPlugFileZip::CLoaderZip::CloseBufferSave(
        const CSystemFid *fid,
        CClassicBuffer *buffer) const {
    (void)fid;
    (void)buffer;
}
