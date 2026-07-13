#include "engine/scene/replay_scene_composite_asset.h"
#include <utility>

namespace {

std::unique_ptr<CPlugTree> MakeCompositeRoot(bool clip) {
    auto root = std::make_unique<CPlugTree>();
    if (clip) {
        root->SetUseLocation(1);
        root->SetModelInstance(true);
        root->SetCollisionEnabled(true);
    }
    return root;
}

}  // namespace

ReplaySceneCompositeAsset ReplaySceneCompositeAsset::ForHelpers(
        const std::vector<ReplaySceneAssetPlacement> &placements) {
    return ReplaySceneCompositeAsset(RootKind::Helpers, placements);
}

ReplaySceneCompositeAsset ReplaySceneCompositeAsset::ForClip(
        const std::vector<ReplaySceneAssetPlacement> &placements) {
    return ReplaySceneCompositeAsset(RootKind::Clip, placements);
}

ReplaySceneCompositeAsset::ReplaySceneCompositeAsset(
        RootKind kind,
        const std::vector<ReplaySceneAssetPlacement> &placements)
        : root_(MakeCompositeRoot(kind == RootKind::Clip)) {
    for (const ReplaySceneAssetPlacement &placement : placements) {
        const StaticSolidPrototype prototype = placement.Prototype();
        std::unique_ptr<CPlugTree> child = prototype.CreateTreeInstance();
        if (!prototype.IsValid() || child == nullptr) {
            missingPrototypeCount_++;
            continue;
        }

        if (placement.RelativeTransform().has_value()) {
            child->SetUseLocation(1);
            GmIso4 localIso = child->LocalIso();
            localIso.Mult(*placement.RelativeTransform());
            child->SetLocation(localIso);
        }
        root_->AddOwnedChild(std::move(child));
        if (!physicalSource_.IsValid()) {
            physicalSource_ = prototype;
        }
    }
    if (!physicalSource_.IsValid()) {
        root_.reset();
    } else {
        root_->UpdateBoundingBox(0u);
    }
}

bool ReplaySceneCompositeAsset::Empty() const {
    return root_ == nullptr || !physicalSource_.IsValid();
}

bool ReplaySceneCompositeAsset::IsComplete() const {
    return missingPrototypeCount_ == 0u;
}

StaticSolidPrototype ReplaySceneCompositeAsset::TakePrototype() {
    if (Empty()) {
        return {};
    }
    return StaticSolidPrototype::Compose(
            std::move(root_), physicalSource_);
}
