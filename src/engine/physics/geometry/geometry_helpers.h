#pragma once

#include "engine/core/gm_types.h"
namespace GmMath {

float Dot(const GmVec3 &left, const GmVec3 &right);
float LengthSquared(const GmVec3 &value);
GmVec3 Add(const GmVec3 &left, const GmVec3 &right);
GmVec3 Subtract(const GmVec3 &left, const GmVec3 &right);
GmVec3 Scale(const GmVec3 &value, float scale);
GmVec3 Negate(const GmVec3 &value);
GmVec3 Cross(const GmVec3 &left, const GmVec3 &right);
GmVec3 NormalizeOr(const GmVec3 &value,
                   const GmVec3 &shortVectorResult,
                   float minimumLengthSquared);

GmVec3 TransformDirection(const GmMat3 &matrix, const GmVec3 &direction);
GmVec3 TransformDirectionTranspose(const GmMat3 &matrix,
                                   const GmVec3 &direction);
GmVec3 TransformPoint(const GmIso4 &transform, const GmVec3 &point);

// Engine composition applies `first`, then `second`.
GmMat3 Compose(const GmMat3 &first, const GmMat3 &second);
GmMat3 MultiplyByTranspose(const GmMat3 &left, const GmMat3 &right);

bool NearlyEqual(float value, float reference);

}  // namespace GmMath
