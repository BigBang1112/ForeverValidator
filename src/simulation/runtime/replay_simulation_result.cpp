#include "simulation/runtime/replay_simulation_result.h"
#include "simulation/replay/replay_map_scene.h"
ReplaySimulationRunResult MapReplaySceneResult(
        ReplayMapSceneResult result) {
    switch (result) {
    case ReplayMapSceneResult::Ready:
        return ReplaySimulationRunResult::Success;
    case ReplayMapSceneResult::MissingModelSources:
        return ReplaySimulationRunResult::StaticSceneSourcesMissing;
    case ReplayMapSceneResult::CorpusCountMismatch:
        return ReplaySimulationRunResult::StaticSceneCorpusCountMismatch;
    case ReplayMapSceneResult::StationaryCorpusInstallFailed:
        return ReplaySimulationRunResult::StaticSceneInstallFailed;
    case ReplayMapSceneResult::Inactive:
    case ReplayMapSceneResult::ChallengeUnavailable:
    case ReplayMapSceneResult::StaticCorpusConstructionFailed:
        return ReplaySimulationRunResult::StaticSceneConstructionFailed;
    }
    return ReplaySimulationRunResult::StaticSceneConstructionFailed;
}
