#pragma once

#include "engine/resources/catalog_asset_repository.h"
#include "engine/game/replay_challenge_map_input.h"
#include "simulation/replay/replay_scene_definition.h"
bool BuildReplaySceneDefinition(
        const CGameCtnReplayMapInput &mapInput,
        CatalogAssetRepository &assets,
        ReplaySceneDefinition &scene);
