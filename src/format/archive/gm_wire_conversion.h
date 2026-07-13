#pragma once

#include "engine/core/gm_types.h"
namespace GmWire {

inline GmMat3 DecodeMat3(const float *values) {
    GmMat3 matrix{};
    matrix.SetRow(GmAxis::X, {values[0], values[1], values[2]});
    matrix.SetRow(GmAxis::Y, {values[3], values[4], values[5]});
    matrix.SetRow(GmAxis::Z, {values[6], values[7], values[8]});
    return matrix;
}

inline GmIso4 DecodeIso4(const float *values) {
    GmIso4 transform{};
    transform.rotation.SetRow(
            GmAxis::X, {values[0], values[1], values[2]});
    transform.rotation.SetRow(
            GmAxis::Y, {values[3], values[4], values[5]});
    transform.rotation.SetRow(
            GmAxis::Z, {values[6], values[7], values[8]});
    transform.translation = {values[9], values[10], values[11]};
    return transform;
}

inline void EncodeMat3(const GmMat3 &matrix, float (&values)[9]) {
    const GmVec3 rowX = matrix.Row(GmAxis::X);
    const GmVec3 rowY = matrix.Row(GmAxis::Y);
    const GmVec3 rowZ = matrix.Row(GmAxis::Z);
    values[0] = rowX.x;
    values[1] = rowX.y;
    values[2] = rowX.z;
    values[3] = rowY.x;
    values[4] = rowY.y;
    values[5] = rowY.z;
    values[6] = rowZ.x;
    values[7] = rowZ.y;
    values[8] = rowZ.z;
}

inline void EncodeIso4(const GmIso4 &transform, float (&values)[12]) {
    const GmVec3 rowX = transform.rotation.Row(GmAxis::X);
    const GmVec3 rowY = transform.rotation.Row(GmAxis::Y);
    const GmVec3 rowZ = transform.rotation.Row(GmAxis::Z);
    values[0] = rowX.x;
    values[1] = rowX.y;
    values[2] = rowX.z;
    values[3] = rowY.x;
    values[4] = rowY.y;
    values[5] = rowY.z;
    values[6] = rowZ.x;
    values[7] = rowZ.y;
    values[8] = rowZ.z;
    values[9] = transform.translation.x;
    values[10] = transform.translation.y;
    values[11] = transform.translation.z;
}

}  // namespace GmWire
