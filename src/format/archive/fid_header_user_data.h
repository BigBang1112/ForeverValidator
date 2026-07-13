#pragma once

#include <memory>
#include <vector>

#include "engine/resources/system_fid.h"
struct CSystemFid::SHeaderUserData {
    SHeaderUserData(void);
    ~SHeaderUserData(void);

    SHeaderUserData(const SHeaderUserData &) = delete;
    SHeaderUserData &operator=(const SHeaderUserData &) = delete;

    unsigned long ComputeByteSizeTotalInFile(void) const;
    unsigned long ComputeDataSizeTotalInMem(void) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    static HeaderUserDataPtr DecodeCache(
            const unsigned char *source,
            unsigned long size);
    const std::vector<unsigned char> &EncodeCache(void) const;

    friend struct CSystemFid;
};
