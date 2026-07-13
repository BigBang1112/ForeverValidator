#include "format/pack/block_info_catalog/installed_pack_asset_repository.h"
#include <utility>
#include <vector>

#include "format/pack/block_info_catalog/block_info_asset_store.h"
#include "engine/resources/block_info_catalog.h"
#include "format/pack/block_info_catalog/block_info_catalog_loader.h"
#include "format/pack/block_info_catalog/collection_surface_replacement_pairs.h"
#include "format/pack/block_info_catalog/installed_pack_asset_repository_internal.h"
#include "format/materials/material_pack_repository.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/pack/block_info_catalog/replay_collection_zone_sources.h"
#include "format/static_solid/static_solid_archive_reference.h"
#include <new>
namespace {

CatalogConstructionZoneKind ToSemanticZoneKind(
        CGameCtnReplayConstructionZoneKind kind) {
    switch (kind) {
        case CGameCtnReplayConstructionZoneKind::Flat:
            return CatalogConstructionZoneKind::Flat;
        case CGameCtnReplayConstructionZoneKind::Frontier:
            return CatalogConstructionZoneKind::Frontier;
        case CGameCtnReplayConstructionZoneKind::Other:
            return CatalogConstructionZoneKind::Other;
        case CGameCtnReplayConstructionZoneKind::None:
            return CatalogConstructionZoneKind::None;
    }
    return CatalogConstructionZoneKind::None;
}

struct LoadedCollection {
    std::string name;
    CGameCtnReplayCollectionZoneSources zoneSources;
    CollectionReplacementPairs replacementPairs;
    CatalogCollectionDefinition definition;
};

}  // namespace

struct InstalledPackAssetRepository::Impl {
    std::vector<std::byte> pakBytes;
    std::vector<std::byte> packlistBytes;
    CPlugFilePack pack;
    bool packReady = false;
    BlockInfoCatalog catalog;
    bool catalogReady = false;
    std::unique_ptr<BlockInfoAssetStore> blockInfos;
    std::unique_ptr<MaterialPackRepository> materials;
    StaticSolidArchiveReferenceCatalog solidReferences;
    std::vector<std::unique_ptr<LoadedCollection>> collections;
};

InstalledPackAssetRepository::InstalledPackAssetRepository()
        : impl_(std::make_unique<Impl>()) {}

InstalledPackAssetRepository::~InstalledPackAssetRepository() = default;

std::unique_ptr<ReplayAssetRepository> OpenReplayAssetRepository(
        const std::byte *pakBytes,
        std::size_t pakByteCount,
        const std::byte *packlistBytes,
        std::size_t packlistByteCount) {
    try {
        auto repository = std::make_unique<InstalledPackAssetRepository>();
        if (!repository->Configure(pakBytes,
                                   pakByteCount,
                                   packlistBytes,
                                   packlistByteCount)) {
            return {};
        }
        return repository;
    } catch (const std::bad_alloc &) {
        return {};
    }
}

bool InstalledPackAssetRepository::Configure(
        const std::byte *pakBytes,
        std::size_t pakByteCount,
        const std::byte *packlistBytes,
        std::size_t packlistByteCount) {
    if (pakBytes == nullptr || pakByteCount == 0u ||
        packlistBytes == nullptr || packlistByteCount == 0u) {
        Clear();
        return false;
    }
    try {
        auto next = std::make_unique<Impl>();
        next->pakBytes.assign(pakBytes, pakBytes + pakByteCount);
        next->packlistBytes.assign(
                packlistBytes, packlistBytes + packlistByteCount);
        impl_ = std::move(next);
        return true;
    } catch (const std::bad_alloc &) {
        Clear();
        return false;
    }
}

void InstalledPackAssetRepository::Clear() {
    impl_ = std::make_unique<Impl>();
}

bool InstalledPackAssetRepository::IsConfigured() const {
    return impl_ != nullptr && !impl_->pakBytes.empty() &&
           !impl_->packlistBytes.empty();
}

bool InstalledPackAssetRepository::EnsurePack() {
    if (!IsConfigured()) {
        return false;
    }
    if (impl_->packReady) {
        return true;
    }
    if (!impl_->pack.OpenFromMemory(
                impl_->pakBytes.data(),
                impl_->pakBytes.size(),
                impl_->packlistBytes.data(),
                impl_->packlistBytes.size())) {
        return false;
    }
    impl_->packReady = true;
    impl_->blockInfos = std::make_unique<BlockInfoAssetStore>(
            impl_->pack, impl_->solidReferences);
    impl_->materials = std::make_unique<MaterialPackRepository>(impl_->pack);
    return true;
}

