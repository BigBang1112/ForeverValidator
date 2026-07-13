#include "engine/core/gm_types.h"
#include <cmath>
#include <utility>

#include "engine/core/binary32_math.h"
#include "engine/physics/geometry/geometry_helpers.h"
#include "engine/physics/geometry/physics_tolerances.h"
namespace {

GmAxis AxisFromIndex(unsigned long index) {
    switch (index) {
        case 0u:
            return GmAxis::X;
        case 1u:
            return GmAxis::Y;
        default:
            return GmAxis::Z;
    }
}

struct RotationTrig {
    float sine = 0.0f;
    float cosine = 1.0f;

    static RotationTrig FromAngle(float angle) {
        return {CIsin(angle), CIcos(angle)};
    }
};

bool HasUnitLength(const GmVec3 &value) {
    return std::fabs(CIsqrt(GmMath::LengthSquared(value)) - 1.0f) <
           0.001f;
}

}  // namespace

void GmMat3::GetLine(unsigned long index, GmVec3 &value) const {
    value = Basis(AxisFromIndex(index));
}

void GmMat3::SetLine(unsigned long index, const GmVec3 &value) {
    Basis(AxisFromIndex(index)) = value;
}

void GmMat3::Set(const GmMat3 &rhs) {
    *this = rhs;
}

void GmMat3::SetIdentity(void) {
    basisX = {1.0f, 0.0f, 0.0f};
    basisY = {0.0f, 1.0f, 0.0f};
    basisZ = {0.0f, 0.0f, 1.0f};
}

void GmMat3::Set(GmQuat quaternion) {
    const float twoX = quaternion.x * 2.0f;
    const float twoY = quaternion.y * 2.0f;
    const float twoZ = quaternion.z * 2.0f;
    const float xw2 = quaternion.w * twoX;
    const float yw2 = quaternion.w * twoY;
    const float zw2 = quaternion.w * twoZ;
    const float xx2 = quaternion.x * twoX;
    const float xy2 = quaternion.x * twoY;
    const float xz2 = quaternion.x * twoZ;
    const float yy2 = quaternion.y * twoY;
    const float yz2 = quaternion.y * twoZ;
    const float zz2 = quaternion.z * twoZ;

    basisX = {
            (1.0f - yy2) - zz2,
            xy2 + zw2,
            xz2 - yw2,
    };
    basisY = {
            xy2 - zw2,
            (1.0f - xx2) - zz2,
            yz2 + xw2,
    };
    basisZ = {
            xz2 + yw2,
            yz2 - xw2,
            (1.0f - xx2) - yy2,
    };
}

void GmMat3::Mult(float scale) {
    basisX = GmMath::Scale(basisX, scale);
    basisY = GmMath::Scale(basisY, scale);
    basisZ = GmMath::Scale(basisZ, scale);
}

void GmMat3::Transpose(void) {
    std::swap(basisX.y, basisY.x);
    std::swap(basisX.z, basisZ.x);
    std::swap(basisY.z, basisZ.y);
}

void GmMat3::SetTranspose(const GmMat3 &rhs) {
    const GmMat3 source = rhs;
    basisX = source.Row(GmAxis::X);
    basisY = source.Row(GmAxis::Y);
    basisZ = source.Row(GmAxis::Z);
}

void GmMat3::SetMult(const GmMat3 &lhs, const GmMat3 &rhs) {
    *this = GmMath::Compose(lhs, rhs);
}

void GmMat3::Mult(const GmMat3 &rhs) {
    *this = GmMath::Compose(*this, rhs);
}

void GmMat3::LeftMult(const GmMat3 &lhs) {
    *this = GmMath::Compose(lhs, *this);
}

void GmMat3::MultTranspose(const GmMat3 &rhs) {
    *this = GmMath::MultiplyByTranspose(*this, rhs);
}

