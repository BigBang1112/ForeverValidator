#pragma once

#include "engine/core/engine_types.h"
class BlockInfoAssetRegistry;

class BlockInfoAssetHandle {
public:
    constexpr BlockInfoAssetHandle() = default;

    constexpr bool IsValid() const { return value_ != 0u; }

    friend constexpr bool operator==(BlockInfoAssetHandle lhs,
                                     BlockInfoAssetHandle rhs) {
        return lhs.value_ == rhs.value_;
    }

    friend constexpr bool operator!=(BlockInfoAssetHandle lhs,
                                     BlockInfoAssetHandle rhs) {
        return !(lhs == rhs);
    }

private:
    friend class BlockInfoAssetRegistry;

    static constexpr BlockInfoAssetHandle FromStorageIndex(u32 index) {
        return BlockInfoAssetHandle(index + 1u);
    }
    constexpr u32 StorageIndex() const { return value_ - 1u; }

    explicit constexpr BlockInfoAssetHandle(u32 value) : value_(value) {}

    u32 value_ = 0u;
};

class BlockInfoAssetRegistry {
protected:
    static constexpr BlockInfoAssetHandle HandleForStorageIndex(u32 index) {
        return BlockInfoAssetHandle::FromStorageIndex(index);
    }

    static constexpr u32 StorageIndex(BlockInfoAssetHandle handle) {
        return handle.StorageIndex();
    }
};
