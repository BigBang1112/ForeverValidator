#include "engine/resources/system_fid.h"
#include <memory>

#include "engine/resources/system_fid_file.h"
#include "engine/resources/system_file.h"
#include "engine/resources/system_fids.h"
CSystemFid::CLoaderFile CSystemFid::s_DefaultLoaderFile;
CSystemFid::CLoaderParametrized CSystemFid::s_DefaultLoaderParametrized;

void CSystemFid::CLoader::OnLoaderOverriden(
        const CSystemFid *fid,
        CLoader *newLoader) {
    (void)fid;
    (void)newLoader;
}

CClassicBuffer *CSystemFid::CLoaderFile::OpenBuffer(
        const CSystemFid *fid,
        CClassicBuffer::EMode mode,
        int forRefs) const {
    (void)forRefs;
    const CSystemFidFile *fileFid =
            dynamic_cast<const CSystemFidFile *>(fid);
    if (fileFid == nullptr) {
        return nullptr;
    }

    CFastStringInt fullName;
    fileFid->GetFullName(fullName, 0, 0);
    if (mode == CClassicBuffer::EMode::Write) {
        const_cast<CSystemFid *>(fid)->RequestWrite();
    }

    auto file = std::make_unique<CSystemFile>();
    if (file->Open(fullName, mode, 0, 1, 0, 0) == 0) {
        return nullptr;
    }
    return file.release();
}

void CSystemFid::CLoaderFile::CloseBuffer(
        const CSystemFid *fid,
        CClassicBuffer *buffer) const {
    (void)fid;
    if (buffer != nullptr) {
        buffer->Close();
        delete buffer;
    }
}

void CSystemFid::CLoaderFile::UpdateFidProps(CSystemFid *fid) const {
    CSystemFidFile *fileFid = dynamic_cast<CSystemFidFile *>(fid);
    if (fileFid == nullptr) {
        return;
    }
    CFastStringInt fullName;
    fileFid->GetFullName(fullName, 0, 0);
    fileFid->UpdateFidPropsFromFile(fullName);
}

CSystemFid *CSystemFid::CLoaderFile::GetRealFileOnDisk(
        CSystemFid *fid) const {
    return fid;
}

CClassicBuffer *CSystemFid::CLoaderParametrized::OpenBuffer(
        const CSystemFid *fid,
        CClassicBuffer::EMode mode,
        int forRefs) const {
    (void)forRefs;
    CSystemFid *loadable = fid->ParametrizedGetLoadableFid();
    if (loadable == fid) {
        return nullptr;
    }
    if (mode == CClassicBuffer::EMode::Write) {
        const_cast<CSystemFid *>(fid)->RequestWrite();
    }
    return loadable->loader->OpenBuffer(loadable, mode, 0);
}

void CSystemFid::CLoaderParametrized::CloseBuffer(
        const CSystemFid *fid,
        CClassicBuffer *buffer) const {
    CSystemFid *loadable = fid->ParametrizedGetLoadableFid();
    if (loadable != fid) {
        loadable->loader->CloseBuffer(loadable, buffer);
    }
}

void CSystemFid::CLoaderParametrized::UpdateFidProps(
        CSystemFid *fid) const {
    CSystemFidFile *fileFid = dynamic_cast<CSystemFidFile *>(fid);
    CSystemFidFile *loadableFile = dynamic_cast<CSystemFidFile *>(
            fid->ParametrizedGetLoadableFid());
    if (fileFid == nullptr || loadableFile == nullptr) {
        return;
    }
    CFastStringInt fullName;
    loadableFile->GetFullName(fullName, 0, 0);
    fileFid->UpdateFidPropsFromFile(fullName);
}

CSystemFid *CSystemFid::CLoaderParametrized::GetRealFileOnDisk(
        CSystemFid *fid) const {
    return fid != nullptr ? fid->ParametrizedGetLoadableFid() : nullptr;
}

void CSystemFid::SetVirtualLoader(
        CLoader *newLoader,
        unsigned long loaderArg) {
    (void)loaderArg;
    if (newLoader == nullptr) {
        newLoader = &s_DefaultLoaderFile;
    }
    if (newLoader == loader) {
        return;
    }

    CLoader *oldLoader = loader;
    if (oldLoader != nullptr &&
        oldLoader != &s_DefaultLoaderFile &&
        oldLoader != &s_DefaultLoaderParametrized) {
        oldLoader->OnLoaderOverriden(this, newLoader);
    }

    CSystemFids *owner = location;
    loader = newLoader;
    if (owner != nullptr) {
        owner->RefreshVirtualRecursive(newLoader != &s_DefaultLoaderFile);
    }
}

CClassicBuffer *CSystemFid::BufferOpen(
        CClassicBuffer::EMode mode) const {
    return BufferOpen(mode, false);
}

CClassicBuffer *CSystemFid::BufferOpen(
        CClassicBuffer::EMode mode,
        bool forReferences) const {
    return loader->OpenBuffer(this, mode, forReferences ? 1 : 0);
}

void CSystemFid::BufferClose(CClassicBuffer *buffer) const {
    loader->CloseBuffer(this, buffer);
}

void CSystemFid::UpdateFidProps(void) {
    if (metadataNeedsRefresh) {
        loader->UpdateFidProps(this);
        metadataNeedsRefresh = false;
    }
}
