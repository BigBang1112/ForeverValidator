#include "engine/game/replay_vehicle_wheel_definition.h"
#include <utility>

#include "engine/core/finite_values.h"
#include "engine/game/replay_vehicle_solid_definition.h"
#include "engine/game/replay_vehicle_source_definition.h"
#include "engine/game/replay_vehicle_tuning_definition.h"
bool VehicleWheelSetDefinition::IsComplete() const {
    if (!FiniteValues::All(restDamperAbsorb)) {
        return false;
    }
    for (std::size_t wheelIndex = 0u;
         wheelIndex < wheels.size();
         ++wheelIndex) {
        const VehicleWheelDefinition &wheel = wheels[wheelIndex];
        if (wheel.surfaceId != VehicleWheelSurfaceIdForIndex(wheelIndex) ||
            wheel.collisionRole != VehicleWheelCollisionRole(wheelIndex) ||
            !FiniteValues::All(wheel.rollingRadius) ||
            !FiniteValues::IsFinite(wheel.forceApplicationPoint) ||
            !FiniteValues::IsFinite(wheel.restSurfacePose.rotation.basisX) ||
            !FiniteValues::IsFinite(wheel.restSurfacePose.rotation.basisY) ||
            !FiniteValues::IsFinite(wheel.restSurfacePose.rotation.basisZ) ||
            !FiniteValues::IsFinite(wheel.restSurfacePose.translation)) {
            return false;
        }
    }
    return true;
}

std::optional<VehicleWheelSetDefinition> BuildVehicleWheelSetDefinition(
        const ReplayVehicleSolidDefinition &solid,
        const ReplayVehicleTuningDefinition &tuning,
        const ReplayVehicleSourceDefinition &vehicle) {
    if (!solid.HasCompleteWheelSet() ||
        !vehicle.HasCompleteWheelContactSettings()) {
        return std::nullopt;
    }

    const float restDamperAbsorb =
            tuning.suspension.wheelRestDamperAbsorb;
    if (!FiniteValues::All(restDamperAbsorb)) {
        return std::nullopt;
    }

    VehicleWheelSetDefinition definition;
    definition.restDamperAbsorb = restDamperAbsorb;
    for (std::size_t index = 0u;
         index < OfficialVehicleWheelCount;
         ++index) {
        const VehicleWheelDefinition &sourceWheel = *solid.wheels[index];
        const VehicleWheelContactSettings &contactSettings =
                *vehicle.wheelContactSettings[index];
        if (contactSettings.surfaceId != sourceWheel.surfaceId) {
            return std::nullopt;
        }

        VehicleWheelDefinition wheel = sourceWheel;
        wheel.killsLateralSpeedOnContact =
                contactSettings.killsLateralSpeedOnContact;
        wheel.axle = contactSettings.axle;
        definition.wheels[index] = wheel;
    }

    return definition.IsComplete()
            ? std::optional<VehicleWheelSetDefinition>{std::move(definition)}
            : std::nullopt;
}
