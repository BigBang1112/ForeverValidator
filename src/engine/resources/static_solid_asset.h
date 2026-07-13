#pragma once

#include <memory>

#include "engine/core/cmw_nod.h"
#include "engine/core/engine_types.h"
#include "engine/scene/plug_solid.h"
class StaticSolidAssetSourceRegistry;

enum class StaticSolidMaterialVariant {
    Base,
    BlockMobil,
    Replacement,
    DecorationSkin,
    Warp,
};

class StaticSolidPrototype {
public:
    StaticSolidPrototype() = default;
    explicit StaticSolidPrototype(CPlugSolid *solid);
    static StaticSolidPrototype Compose(
            std::unique_ptr<CPlugTree> tree,
            const StaticSolidPrototype &physicalSource);

    bool IsValid() const;
    CPlugSolid *SourceSolid() const;
    CMwNodRef<CPlugSolid> CreateInstance() const;
    std::unique_ptr<CPlugTree> CreateTreeInstance() const;
    const CPlugPhysicalObject *Physical() const;

private:
    CMwNodRef<CPlugSolid> solid_;
};

class StaticSolidAsset {
public:
    StaticSolidAsset() = default;
    StaticSolidAsset(const StaticSolidAsset &) = delete;
    StaticSolidAsset &operator=(const StaticSolidAsset &) = delete;

    void SetPrototype(StaticSolidMaterialVariant variant, CPlugSolid *solid);
    StaticSolidPrototype Prototype(
            StaticSolidMaterialVariant variant =
                    StaticSolidMaterialVariant::Base) const;
    bool HasPrototype(StaticSolidMaterialVariant variant) const;

private:
    CMwNodRef<CPlugSolid> &PrototypeSlot(StaticSolidMaterialVariant variant);
    const CMwNodRef<CPlugSolid> &PrototypeSlot(
            StaticSolidMaterialVariant variant) const;

    CMwNodRef<CPlugSolid> basePrototype_;
    CMwNodRef<CPlugSolid> blockMobilPrototype_;
    CMwNodRef<CPlugSolid> replacementPrototype_;
    CMwNodRef<CPlugSolid> decorationSkinPrototype_;
    CMwNodRef<CPlugSolid> warpPrototype_;
};

using StaticSolidAssetHandle = std::shared_ptr<const StaticSolidAsset>;

class StaticSolidAssetSourceHandle {
public:
    constexpr StaticSolidAssetSourceHandle() = default;

    constexpr bool IsValid() const { return value_ != 0u; }

    friend constexpr bool operator==(StaticSolidAssetSourceHandle lhs,
                                     StaticSolidAssetSourceHandle rhs) {
        return lhs.value_ == rhs.value_;
    }

    friend constexpr bool operator!=(StaticSolidAssetSourceHandle lhs,
                                     StaticSolidAssetSourceHandle rhs) {
        return !(lhs == rhs);
    }

private:
    friend class StaticSolidAssetSourceRegistry;

    static constexpr StaticSolidAssetSourceHandle FromStorageIndex(
            u32 index) {
        return StaticSolidAssetSourceHandle(index + 1u);
    }
    constexpr u32 StorageIndex() const { return value_ - 1u; }

    explicit constexpr StaticSolidAssetSourceHandle(u32 value)
            : value_(value) {}

    u32 value_ = 0u;
};

class StaticSolidAssetSourceRegistry {
protected:
    static constexpr StaticSolidAssetSourceHandle HandleForStorageIndex(
            u32 index) {
        return StaticSolidAssetSourceHandle::FromStorageIndex(index);
    }

    static constexpr u32 StorageIndex(StaticSolidAssetSourceHandle handle) {
        return handle.StorageIndex();
    }
};

class StaticSolidAssetReference {
public:
    static StaticSolidAssetReference Request(
            StaticSolidAssetSourceHandle source);

    bool IsSpecified() const;
    bool IsResolved() const;
    StaticSolidAssetSourceHandle Source() const;
    const StaticSolidAssetHandle &Asset() const;
    void Resolve(StaticSolidAssetHandle asset);

private:
    StaticSolidAssetSourceHandle source_;
    StaticSolidAssetHandle asset_;
};
