#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "engine/core/fast_string.h"
#include "engine/resources/system_fid.h"
struct CSystemFids;

struct CSystemFidFile : CSystemFid {
    CSystemFidFile(void);
    ~CSystemFidFile(void) override;
    int IsEqualFid(CSystemFid *other) override;
    void CopyFromFid(CSystemFid *other) override;
    void UpdateNameDown(void) override;
    virtual void UpdateFidPropsFromFile(const CFastStringInt &fullName);
    virtual void UpdatedFileName(void);
    int RequestWrite(void) override;
    void ForceUpdateFidProps(void);
    void SetFileName(const CFastStringInt &name);
    void GetFullName(
            CFastStringInt &outName,
            int fullName,
            int unused) const;
    int GetFullNameUpTo(
            CFastStringInt &outName,
            const CSystemFids *upToFids) const;
    int IsReadOnly(void) const;
    int OSCheckIfExists(void);

    const CFastStringInt &FileName(void) const;
    int FileNameEquals(const SStringParamInt &name) const;
    void AppendFileNameTo(CFastStringInt &outName) const;
    std::uint64_t FileByteSize(void) const;
    bool FileExists(void) const;

private:
    friend struct CSystemEngine;
    friend struct CSystemManagerFile;

    CFastStringInt fileName;
    std::optional<std::size_t> resourceIndex;
    std::uint64_t byteSize = 0u;
    bool readOnly = true;
    bool exists = false;
    bool resourceFile = false;

    void StoreFileMetadata(
            bool exists,
            bool readOnly,
            std::uint64_t byteSize);
};
