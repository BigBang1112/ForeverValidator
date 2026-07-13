#include "format/static_solid/static_solid_archive_reference.h"
#include <utility>
#include <new>

StaticSolidAssetReference
StaticSolidArchiveReferenceCatalog::RegisterArchivePath(
        std::string_view archivePath) {
    return Register({std::string{},
                     std::string(archivePath),
                     std::string(archivePath),
                     true});
}

StaticSolidAssetReference
StaticSolidArchiveReferenceCatalog::RegisterMobilSource(
        std::string_view modelName,
        std::string_view plainPackPath,
        std::string_view selectedDescriptorPath,
        bool loadable) {
    return Register({std::string(modelName),
                     std::string(plainPackPath),
                     std::string(selectedDescriptorPath),
                     loadable});
}

StaticSolidAssetReference StaticSolidArchiveReferenceCatalog::Register(
        StaticSolidArchiveAssetSource source) {
    if (source.selectedDescriptorPath.empty()) {
        return {};
    }
    for (u32 index = 0u; index < sources_.size(); ++index) {
        const StaticSolidArchiveAssetSource &existing = sources_[index];
        if (existing.modelName == source.modelName &&
            existing.plainPackPath == source.plainPackPath &&
            existing.selectedDescriptorPath == source.selectedDescriptorPath &&
            existing.loadable == source.loadable) {
            return StaticSolidAssetReference::Request(
                    HandleForStorageIndex(index));
        }
    }
    if (sources_.size() >= UINT32_MAX) {
        return {};
    }
    try {
        sources_.push_back(std::move(source));
    } catch (const std::bad_alloc &) {
        return {};
    }
    return StaticSolidAssetReference::Request(
            HandleForStorageIndex(
                    static_cast<u32>(sources_.size() - 1u)));
}

const StaticSolidArchiveAssetSource *
StaticSolidArchiveReferenceCatalog::Source(
        const StaticSolidAssetReference &reference) const {
    const StaticSolidAssetSourceHandle source = reference.Source();
    return source.IsValid() && StorageIndex(source) < sources_.size()
            ? &sources_[StorageIndex(source)]
            : nullptr;
}

const std::string &
StaticSolidArchiveReferenceCatalog::SelectedDescriptorPath(
        const StaticSolidAssetReference &reference) const {
    static const std::string empty;
    const StaticSolidArchiveAssetSource *source = Source(reference);
    return source != nullptr ? source->selectedDescriptorPath : empty;
}

const std::string &StaticSolidArchiveReferenceCatalog::ModelName(
        const StaticSolidAssetReference &reference) const {
    static const std::string empty;
    const StaticSolidArchiveAssetSource *source = Source(reference);
    return source != nullptr ? source->modelName : empty;
}

const std::string &StaticSolidArchiveReferenceCatalog::PlainPackPath(
        const StaticSolidAssetReference &reference) const {
    static const std::string empty;
    const StaticSolidArchiveAssetSource *source = Source(reference);
    return source != nullptr ? source->plainPackPath : empty;
}

bool StaticSolidArchiveReferenceCatalog::IsLoadable(
        const StaticSolidAssetReference &reference) const {
    const StaticSolidArchiveAssetSource *source = Source(reference);
    return source != nullptr && source->loadable;
}

void StaticSolidArchiveReferenceCatalog::Bind(
        StaticSolidAssetReference &reference,
        StaticSolidAssetHandle asset) const {
    reference.Resolve(std::move(asset));
}
