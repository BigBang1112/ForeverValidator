#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "engine/resources/catalog_asset_handle.h"
#include "engine/core/gm_types.h"
#include "engine/game/replay_challenge_map_input.h"
struct BlockInfoCatalogEntry {
    std::string identifier;
    std::string collection;
    std::string selectedDescriptorPath;
    EBlockType blockType = EBlockType::Classic;
    BlockInfoAssetHandle asset;
    std::optional<u32> collectionLandZoneHeight;
};

struct BlockInfoCatalogLandZoneHeightAssignment {
    std::string identifier;
    std::string alternateIdentifier;
    std::string collection;
    EBlockType blockType = EBlockType::Flat;
    u32 height = 0u;
};

enum class BlockInfoCatalogLandZoneHeightResult {
    Success,
    Missing,
    Ambiguous,
    TypeMismatch,
    Conflict,
    Duplicate,
};

class BlockInfoCatalog {
public:
    void Clear();
    bool Add(BlockInfoCatalogEntry entry);
    BlockInfoCatalogLandZoneHeightResult ApplyCollectionLandZoneHeights(
            const std::vector<BlockInfoCatalogLandZoneHeightAssignment> &
                    assignments);

    std::size_t Size() const;
    const std::vector<BlockInfoCatalogEntry> &Entries() const;
    std::size_t CollectionLandZoneHeightCount(
            std::string_view collection) const;
    const BlockInfoCatalogEntry *FindForBlock(
            const CGameCtnReplayMapInputBlock &block,
            bool *ambiguous = nullptr) const;
    const BlockInfoCatalogEntry *FindForIdentifierAndCollection(
            std::string_view identifier,
            std::string_view collection) const;
    std::optional<std::string> CollectionNameForMapInput(
            const CGameCtnReplayMapInput &mapInput) const;

private:
    std::vector<BlockInfoCatalogEntry> entries_;
};
