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
    EBlockType blockType = EBlockType::Classic;
    BlockInfoAssetHandle asset;
    std::optional<u32> collectionLandZoneHeight;
};

class BlockInfoCatalog {
public:
    void Clear();
    bool Add(BlockInfoCatalogEntry entry);

    std::size_t Size() const;
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
