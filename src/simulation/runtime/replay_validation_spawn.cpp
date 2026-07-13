#include "simulation/runtime/replay_validation_spawn.h"

#include "engine/core/binary32_math.h"

namespace {

constexpr std::uint32_t ValidationSeedModulo = 100000u;
constexpr double ValidationSeedDivisor = 100000.0;
constexpr double ValidationSeedCenter = 0.5;
constexpr double ValidationYawRangeDegrees = 0.1000000014901161;
constexpr float PiBinary32 = 3.1415927410125732f;
constexpr double HalfCircleDegrees = 180.0;

float ValidationSeedYawRadians(std::uint32_t validationSeed) {
    const double normalized =
            static_cast<double>(validationSeed % ValidationSeedModulo) /
            ValidationSeedDivisor;
    const float yawDegrees = Binary32::FromDouble(
            (normalized - ValidationSeedCenter) *
            ValidationYawRangeDegrees);
    return Binary32::FromDouble(
            static_cast<double>(yawDegrees * PiBinary32) /
            HalfCircleDegrees);
}

}  // namespace

GmIso4 BuildReplayValidationSpawnLocation(
        const GmIso4 &location,
        std::uint32_t validationSeed) {
    if (validationSeed == 0u) {
        return location;
    }

    GmMat3 validationRotation;
    validationRotation.SetIdentity();
    validationRotation.RotateY(
            ValidationSeedYawRadians(validationSeed));

    GmIso4 seeded = location;
    seeded.rotation.LeftMult(validationRotation);
    return seeded;
}
