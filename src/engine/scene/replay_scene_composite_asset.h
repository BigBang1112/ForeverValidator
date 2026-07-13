#pragma once

#include <memory>
#include <vector>

#include "engine/scene/replay_scene_placements.h"
class ReplaySceneCompositeAsset {
public:
    static ReplaySceneCompositeAsset ForHelpers(
            const std::vector<ReplaySceneAssetPlacement> &placements);
    static ReplaySceneCompositeAsset ForClip(
            const std::vector<ReplaySceneAssetPlacement> &placements);

    bool Empty() const;
    bool IsComplete() const;
    StaticSolidPrototype TakePrototype();

private:
    enum class RootKind {
        Helpers,
        Clip,
    };

    ReplaySceneCompositeAsset(
            RootKind kind,
            const std::vector<ReplaySceneAssetPlacement> &placements);

    std::unique_ptr<CPlugTree> root_;
    StaticSolidPrototype physicalSource_;
    u32 missingPrototypeCount_ = 0u;
};
