#include "simulation/runtime/replay_vehicle_tuning_assembly.h"
#include <utility>
#include <vector>

namespace {

template <typename Target, typename Source>
void CopyVisual(Target &target, const Source &source) {
  target.wheelSpeedBase = source.wheelSpeedBase;
  target.wheelSpeedScale = source.wheelSpeedScale;
}

template <typename Target, typename Source>
void CopySteering(Target &target, const Source &source) {
  target.slewRate = source.slewRate;
  target.assistFullSpeed = source.assistFullSpeed;
  target.slowDownScale = source.slowDownScale;
}

template <typename Target, typename Source>
void CopySuspension(Target &target, const Source &source) {
  target.wheelSpringCoef = source.wheelSpringCoef;
  target.wheelDamperCoef = source.wheelDamperCoef;
  target.damperModulationMaxAbsorb = source.damperModulationMaxAbsorb;
  target.damperModulationMinAbsorb = source.damperModulationMinAbsorb;
  target.wheelRestDamperAbsorb = source.wheelRestDamperAbsorb;
  target.wheelStaticSpringScale = source.wheelStaticSpringScale;
  target.wheelAbsorbFollowCoef = source.wheelAbsorbFollowCoef;
}

template <typename Target, typename Source>
void CopyContactResponse(Target &target, const Source &source) {
  target.bodyImpactFeedbackHighThreshold =
      source.bodyImpactFeedbackHighThreshold;
  target.bodyImpactFeedbackLowThreshold = source.bodyImpactFeedbackLowThreshold;
  target.wheelImpactFeedbackHighThreshold =
      source.wheelImpactFeedbackHighThreshold;
  target.wheelImpactFeedbackLowThreshold =
      source.wheelImpactFeedbackLowThreshold;
  target.bodyContactTangentLimitOther = source.bodyContactTangentLimitOther;
  target.bodyContactTangentLimitMetal = source.bodyContactTangentLimitMetal;
  target.bodyContactImpulseMetal = source.bodyContactImpulseMetal;
  target.bodyContactImpulseOther = source.bodyContactImpulseOther;
  target.wheelContactImpulseOther = source.wheelContactImpulseOther;
  target.wheelContactImpulseMetal = source.wheelContactImpulseMetal;
  target.pointImpulseAngularYScale = source.pointImpulseAngularYScale;
  target.pointImpulseAngularScale = source.pointImpulseAngularScale;
  target.pointImpulseAngularSpeedMax = source.pointImpulseAngularSpeedMax;
  target.pointImpulseLinearSpeedGrowthLimitSq =
      source.pointImpulseLinearSpeedGrowthLimitSq;
  target.specialContactImpulseMagnitude = source.specialContactImpulseMagnitude;
  target.specialSolidFeedbackValue = source.specialSolidFeedbackValue;
  target.singleMaterial = source.singleMaterial;
}

template <typename Target, typename Source>
void CopyBodyAirResponse(Target &target, const Source &source) {
  target.solidPhysicalMass = source.solidPhysicalMass;
  target.solidInertiaMass = source.solidInertiaMass;
  target.solidInertiaBoxSize = source.solidInertiaBoxSize;
  target.solidCenterZHalfExtentScale = source.solidCenterZHalfExtentScale;
  target.solidCenterYOffset = source.solidCenterYOffset;
  target.solidPhysicalResponseCoefB = source.solidPhysicalResponseCoefB;
  target.airborneSolidFeedback0 = source.airborneSolidFeedback0;
  target.airTorqueLinearCoef = source.airTorqueLinearCoef;
  target.airTorqueQuadraticCoef = source.airTorqueQuadraticCoef;
  target.groundedSolidFeedback1 = source.groundedSolidFeedback1;
  target.airborneSolidFeedback1 = source.airborneSolidFeedback1;
  target.slopeAdherence1Min = source.slopeAdherence1Min;
  target.slopeAdherence1Max = source.slopeAdherence1Max;
  target.slopeAdherence2Min = source.slopeAdherence2Min;
  target.slopeAdherence2Max = source.slopeAdherence2Max;
  target.airControlMemoryTickWindow = source.airControlMemoryTickWindow;
  target.airControlYSwitchThreshold = source.airControlYSwitchThreshold;
}

template <typename Target, typename Source>
void CopyRadiusSteering(Target &target, const Source &source) {
  target.steerTorqueSpeedScale = source.steerTorqueSpeedScale;
  target.angularDampingLinear = source.angularDampingLinear;
  target.angularDampingQuadratic = source.angularDampingQuadratic;
  target.lateralFrictionLinear = source.lateralFrictionLinear;
  target.lateralFrictionQuadratic = source.lateralFrictionQuadratic;
  target.captureExitSideSpeedMax = source.captureExitSideSpeedMax;
  target.inputSteerRadiusScale = source.inputSteerRadiusScale;
  target.slippingFrictionScale = source.slippingFrictionScale;
  target.capturedAngleRate = source.capturedAngleRate;
  target.capturedAngleRadiusScale = source.capturedAngleRadiusScale;
  target.steerAngleLimit = source.steerAngleLimit;
  target.steerAngleFromInputScale = source.steerAngleFromInputScale;
}

template <typename Target, typename Source>
void CopySlipResponse(Target &target, const Source &source) {
  target.slippingAccelScale = source.slippingAccelScale;
  target.lateralSlowDownTickWindow = source.lateralSlowDownTickWindow;
  target.rolloverTorqueCap = source.rolloverTorqueCap;
  target.steeringMemoryTicks = source.steeringMemoryTicks;
  target.slipSlowdownTicks = source.slipSlowdownTicks;
  target.longitudinalTorqueScale = source.longitudinalTorqueScale;
}

template <typename Target, typename Source>
void CopyTransmission(Target &target, const Source &source) {
  target.reverseSpeedNorm = source.reverseSpeedNorm;
  target.gearSpeedRatio = source.gearSpeedRatio;
  target.upshiftThreshold = source.upshiftThreshold;
  target.downshiftThreshold = source.downshiftThreshold;
  target.rpmWanted = source.rpmWanted;
  target.targetInputBias = source.targetInputBias;
  target.rpmDelta = source.rpmDelta;
}

template <typename Target, typename Source>
void CopyBurnout(Target &target, const Source &source) {
  target.reverseForceThreshold = source.reverseForceThreshold;
  target.donutSpeedHigh = source.donutSpeedHigh;
  target.donutSpeedLow = source.donutSpeedLow;
  target.lateralCorrectionScale = source.lateralCorrectionScale;
  target.angleTorqueScale = source.angleTorqueScale;
  target.angularDampingLinear = source.angularDampingLinear;
  target.angleReturnQuadratic = source.angleReturnQuadratic;
  target.tangentAngularDamping = source.tangentAngularDamping;
  target.radiusCorrectionScale = source.radiusCorrectionScale;
  target.radiusCorrectionSpeedScale = source.radiusCorrectionSpeedScale;
  target.radiusMin = source.radiusMin;
  target.tangentSpeedMax = source.tangentSpeedMax;
  target.angleLimit = source.angleLimit;
  target.angleLimitPositive = source.angleLimitPositive;
  target.angleLimitNegative = source.angleLimitNegative;
  target.durationTicks = source.durationTicks;
  target.driveFadeScale = source.driveFadeScale;
  target.sideForceFadeScale = source.sideForceFadeScale;
  target.exitDurationTicks = source.exitDurationTicks;
  target.exitAccelFadeScale = source.exitAccelFadeScale;
  target.exitMinSpeed = source.exitMinSpeed;
  target.exitSteerGripScale = source.exitSteerGripScale;
  target.exitBonusAccelScale = source.exitBonusAccelScale;
  target.wheelAngularSpeedOverride = source.wheelAngularSpeedOverride;
  target.exitSideFrictionScale = source.exitSideFrictionScale;
}

template <typename Target, typename Source>
void CopyGearedDrive(Target &target, const Source &source) {
  target.forwardAccelBase = source.forwardAccelBase;
  target.forwardAccelSpeedCoef = source.forwardAccelSpeedCoef;
  target.forwardAccelCapWhenSlipping = source.forwardAccelCapWhenSlipping;
  target.forwardAccelCap = source.forwardAccelCap;
  target.speedLimitForce = source.speedLimitForce;
  target.forceZScale = source.forceZScale;
  target.sideForceToDriveTorqueScale = source.sideForceToDriveTorqueScale;
  target.slippingSteerTorqueScale = source.slippingSteerTorqueScale;
  target.lateralForceScale = source.lateralForceScale;
  target.slippingSideFrictionScale = source.slippingSideFrictionScale;
  target.sideFrictionSlipBlend = source.sideFrictionSlipBlend;
  target.driveSideFrictionSlipBlend = source.driveSideFrictionSlipBlend;
  target.slipRatioScale = source.slipRatioScale;
  target.currentForceTorqueMin = source.currentForceTorqueMin;
  target.currentTorqueXScale = source.currentTorqueXScale;
  target.currentTorqueZScale = source.currentTorqueZScale;
  target.perSlippingWheelAccelScale = source.perSlippingWheelAccelScale;
  target.lowSpeedBSlippingGripScale = source.lowSpeedBSlippingGripScale;
  target.forwardAccelCapWhenSlippingReverse =
      source.forwardAccelCapWhenSlippingReverse;
  target.forwardAccelCapReverse = source.forwardAccelCapReverse;
  target.dirtSlideSideForceScale = source.dirtSlideSideForceScale;
  target.dirtSlideGateScale = source.dirtSlideGateScale;
  target.dirtSlideForwardForceScale = source.dirtSlideForwardForceScale;
  target.dirtSlideForwardGateScale = source.dirtSlideForwardGateScale;
}

template <typename Target, typename Source>
void CopyEngineInput(Target &target, const Source &source) {
  target.burnoutHoldInputRise = source.burnoutHoldInputRise;
  target.airborneInputRise = source.airborneInputRise;
  target.airborneInputFall = source.airborneInputFall;
  target.groundInputBrake = source.groundInputBrake;
  target.groundInputRise = source.groundInputRise;
  target.transitionInputRise = source.transitionInputRise;
  target.groundInputFall = source.groundInputFall;
  target.forwardTransitionSpeedHigh = source.forwardTransitionSpeedHigh;
  target.forwardTransitionSpeedLow = source.forwardTransitionSpeedLow;
  target.reverseTransitionSpeedHigh = source.reverseTransitionSpeedHigh;
  target.reverseTransitionSpeedLow = source.reverseTransitionSpeedLow;
}

template <typename Target, typename Source>
void CopyWater(Target &target, const Source &source) {
  target.buoyancyForce = source.buoyancyForce;
  target.splashHorizontalSpeedThreshold = source.splashHorizontalSpeedThreshold;
  target.splashTotalSpeedThreshold = source.splashTotalSpeedThreshold;
  target.angularLinearDamping = source.angularLinearDamping;
  target.angularSpeedDamping = source.angularSpeedDamping;
}

template <typename Target, typename Source>
void CopyTurbo(Target &target, const Source &source) {
  target.impulseScaleA = source.impulseScaleA;
  target.impulseScaleB = source.impulseScaleB;
  target.durationA = source.durationA;
  target.durationB = source.durationB;
}

template <typename Target, typename Source>
void CopyFeedback(Target &target, const Source &source) {
  target.surfaceBaseRate = source.surfaceBaseRate;
  target.forceDivisor = source.forceDivisor;
}

void ResetCurve(CFuncKeysReal &curve) {
  curve.Reset();
  curve.SetInterpolation(CFuncKeysReal::Linear);
}

} // namespace

