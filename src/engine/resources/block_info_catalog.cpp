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

BlockInfoCatalogLandZoneHeightResult
BlockInfoCatalog::ApplyCollectionLandZoneHeights(
        const std::vector<BlockInfoCatalogLandZoneHeightAssignment> &
                assignments) {
    const auto matchesAssignment = [](
            const BlockInfoCatalogEntry &entry,
            const BlockInfoCatalogLandZoneHeightAssignment &assignment) {
        return entry.collection == assignment.collection &&
               (entry.identifier == assignment.identifier ||
                (!assignment.alternateIdentifier.empty() &&
                 entry.identifier == assignment.alternateIdentifier));
    };
    for (std::size_t index = 0u; index < assignments.size(); ++index) {
        const BlockInfoCatalogLandZoneHeightAssignment &assignment =
                assignments[index];
        const BlockInfoCatalogEntry *match = nullptr;
        std::size_t matchCount = 0u;
        for (const BlockInfoCatalogEntry &entry : entries_) {
            if (matchesAssignment(entry, assignment)) {
                match = &entry;
                ++matchCount;
            }
        }
        if (matchCount == 0u) {
            return BlockInfoCatalogLandZoneHeightResult::Missing;
        }
        if (matchCount != 1u) {
            return BlockInfoCatalogLandZoneHeightResult::Ambiguous;
        }
        for (std::size_t previous = 0u; previous < index; ++previous) {
            const BlockInfoCatalogLandZoneHeightAssignment &other =
                    assignments[previous];
            const BlockInfoCatalogEntry *otherMatch = nullptr;
            for (const BlockInfoCatalogEntry &entry : entries_) {
                if (matchesAssignment(entry, other)) {
                    otherMatch = &entry;
                    break;
                }
            }
            if (otherMatch != match) {
                continue;
            }
            if (other.blockType != assignment.blockType ||
                other.height != assignment.height) {
                return BlockInfoCatalogLandZoneHeightResult::Conflict;
            }
            return BlockInfoCatalogLandZoneHeightResult::Duplicate;
        }
        if (match->blockType != assignment.blockType) {
            return BlockInfoCatalogLandZoneHeightResult::TypeMismatch;
        }
        if (match->collectionLandZoneHeight.has_value() &&
            *match->collectionLandZoneHeight != assignment.height) {
            return BlockInfoCatalogLandZoneHeightResult::Conflict;
        }
    }

    for (const BlockInfoCatalogLandZoneHeightAssignment &assignment :
         assignments) {
        for (BlockInfoCatalogEntry &entry : entries_) {
            if (matchesAssignment(entry, assignment)) {
                entry.collectionLandZoneHeight = assignment.height;
                break;
            }
        }
    }
    return BlockInfoCatalogLandZoneHeightResult::Success;
}

std::size_t BlockInfoCatalog::Size() const {
    return entries_.size();
}

const std::vector<BlockInfoCatalogEntry> &BlockInfoCatalog::Entries() const {
    return entries_;
}

std::size_t BlockInfoCatalog::CollectionLandZoneHeightCount(
        std::string_view collection) const {
    std::size_t count = 0u;
    for (const BlockInfoCatalogEntry &entry : entries_) {
        if (entry.collection == collection &&
            entry.collectionLandZoneHeight.has_value()) {
            ++count;
        }
    }
    return count;
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
