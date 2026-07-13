#include "engine/physics/geometry/geometry_helpers.h"
#include <cmath>

#include "engine/core/binary32_math.h"
namespace {

float SumProducts(const GmVec3 &left, const GmVec3 &right) {
    const float xy = left.x * right.x + left.y * right.y;
    return xy + left.z * right.z;
}

GmVec3 Normalize(const GmVec3 &value, float lengthSquared) {
    return GmMath::Scale(value, 1.0f / CIsqrt(lengthSquared));
}

}  // namespace

namespace GmMath {

float Dot(const GmVec3 &left, const GmVec3 &right) {
    return SumProducts(left, right);
}

float LengthSquared(const GmVec3 &value) {
    return SumProducts(value, value);
}

GmVec3 Add(const GmVec3 &left, const GmVec3 &right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

GmVec3 Subtract(const GmVec3 &left, const GmVec3 &right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

GmVec3 Scale(const GmVec3 &value, float scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}

GmVec3 Negate(const GmVec3 &value) {
    return {-value.x, -value.y, -value.z};
}

GmVec3 Cross(const GmVec3 &left, const GmVec3 &right) {
    return {
            left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x,
    };
}

GmVec3 NormalizeOr(const GmVec3 &value,
                   const GmVec3 &shortVectorResult,
                   float minimumLengthSquared) {
    const float lengthSquared = LengthSquared(value);
    return lengthSquared > minimumLengthSquared
            ? Normalize(value, lengthSquared)
            : shortVectorResult;
}

GmVec3 TransformDirection(const GmMat3 &matrix,
                          const GmVec3 &direction) {
    return {
            Dot(matrix.Row(GmAxis::X), direction),
            Dot(matrix.Row(GmAxis::Y), direction),
            Dot(matrix.Row(GmAxis::Z), direction),
    };
}

GmVec3 TransformDirectionTranspose(const GmMat3 &matrix,
                                   const GmVec3 &direction) {
    return {
            Dot(matrix.basisX, direction),
            Dot(matrix.basisY, direction),
            Dot(matrix.basisZ, direction),
    };
}

GmVec3 TransformPoint(const GmIso4 &transform, const GmVec3 &point) {
    return Add(TransformDirection(transform.rotation, point),
               transform.translation);
}

GmMat3 Compose(const GmMat3 &first, const GmMat3 &second) {
    GmMat3 result;
    result.basisX = TransformDirection(second, first.basisX);
    result.basisY = TransformDirection(second, first.basisY);
    result.basisZ = TransformDirection(second, first.basisZ);
    return result;
}

GmMat3 MultiplyByTranspose(const GmMat3 &left, const GmMat3 &right) {
    GmMat3 result;
    const GmVec3 rightColumnX = right.basisX;
    const GmVec3 rightColumnY = right.basisY;
    const GmVec3 rightColumnZ = right.basisZ;
    result.basisX = {
            Dot(rightColumnX, left.basisX),
            Dot(rightColumnY, left.basisX),
            Dot(rightColumnZ, left.basisX),
    };
    result.basisY = {
            Dot(rightColumnX, left.basisY),
            Dot(rightColumnY, left.basisY),
            Dot(rightColumnZ, left.basisY),
    };
    result.basisZ = {
            Dot(rightColumnX, left.basisZ),
            Dot(rightColumnY, left.basisZ),
            Dot(rightColumnZ, left.basisZ),
    };
    return result;
}

bool NearlyEqual(float value, float reference) {
    constexpr float RelativeTolerance = 1.0e-5f;
    const float tolerance = std::fabs(reference) * RelativeTolerance;
    return reference - tolerance <= value &&
           value <= reference + tolerance;
}

}  // namespace GmMath
