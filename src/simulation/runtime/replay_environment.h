#ifndef TMNF_REPLAY_ENVIRONMENT_H
#define TMNF_REPLAY_ENVIRONMENT_H

#include <memory>
#include <optional>
#include <vector>

#include "simulation/runtime/replay_environment_definition.h"
#include "engine/physics/collision/hms_collision_manager.h"
#include "engine/physics/dynamics/hms_force_field.h"
#include "engine/physics/world/hms_zone.h"
class ReplayForceFieldSet {
public:
    void BuildFromDefinition(const ReplayEnvironmentDefinition &definition);
    void InstallOnZone(CHmsZoneDynamic &zone,
                       bool itemHooksEnabled,
                       float linearDamping,
                       float angularDamping) const;

private:
    std::vector<std::unique_ptr<CHmsForceField>> fields;
};

class ReplayEnvironment {
public:
    void Build(const ReplayEnvironmentDefinition &definition);
    void InstallOnZone(CHmsZoneDynamic &zone,
                       const ReplayEnvironmentDefinition &definition,
                       bool itemHooksEnabled) const;
    void InstallWater(CHmsZoneDynamic &zone,
                      CHmsCollisionManagerSZone &collisionZone,
                      const std::optional<ReplayWaterDefinition> &water) const;

private:
    ReplayForceFieldSet forceFields;
};

#endif // TMNF_REPLAY_ENVIRONMENT_H
