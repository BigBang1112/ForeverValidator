#pragma once

struct GmFunc {
    static float Sign(float value);
    static float AcosSafe(float value);
    static float AsinSafe(float value);
    static float Mod(float value, float minValue, float maxValue);
    static unsigned long Mod(int value, int modulus);
    static unsigned long Min(unsigned long left, unsigned long right);
    static unsigned long RandNat(
            unsigned long minValue,
            unsigned long maxValue);
    static unsigned long IsPowerOfTwo(unsigned long value);
    static int IsANumber(float value);
};
