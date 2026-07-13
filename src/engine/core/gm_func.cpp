#include "engine/core/gm_func.h"
#include <cmath>
#include <cstdint>

#include "engine/core/binary32_math.h"
#include "engine/game/game_random_sequence.h"
namespace {

constexpr float Pi = 3.1415927f;
constexpr float HalfPi = Pi * 0.5f;
constexpr float SafeTrigInteriorLimit = 1.0f - 1e-6f;
constexpr float AcosLowClampResult = Pi;
constexpr float AsinHighClampResult = HalfPi;
constexpr float AsinLowClampResult = -HalfPi;

class DeterministicRandomSequence {
  public:
    unsigned long Next15Bits(void) {
        state = state * 214013u + 2531011u;
        return static_cast<unsigned long>((state >> 16u) & 0x7fffu);
    }

    std::uint32_t State() const noexcept { return state; }
    void SetState(std::uint32_t value) noexcept { state = value; }

  private:
    std::uint32_t state = 1u;
};

DeterministicRandomSequence &GameRandomSequence(void) {
    static DeterministicRandomSequence sequence;
    return sequence;
}

} // namespace

namespace tmnf::simulation {

std::uint32_t CaptureGameRandomState() noexcept {
    return GameRandomSequence().State();
}

void RestoreGameRandomState(std::uint32_t state) noexcept {
    GameRandomSequence().SetState(state);
}

void ResetGameRandomSequence() noexcept {
    GameRandomSequence().SetState(1u);
}

}  // namespace tmnf::simulation

float GmFunc::Sign(float value) { return std::signbit(value) ? -1.0f : 1.0f; }

float GmFunc::AcosSafe(float value) {
    if (value < -SafeTrigInteriorLimit) {
        return AcosLowClampResult;
    }
    if (SafeTrigInteriorLimit < value) {
        return 0.0f;
    }
    return CIacos(value);
}

float GmFunc::AsinSafe(float value) {
    if (value < -SafeTrigInteriorLimit) {
        return AsinLowClampResult;
    }
    if (SafeTrigInteriorLimit < value) {
        return AsinHighClampResult;
    }
    return CIasin(value);
}

float GmFunc::Mod(float value, float minValue, float maxValue) {
    if (minValue < value && value < maxValue) {
        return value;
    }

    const float period = maxValue - minValue;
    float wrapped = CIfmod(value - minValue, period);
    if (wrapped < 0.0f) {
        wrapped += period;
    }
    return wrapped + minValue;
}

unsigned long GmFunc::Mod(int value, int modulus) {
    int result = value % modulus;
    if (result < 0) {
        result += modulus;
    }
    return static_cast<unsigned long>(result);
}

unsigned long GmFunc::Min(unsigned long left, unsigned long right) {
    return left > right ? right : left;
}

unsigned long GmFunc::RandNat(unsigned long minValue, unsigned long maxValue) {
    const unsigned long randomValue = GameRandomSequence().Next15Bits();
    const unsigned long range = maxValue - minValue + 1ul;
    const float unitRandom =
            Binary32::FromUnsignedInteger(static_cast<std::uint32_t>(randomValue)) / 32768.0f;
    const float scaledRange =
            unitRandom * Binary32::FromUnsignedInteger(static_cast<std::uint32_t>(range));
    const float result =
            scaledRange + Binary32::FromUnsignedInteger(static_cast<std::uint32_t>(minValue));
    return static_cast<unsigned long>(result);
}

unsigned long GmFunc::IsPowerOfTwo(unsigned long value) {
    if (value == 0ul || value > UINT32_MAX) {
        return 0ul;
    }
    const std::uint32_t bits = static_cast<std::uint32_t>(value);
    return bits != 0x80000000u && (bits & (bits - 1u)) == 0u ? 1ul : 0ul;
}

int GmFunc::IsANumber(float value) { return value == value; }
