#pragma once

#include "engine/resources/catalog_asset_repository.h"
#include "engine/game/replay_challenge_map_input.h"
#include "simulation/replay/replay_scene_definition.h"
bool ResolveReplaySceneConstructionZones(
        const CatalogCollectionDefinition &collection,
        const BlockInfoCatalog &catalog,
        CatalogAssetRepository &assets,
        ReplaySceneDefinition &scene);
bool ResolveReplaySceneZoneClips(
        const CatalogCollectionDefinition &collection,
        const BlockInfoCatalog &catalog,
        CatalogAssetRepository &assets,
        ReplaySceneDefinition &scene);
bool ResolveReplaySceneZonePylons(
        const CatalogCollectionDefinition &collection,
        const BlockInfoCatalog &catalog,
        CatalogAssetRepository &assets,
        ReplaySceneDefinition &scene);
bool BuildReplaySceneDefinition(
        const CGameCtnReplayMapInput &mapInput,
        CatalogAssetRepository &assets,
        ReplaySceneDefinition &scene);
bool BuildReplaySceneDefinition(
        const CGameCtnReplayMapInput &mapInput,
        CatalogAssetRepository &mapAssets,
        CatalogAssetRepository &decorationAssets,
        ReplaySceneDefinition &scene);
