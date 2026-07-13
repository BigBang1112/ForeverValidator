#include "engine/scene/scene_vehicle_car_tuning.h"
#include "engine/core/binary32_math.h"
#include "engine/core/func_keys_real.h"
#include "engine/scene/scene_vehicle_math.h"
CSceneVehicleTuning::CSceneVehicleTuning(void) = default;

CSceneVehicleTuning::~CSceneVehicleTuning(void) = default;

void CSceneVehicleTuning::CreateDefaultData(void) {}

CSceneVehicleCarTuning::CSceneVehicleCarTuning(void) { ResetToDefaults(); }

CSceneVehicleCarTuning::~CSceneVehicleCarTuning(void) = default;

void CSceneVehicleCarTuning::ResetToDefaults(void) {
  engineSpeedNorm = 0.0f;
  lowSpeedFrictionMagnitude = 0.0f;
  lowSpeedLinearDamping = 0.0f;
  lateralContactSlowDownFromSpeedCurve.Reset();
  maxSideFrictionFromSpeedCurve.Reset();
  rolloverLateralFromSpeedCurve.Reset();
  rolloverLateralCoefFromAngleCurve.Reset();
  wheelForceMode = CSceneVehicleCarWheelForceMode_DirectSpring;
  handlingModel = CSceneVehicleCarHandlingModel_Standard;
  visual = {};
  steering = {};
  suspension = {};
  contactResponse = {};
  bodyAirResponse = {};
  radiusSteering = {};
  slipResponse = {};
  gearedDrive = {};
  water = {};
  turbo = {};
  feedback = {};

  contactResponse.bodyImpactFeedbackHighThreshold = 20.0f;
  engineSpeedNorm = 200.0f / 3.6f;
  gearedDrive.transmission.reverseSpeedNorm = 50.0f / 3.6f;
  gearedDrive.forwardAccelBase = 20.0f;
  gearedDrive.forwardAccelCapWhenSlipping = 500.0f;
  gearedDrive.forwardAccelCap = 500.0f;
  lowSpeedFrictionMagnitude = 1.0f;
  gearedDrive.speedLimitForce = 10.0f;
  visual.wheelSpeedBase = 1.0f;
  visual.wheelSpeedScale = 0.5f;
  steering.assistFullSpeed = 20.0f;
  steering.slewRate = 20.0f;
  gearedDrive.sideForceToDriveTorqueScale = 0.01f;
  gearedDrive.lateralForceScale = -1.0f;
  gearedDrive.slippingSideFrictionScale = 0.6f;
  gearedDrive.driveSideFrictionSlipBlend = 1.0f;
  contactResponse.pointImpulseAngularYScale = 1.0f;
  contactResponse.pointImpulseAngularScale = 1.0f;
  turbo.impulseScaleA = 3.0f;
  turbo.impulseScaleB = 3.0f;
  turbo.durationA = 1000u;
  turbo.durationB = 1000u;
  contactResponse.specialContactImpulseMagnitude = 10.0f;
  contactResponse.specialSolidFeedbackValue = 0.1f;
  suspension.wheelStaticSpringScale = 0.2f;
  bodyAirResponse.solidPhysicalMass = 800.0f;
  bodyAirResponse.solidInertiaMass = 1.0f;
  bodyAirResponse.solidInertiaBoxSize = {0.5f, 0.5f, 0.5f};
  contactResponse.pointImpulseAngularSpeedMax = 100.0f;
  contactResponse.pointImpulseLinearSpeedGrowthLimitSq = 10000.0f;
  bodyAirResponse.airborneSolidFeedback0 = 0.1f;
  bodyAirResponse.airTorqueLinearCoef = 0.5f;
  bodyAirResponse.airTorqueQuadraticCoef = 1.0f;
  bodyAirResponse.groundedSolidFeedback1 = 1.0f;
  bodyAirResponse.airborneSolidFeedback1 = 1.0f;
  bodyAirResponse.solidPhysicalResponseCoefB = 0.3f;
  contactResponse.singleMaterial = EPlugSurfaceMaterialId_Rubber;
  contactResponse.bodyContactTangentLimitOther = 0.8f;
  contactResponse.bodyContactTangentLimitMetal = 0.4f;
  suspension.wheelAbsorbFollowCoef = 5.0f;
  radiusSteering.steerTorqueSpeedScale = 1.0f;
  radiusSteering.angularDampingLinear = 1.0f;
  radiusSteering.lateralFrictionLinear = 1.0f;
  radiusSteering.captureExitSideSpeedMax = 1.0f;
  radiusSteering.inputSteerRadiusScale = 1.0f;
  radiusSteering.slippingFrictionScale = 1.0f;
  radiusSteering.capturedAngleRate = 0.005f;
  radiusSteering.capturedAngleRadiusScale = 0.3f;
  radiusSteering.steerAngleLimit = SceneVehicleMath::QuarterPi;
  radiusSteering.steerAngleFromInputScale = SceneVehicleMath::QuarterPi;
  slipResponse.slippingAccelScale = 1.0f;
  slipResponse.lateralSlowDownTickWindow = 500u;
  slipResponse.rolloverTorqueCap = 20.0f;
  slipResponse.steeringMemoryTicks = 1u;
  gearedDrive.slipRatioScale = 1.0f;
  water.buoyancyForce = 1.0f;
  water.splashHorizontalSpeedThreshold = 200.0f / 3.6f;
  water.splashTotalSpeedThreshold = 50.0f;
  water.angularLinearDamping = 0.1f;
  water.angularSpeedDamping = 0.2f;
  feedback.forceDivisor = 5.0f;
  gearedDrive.burnout.reverseForceThreshold = 200.0f;
  gearedDrive.currentForceTorqueMin = 0.01f;
  gearedDrive.currentTorqueXScale = 0.1f;
  gearedDrive.currentTorqueZScale = 0.1f;
  gearedDrive.perSlippingWheelAccelScale = 0.5f;
  gearedDrive.lowSpeedBSlippingGripScale = 0.2f;
  gearedDrive.forwardAccelCapWhenSlippingReverse = 100.0f;
  gearedDrive.forwardAccelCapReverse = 50.0f;
  gearedDrive.burnout.donutSpeedHigh = 10.0f;
  gearedDrive.burnout.donutSpeedLow = 3.0f;
  gearedDrive.burnout.lateralCorrectionScale = 1.0f;
  gearedDrive.burnout.angleTorqueScale = 6.0f;
  gearedDrive.burnout.angularDampingLinear = 2.0f;
  gearedDrive.burnout.angleReturnQuadratic = 0.5f;
  gearedDrive.burnout.tangentAngularDamping = 0.5f;
  gearedDrive.burnout.radiusCorrectionScale = 1.0f;
  gearedDrive.burnout.radiusCorrectionSpeedScale = 0.2f;
  gearedDrive.burnout.radiusMin = 20.0f;
  gearedDrive.burnout.tangentSpeedMax = 10.0f;
  gearedDrive.burnout.angleLimit = 0.35f;
  gearedDrive.burnout.angleLimitPositive = 1.7453f;
  gearedDrive.burnout.angleLimitNegative = 0.785f;
  gearedDrive.burnout.durationTicks = 1000u;
  gearedDrive.burnout.driveFadeScale = 0.5f;
  gearedDrive.burnout.sideForceFadeScale = 0.01f;
  gearedDrive.burnout.exitDurationTicks = 500u;
  gearedDrive.burnout.exitAccelFadeScale = 4.0f;
  gearedDrive.burnout.exitMinSpeed = 5.0f;
  gearedDrive.burnout.exitSteerGripScale = 1.0f;
  gearedDrive.burnout.exitBonusAccelScale = 10.0f;
  gearedDrive.burnout.wheelAngularSpeedOverride = 150.0f;
  gearedDrive.burnout.exitSideFrictionScale = 1.0f;
  gearedDrive.input.engineInputMaximum = 0.8f;
  gearedDrive.input.burnoutHoldInputRise = 5000.0f;
  gearedDrive.input.airborneInputRise = 5000.0f;
  gearedDrive.input.airborneInputFall = 2500.0f;
  gearedDrive.input.groundInputBrake = 5000.0f;
  gearedDrive.input.groundInputRise = 10000.0f;
  gearedDrive.input.transitionInputRise = 10000.0f;
  gearedDrive.input.groundInputFall = 4000.0f;
  gearedDrive.input.forwardTransitionSpeedHigh = 3.0f;
  gearedDrive.input.forwardTransitionSpeedLow = -2.0f;
  gearedDrive.input.reverseTransitionSpeedHigh = 2.0f;
  gearedDrive.input.reverseTransitionSpeedLow = -3.0f;
  gearedDrive.dirtSlideSideForceScale = 0.05f;
  gearedDrive.dirtSlideGateScale = 20.0f;
  gearedDrive.dirtSlideForwardForceScale = 15.0f;
  gearedDrive.dirtSlideForwardGateScale = 1.0f;
  bodyAirResponse.airControlYSwitchThreshold = SceneVehicleMath::Pi;
  feedback.surfaceBaseRate = -0.3f;
  contactResponse.wheelImpactFeedbackHighThreshold = 20.0f;
  contactResponse.wheelImpactFeedbackLowThreshold = 5.0f;
  contactResponse.bodyImpactFeedbackLowThreshold = 5.0f;
}

