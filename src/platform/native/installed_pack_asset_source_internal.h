#pragma once

#include <string>

#include <forevervalidator/validation.h>

namespace forevervalidator::native_detail {

struct InstalledPackRoot {
    std::string canonicalPath;
};

Result<InstalledPackRoot> ResolveInstalledPackRoot(
        const std::string &packDirectory) noexcept;
Result<AssetBytes> ReadInstalledPackAsset(
        const InstalledPackRoot &root,
        const AssetRequest &request) noexcept;

}  // namespace forevervalidator::native_detail