void ReplayVehicleTuningInstallation::Reset() {
#define RESET_CURVE(name) ResetCurve(curves.name)
  RESET_CURVE(lateralContactSlowDownFromSpeed);
  RESET_CURVE(maxSideFrictionFromSpeed);
  RESET_CURVE(rolloverLateralFromSpeed);
  RESET_CURVE(rolloverLateralCoefficientFromAngle);
  RESET_CURVE(wheelVisualSteerAngleFromSpeed);
  RESET_CURVE(steeringDriveTorqueFromSpeed);
  RESET_CURVE(steerSlowDownFromSpeed);
  RESET_CURVE(suspensionDamperAbsorbModulation);
  RESET_CURVE(airControlZScale);
  RESET_CURVE(radiusSteeringRadiusFromSpeed);
  RESET_CURVE(radiusSteeringMaxFrictionFromSpeed);
  RESET_CURVE(slipResponseAccelFromSpeed);
  RESET_CURVE(slipResponseSlippingAccelFromSpeed);
  RESET_CURVE(reverseGearAccelFromSpeed);
  RESET_CURVE(burnoutRolloverLateralFromSpeedRatio);
  RESET_CURVE(burnoutRadiusFromSpeed);
  RESET_CURVE(burnoutLateralSpeedFromRadius);
  RESET_CURVE(donutRolloverFromSpeed);
  RESET_CURVE(burnoutRolloverFromSpeed);
  RESET_CURVE(splashVerticalImpulse);
  RESET_CURVE(splashHorizontalImpulse);
  RESET_CURVE(waterFrictionFromSpeed);
  RESET_CURVE(surfaceFeedback);
  RESET_CURVE(vehicleFeedbackRamp1);
  RESET_CURVE(vehicleFeedbackRamp0);
  RESET_CURVE(vehicleDefault30To100);
#undef RESET_CURVE
}

