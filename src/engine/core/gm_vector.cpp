#include "engine/core/gm_types.h"
#include <cmath>

#include "engine/core/binary32_math.h"
#include "engine/physics/geometry/geometry_helpers.h"
#include "engine/physics/geometry/physics_tolerances.h"

unsigned long GmVec4::PlaneEqIsNearlyEqual(
        const GmVec4 &other,
        float normalDotThreshold,
        float distanceThreshold) const {
    (void)normalDotThreshold;
    (void)distanceThreshold;
    const float normalDot =
            other.y * y + other.x * x + other.z * z;
    return normalDot >= 0.9900000095367432f &&
                   std::fabs(w - other.w) <= 0.10000000149011612f
            ? 1ul
            : 0ul;
}

void GmVec4::PlaneEqMult(const GmIso4 &iso) {
    const GmVec4 source = *this;
    const GmVec3 transformedNormal = {
            (iso.rotation.basisX.y * source.y +
             iso.rotation.basisX.x * source.x) +
                    iso.rotation.basisX.z * source.z,
            (iso.rotation.basisY.x * source.x +
             iso.rotation.basisY.y * source.y) +
                    iso.rotation.basisY.z * source.z,
            (iso.rotation.basisZ.x * source.x +
             iso.rotation.basisZ.y * source.y) +
                    iso.rotation.basisZ.z * source.z,
    };
    x = transformedNormal.x;
    y = transformedNormal.y;
    z = transformedNormal.z;
    w = source.w -
            ((transformedNormal.x * iso.translation.x +
              transformedNormal.y * iso.translation.y) +
             transformedNormal.z * iso.translation.z);
}

GmVec3 GmVec3::Zero(void) {
    GmVec3 zeroVector = {0.0f, 0.0f, 0.0f};
    return zeroVector;
}

GmVec3 GmVec3::ZeroForComputeCorpusForces(void) {
    GmVec3 zeroVector = {0.0f, 0.0f, 0.0f};
    return zeroVector;
}

float GmVec3::Dot(const GmVec3 &rhs) const {
    return x * rhs.x + y * rhs.y + z * rhs.z;
}

float GmVec3::Component(GmAxis axis) const {
    if (axis == GmAxis::X) {
        return x;
    }
    if (axis == GmAxis::Y) {
        return y;
    }
    return z;
}

float GmVec3::LengthSquaredForCollision(void) const {
    return y * y + x * x + z * z;
}

float GmVec3::LengthSquaredYXZ(void) const {
    float xy = y * y +
                     x * x;
    return (xy +
                                        z * z);
}

GmVec3 GmVec3::Negated(void) const {
    GmVec3 out = {-x, -y, -z};
    return out;
}

GmVec3 GmVec3::operator-(const GmVec3 &rhs) const {
    GmVec3 out = {x - rhs.x, y - rhs.y, z - rhs.z};
    return out;
}

GmVec3 GmVec3::Cross(const GmVec3 &rhs) const {
    GmVec3 out = {
        y * rhs.z - z * rhs.y,
        z * rhs.x - x * rhs.z,
        x * rhs.y - y * rhs.x,
    };
    return out;
}

GmVec3 GmVec3::SubtractForCollision(const GmVec3 &rhs) const {
    GmVec3 out = {
        (x - rhs.x),
        (y - rhs.y),
        (z - rhs.z),
    };
    return out;
}

GmVec3 GmVec3::AddForCollision(const GmVec3 &rhs) const {
    GmVec3 out = {
        (x + rhs.x),
        (y + rhs.y),
        (z + rhs.z),
    };
    return out;
}

GmVec3 GmVec3::ScaleForCollision(float scale) const {
    GmVec3 out = {
        (x * scale),
        (y * scale),
        (z * scale),
    };
    return out;
}

GmVec3 GmVec3::NormalizeForCollision(float epsilonSq) const {
    GmVec3 out = *this;
    float lengthSq = out.Dot(out);
    if (epsilonSq < lengthSq) {
        out = out.ScaleForCollision(1.0f / CIsqrt(lengthSq));
    }
    return out;
}

float GmVec3::MinComponentForGmSurf(void) const {
    float out = x < y ? x : y;
    return out < z ? out : z;
}

float GmVec3::MaxComponentForGmSurf(void) const {
    float out = x > y ? x : y;
    return out > z ? out : z;
}

void GmVec3::MultEllipsoidMeshWorldNormalForGmSurf(
        const GmMat3 &rotation) {
    const GmVec3 sourceVector = *this;
    const float resultX = (
            (
                    rotation.Element(GmAxis::X, GmAxis::X) * sourceVector.x +
                    rotation.Element(GmAxis::X, GmAxis::Y) * sourceVector.y) +
            rotation.Element(GmAxis::X, GmAxis::Z) * sourceVector.z);
    const float resultY = (
            (
                    rotation.Element(GmAxis::Y, GmAxis::X) * sourceVector.x +
                    rotation.Element(GmAxis::Y, GmAxis::Y) * sourceVector.y) +
            rotation.Element(GmAxis::Y, GmAxis::Z) * sourceVector.z);
    const float resultZ = (
            (
                    rotation.Element(GmAxis::Z, GmAxis::X) * sourceVector.x +
                    rotation.Element(GmAxis::Z, GmAxis::Y) * sourceVector.y) +
            rotation.Element(GmAxis::Z, GmAxis::Z) * sourceVector.z);
    x = resultX;
    y = resultY;
    z = resultZ;
}

void GmVec3::TransformAndNormalizeDirectionForGmSurf(
        const GmIso4 &iso,
        float epsilonSq) {
    Mult(iso.RotationMatrix());
    *this = NormalizeForCollision(epsilonSq);
}

