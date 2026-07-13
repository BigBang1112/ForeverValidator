#include "simulation/runtime/replay_environment.h"
#include <memory>
#include <utility>

void ReplayForceFieldSet::BuildFromDefinition(
        const ReplayEnvironmentDefinition &definition) {
    fields.clear();
    fields.reserve(definition.forceFields.size());
    for (const ReplayForceFieldDefinition &fieldDefinition :
         definition.forceFields) {
        std::unique_ptr<CHmsForceField> field;
        if (const auto *uniform =
                    std::get_if<ReplayUniformForceFieldDefinition>(
                            &fieldDefinition)) {
            auto uniformField = std::make_unique<CHmsForceFieldUniform>();
            uniformField->Configure(true, uniform->acceleration);
            field = std::move(uniformField);
        } else if (const auto *ball =
                           std::get_if<ReplayBallForceFieldDefinition>(
                                   &fieldDefinition)) {
            auto ballField = std::make_unique<CHmsForceFieldBall>();
            ballField->Configure(
                    ball->center,
                    ball->radius,
                    ball->strength);
            field = std::move(ballField);
        }
        if (field != nullptr) {
            fields.push_back(std::move(field));
        }
    }
}

void ReplayForceFieldSet::InstallOnZone(
        CHmsZoneDynamic &zone,
        bool itemHooksEnabled,
        float linearDamping,
        float angularDamping) const {
    zone.ResetForceFields(linearDamping, angularDamping);
    if (itemHooksEnabled) {
        for (const std::unique_ptr<CHmsForceField> &field : fields) {
            zone.AddField(field.get());
        }
    }
}

void ReplayEnvironment::Build(
        const ReplayEnvironmentDefinition &definition) {
    forceFields.BuildFromDefinition(definition);
}

void ReplayEnvironment::InstallOnZone(
        CHmsZoneDynamic &zone,
        const ReplayEnvironmentDefinition &definition,
        bool itemHooksEnabled) const {
    forceFields.InstallOnZone(zone,
                              itemHooksEnabled,
                              definition.zoneLinearDampingCoefficient,
                              definition.zoneAngularDampingCoefficient);
}

static void ConfigureWaterMap(
        CHmsCollisionManagerSZone &zone,
        const ReplayWaterDefinition &water) {
    zone.ConfigureWater(water.OccupancyGrid(),
                        water.SurfaceHeight(),
                        water.SecondaryCullHeight());
}

void ReplayEnvironment::InstallWater(
        CHmsZoneDynamic &zone,
        CHmsCollisionManagerSZone &szone,
        const std::optional<ReplayWaterDefinition> &water) const {
    zone.SetCollisionManagerZone(szone);
    szone.waterZone.Disable();
    if (!water.has_value()) {
        return;
    }

    ConfigureWaterMap(szone, *water);
}