void ReplayVehicleTuningInstallation::InstallCurve(
    const std::optional<ReplayTuningCurveDefinition> &definition,
    CFuncKeysReal &storage, CSceneVehicleTuningCurve &target) {
  target.Reset();
  if (!definition.has_value()) {
    return;
  }

  std::vector<CFuncKeysReal::Key> keys;
  keys.reserve(definition->keys.size());
  for (const ReplayTuningCurveKey &key : definition->keys) {
    keys.push_back({key.position, key.value});
  }
  const CFuncKeysReal::ERealInterp interpolation =
      definition->interpolation == ReplayTuningCurveInterpolation::Constant
          ? CFuncKeysReal::Constant
          : CFuncKeysReal::Linear;
  storage.SetKeys(std::move(keys), interpolation);
  target.Bind(storage);
}

void ReplayVehicleTuningInstallation::Install(
    const ReplayVehicleTuningDefinition &definition,
    CSceneVehicleCarTuning &tuning, CSceneVehicleStruct &vehicleStruct) {
  tuning.ResetToDefaults();
  tuning.engineSpeedNorm = definition.engineSpeedNorm;
  tuning.lowSpeedFrictionMagnitude = definition.lowSpeedFrictionMagnitude;
  tuning.lowSpeedLinearDamping = definition.lowSpeedLinearDamping;
  tuning.wheelForceMode =
      static_cast<CSceneVehicleCarWheelForceMode>(definition.wheelForceMode);
  tuning.handlingModel =
      static_cast<CSceneVehicleCarHandlingModel>(definition.handlingModel);
  CopyVisual(tuning.visual, definition.visual);
  CopySteering(tuning.steering, definition.steering);
  CopySuspension(tuning.suspension, definition.suspension);
  CopyContactResponse(tuning.contactResponse, definition.contactResponse);
  CopyBodyAirResponse(tuning.bodyAirResponse, definition.bodyAirResponse);
  CopyRadiusSteering(tuning.radiusSteering, definition.radiusSteering);
  CopySlipResponse(tuning.slipResponse, definition.slipResponse);
  tuning.slipResponse.slipSlowdownEnabled =
      definition.slipResponse.slipSlowdownEnabled ? 1u : 0u;
  CopyGearedDrive(tuning.gearedDrive, definition.gearedDrive);
  CopyTransmission(tuning.gearedDrive.transmission,
                   definition.gearedDrive.transmission);
  CopyBurnout(tuning.gearedDrive.burnout, definition.gearedDrive.burnout);
  CopyEngineInput(tuning.gearedDrive.input, definition.gearedDrive.input);
  tuning.gearedDrive.input.engineInputMaximum =
      definition.gearedDrive.input.engineInputMaximum;
  CopyWater(tuning.water, definition.water);
  CopyTurbo(tuning.turbo, definition.turbo);
  CopyFeedback(tuning.feedback, definition.feedback);

  Reset();
  const ReplayVehicleTuningCurves &source = definition.curves;
#define INSTALL_CURVE(name, target)                                            \
  InstallCurve(source.name, curves.name, target)
  INSTALL_CURVE(lateralContactSlowDownFromSpeed,
                tuning.lateralContactSlowDownFromSpeedCurve);
  INSTALL_CURVE(maxSideFrictionFromSpeed, tuning.maxSideFrictionFromSpeedCurve);
  INSTALL_CURVE(rolloverLateralFromSpeed, tuning.rolloverLateralFromSpeedCurve);
  INSTALL_CURVE(rolloverLateralCoefficientFromAngle,
                tuning.rolloverLateralCoefFromAngleCurve);
  INSTALL_CURVE(wheelVisualSteerAngleFromSpeed,
                tuning.visual.wheelSteerAngleFromSpeedCurve);
  INSTALL_CURVE(steeringDriveTorqueFromSpeed,
                tuning.steering.driveTorqueFromSpeedCurve);
  INSTALL_CURVE(steerSlowDownFromSpeed, tuning.steering.slowDownFromSpeedCurve);
  INSTALL_CURVE(suspensionDamperAbsorbModulation,
                tuning.suspension.damperAbsorbModulationCurve);
  INSTALL_CURVE(airControlZScale, tuning.bodyAirResponse.airControlZScaleCurve);
  INSTALL_CURVE(radiusSteeringRadiusFromSpeed,
                tuning.radiusSteering.steerRadiusFromSpeedCurve);
  INSTALL_CURVE(radiusSteeringMaxFrictionFromSpeed,
                tuning.radiusSteering.maxFrictionForceFromSpeedCurve);
  INSTALL_CURVE(slipResponseAccelFromSpeed,
                tuning.slipResponse.accelFromSpeedCurve);
  INSTALL_CURVE(slipResponseSlippingAccelFromSpeed,
                tuning.slipResponse.slippingAccelFromSpeedCurve);
  INSTALL_CURVE(reverseGearAccelFromSpeed,
                tuning.gearedDrive.transmission.rearGearAccelFromSpeedCurve);
  INSTALL_CURVE(burnoutRolloverLateralFromSpeedRatio,
                tuning.gearedDrive.burnout.rolloverLateralFromSpeedRatioCurve);
  INSTALL_CURVE(burnoutRadiusFromSpeed,
                tuning.gearedDrive.burnout.radiusFromSpeedCurve);
  INSTALL_CURVE(burnoutLateralSpeedFromRadius,
                tuning.gearedDrive.burnout.lateralSpeedFromRadiusCurve);
  INSTALL_CURVE(donutRolloverFromSpeed,
                tuning.gearedDrive.burnout.donutRolloverFromSpeedCurve);
  INSTALL_CURVE(burnoutRolloverFromSpeed,
                tuning.gearedDrive.burnout.rolloverFromSpeedCurve);
  INSTALL_CURVE(splashVerticalImpulse, tuning.water.splashVerticalImpulseCurve);
  INSTALL_CURVE(splashHorizontalImpulse,
                tuning.water.splashHorizontalImpulseCurve);
  INSTALL_CURVE(waterFrictionFromSpeed, tuning.water.frictionFromSpeedCurve);
  INSTALL_CURVE(surfaceFeedback, tuning.feedback.surfaceCurve);
  CSceneVehicleTuningCurve feedbackRamp1;
  CSceneVehicleTuningCurve feedbackRamp0;
  CSceneVehicleTuningCurve default30To100;
  INSTALL_CURVE(vehicleFeedbackRamp1, feedbackRamp1);
  INSTALL_CURVE(vehicleFeedbackRamp0, feedbackRamp0);
  INSTALL_CURVE(vehicleDefault30To100, default30To100);
  vehicleStruct.BindFeedbackCurves(feedbackRamp1, feedbackRamp0,
                                   default30To100);
#undef INSTALL_CURVE
}
