#pragma once

namespace PhysicsTolerance {

inline constexpr float CollisionDistance = 1.0e-5f;
inline constexpr float SurfaceDirectionLength = 1.0e-5f;
inline constexpr float SurfaceDirectionLengthSquared =
        SurfaceDirectionLength * SurfaceDirectionLength;
inline constexpr float CollisionSpeedSquared = 1.0e-5f;
inline constexpr float SphereContactNormalAlignment = 0.8660254f;
inline constexpr float ForceFieldDistance = 1.0e-5f;
inline constexpr float ForceFieldDistanceSquared =
        ForceFieldDistance * ForceFieldDistance;

}
