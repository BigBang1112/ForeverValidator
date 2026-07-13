#ifndef TMNF_BINARY32_MATH_H
#define TMNF_BINARY32_MATH_H

#include <stdint.h>

namespace Binary32 {

// Explicit round-to-nearest-even conversion for algorithms evaluated in
// double precision before producing a simulation value.
float FromDouble(double value);
float FromUnsignedInteger(uint32_t value);
uint32_t TruncateToUint32Modulo(float value);
bool HaveSameEncoding(float left, float right);

} // namespace Binary32

float CIsqrt(float value);
float CIacos(float value);
float CIasin(float value);
float CIcos(float value);
float CIsin(float value);
float CIatan2(float y, float x);
float CItan(float value);
float CIexp(float value);
float CIfmod(float value, float modulus);

#endif // TMNF_BINARY32_MATH_H
