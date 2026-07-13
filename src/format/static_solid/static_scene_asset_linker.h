#pragma once

class CGameCtnReplayArchiveStaticModelCollection;
class ReplaySceneBlockPlacements;
class StaticSolidArchiveLoadSession;
class StaticSceneModelCollection;

bool BuildStaticSceneFromArchive(
        StaticSolidArchiveLoadSession &payloads,
        CGameCtnReplayArchiveStaticModelCollection &archiveModels,
        const ReplaySceneBlockPlacements &placements,
        StaticSceneModelCollection &models);
