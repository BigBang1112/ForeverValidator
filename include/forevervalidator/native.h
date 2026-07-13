#ifndef FOREVERVALIDATOR_NATIVE_H
#define FOREVERVALIDATOR_NATIVE_H

#include <string>
#include <forevervalidator/validation.h>

namespace forevervalidator {
Result<AssetSource> OpenInstalledPackDirectory(
        const std::string &packDirectory) noexcept;
Result<AssetBytes> ReadNativeReplayFile(
        const std::string &path,
        const ReplayIdentity &identity = {}) noexcept;
}  // namespace forevervalidator

#endif
