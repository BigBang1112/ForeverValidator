#include "engine/core/gm_types.h"
#include "engine/physics/geometry/geometry_helpers.h"
void GmIso4::Set(const GmMat3 &newRotation,
                 const GmVec3 &newTranslation) {
    rotation = newRotation;
    translation = newTranslation;
}

void GmIso4::SetTranslation(const GmVec3 &newTranslation) {
    translation = newTranslation;
}

void GmIso4::SetIdentity(void) {
    rotation.SetIdentity();
    translation = {};
}

void GmIso4::Inverse(void) {
    SetInverse(*this);
}

void GmIso4::SetUScaleTrans(float scale,
                            const GmVec3 &newTranslation) {
    rotation.SetDiagonal(scale, scale, scale);
    translation = newTranslation;
}

void GmIso4::SetNUScaleTrans(const GmVec3 &scale,
                             const GmVec3 &newTranslation) {
    rotation.SetDiagonal(scale.x, scale.y, scale.z);
    translation = newTranslation;
}

void GmIso4::SetInverse(const GmIso4 &rhs) {
    GmMat3 inverseRotation;
    inverseRotation.SetTranspose(rhs.rotation);
    const GmVec3 inverseTranslation = GmMath::TransformDirection(
            inverseRotation,
            GmMath::Negate(rhs.translation));
    Set(inverseRotation, inverseTranslation);
}

void GmIso4::SetMult(const GmIso4 &lhs, const GmIso4 &rhs) {
    const GmIso4 left = lhs;
    const GmIso4 right = rhs;
    rotation = GmMath::Compose(left.rotation, right.rotation);
    translation = GmMath::TransformPoint(right, left.translation);
}

void GmIso4::Mult(const GmIso4 &rhs) {
    SetMult(*this, rhs);
}

void GmIso4::MultInverse(const GmIso4 &rhs) {
    GmIso4 inverse;
    inverse.SetInverse(rhs);
    Mult(inverse);
}

void GmIso4::UScaleSetInverse(const GmIso4 &rhs) {
    const float scaleLengthSquared =
            GmMath::LengthSquared(rhs.rotation.Row(GmAxis::X));
    const float inverseScaleLengthSquared = 1.0f / scaleLengthSquared;

    GmMat3 inverseRotation;
    inverseRotation.SetTranspose(rhs.rotation);
    inverseRotation.Mult(inverseScaleLengthSquared);
    const GmVec3 inverseTranslation = GmMath::TransformDirection(
            inverseRotation,
            GmMath::Negate(rhs.translation));
    Set(inverseRotation, inverseTranslation);
}

void GmIso4::NUScaleSetInverse(const GmIso4 &rhs) {
    GmMat3 inverseRotation;
    inverseRotation.SetTranspose(rhs.rotation);
    constexpr GmAxis Axes[] = {GmAxis::X, GmAxis::Y, GmAxis::Z};
    for (GmAxis axis : Axes) {
        const GmVec3 row = inverseRotation.Row(axis);
        inverseRotation.SetRow(
                axis,
                GmMath::Scale(row, 1.0f / GmMath::LengthSquared(row)));
    }
    const GmVec3 inverseTranslation = GmMath::TransformDirection(
            inverseRotation,
            GmMath::Negate(rhs.translation));
    Set(inverseRotation, inverseTranslation);
}

void GmIso4::LeftMult(const GmIso4 &lhs) {
    SetMult(lhs, *this);
}

void GmIso4::RotateX(float angle) {
    rotation.RotateX(angle);
}

void GmIso4::RotateY(float angle) {
    rotation.RotateY(angle);
}

void GmIso4::RotateZ(float angle) {
    rotation.RotateZ(angle);
}

void GmIso4::SetBlend(const GmIso4 &from,
                      const GmIso4 &to,
                      float blend) {
    rotation.SetBlend(from.rotation, to.rotation, blend);
    translation = GmMath::Add(
            GmMath::Scale(from.translation, 1.0f - blend),
            GmMath::Scale(to.translation, blend));
}

unsigned long GmIso4::IsNearlyEqual(const GmIso4 &rhs) const {
    return translation.IsNearlyEqual(rhs.translation) &&
           rotation.IsNearlyEqual(rhs.rotation);
}
