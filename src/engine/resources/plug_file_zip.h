#pragma once

#include <memory>
#include <vector>

#include "engine/resources/plug_file.h"
#include "engine/resources/system_fid.h"
struct CSystemFids;
struct CSystemFidsFolder;
struct CSystemFidFile;
struct CSystemPackDesc;

struct CPlugFileFidContainer : CPlugFile {
    struct SFolderDesc {
        CFastStringInt name;
    };

    struct SFileDesc {
        CFastStringInt name;
        CSystemFidFile *fid = nullptr;
    };

    struct CLoaderFidContainer : CSystemFid::CLoader {
        void OnLoaderOverriden(
                const CSystemFid *fid,
                CSystemFid::CLoader *newLoader) override;
        void UpdateFidProps(CSystemFid *fid) const override;
        CSystemFid *GetRealFileOnDisk(CSystemFid *fid) const override;
    };

    CPlugFileFidContainer(void);
    ~CPlugFileFidContainer(void) override;
    virtual void InstallFids(CSystemFids *owningFids, int recursive);
    virtual void UninstallFids(void);

protected:
    std::unique_ptr<CSystemFidsFolder> contents;
};

struct CPlugFileZip : CPlugFileFidContainer {
    struct CLoaderZip : CLoaderFidContainer {
        CClassicBuffer *OpenBuffer(
                const CSystemFid *fid,
                CClassicBuffer::EMode mode,
                int forRefs) const override;
        void CloseBuffer(
                const CSystemFid *fid,
                CClassicBuffer *buffer) const override;
        CClassicBuffer *OpenBufferLoad(const CSystemFid *fid) const;
        CClassicBuffer *OpenBufferSave(const CSystemFid *fid) const;
        void CloseBufferLoad(
                const CSystemFid *fid,
                CClassicBuffer *buffer) const;
        void CloseBufferSave(
                const CSystemFid *fid,
                CClassicBuffer *buffer) const;
    };

    CPlugFileZip(void);
    ~CPlugFileZip(void) override;
    void InstallFids(CSystemFids *owningFids, int recursive) override;
    void UninstallFids(void) override;

private:
    friend struct CSystemPackDesc;

    CSystemFids *ContentsFolder(void) const;
    int ContentsMissing(void) const;
};
