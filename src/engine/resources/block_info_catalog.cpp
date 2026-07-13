#include "engine/resources/block_info_catalog.h"
#include <utility>
#include <new>

void BlockInfoCatalog::Clear() {
    entries_.clear();
}

bool BlockInfoCatalog::Add(BlockInfoCatalogEntry entry) {
    try {
        entries_.push_back(std::move(entry));
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

std::size_t BlockInfoCatalog::Size() const {
    return entries_.size();
}

const BlockInfoCatalogEntry *BlockInfoCatalog::FindForBlock(
        const CGameCtnReplayMapInputBlock &block,
        bool *ambiguous) const {
    if (ambiguous != nullptr) {
        *ambiguous = false;
    }
    if (!block.Identifier().HasName()) {
        return nullptr;
    }

    const std::string &collection = block.Collection().Name();
    const BlockInfoCatalogEntry *match = nullptr;
    std::size_t matchCount = 0u;
    for (const BlockInfoCatalogEntry &entry : entries_) {
        if (!block.Identifier().NameEquals(entry.identifier) ||
            (!collection.empty() && collection != entry.collection)) {
            continue;
        }
        match = &entry;
        ++matchCount;
        if (matchCount > 1u && collection.empty()) {
            if (ambiguous != nullptr) {
                *ambiguous = true;
            }
            return nullptr;
        }
    }
    return matchCount == 1u ? match : nullptr;
}

const BlockInfoCatalogEntry *
BlockInfoCatalog::FindForIdentifierAndCollection(
        std::string_view identifier,
        std::string_view collection) const {
    const BlockInfoCatalogEntry *match = nullptr;
    std::size_t matchCount = 0u;
    for (const BlockInfoCatalogEntry &entry : entries_) {
        if (entry.identifier != identifier || entry.collection != collection) {
            continue;
        }
        match = &entry;
        ++matchCount;
    }
    return matchCount == 1u ? match : nullptr;
}

std::optional<std::string> BlockInfoCatalog::CollectionNameForMapInput(
        const CGameCtnReplayMapInput &mapInput) const {
    for (const CGameCtnReplayMapInputBlock &block : mapInput.Blocks()) {
        const BlockInfoCatalogEntry *entry = FindForBlock(block);
        if (entry != nullptr && !entry->collection.empty()) {
            return entry->collection;
        }
    }
    return std::nullopt;
}
