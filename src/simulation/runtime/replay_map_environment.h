#pragma once

#include <optional>

#include "simulation/runtime/replay_environment_definition.h"
#include "engine/resources/catalog_asset_repository.h"
class CGameCtnChallenge;
class ReplaySceneBlockPlacements;
class StaticSceneModelCollection;
struct CGameCtnReplayMapInput;

int BuildReplayWaterDefinition(
        const CGameCtnReplayMapInput &map,
        std::optional<ReplayWaterDefinition> *out);

int BuildReplayWaterDefinition(
        CGameCtnChallenge &challenge,
        const CatalogCollectionDefinition &collection,
        const CatalogDecorationSizeDefinition &decoration,
        std::optional<ReplayWaterDefinition> *out);

int BuildReplayGeometryWaterDefinition(
        CGameCtnChallenge &challenge,
        const CatalogCollectionDefinition &collection,
        const CatalogDecorationSizeDefinition &decoration,
        const ReplaySceneBlockPlacements &placements,
        const StaticSceneModelCollection &sceneModels,
        std::optional<ReplayWaterDefinition> *out);