float CSceneVehicleCarTuning::EvaluateCurve(
    const CSceneVehicleTuningCurve &curve, float x) const {
  unsigned long keyIndex = 0ul;
  float out = 0.0f;
  curve.Value().GetValue(x, out, keyIndex);
  return out;
}

float CSceneVehicleCarTuning::EvaluateSpeedCurve(
    const CSceneVehicleTuningCurve &curve, float speed) const {
  return EvaluateCurve(curve, (Binary32::FromDouble(static_cast<double>((speed)) *
                              static_cast<double>(3.6f))));
}

float CSceneVehicleCarTuning::EvaluateLinearSpeedCurve(
    const CSceneVehicleTuningCurve &curve, float speed) const {
  curve.Value().SetInterpolation(CFuncKeysReal::Constant);
  return EvaluateSpeedCurve(curve, speed);
}

float CSceneVehicleCarTuning::GetMaxSideFrictionFromSpeed(float speed) const {
  return EvaluateSpeedCurve(maxSideFrictionFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::GetAccelFromSpeed(float speed) const {
  return EvaluateLinearSpeedCurve(slipResponse.accelFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::GetRolloverLateralFromSpeed(float speed) const {
  return EvaluateSpeedCurve(rolloverLateralFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::GetSteerDriveTorqueFromSpeed(float speed) const {
  return EvaluateSpeedCurve(steering.driveTorqueFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::GetLateralContactSlowDownFromSpeed(
    float speed) const {
  return EvaluateLinearSpeedCurve(lateralContactSlowDownFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::GetSteerSlowDownFromSpeed(float speed) const {
  return EvaluateLinearSpeedCurve(steering.slowDownFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::GetRolloverLateralCoefFromAngle(
    float angle) const {
  return EvaluateCurve(rolloverLateralCoefFromAngleCurve, angle);
}

float CSceneVehicleCarTuning::M4GetSteerRadiusFromSpeed(float speed) const {
  return EvaluateSpeedCurve(radiusSteering.steerRadiusFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::M4GetMaxFrictionForceFromSpeed(
    float speed) const {
  return EvaluateSpeedCurve(radiusSteering.maxFrictionForceFromSpeedCurve,
                            speed);
}

float CSceneVehicleCarTuning::M5GetAccelFromSpeed(float speed) const {
  return EvaluateSpeedCurve(slipResponse.accelFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::M5GetSlippingAccelFromSpeed(float speed) const {
  const float value =
      EvaluateSpeedCurve(slipResponse.slippingAccelFromSpeedCurve, speed);
  return static_cast<float>(slipResponse.slippingAccelScale * value);
}

float CSceneVehicleCarTuning::M5GetSteerSlowDownFromSpeed(float speed) const {
  return EvaluateSpeedCurve(steering.slowDownFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::GetWaterFrictionFromSpeed(float speed) const {
  return EvaluateSpeedCurve(water.frictionFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::M5GetLateralContactSlowDownFromSpeed(
    float speed) const {
  return lateralContactSlowDownFromSpeedCurve.Value().GetValue(
      (Binary32::FromDouble(static_cast<double>((speed)) *
                              static_cast<double>(3.6f))), nullptr);
}

float CSceneVehicleCarTuning::M6GetModulationFromDamperAbsorbVal(
    float damperAbsorb) const {
  float normalized = 0.0f;
  if (suspension.damperModulationMinAbsorb ==
      suspension.damperModulationMaxAbsorb) {
    normalized = 0.0f;
  } else {
    normalized = (damperAbsorb - suspension.damperModulationMinAbsorb) /
                 (suspension.damperModulationMaxAbsorb -
                  suspension.damperModulationMinAbsorb);
  }
  return EvaluateCurve(suspension.damperAbsorbModulationCurve, normalized);
}

float CSceneVehicleCarTuning::M6GetRearGearAccelFromSpeed(float speed) const {
  return EvaluateSpeedCurve(
      gearedDrive.transmission.rearGearAccelFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::M6GetBurnoutRadiusFromSpeed(float speed) const {
  return EvaluateSpeedCurve(gearedDrive.burnout.radiusFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::M6GetLateralSpeedFromBurnoutRadius(
    float radius) const {
  const float value =
      EvaluateCurve(gearedDrive.burnout.lateralSpeedFromRadiusCurve, radius);
  return (Binary32::FromDouble(static_cast<double>((value)) /
                              static_cast<double>(3.6f)));
}

float CSceneVehicleCarTuning::M6GetBurnoutRolloverFromSpeed(float speed) const {
  return EvaluateSpeedCurve(gearedDrive.burnout.rolloverFromSpeedCurve, speed);
}

float CSceneVehicleCarTuning::M6GetDonutRolloverFromSpeed(float speed) const {
  return EvaluateSpeedCurve(gearedDrive.burnout.donutRolloverFromSpeedCurve,
                            speed);
}

float CSceneVehicleCarTuning::M6GetRolloverLateralFromSpeedRatio(
    float speedRatio) const {
  return EvaluateSpeedCurve(
      gearedDrive.burnout.rolloverLateralFromSpeedRatioCurve, speedRatio);
}

void CSceneVehicleCarTuning::M6InitRpmDelta(void) {
  CSceneVehicleCarTransmissionTuning &transmission = gearedDrive.transmission;
  const uint32_t gearCount =
      static_cast<uint32_t>(transmission.upshiftThreshold.size());
  transmission.targetInputBias[0] = 0.0f;
  transmission.targetInputBias[1] = 0.0f;

  for (uint32_t gearIndex = 2u; gearIndex < gearCount; gearIndex++) {
    float ratio = (transmission.gearSpeedRatio[gearIndex] /
                   transmission.gearSpeedRatio[gearIndex - 1u]);
    float biasTerm = (transmission.rpmWanted[gearIndex] -
                      transmission.upshiftThreshold[gearIndex - 1u] * ratio) *
                     gearedDrive.input.engineInputMaximum;
    transmission.targetInputBias[gearIndex] =
        (ratio * transmission.targetInputBias[gearIndex - 1u] + biasTerm);
  }

  const uint32_t lastIndex = gearCount - 1u;
  for (uint32_t gearIndex = 1u; gearIndex < lastIndex; gearIndex++) {
    float rpmDelta;
    if (gearedDrive.input.engineInputMaximum == 0.0f) {
      rpmDelta = 0.0f;
    } else {
      float downshiftTerm = (transmission.downshiftThreshold[gearIndex + 1u] -
                             transmission.targetInputBias[gearIndex + 1u] /
                                 gearedDrive.input.engineInputMaximum) *
                            transmission.gearSpeedRatio[gearIndex] /
                            transmission.gearSpeedRatio[gearIndex + 1u];
      rpmDelta = (downshiftTerm + transmission.targetInputBias[gearIndex] /
                                      gearedDrive.input.engineInputMaximum);
    }
    transmission.rpmDelta[gearIndex] = rpmDelta;
  }
}

void CSceneVehicleCarTuning::M6CheckRpmWantedConstistensy(void) {
  CSceneVehicleCarTransmissionTuning &transmission = gearedDrive.transmission;
  for (uint32_t index = 0u; index < transmission.downshiftThreshold.size();
       ++index) {
    const float threshold = transmission.downshiftThreshold[index];
    if (transmission.rpmWanted[index] < threshold) {
      transmission.rpmWanted[index] = threshold;
    }
  }
}

void CSceneVehicleCarTuning::OnNodLoaded(void) {
  CMwNod::OnNodLoaded();
  M6InitRpmDelta();
}

CSceneVehicleTunings::CSceneVehicleTunings(void) = default;

CSceneVehicleTunings::~CSceneVehicleTunings(void) = default;

void CSceneVehicleTunings::SetActiveTuning(
    CSceneVehicleCarTuning &activeTuning) {
  activeTuning_ = std::ref(activeTuning);
}

CSceneVehicleCarTuning *CSceneVehicleTunings::ActiveTuning(void) const {
  return activeTuning_.has_value() ? &activeTuning_->get() : nullptr;
}
