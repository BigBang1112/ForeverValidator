#pragma once

#include <cstddef>
#include <memory>

#include "engine/resources/catalog_asset_repository.h"
#include "engine/game/material_definition.h"
#include "engine/scene/static_scene_model.h"
class CGameCtnReplayMapInput;
class ReplaySceneBlockPlacements;

// Runtime-facing access to the assets needed to construct one replay scene.
// Installed-pack decoding stays behind this interface in src/format.
class ReplayAssetRepository : public CatalogAssetRepository,
                              public MaterialAssetRepository {
public:
    ~ReplayAssetRepository() override = default;

    virtual bool BuildStaticScene(
            const CGameCtnReplayMapInput &mapInput,
            const ReplaySceneBlockPlacements &placements,
            StaticSceneModelCollection *out) = 0;
};

std::unique_ptr<ReplayAssetRepository> OpenReplayAssetRepository(
        const std::byte *pakBytes,
        std::size_t pakByteCount,
        const std::byte *packlistBytes,
        std::size_t packlistByteCount);
