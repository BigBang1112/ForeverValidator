#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "engine/resources/catalog_asset_handle.h"
class BlockInfoCatalog;
class CGameCtnBlockInfo;
class CSceneMobil;

enum class CatalogConstructionZoneKind {
    None,
    Flat,
    Frontier,
    Other,
};

struct CatalogCollectionDefinition {
    std::string name;
    std::optional<std::string> defaultBaseIdentifier;
    CatalogConstructionZoneKind defaultZoneKind =
            CatalogConstructionZoneKind::None;
};

class CatalogAssetRepository {
public:
    virtual ~CatalogAssetRepository() = default;

    virtual const BlockInfoCatalog *Catalog() = 0;
    virtual CGameCtnBlockInfo *BlockInfo(BlockInfoAssetHandle asset) = 0;
    virtual CSceneMobil *Mobil(BlockInfoAssetHandle asset,
                              bool ground,
                              u32 variant) = 0;
    virtual std::optional<std::string> FirstGroundSurface(
            BlockInfoAssetHandle asset) = 0;
    virtual std::optional<CatalogCollectionDefinition> Collection(
            std::string_view name) = 0;
    virtual bool HasSurfaceReplacement(std::string_view collection,
                                       std::string_view sourceSurface,
                                       std::string_view targetSurface) = 0;
};
