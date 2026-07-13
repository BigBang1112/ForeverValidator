#include "engine/game/replay_vehicle_dyna_definition.h"
#include <array>

#include "engine/core/binary32_math.h"
#include "engine/core/finite_values.h"
namespace {

struct Vec3d {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

void SetInverseBodyInertia(
        double mass,
        const Vec3d &halfExtents,
        ReplayDynaParameters &parameters) {
    const double width = halfExtents.x * 2.0;
    const double height = halfExtents.y * 2.0;
    const double depth = halfExtents.z * 2.0;
    const double inverseMassScale = 12.0 * (1.0 / mass);
    parameters.inverseBodyInertia.SetDiagonal(
            Binary32::FromDouble(
                    inverseMassScale /
                    (height * height + depth * depth)),
            Binary32::FromDouble(
                    inverseMassScale /
                    (depth * depth + width * width)),
            Binary32::FromDouble(
                    inverseMassScale /
                    (width * width + height * height)));
}

}  // namespace

std::optional<ReplayDynaParameters> BuildVehicleDynaDefinition(
        const ReplayVehicleTuningDefinition &tuning,
        const ReplayVehicleSolidDefinition &solid) {
    if (!solid.HasCompleteWheelSet()) {
        return std::nullopt;
    }

    const ReplayVehicleTuningBodyAirResponse &body =
            tuning.bodyAirResponse;
    const Vec3d inertiaHalfExtents{
            body.solidInertiaBoxSize.x,
            body.solidInertiaBoxSize.y,
            body.solidInertiaBoxSize.z,
    };
    if (!FiniteValues::All(
                body.solidPhysicalMass,
                body.solidInertiaMass,
                inertiaHalfExtents.x,
                inertiaHalfExtents.y,
                inertiaHalfExtents.z,
                body.solidCenterZHalfExtentScale,
                body.solidCenterYOffset,
                body.groundedSolidFeedback1,
                body.solidPhysicalResponseCoefB) ||
        body.solidInertiaMass == 0.0f) {
        return std::nullopt;
    }

    std::array<Vec3d, OfficialVehicleWheelCount> surfacePoints;
    std::array<double, OfficialVehicleWheelCount> rollingRadii;
    for (std::size_t wheel = 0u;
         wheel < OfficialVehicleWheelCount;
         ++wheel) {
        const VehicleWheelDefinition &definition = *solid.wheels[wheel];
        surfacePoints[wheel] = {
                definition.initialSurfacePoint.x,
                definition.initialSurfacePoint.y,
                definition.initialSurfacePoint.z,
        };
        rollingRadii[wheel] = definition.rollingRadius;
        if (!FiniteValues::All(
                    surfacePoints[wheel].x,
                    surfacePoints[wheel].y,
                    surfacePoints[wheel].z,
                    rollingRadii[wheel])) {
            return std::nullopt;
        }
    }

    Vec3d minPoint = surfacePoints[0];
    Vec3d maxPoint = surfacePoints[0];
    double bottomYSum = 0.0;
    for (std::size_t wheel = 0u;
         wheel < OfficialVehicleWheelCount;
         ++wheel) {
        const Vec3d &point = surfacePoints[wheel];
        if (point.x < minPoint.x) minPoint.x = point.x;
        if (point.x > maxPoint.x) maxPoint.x = point.x;
        if (point.y < minPoint.y) minPoint.y = point.y;
        if (point.y > maxPoint.y) maxPoint.y = point.y;
        if (point.z < minPoint.z) minPoint.z = point.z;
        if (point.z > maxPoint.z) maxPoint.z = point.z;
        bottomYSum += point.y - rollingRadii[wheel];
    }

    const GmVec3 center{
            Binary32::FromDouble((minPoint.x + maxPoint.x) * 0.5),
            Binary32::FromDouble((minPoint.y + maxPoint.y) * 0.5),
            Binary32::FromDouble((minPoint.z + maxPoint.z) * 0.5),
    };
    const GmVec3 wheelHalfExtents{
            Binary32::FromDouble((maxPoint.x - minPoint.x) * 0.5),
            Binary32::FromDouble((maxPoint.y - minPoint.y) * 0.5),
            Binary32::FromDouble((maxPoint.z - minPoint.z) * 0.5),
    };

    ReplayDynaParameters parameters;
    parameters.mass = body.solidPhysicalMass;
    SetInverseBodyInertia(
            static_cast<double>(body.solidInertiaMass),
            inertiaHalfExtents,
            parameters);
    parameters.linearDampingScale = 0.0f;
    parameters.angularDampingScale = 0.0f;
    parameters.maxStepDistance = body.solidPhysicalResponseCoefB;
    parameters.forceScale = body.groundedSolidFeedback1;
    parameters.localCenterOfMass = {
            center.x,
            Binary32::FromDouble(
                    (1.0 /
                     static_cast<double>(OfficialVehicleWheelCount)) *
                            bottomYSum +
                    static_cast<double>(body.solidCenterYOffset)),
            Binary32::FromDouble(
                    static_cast<double>(center.z) +
                    static_cast<double>(body.solidCenterZHalfExtentScale) *
                            static_cast<double>(wheelHalfExtents.z)),
    };
    return parameters;
}