void GmMat3::RotateX(float angle) {
    const RotationTrig trig = RotationTrig::FromAngle(angle);
    const GmVec3 oldY = Row(GmAxis::Y);
    const GmVec3 oldZ = Row(GmAxis::Z);
    SetRow(GmAxis::Y,
           GmMath::Add(GmMath::Scale(oldY, trig.cosine),
                       GmMath::Scale(oldZ, -trig.sine)));
    SetRow(GmAxis::Z,
           GmMath::Add(GmMath::Scale(oldY, trig.sine),
                       GmMath::Scale(oldZ, trig.cosine)));
}

void GmMat3::RotateY(float angle) {
    const RotationTrig trig = RotationTrig::FromAngle(angle);
    const GmVec3 oldX = Row(GmAxis::X);
    const GmVec3 oldZ = Row(GmAxis::Z);
    SetRow(GmAxis::X,
           GmMath::Add(GmMath::Scale(oldX, trig.cosine),
                       GmMath::Scale(oldZ, trig.sine)));
    SetRow(GmAxis::Z,
           GmMath::Add(GmMath::Scale(oldX, -trig.sine),
                       GmMath::Scale(oldZ, trig.cosine)));
}

void GmMat3::RotateZ(float angle) {
    const RotationTrig trig = RotationTrig::FromAngle(angle);
    const GmVec3 oldX = Row(GmAxis::X);
    const GmVec3 oldY = Row(GmAxis::Y);
    SetRow(GmAxis::X,
           GmMath::Add(GmMath::Scale(oldX, trig.cosine),
                       GmMath::Scale(oldY, -trig.sine)));
    SetRow(GmAxis::Y,
           GmMath::Add(GmMath::Scale(oldX, trig.sine),
                       GmMath::Scale(oldY, trig.cosine)));
}

void GmMat3::SetRotateQuarterY(unsigned long quarterTurn) {
    constexpr float QuarterTurnCosines[4] = {
            1.0f,
            0.0f,
            -1.0f,
            0.0f,
    };
    const unsigned index = static_cast<unsigned>(quarterTurn) & 3u;
    const float cosine = QuarterTurnCosines[index];
    const float sine = QuarterTurnCosines[(index + 3u) & 3u];
    basisX = {cosine, 0.0f, -sine};
    basisY = {0.0f, 1.0f, 0.0f};
    basisZ = {sine, 0.0f, cosine};
}

void GmMat3::SetUpVandDOV(const GmVec3 &up,
                          const GmVec3 &directionOfView) {
    const GmVec3 sideSeed = GmMath::Cross(up, directionOfView);
    const GmVec3 side = GmMath::NormalizeOr(
            sideSeed,
            sideSeed,
            PhysicsTolerance::SurfaceDirectionLengthSquared);
    const GmVec3 normalizedUp = GmMath::NormalizeOr(
            up,
            up,
            PhysicsTolerance::SurfaceDirectionLengthSquared);
    basisX = side;
    basisY = normalizedUp;
    basisZ = GmMath::Cross(side, normalizedUp);
}

unsigned long GmMat3::IsNearlyEqual(const GmMat3 &rhs) const {
    return Row(GmAxis::X).IsNearlyEqual(rhs.Row(GmAxis::X)) &&
           Row(GmAxis::Y).IsNearlyEqual(rhs.Row(GmAxis::Y)) &&
           Row(GmAxis::Z).IsNearlyEqual(rhs.Row(GmAxis::Z));
}

