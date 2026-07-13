#include "engine/core/gm_types.h"
#include <algorithm>
#include <cmath>

#include "engine/physics/geometry/geometry_helpers.h"
namespace {

bool AxisIntervalsOverlap(float centerA,
                          float extentA,
                          float centerB,
                          float extentB) {
    const float distance = std::fabs(centerB - centerA);
    const float extentSum = extentB + extentA;
    return !(extentSum < distance);
}

GmVec3 Minimum(const GmVec3 &left, const GmVec3 &right) {
    return {
            std::min(left.x, right.x),
            std::min(left.y, right.y),
            std::min(left.z, right.z),
    };
}

GmVec3 Maximum(const GmVec3 &left, const GmVec3 &right) {
    return {
            std::max(left.x, right.x),
            std::max(left.y, right.y),
            std::max(left.z, right.z),
    };
}

GmMat3 AbsoluteElements(const GmMat3 &matrix) {
    GmMat3 result;
    result.basisX = {
            std::fabs(matrix.basisX.x),
            std::fabs(matrix.basisX.y),
            std::fabs(matrix.basisX.z),
    };
    result.basisY = {
            std::fabs(matrix.basisY.x),
            std::fabs(matrix.basisY.y),
            std::fabs(matrix.basisY.z),
    };
    result.basisZ = {
            std::fabs(matrix.basisZ.x),
            std::fabs(matrix.basisZ.y),
            std::fabs(matrix.basisZ.z),
    };
    return result;
}

}  // namespace

int GmBoxAligned::TestInter(const GmBoxAligned &rhs) const {
    if (!AxisIntervalsOverlap(center.z, halfExtents.z,
                              rhs.center.z, rhs.halfExtents.z)) {
        return 0;
    }
    if (!AxisIntervalsOverlap(center.y, halfExtents.y,
                              rhs.center.y, rhs.halfExtents.y)) {
        return 0;
    }
    return AxisIntervalsOverlap(center.x, halfExtents.x,
                                rhs.center.x, rhs.halfExtents.x)
            ? 1
            : 0;
}

void GmBoxAligned::SetMinMax(const GmVec3 &minPoint,
                             const GmVec3 &maxPoint) {
    center = GmMath::Scale(GmMath::Add(minPoint, maxPoint), 0.5f);
    halfExtents = GmMath::Scale(
            GmMath::Subtract(maxPoint, minPoint),
            0.5f);
}

void GmBoxAligned::GetDiag(GmVec3 &out) const {
    out = GmMath::Scale(halfExtents, 2.0f);
}

void GmBoxAligned::SetMult(const GmBoxAligned &box,
                           const GmIso4 &transform) {
    const GmBoxAligned source = box;
    center = GmMath::TransformPoint(transform, source.center);
    const GmMat3 absoluteRotation = AbsoluteElements(transform.rotation);
    halfExtents = GmMath::TransformDirection(
            absoluteRotation,
            source.halfExtents);
}

void GmBoxAligned::Mult(const GmIso4 &transform) {
    SetMult(*this, transform);
}

void GmBoxAligned::Union(const GmBoxAligned &other) {
    if (!(halfExtents.x >= 0.0f)) {
        *this = other;
        return;
    }
    if (!(other.halfExtents.x >= 0.0f)) {
        return;
    }

    const GmVec3 minPoint = Minimum(
            GmMath::Subtract(center, halfExtents),
            GmMath::Subtract(other.center, other.halfExtents));
    const GmVec3 maxPoint = Maximum(
            GmMath::Add(center, halfExtents),
            GmMath::Add(other.center, other.halfExtents));
    SetMinMax(minPoint, maxPoint);
}

unsigned long GmRectAligned::IsInside(const GmVec2 &point) const {
    return minX < point.x && point.x < maxX &&
           minY < point.y && point.y < maxY;
}
