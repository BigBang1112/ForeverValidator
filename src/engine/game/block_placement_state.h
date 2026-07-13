#pragma once

#include <optional>

#include "engine/core/engine_types.h"
enum class BlockMobilFamily {
    Air,
    Ground,
};

enum class BlockPlacementSource {
    Primary,
    Secondary,
};

class BlockPlacementState {
public:
    BlockPlacementState() = default;

    static BlockPlacementState Create(
            u32 variantIndex,
            std::optional<u32> mobilSelection,
            BlockMobilFamily mobilFamily,
            bool createsMobil,
            bool usesCustomSize,
            BlockPlacementSource source) {
        BlockPlacementState state;
        state.variantIndex_ = variantIndex;
        state.mobilSelection_ = mobilSelection;
        state.mobilFamily_ = mobilFamily;
        state.createsMobil_ = createsMobil;
        state.usesCustomSize_ = usesCustomSize;
        state.source_ = source;
        return state;
    }

    static BlockPlacementState AutomaticBase() {
        return Create(0u,
                      std::nullopt,
                      BlockMobilFamily::Ground,
                      false,
                      false,
                      BlockPlacementSource::Primary);
    }

    u32 VariantIndex() const { return variantIndex_; }
    const std::optional<u32> &MobilSelection() const {
        return mobilSelection_;
    }
    BlockMobilFamily MobilFamily() const { return mobilFamily_; }
    bool UsesGroundMobilFamily() const {
        return mobilFamily_ == BlockMobilFamily::Ground;
    }
    bool CreatesMobil() const { return createsMobil_; }
    bool UsesCustomSize() const { return usesCustomSize_; }
    BlockPlacementSource Source() const { return source_; }

    void SelectMobil(u32 mobilIndex) { mobilSelection_ = mobilIndex; }

    bool operator==(const BlockPlacementState &other) const {
        return variantIndex_ == other.variantIndex_ &&
               mobilSelection_ == other.mobilSelection_ &&
               mobilFamily_ == other.mobilFamily_ &&
               createsMobil_ == other.createsMobil_ &&
               usesCustomSize_ == other.usesCustomSize_ &&
               source_ == other.source_;
    }

    bool operator!=(const BlockPlacementState &other) const {
        return !(*this == other);
    }

private:
    u32 variantIndex_ = 0u;
    std::optional<u32> mobilSelection_;
    BlockMobilFamily mobilFamily_ = BlockMobilFamily::Air;
    bool createsMobil_ = false;
    bool usesCustomSize_ = false;
    BlockPlacementSource source_ = BlockPlacementSource::Primary;
};
