#pragma once

#include <memory>
#include <cstddef>
#include <string_view>

#include "format/assets/replay_asset_repository.h"
struct InstalledPackAssetRepositoryAccess;

class InstalledPackAssetRepository final : public ReplayAssetRepository {
public:
    InstalledPackAssetRepository();
    ~InstalledPackAssetRepository() override;

    InstalledPackAssetRepository(
            const InstalledPackAssetRepository &) = delete;
    InstalledPackAssetRepository &operator=(
            const InstalledPackAssetRepository &) = delete;

    bool Configure(const std::byte *pakBytes,
                   std::size_t pakByteCount,
                   const std::byte *packlistBytes,
                   std::size_t packlistByteCount);
    void Clear();
    bool IsConfigured() const;

    const BlockInfoCatalog *Catalog() override;
    CGameCtnBlockInfo *BlockInfo(BlockInfoAssetHandle asset) override;
    CGameCtnBlockInfo *ArchiveBlockInfo(BlockInfoAssetHandle asset);
    CSceneMobil *Mobil(BlockInfoAssetHandle asset,
                       bool ground,
                       u32 variant) override;
    std::optional<std::string> FirstGroundSurface(
            BlockInfoAssetHandle asset) override;
    std::optional<CatalogCollectionDefinition> Collection(
            std::string_view name) override;
    bool HasSurfaceReplacement(std::string_view collection,
                               std::string_view sourceSurface,
                               std::string_view targetSurface) override;
    std::optional<ResolvedMaterialDefinition> ResolveMaterial(
            std::string_view identifier) override;
    std::optional<ResolvedMaterialDefinition> ResolveMaterialPath(
            std::string_view plainPath) override;
    bool BuildStaticScene(
            const CGameCtnReplayMapInput &mapInput,
            const ReplaySceneBlockPlacements &placements,
            StaticSceneModelCollection *out) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    bool EnsurePack();

    friend struct InstalledPackAssetRepositoryAccess;
};
