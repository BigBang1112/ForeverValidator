#include "engine/core/gm_types.h"
#include <algorithm>
#include <array>
#include <cmath>

#include "engine/core/binary32_math.h"
namespace {

GmAxis AxisFromIndex(unsigned index) {
    switch (index) {
        case 0u:
            return GmAxis::X;
        case 1u:
            return GmAxis::Y;
        default:
            return GmAxis::Z;
    }
}

float &VectorComponent(GmQuat &quaternion, unsigned index) {
    switch (index) {
        case 0u:
            return quaternion.x;
        case 1u:
            return quaternion.y;
        default:
            return quaternion.z;
    }
}

float QuaternionDot(const GmQuat &left, const GmQuat &right) {
    const float xy = left.x * right.x + left.y * right.y;
    const float zw = left.z * right.z + left.w * right.w;
    return xy + zw;
}

}  // namespace

void GmQuat::Set(const GmQuat &rhs) {
    *this = rhs;
}

void GmQuat::SetIdentity(void) {
    *this = {1.0f, 0.0f, 0.0f, 0.0f};
}

void GmQuat::SetMult(const GmQuat &lhs, const GmQuat &rhs) {
    const GmQuat left = lhs;
    const GmQuat right = rhs;
    const float vectorDot =
            (left.x * right.x + left.y * right.y) +
            left.z * right.z;

    GmQuat result;
    result.w = left.w * right.w - vectorDot;
    result.x = ((left.x * right.w + left.w * right.x) +
                left.y * right.z) -
               left.z * right.y;
    result.y = ((left.w * right.y + left.y * right.w) +
                left.z * right.x) -
               left.x * right.z;
    result.z = ((left.w * right.z + left.z * right.w) +
                left.x * right.y) -
               left.y * right.x;
    *this = result;
}

void GmQuat::Mult(const GmQuat &rhs) {
    SetMult(*this, rhs);
}

void GmQuat::SetInverse(const GmQuat &rhs) {
    *this = {rhs.w, -rhs.x, -rhs.y, -rhs.z};
}

void GmQuat::Normalize(void) {
    const float xy = x * x + w * w;
    const float lengthSquared = (xy + y * y) + z * z;
    const float inverseLength = 1.0f / CIsqrt(lengthSquared);
    w *= inverseLength;
    x *= inverseLength;
    y *= inverseLength;
    z *= inverseLength;
}

void GmQuat::Set(const GmMat3 &matrix) {
    const float trace =
            (matrix.Element(GmAxis::X, GmAxis::X) +
             matrix.Element(GmAxis::Y, GmAxis::Y)) +
            matrix.Element(GmAxis::Z, GmAxis::Z);
    if (trace > 0.0f) {
        const float root = CIsqrt(trace + 1.0f);
        const float scale = 0.5f / root;
        w = root * 0.5f;
        x = (matrix.Element(GmAxis::Z, GmAxis::Y) -
             matrix.Element(GmAxis::Y, GmAxis::Z)) * scale;
        y = (matrix.Element(GmAxis::X, GmAxis::Z) -
             matrix.Element(GmAxis::Z, GmAxis::X)) * scale;
        z = (matrix.Element(GmAxis::Y, GmAxis::X) -
             matrix.Element(GmAxis::X, GmAxis::Y)) * scale;
        return;
    }

    constexpr std::array<unsigned, 3> NextAxis{{1u, 2u, 0u}};
    unsigned dominant = 0u;
    if (matrix.Element(GmAxis::X, GmAxis::X) <
        matrix.Element(GmAxis::Y, GmAxis::Y)) {
        dominant = 1u;
    }
    if (matrix.Element(AxisFromIndex(dominant), AxisFromIndex(dominant)) <
        matrix.Element(GmAxis::Z, GmAxis::Z)) {
        dominant = 2u;
    }

    const unsigned next = NextAxis[dominant];
    const unsigned final = NextAxis[next];
    const GmAxis dominantAxis = AxisFromIndex(dominant);
    const GmAxis nextAxis = AxisFromIndex(next);
    const GmAxis finalAxis = AxisFromIndex(final);
    const float rootArgument =
            (matrix.Element(dominantAxis, dominantAxis) -
             (matrix.Element(finalAxis, finalAxis) +
              matrix.Element(nextAxis, nextAxis))) +
            1.0f;
    const float root = CIsqrt(rootArgument);
    const float scale = 0.5f / root;

    VectorComponent(*this, dominant) = root * 0.5f;
    w = (matrix.Element(finalAxis, nextAxis) -
         matrix.Element(nextAxis, finalAxis)) * scale;
    VectorComponent(*this, next) =
            (matrix.Element(dominantAxis, nextAxis) +
             matrix.Element(nextAxis, dominantAxis)) * scale;
    VectorComponent(*this, final) =
            (matrix.Element(dominantAxis, finalAxis) +
             matrix.Element(finalAxis, dominantAxis)) * scale;
}

void GmQuat::SetSlerp(GmQuat from, const GmQuat &to, float blend) {
    float dot = QuaternionDot(from, to);
    if (dot < 0.0f) {
        dot = -dot;
        from.w = -from.w;
        from.x = -from.x;
        from.y = -from.y;
        from.z = -from.z;
    }
    dot = std::clamp(dot, 0.0f, 1.0f);

    float fromWeight = 1.0f - blend;
    float toWeight = blend;
    const float weightMagnitude =
            std::max(std::fabs(fromWeight), std::fabs(toWeight));
    if ((1.0f - dot) * weightMagnitude > 1.0e-5f) {
        const float angle = CIacos(dot);
        const float sine = CIsin(angle);
        if (std::fabs(sine) >= 1.0e-5f) {
            const float inverseSine = 1.0f / sine;
            fromWeight = CIsin(angle * fromWeight) * inverseSine;
            toWeight = CIsin(angle * blend) * inverseSine;
        }
    }

    w = fromWeight * from.w + toWeight * to.w;
    x = fromWeight * from.x + toWeight * to.x;
    y = fromWeight * from.y + toWeight * to.y;
    z = fromWeight * from.z + toWeight * to.z;
}

GmSpring<float>::GmSpring(void)
        : stiffness(40.0f),
          damping(GetCriticalKa()),
          value(0.0f),
          target(0.0f),
          velocity(0.0f) {
}

float GmSpring<float>::GetCriticalKa(void) {
    const float root = CIsqrt(stiffness);
    return root + root;
}

void GmSpring<float>::ClearVals(void) {
    value = 0.0f;
    target = 0.0f;
    velocity = 0.0f;
}

void GmSpring<float>::Integrate(float dt) {
    const float acceleration =
            (target - value) * stiffness - damping * velocity;
    const float nextVelocity = acceleration * dt + velocity;
    velocity = nextVelocity;
    value = dt * nextVelocity + value;
}
