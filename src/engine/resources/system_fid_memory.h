#pragma once

#include <memory>
#include <optional>

#include "engine/core/owned_or_borrowed.h"
#include "engine/resources/system_fid.h"
struct CSystemFidMemory : CSystemFid {
    CSystemFidMemory(void);
    ~CSystemFidMemory(void) override;
    int IsEqualFid(CSystemFid *other) override;
    void CopyFromFid(CSystemFid *other) override;

    CClassicBufferMemory *MemoryBuffer(void);
    const CClassicBufferMemory *MemoryBuffer(void) const;
    void BorrowMemoryBuffer(CClassicBufferMemory *buffer);
    void OwnMemoryBuffer(std::unique_ptr<CClassicBufferMemory> buffer);
    bool OwnsMemoryBuffer(void) const;

private:
    std::optional<OwnedOrBorrowed<CClassicBufferMemory>> memoryBuffer;
};
