#ifndef TMNF_REPLAY_VALIDATION_EVALUATION_H
#define TMNF_REPLAY_VALIDATION_EVALUATION_H

#include <vector>

#include "simulation/runtime/replay_trajectory_observation.h"
#include "validation/evaluation/replay_validation_model.h"
struct ReplaySimulationTimelineResult;

ReplayValidationExecutionOutput EvaluateReplayValidation(
        const ReplayValidationPlan &plan,
        const std::vector<ReplayTrajectoryObservation> &observations,
        const ReplaySimulationTimelineResult &simulationResult);

#endif
