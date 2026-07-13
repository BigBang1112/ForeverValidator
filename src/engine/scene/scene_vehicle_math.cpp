#include "engine/scene/scene_vehicle_math.h"
#include "engine/physics/geometry/geometry_helpers.h"
float SmoothValue(float current, float target, float maxStep) {
    if (target + maxStep < current) {
        return current - maxStep;
    }
    if (target - maxStep > current) {
        return current + maxStep;
    }
    return target;
}

namespace SceneVehicleMath {

GmVec3 Zero() { return {}; }

void Accumulate(GmVec3 &accumulator, const GmVec3 &value) {
    accumulator = GmMath::Add(accumulator, value);
}

float LengthSquared(const GmVec3 &value) {
    return GmMath::LengthSquared(value);
}

float LengthSquaredYXZ(const GmVec3 &value) {
    return value.y * value.y + value.x * value.x + value.z * value.z;
}

float Dot(const GmVec3 &left, const GmVec3 &right) {
    return GmMath::Dot(left, right);
}

float DotXYZ(const GmVec3 &left, const GmVec3 &right) {
    return GmMath::Dot(left, right);
}

float DotYXZ(const GmVec3 &left, const GmVec3 &right) {
    return left.y * right.y + left.x * right.x + left.z * right.z;
}

GmVec3 Scale(const GmVec3 &value, float scale) {
    return GmMath::Scale(value, scale);
}

GmVec3 Add(const GmVec3 &left, const GmVec3 &right) {
    return GmMath::Add(left, right);
}

GmVec3 Subtract(const GmVec3 &left, const GmVec3 &right) {
    return GmMath::Subtract(left, right);
}

GmVec3 Cross(const GmVec3 &left, const GmVec3 &right) {
    return GmMath::Cross(left, right);
}

GmVec3 NormalizeOr(const GmVec3 &value, const GmVec3 &shortVectorResult,
                   float minimumLengthSquared) {
    return GmMath::NormalizeOr(
            value, shortVectorResult, minimumLengthSquared);
}

GmVec3 NormalizeIfLongEnough(const GmVec3 &value, float minimumLengthSquared) {
    return NormalizeOr(value, value, minimumLengthSquared);
}

float Angle(const GmVec3 &left, const GmVec3 &right) { return GmVec3::GetAngle(left, right); }

float SubtractProducts(float a, float b, float c, float d) { return a * b - c * d; }

float Reciprocal(float value) { return 1.0f / value; }

float VerticalComponent(const GmVec3 &value) {
    return value.y;
}

float ZeroScaled(float value) {
    (void)value;
    return 0.0f;
}

float DirectSpringForce(float restAbsorb, float damperAbsorb, float springCoefficient,
                        float staticSpringScale) {
    return (restAbsorb - damperAbsorb) * (springCoefficient * staticSpringScale);
}

float FollowAbsorbForce(float restAbsorb, float damperAbsorb, float springCoefficient,
                        float damperCoefficient, float damperVelocity) {
    const float springForce = (restAbsorb - damperAbsorb) * springCoefficient;
    return springForce - damperCoefficient * damperVelocity;
}

float FollowAbsorbTarget(float restAbsorb, float baseAbsorb, float deltaTime,
                         float followCoefficient) {
    const float displacement = restAbsorb - baseAbsorb;
    return displacement * deltaTime * followCoefficient + baseAbsorb;
}

} // namespace SceneVehicleMath
