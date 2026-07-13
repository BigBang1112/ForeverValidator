#pragma once

// Shared numerical vocabulary for the focused vehicle-dynamics translation
// units. These helpers state the operation order used by the simulation and
// carry no serialized state.

#include "engine/scene/scene_vehicle_car.h"
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "engine/core/binary32_math.h"
#include "engine/core/cmw_nod.h"
#include "engine/core/func_keys_real.h"
#include "engine/physics/geometry/geometry_helpers.h"
#include "engine/core/gm_func.h"
#include "engine/physics/world/hms_zone.h"
#include "engine/core/mw_cmd_buffer_core.h"
#include "engine/rendering/plug_bitmap.h"
#include "engine/resources/plug_file_img.h"
#include "engine/scene/plug_physical_object.h"
#include "engine/scene/plug_solid.h"
#include "engine/physics/geometry/plug_surface_material_library.h"
#include "engine/rendering/plug_tree.h"
#include "engine/scene/scene_vehicle_car_tuning.h"
#include "engine/scene/scene_vehicle_math.h"
namespace SceneVehicleCarDynamics {

inline constexpr u32 FeedbackRampContactId = 6u;
inline constexpr u32 TurboDurationAContactId = 7u;
inline constexpr u32 TurboDurationBContactId = 0x1au;
inline constexpr u32 TurboRouletteContactId = 0x1eu;
inline constexpr u32 ForcedLowSpeedFrictionContactId = 0x1du;

inline constexpr float VectorEpsilonSquared = 1.0e-10f;
inline constexpr float ScalarEpsilon = 1.0e-5f;
inline constexpr float LowSpeedGateThreshold = 0.1f;
inline constexpr float EngineInputActiveThreshold = 0.1f;
inline constexpr float GearShiftInputMemoryThreshold = 1000.0f;
inline constexpr float GearShiftCooldown = 0.025f;
inline constexpr float BurnoutReverseGearShiftCooldown = 0.002f;
inline constexpr float LegacyEngineGearChangeCooldown = 0.04f;
inline constexpr float LegacyEngineWeightedSpeedXScale = 0.3f;
inline constexpr float LegacyEngineWeightedSpeedYScale = 0.1f;
inline constexpr float LegacyEngineSpeedNormScale = 0.2f;
inline constexpr float LegacyEngineCooldownDecayScale = 1.9f;
inline constexpr float LegacyEngineGroundInputRate = 12.0f;
inline constexpr float LegacyEngineAirborneInputRate = 3.5f;
inline constexpr float SlipRpmScaleMax = 1.15f;
inline constexpr float SlipRpmScaleApproachRate = 0.3f;
inline constexpr float DirtSlideSpeedGate = 6.0f;
inline constexpr EPlugSurfaceMaterialId DirtSlideMaterial =
    EPlugSurfaceMaterialId_Dirt;
inline constexpr float DirtSlideSlowdownScale = 0.1f;
inline constexpr float DirtSlideFrontRearRatio = 1.5f;
inline constexpr float DirtSlideAbsXScale = 20.0f;
inline constexpr float TurboRouletteZeroPhaseEnd = 4.0f / 7.0f;
inline constexpr float TurboRouletteHalfPhaseEnd = 6.0f / 7.0f;
inline constexpr double TurboRouletteBoostBase = 1.0;
inline constexpr double SlopeAdherenceCosineRampPi =
    static_cast<double>(SceneVehicleMath::Pi);
inline constexpr double SlopeAdherenceCosineRampHalf = 0.5;
inline constexpr float WheelVisualDefaultMaxSteerDegrees = 30.0f;
inline constexpr float DegreesToRadiansDivisor = 180.0f;
inline constexpr double WheelLowSpeedGateASpeedScale = 200.0;
inline constexpr double WheelNoContactDamping = static_cast<double>(0.995f);
inline constexpr float WheelAngularAccelNegative = -100.0f;
inline constexpr float WheelAngularAccelPositive = 100.0f;

inline float UnsignedTickDelta(u32 later, u32 earlier) {
  return Binary32::FromUnsignedInteger(later - earlier);
}

inline GmVec3 TransformDirection(const GmIso4 &transform,
                                 const GmVec3 &vector) {
  return {
      transform.Element(GmAxis::X, GmAxis::X) * vector.x +
          transform.Element(GmAxis::X, GmAxis::Y) * vector.y +
          transform.Element(GmAxis::X, GmAxis::Z) * vector.z,
      transform.Element(GmAxis::Y, GmAxis::X) * vector.x +
          transform.Element(GmAxis::Y, GmAxis::Y) * vector.y +
          transform.Element(GmAxis::Y, GmAxis::Z) * vector.z,
      transform.Element(GmAxis::Z, GmAxis::X) * vector.x +
          transform.Element(GmAxis::Z, GmAxis::Y) * vector.y +
          transform.Element(GmAxis::Z, GmAxis::Z) * vector.z,
  };
}

inline GmVec3 TransformPoint(const GmIso4 &transform, const GmVec3 &point) {
  GmVec3 result = TransformDirection(transform, point);
  result.x += transform.translation.x;
  result.y += transform.translation.y;
  result.z += transform.translation.z;
  return result;
}

inline GmVec3 InverseTransformPoint(const GmIso4 &transform,
                                    const GmVec3 &point) {
  GmIso4 inverse;
  inverse.SetInverse(transform);
  GmVec3 result;
  result.SetMult(point, inverse);
  return result;
}

inline GmVec3 SolidLeverToPoint(const CPlugSolid &solid, const GmVec3 &point) {
  const GmVec3 &center = solid.Physical().Parameters().localCenterOfMass;
  return {point.x - center.x, point.y - center.y, point.z - center.z};
}

inline float SlopeAdherenceBlend(float slope, float minimumSlope,
                                 float maximumSlope) {
  if (!(minimumSlope <= slope)) {
    return 0.0f;
  }
  if (!(maximumSlope >= slope)) {
    return 1.0f;
  }
  const float angle = ((slope - minimumSlope) / (maximumSlope - minimumSlope)) *
                      SlopeAdherenceCosineRampPi * SlopeAdherenceCosineRampHalf;
  return 1.0f - CIcos(angle);
}

inline float ClampZeroOne(float value) {
  float clamped = 0.0f;
  if (!(value < 0.0f) && value != 0.0f) {
    clamped = value;
    if (value == value && 1.0f < value) {
      clamped = 1.0f;
    }
  }
  return clamped;
}

inline float ClampSymmetric(float value, float magnitude) {
  const float minimum = -magnitude;
  const float maximum = magnitude;
  if (value != value) {
    return value;
  }
  if (!(minimum < value)) {
    return minimum;
  }
  if (!(value < maximum)) {
    return maximum;
  }
  return value;
}

inline float SignNonNegative(float value) {
  return value < 0.0f ? -1.0f : 1.0f;
}

} // namespace SceneVehicleCarDynamics
