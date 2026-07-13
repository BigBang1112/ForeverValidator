#include "simulation/runtime/replay_simulation_definition.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>

#include "engine/core/finite_values.h"
#include "engine/game/replay_vehicle_dyna_definition.h"
#include "format/pack/replay_vehicle_source_bundle.h"
#include <new>
bool VehicleMaterialDefinition::HasFiniteParameters() const {
    return FiniteValues::All(
            blendableValues.x,
            blendableValues.y,
            blendableValues.z,
            blendableValues.w,
            fakeContactPeriodX,
            fakeContactPeriodZ,
            fakeContactSpeedScale,
            fakeContactDepthMax,
            feedbackSpeedDivisor,
            feedbackScale);
}

bool VehicleMaterialSetDefinition::IsValid() const {
    const auto maximumMaterialCount =
            static_cast<std::size_t>(
                    std::numeric_limits<std::uint32_t>::max());
    return fakeContactTexture.has_value() &&
           fakeContactTexture->HasExpectedShape() &&
           !materialIndexByNaturalId.empty() &&
           materialIndexByNaturalId.size() <= maximumMaterialCount &&
           materials.size() <= maximumMaterialCount &&
           std::all_of(
                   materials.begin(),
                   materials.end(),
                   [](const VehicleMaterialDefinition &material) {
                       return material.HasFiniteParameters();
                   }) &&
           std::all_of(
                   materialIndexByNaturalId.begin(),
                   materialIndexByNaturalId.end(),
                   [this](std::uint32_t index) {
                       return index < materials.size();
                   });
}

namespace {

bool BuildEnvironment(
        const std::optional<ReplayWaterDefinition> &water,
        ReplayEnvironmentDefinition &environment) {
    environment.zoneLinearDampingCoefficient = 1.0f;
    environment.zoneAngularDampingCoefficient = 1.0f;
    environment.water = water;
    environment.forceFields.clear();
    try {
        environment.forceFields.emplace_back(
                ReplayUniformForceFieldDefinition{
                        GmVec3{0.0f, -10.0f, 0.0f}});
    } catch (const std::bad_alloc &) {
        environment.forceFields.clear();
        return false;
    }
    return true;
}

ReplaySimulationDefinitionBuild Failure(
        ReplaySimulationDefinitionBuildResult result) {
    return ReplaySimulationDefinitionBuild::Failure(result);
}

}  // namespace

ReplaySimulationDefinitionBuild BuildReplaySimulationDefinition(
        const ReplayVehicleSourceBundle &sources,
        const std::optional<ReplayWaterDefinition> &water) {
    if (!sources.HasRequiredAssets()) {
        return Failure(
                ReplaySimulationDefinitionBuildResult::
                        MissingVehicleDefinition);
    }
    if (!sources.vehicle.initialParameters.has_value()) {
        return Failure(
                ReplaySimulationDefinitionBuildResult::
                        InvalidInitialParameters);
    }
    if (!sources.vehicle.materials->IsValid()) {
        return Failure(
                ReplaySimulationDefinitionBuildResult::InvalidMaterials);
    }

    std::optional<VehicleWheelSetDefinition> wheels =
            BuildVehicleWheelSetDefinition(
                    sources.solid,
                    sources.tuning,
                    sources.vehicle);
    std::optional<ReplayDynaParameters> dyna =
            BuildVehicleDynaDefinition(sources.tuning, sources.solid);
    if (!wheels.has_value() || !dyna.has_value()) {
        return Failure(
                ReplaySimulationDefinitionBuildResult::InvalidVehiclePhysics);
    }

    ReplaySimulationDefinition definition;
    try {
        definition.vehicle.initialParameters =
                *sources.vehicle.initialParameters;
        definition.vehicle.tuning = sources.tuning;
        definition.vehicle.wheels = std::move(*wheels);
        definition.vehicle.collisionModel = sources.solid.collisionModel;
        definition.vehicle.materials = *sources.vehicle.materials;
        definition.vehicle.dynaParameters = *dyna;
    } catch (const std::bad_alloc &) {
        return Failure(
                ReplaySimulationDefinitionBuildResult::AllocationFailure);
    }

    if (!BuildEnvironment(water, definition.environment)) {
        return Failure(
                ReplaySimulationDefinitionBuildResult::InvalidEnvironment);
    }

    return ReplaySimulationDefinitionBuild::Success(
            std::move(definition));
}
