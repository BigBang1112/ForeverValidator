#include "engine/resources/static_solid_asset.h"
#include <utility>

StaticSolidPrototype::StaticSolidPrototype(CPlugSolid *solid)
        : solid_(solid) {}

StaticSolidPrototype StaticSolidPrototype::Compose(
        std::unique_ptr<CPlugTree> tree,
        const StaticSolidPrototype &physicalSource) {
    if (tree == nullptr || !physicalSource.IsValid()) {
        return {};
    }

    CMwNodRef<CPlugSolid> solid = MakeMwNod<CPlugSolid>();
    const CPlugPhysicalObject *physical = physicalSource.Physical();
    if (physical != nullptr) {
        solid->Physical().CopyFrom(*physical);
    }
    solid->SetOwnedTree(std::move(tree), 0);
    return StaticSolidPrototype(solid.Get());
}

bool StaticSolidPrototype::IsValid() const {
    return solid_.Get() != nullptr && solid_->CollisionTree() != nullptr;
}

CPlugSolid *StaticSolidPrototype::SourceSolid() const {
    return solid_.Get();
}

CMwNodRef<CPlugSolid> StaticSolidPrototype::CreateInstance() const {
    return IsValid() ? CMwNodRef<CPlugSolid>(solid_->CreateModelInstance())
                     : CMwNodRef<CPlugSolid>();
}

std::unique_ptr<CPlugTree> StaticSolidPrototype::CreateTreeInstance() const {
    return IsValid()
            ? std::unique_ptr<CPlugTree>(
                      solid_->CollisionTree()
                              ->InternalCreateSolidModelInstance())
            : nullptr;
}

const CPlugPhysicalObject *StaticSolidPrototype::Physical() const {
    return solid_.Get() != nullptr ? &solid_->Physical() : nullptr;
}

CMwNodRef<CPlugSolid> &StaticSolidAsset::PrototypeSlot(
        StaticSolidMaterialVariant variant) {
    switch (variant) {
    case StaticSolidMaterialVariant::BlockMobil:
        return blockMobilPrototype_;
    case StaticSolidMaterialVariant::Replacement:
        return replacementPrototype_;
    case StaticSolidMaterialVariant::DecorationSkin:
        return decorationSkinPrototype_;
    case StaticSolidMaterialVariant::Warp:
        return warpPrototype_;
    case StaticSolidMaterialVariant::Base:
    default:
        return basePrototype_;
    }
}

const CMwNodRef<CPlugSolid> &StaticSolidAsset::PrototypeSlot(
        StaticSolidMaterialVariant variant) const {
    switch (variant) {
    case StaticSolidMaterialVariant::BlockMobil:
        return blockMobilPrototype_;
    case StaticSolidMaterialVariant::Replacement:
        return replacementPrototype_;
    case StaticSolidMaterialVariant::DecorationSkin:
        return decorationSkinPrototype_;
    case StaticSolidMaterialVariant::Warp:
        return warpPrototype_;
    case StaticSolidMaterialVariant::Base:
    default:
        return basePrototype_;
    }
}

void StaticSolidAsset::SetPrototype(StaticSolidMaterialVariant variant,
                                    CPlugSolid *solid) {
    PrototypeSlot(variant).MwSetNod(solid);
}

StaticSolidPrototype StaticSolidAsset::Prototype(
        StaticSolidMaterialVariant variant) const {
    CPlugSolid *solid = PrototypeSlot(variant).Get();
    if (solid == nullptr && variant != StaticSolidMaterialVariant::Base) {
        solid = basePrototype_.Get();
    }
    return StaticSolidPrototype(solid);
}

bool StaticSolidAsset::HasPrototype(StaticSolidMaterialVariant variant) const {
    return PrototypeSlot(variant).Get() != nullptr;
}

StaticSolidAssetReference StaticSolidAssetReference::Request(
        StaticSolidAssetSourceHandle source) {
    StaticSolidAssetReference reference;
    reference.source_ = source;
    return reference;
}

bool StaticSolidAssetReference::IsSpecified() const {
    return source_.IsValid();
}

bool StaticSolidAssetReference::IsResolved() const {
    return asset_ != nullptr;
}

const StaticSolidAssetHandle &StaticSolidAssetReference::Asset() const {
    return asset_;
}

StaticSolidAssetSourceHandle StaticSolidAssetReference::Source() const {
    return source_;
}

void StaticSolidAssetReference::Resolve(StaticSolidAssetHandle asset) {
    asset_ = std::move(asset);
}