const BlockInfoCatalog *InstalledPackAssetRepository::Catalog() {
    if (!EnsurePack() || impl_->blockInfos == nullptr) {
        return nullptr;
    }
    if (!impl_->catalogReady) {
        if (!LoadBlockInfoCatalog(
                    impl_->pack, *impl_->blockInfos, impl_->catalog)) {
            return nullptr;
        }
        impl_->catalogReady = true;
    }
    return &impl_->catalog;
}

CGameCtnBlockInfo *InstalledPackAssetRepository::BlockInfo(
        BlockInfoAssetHandle asset) {
    return Catalog() != nullptr ? impl_->blockInfos->Load(asset) : nullptr;
}

CGameCtnBlockInfo *InstalledPackAssetRepository::ArchiveBlockInfo(
        BlockInfoAssetHandle asset) {
    return Catalog() != nullptr
            ? impl_->blockInfos->Load(asset, true)
            : nullptr;
}

CSceneMobil *InstalledPackAssetRepository::Mobil(
        BlockInfoAssetHandle asset,
        bool ground,
        u32 variant) {
    return Catalog() != nullptr
            ? impl_->blockInfos->Mobil(asset, ground, variant)
            : nullptr;
}

std::optional<std::string>
InstalledPackAssetRepository::FirstGroundSurface(
        BlockInfoAssetHandle asset) {
    return Catalog() != nullptr
            ? impl_->blockInfos->FirstGroundSurface(asset)
            : std::nullopt;
}

std::optional<CatalogCollectionDefinition>
InstalledPackAssetRepository::Collection(std::string_view name) {
    if (name.empty() || !EnsurePack()) {
        return std::nullopt;
    }
    for (const auto &collection : impl_->collections) {
        if (collection->name == name) {
            return collection->definition;
        }
    }

    try {
        auto collection = std::make_unique<LoadedCollection>();
        collection->name.assign(name.data(), name.size());
        if (!collection->zoneSources.LoadFromCatalogCollection(
                    &impl_->pack, collection->name.c_str()) ||
            !collection->replacementPairs.LoadCatalogCollection(
                    &impl_->pack, collection->name.c_str())) {
            return std::nullopt;
        }
        collection->definition.name = collection->name;
        collection->definition.defaultBaseIdentifier =
                collection->zoneSources.DefaultBaseIdentifier();
        collection->definition.defaultZoneKind = ToSemanticZoneKind(
                collection->zoneSources.DefaultZoneKind());
        CatalogCollectionDefinition result = collection->definition;
        impl_->collections.push_back(std::move(collection));
        return result;
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
}

bool InstalledPackAssetRepository::HasSurfaceReplacement(
        std::string_view collection,
        std::string_view sourceSurface,
        std::string_view targetSurface) {
    if (!Collection(collection) || sourceSurface.empty() ||
        targetSurface.empty()) {
        return false;
    }
    for (const auto &loaded : impl_->collections) {
        if (loaded->name != collection) {
            continue;
        }
        const std::string source(sourceSurface);
        const std::string target(targetSurface);
        return loaded->replacementPairs.Contains(source.c_str(),
                                                  target.c_str()) != 0;
    }
    return false;
}

std::optional<ResolvedMaterialDefinition>
InstalledPackAssetRepository::ResolveMaterial(std::string_view identifier) {
    return EnsurePack() && impl_->materials != nullptr
            ? impl_->materials->Resolve(identifier)
            : std::nullopt;
}

std::optional<ResolvedMaterialDefinition>
InstalledPackAssetRepository::ResolveMaterialPath(std::string_view plainPath) {
    return EnsurePack() && impl_->materials != nullptr
            ? impl_->materials->ResolvePath(plainPath)
            : std::nullopt;
}

const CPlugFilePack *InstalledPackAssetRepositoryAccess::Pack(
        InstalledPackAssetRepository &repository) {
    return repository.EnsurePack() ? &repository.impl_->pack : nullptr;
}

StaticSolidArchiveReferenceCatalog &
InstalledPackAssetRepositoryAccess::StaticSolidReferences(
        InstalledPackAssetRepository &repository) {
    return repository.impl_->solidReferences;
}
