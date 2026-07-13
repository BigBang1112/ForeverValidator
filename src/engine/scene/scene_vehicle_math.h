#pragma once

#include "engine/core/gm_types.h"
float SmoothValue(float current, float target, float maxStep);

namespace SceneVehicleMath {

inline constexpr float Pi = 3.1415927f;
inline constexpr float HalfPi = Pi * 0.5f;
inline constexpr float QuarterPi = Pi * 0.25f;
inline constexpr float SpeedKilometersPerHourScale = 3.6f;
inline constexpr float WheelSpinAnglePeriod = 512.0f * Pi;

GmVec3 Zero();
void Accumulate(GmVec3 &accumulator, const GmVec3 &value);
float LengthSquared(const GmVec3 &value);
float LengthSquaredYXZ(const GmVec3 &value);
float Dot(const GmVec3 &left, const GmVec3 &right);
float DotXYZ(const GmVec3 &left, const GmVec3 &right);
float DotYXZ(const GmVec3 &left, const GmVec3 &right);
GmVec3 Scale(const GmVec3 &value, float scale);
GmVec3 Add(const GmVec3 &left, const GmVec3 &right);
GmVec3 Subtract(const GmVec3 &left, const GmVec3 &right);
GmVec3 Cross(const GmVec3 &left, const GmVec3 &right);
GmVec3 NormalizeOr(const GmVec3 &value,
                   const GmVec3 &shortVectorResult,
                   float minimumLengthSquared);
GmVec3 NormalizeIfLongEnough(const GmVec3 &value,
                             float minimumLengthSquared);
float Angle(const GmVec3 &left, const GmVec3 &right);
float SubtractProducts(float a, float b, float c, float d);
float Reciprocal(float value);
float VerticalComponent(const GmVec3 &value);
float ZeroScaled(float value);

float DirectSpringForce(float restAbsorb,
                        float damperAbsorb,
                        float springCoefficient,
                        float staticSpringScale);
float FollowAbsorbForce(float restAbsorb,
                        float damperAbsorb,
                        float springCoefficient,
                        float damperCoefficient,
                        float damperVelocity);
float FollowAbsorbTarget(float restAbsorb,
                         float baseAbsorb,
                         float deltaTime,
                         float followCoefficient);

}  // namespace SceneVehicleMath