void GmVec3::MultInverseIso4LocalForGmSurf(const GmIso4 &iso) {
    x -= iso.translation.x;
    y -= iso.translation.y;
    z -= iso.translation.z;
    MultTranspose(iso.RotationMatrix());
}

GmVec3 GmVec3::LocalToWorldMat3ForSolveImpulse(
        const GmMat3 &rotation) const {
    GmVec3 out = {
        (rotation.Element(GmAxis::X, GmAxis::Y) * y +
         rotation.Element(GmAxis::X, GmAxis::X) * x) +
                rotation.Element(GmAxis::X, GmAxis::Z) * z,
        (rotation.Element(GmAxis::Y, GmAxis::X) * x +
         rotation.Element(GmAxis::Y, GmAxis::Y) * y) +
                rotation.Element(GmAxis::Y, GmAxis::Z) * z,
        (rotation.Element(GmAxis::Z, GmAxis::X) * x +
         rotation.Element(GmAxis::Z, GmAxis::Y) * y) +
                rotation.Element(GmAxis::Z, GmAxis::Z) * z,
    };
    return out;
}

GmVec3 GmVec3::LocalToWorldMat3SideAForSolveImpulse(
        const GmMat3 &rotation) const {
    GmVec3 out = {
        (rotation.Element(GmAxis::X, GmAxis::Y) * y +
         rotation.Element(GmAxis::X, GmAxis::X) * x) +
                rotation.Element(GmAxis::X, GmAxis::Z) * z,
        (rotation.Element(GmAxis::Y, GmAxis::Y) * y +
         rotation.Element(GmAxis::Y, GmAxis::X) * x) +
                rotation.Element(GmAxis::Y, GmAxis::Z) * z,
        (rotation.Element(GmAxis::Z, GmAxis::Y) * y +
         rotation.Element(GmAxis::Z, GmAxis::X) * x) +
                rotation.Element(GmAxis::Z, GmAxis::Z) * z,
    };
    return out;
}

void GmVec3::ScaleInPlaceForComputeCorpusForces(float scale) {
    x = x * scale;
    y = y * scale;
    z = scale * z;
}

void GmVec3::AccumulateForComputeCorpusForces(const GmVec3 &value) {
    x = value.x + x;
    y = value.y + y;
    z = value.z + z;
}

void GmVec3::AddScaledForComputeCorpusForces(GmVec3 value, float scale) {
    value.ScaleInPlaceForComputeCorpusForces(scale);
    AccumulateForComputeCorpusForces(value);
}

float GmVec3::PhysicsStep2LinearSpeedLength(void) const {
    const float speedLenSq = (y * y + x * x) + z * z;
    return CIsqrt(speedLenSq);
}

float GmVec3::PhysicsStep2AngularSpeedLength(void) const {
    const float speedLenSq = (x * x + y * y) + z * z;
    return CIsqrt(speedLenSq);
}
void GmVec3::NormalizeIfAboveAngleEpsilon(void) {
    *this = GmMath::NormalizeOr(
            *this,
            *this,
            PhysicsTolerance::SurfaceDirectionLengthSquared);
}

float GmVec3::GetAngle(const GmVec3 &lhs, const GmVec3 &rhs) {
    constexpr float AngleEpsilon = 1.0e-5f;
    const float contractedDot = Binary32::FromDouble(
            static_cast<double>(GmMath::Dot(lhs, rhs)) *
            (1.0 - static_cast<double>(AngleEpsilon)));
    float angle = CIacos(contractedDot);
    if (angle <= AngleEpsilon) {
        return angle;
    }

    const GmVec3 normalizedLhs = GmMath::NormalizeOr(
            lhs,
            lhs,
            PhysicsTolerance::SurfaceDirectionLengthSquared);
    const GmVec3 normalizedRhs = GmMath::NormalizeOr(
            rhs,
            rhs,
            PhysicsTolerance::SurfaceDirectionLengthSquared);
    if (GmMath::Cross(normalizedLhs, normalizedRhs).y < 0.0f) {
        angle = -angle;
    }
    return angle;
}

void GmVec3::SetMult(const GmVec3 &source, const GmMat3 &matrix) {
    *this = GmMath::TransformDirection(matrix, source);
}

void GmVec3::SetMult(const GmVec3 &source, const GmIso4 &transform) {
    *this = GmMath::TransformPoint(transform, source);
}

void GmVec3::SetMultTranspose(const GmVec3 &source,
                              const GmMat3 &matrix) {
    *this = GmMath::TransformDirectionTranspose(matrix, source);
}

void GmVec3::Mult(const GmIso4 &transform) {
    *this = GmMath::TransformPoint(transform, *this);
}

void GmVec3::Mult(const GmMat3 &matrix) {
    *this = GmMath::TransformDirection(matrix, *this);
}

void GmVec3::MultTranspose(const GmMat3 &matrix) {
    *this = GmMath::TransformDirectionTranspose(matrix, *this);
}

void GmVec3::MultInverse(const GmIso4 &transform) {
    *this = GmMath::TransformDirectionTranspose(
            transform.rotation,
            GmMath::Subtract(*this, transform.translation));
}

unsigned long GmVec3::IsNearlyEqual(const GmVec3 &rhs) const {
    return GmMath::NearlyEqual(x, rhs.x) &&
           GmMath::NearlyEqual(y, rhs.y) &&
           GmMath::NearlyEqual(z, rhs.z);
}

void GmIso4::GetDir(GmVec3 &out) const {
    out = rotation.basisZ;
}
