#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "engine/resources/static_solid_asset.h"
struct StaticSolidArchiveAssetSource {
    std::string modelName;
    std::string plainPackPath;
    std::string selectedDescriptorPath;
    bool loadable = false;
};

class StaticSolidArchiveReferenceCatalog
        : private StaticSolidAssetSourceRegistry {
public:
    StaticSolidAssetReference RegisterArchivePath(
            std::string_view archivePath);
    StaticSolidAssetReference RegisterMobilSource(
            std::string_view modelName,
            std::string_view plainPackPath,
            std::string_view selectedDescriptorPath,
            bool loadable);

    const StaticSolidArchiveAssetSource *Source(
            const StaticSolidAssetReference &reference) const;
    const std::string &SelectedDescriptorPath(
            const StaticSolidAssetReference &reference) const;
    const std::string &ModelName(
            const StaticSolidAssetReference &reference) const;
    const std::string &PlainPackPath(
            const StaticSolidAssetReference &reference) const;
    bool IsLoadable(const StaticSolidAssetReference &reference) const;
    void Bind(StaticSolidAssetReference &reference,
              StaticSolidAssetHandle asset) const;

private:
    StaticSolidAssetReference Register(StaticSolidArchiveAssetSource source);

    std::vector<StaticSolidArchiveAssetSource> sources_;
};
