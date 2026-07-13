#pragma once

#include <cmath>

#include "engine/core/gm_types.h"
namespace FiniteValues {

template <typename... Values>
bool All(Values... values) noexcept {
    return (... && std::isfinite(values));
}

inline bool IsFinite(const GmVec2 &value) noexcept {
    return All(value.x, value.y);
}

inline bool IsFinite(const GmVec3 &value) noexcept {
    return All(value.x, value.y, value.z);
}

inline bool IsFinite(const GmVec4 &value) noexcept {
    return All(value.x, value.y, value.z, value.w);
}

}  // namespace FiniteValues