unsigned long GmMat3::Inverse(void) {
    const GmVec3 rowX = Row(GmAxis::X);
    const GmVec3 rowY = Row(GmAxis::Y);
    const GmVec3 rowZ = Row(GmAxis::Z);

    const float cofactor00 = rowZ.z * rowY.y - rowY.z * rowZ.y;
    const float cofactor01 = rowX.z * rowZ.y - rowX.y * rowZ.z;
    const float cofactor02 = rowX.y * rowY.z - rowX.z * rowY.y;
    const float determinant =
            (rowZ.x * cofactor02 + rowX.x * cofactor00) +
            rowY.x * cofactor01;
    if (determinant == 0.0f) {
        return 0u;
    }

    const float inverseDeterminant = 1.0f / determinant;
    GmMat3 inverse;
    inverse.basisX = {
            cofactor00 * inverseDeterminant,
            (rowZ.x * rowY.z - rowY.x * rowZ.z) * inverseDeterminant,
            (rowY.x * rowZ.y - rowZ.x * rowY.y) * inverseDeterminant,
    };
    inverse.basisY = {
            cofactor01 * inverseDeterminant,
            (rowZ.z * rowX.x - rowX.z * rowZ.x) * inverseDeterminant,
            (rowX.y * rowZ.x - rowX.x * rowZ.y) * inverseDeterminant,
    };
    inverse.basisZ = {
            cofactor02 * inverseDeterminant,
            (rowX.z * rowY.x - rowY.z * rowX.x) * inverseDeterminant,
            (rowY.y * rowX.x - rowX.y * rowY.x) * inverseDeterminant,
    };
    *this = inverse;
    return 1u;
}

unsigned long GmMat3::IsOrthogonal(void) const {
    const GmVec3 rowX = Row(GmAxis::X);
    const GmVec3 rowY = Row(GmAxis::Y);
    const GmVec3 rowZ = Row(GmAxis::Z);
    return std::fabs(GmMath::Dot(rowX, rowY)) < 0.001f &&
           std::fabs(GmMath::Dot(rowX, rowZ)) < 0.001f &&
           std::fabs(GmMath::Dot(rowY, rowZ)) < 0.001f;
}

unsigned long GmMat3::IsIndirect(void) const {
    const GmVec3 rowX = Row(GmAxis::X);
    const GmVec3 rowY = Row(GmAxis::Y);
    const GmVec3 rowZ = Row(GmAxis::Z);
    return GmMath::Dot(rowZ, GmMath::Cross(rowX, rowY)) < 0.0f;
}

unsigned long GmMat3::IsOrthonormal(void) const {
    return HasUnitLength(Row(GmAxis::X)) &&
           HasUnitLength(Row(GmAxis::Y)) &&
           HasUnitLength(Row(GmAxis::Z)) &&
           IsOrthogonal();
}

void GmMat3::OrthoNormalize(void) {
    const GmVec3 rowX = GmMath::NormalizeOr(
            Row(GmAxis::X),
            Row(GmAxis::X),
            PhysicsTolerance::SurfaceDirectionLengthSquared);
    const GmVec3 rowZSeed = GmMath::Cross(rowX, Row(GmAxis::Y));
    const GmVec3 rowZ = GmMath::NormalizeOr(
            rowZSeed,
            rowZSeed,
            PhysicsTolerance::SurfaceDirectionLengthSquared);
    SetRow(GmAxis::X, rowX);
    SetRow(GmAxis::Y, GmMath::Cross(rowZ, rowX));
    SetRow(GmAxis::Z, rowZ);
}

void GmMat3::SetBlend(const GmMat3 &from,
                      const GmMat3 &to,
                      float blend) {
    GmQuat fromRotation;
    fromRotation.Set(from);
    GmQuat toRotation;
    toRotation.Set(to);
    GmQuat blendedRotation;
    blendedRotation.SetSlerp(fromRotation, toRotation, blend);

    constexpr float RotationEpsilon = 1.0e-5f;
    if (std::fabs(fromRotation.x - blendedRotation.x) < RotationEpsilon &&
        std::fabs(fromRotation.y - blendedRotation.y) < RotationEpsilon &&
        std::fabs(fromRotation.z - blendedRotation.z) < RotationEpsilon &&
        std::fabs(fromRotation.w - blendedRotation.w) < RotationEpsilon) {
        Set(from);
        return;
    }
    Set(blendedRotation);
}
