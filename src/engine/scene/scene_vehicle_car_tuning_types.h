#pragma once

#include <cstdint>
#include <vector>

#include "engine/core/engine_types.h"
#include "engine/core/gm_types.h"
#include "engine/rendering/plug_material.h"
#include "engine/scene/scene_vehicle_tuning.h"
enum CSceneVehicleCarWheelForceMode : std::uint8_t {
  CSceneVehicleCarWheelForceMode_DirectSpring = 0,
  CSceneVehicleCarWheelForceMode_FollowAbsorb = 1,
  CSceneVehicleCarWheelForceMode_FollowAbsorbWithImpulse = 2,
};
enum CSceneVehicleCarHandlingModel : std::uint8_t {
  CSceneVehicleCarHandlingModel_Standard = 0,
  CSceneVehicleCarHandlingModel_Lateral = 1,
  CSceneVehicleCarHandlingModel_RadiusSteering = 3,
  CSceneVehicleCarHandlingModel_SlipResponse = 4,
  CSceneVehicleCarHandlingModel_GearedDrive = 5,
};
struct CSceneVehicleCarTuningVisualSettings {
  float wheelSpeedBase;
  float wheelSpeedScale;
  CSceneVehicleTuningCurve wheelSteerAngleFromSpeedCurve;
};
struct CSceneVehicleCarTuningSteering {
  float slewRate;
  float assistFullSpeed;
  float slowDownScale;
  CSceneVehicleTuningCurve driveTorqueFromSpeedCurve;
  CSceneVehicleTuningCurve slowDownFromSpeedCurve;
};
struct CSceneVehicleCarTuningSuspension {
  float wheelSpringCoef;
  float wheelDamperCoef;
  float damperModulationMaxAbsorb;
  float damperModulationMinAbsorb;
  float wheelRestDamperAbsorb;
  float wheelStaticSpringScale;
  float wheelAbsorbFollowCoef;
  CSceneVehicleTuningCurve damperAbsorbModulationCurve;
};
struct CSceneVehicleCarTuningContactResponse {
  float bodyImpactFeedbackHighThreshold;
  float bodyImpactFeedbackLowThreshold;
  float wheelImpactFeedbackHighThreshold;
  float wheelImpactFeedbackLowThreshold;
  float bodyContactTangentLimitOther;
  float bodyContactTangentLimitMetal;
  float bodyContactImpulseMetal;
  float bodyContactImpulseOther;
  float wheelContactImpulseOther;
  float wheelContactImpulseMetal;
  float pointImpulseAngularYScale;
  float pointImpulseAngularScale;
  float pointImpulseAngularSpeedMax;
  float pointImpulseLinearSpeedGrowthLimitSq;
  float specialContactImpulseMagnitude;
  float specialSolidFeedbackValue;
  EPlugSurfaceMaterialId singleMaterial;
};
struct CSceneVehicleCarTuningBodyAirResponse {
  float solidPhysicalMass;
  float solidInertiaMass;
  GmVec3 solidInertiaBoxSize;
  float solidCenterZHalfExtentScale;
  float solidCenterYOffset;
  float solidPhysicalResponseCoefB;
  float airborneSolidFeedback0;
  float airTorqueLinearCoef;
  float airTorqueQuadraticCoef;
  float groundedSolidFeedback1;
  float airborneSolidFeedback1;
  float slopeAdherence1Min;
  float slopeAdherence1Max;
  float slopeAdherence2Min;
  float slopeAdherence2Max;
  u32 airControlMemoryTickWindow;
  float airControlYSwitchThreshold;
  CSceneVehicleTuningCurve airControlZScaleCurve;
};
struct CSceneVehicleCarRadiusSteeringTuning {
  float steerTorqueSpeedScale;
  float angularDampingLinear;
  float angularDampingQuadratic;
  float lateralFrictionLinear;
  float lateralFrictionQuadratic;
  float captureExitSideSpeedMax;
  CSceneVehicleTuningCurve steerRadiusFromSpeedCurve;
  float inputSteerRadiusScale;
  CSceneVehicleTuningCurve maxFrictionForceFromSpeedCurve;
  float slippingFrictionScale;
  float capturedAngleRate;
  float capturedAngleRadiusScale;
  float steerAngleLimit;
  float steerAngleFromInputScale;
};
struct CSceneVehicleCarSlipResponseTuning {
  CSceneVehicleTuningCurve accelFromSpeedCurve;
  CSceneVehicleTuningCurve slippingAccelFromSpeedCurve;
  float slippingAccelScale;
  u32 lateralSlowDownTickWindow;
  float rolloverTorqueCap;
  u32 steeringMemoryTicks;
  u32 slipSlowdownEnabled;
  u32 slipSlowdownTicks;
  float longitudinalTorqueScale;
};
struct CSceneVehicleCarTransmissionTuning {
  float reverseSpeedNorm;
  CSceneVehicleTuningCurve rearGearAccelFromSpeedCurve;
  std::vector<float> gearSpeedRatio;
  std::vector<float> upshiftThreshold;
  std::vector<float> downshiftThreshold;
  std::vector<float> rpmWanted;
  std::vector<float> targetInputBias;
  std::vector<float> rpmDelta;
};
struct CSceneVehicleCarBurnoutTuning {
  float reverseForceThreshold;
  CSceneVehicleTuningCurve rolloverLateralFromSpeedRatioCurve;
  float donutSpeedHigh;
  float donutSpeedLow;
  CSceneVehicleTuningCurve radiusFromSpeedCurve;
  CSceneVehicleTuningCurve lateralSpeedFromRadiusCurve;
  float lateralCorrectionScale;
  float angleTorqueScale;
  float angularDampingLinear;
  float angleReturnQuadratic;
  float tangentAngularDamping;
  float radiusCorrectionScale;
  float radiusCorrectionSpeedScale;
  float radiusMin;
  float tangentSpeedMax;
  CSceneVehicleTuningCurve donutRolloverFromSpeedCurve;
  float angleLimit;
  float angleLimitPositive;
  float angleLimitNegative;
  u32 durationTicks;
  float driveFadeScale;
  float sideForceFadeScale;
  CSceneVehicleTuningCurve rolloverFromSpeedCurve;
  u32 exitDurationTicks;
  float exitAccelFadeScale;
  float exitMinSpeed;
  float exitSteerGripScale;
  float exitBonusAccelScale;
  float wheelAngularSpeedOverride;
  float exitSideFrictionScale;
};
struct CSceneVehicleCarEngineInputTuning {
  float engineInputMaximum;
  float burnoutHoldInputRise;
  float airborneInputRise;
  float airborneInputFall;
  float groundInputBrake;
  float groundInputRise;
  float transitionInputRise;
  float groundInputFall;
  float forwardTransitionSpeedHigh;
  float forwardTransitionSpeedLow;
  float reverseTransitionSpeedHigh;
  float reverseTransitionSpeedLow;
};
struct CSceneVehicleCarGearedDriveTuning {
  float forwardAccelBase;
  float forwardAccelSpeedCoef;
  float forwardAccelCapWhenSlipping;
  float forwardAccelCap;
  float speedLimitForce;
  float forceZScale;
  float sideForceToDriveTorqueScale;
  float slippingSteerTorqueScale;
  float lateralForceScale;
  float slippingSideFrictionScale;
  float sideFrictionSlipBlend;
  float driveSideFrictionSlipBlend;
  float slipRatioScale;
  float currentForceTorqueMin;
  float currentTorqueXScale;
  float currentTorqueZScale;
  float perSlippingWheelAccelScale;
  float lowSpeedBSlippingGripScale;
  float forwardAccelCapWhenSlippingReverse;
  float forwardAccelCapReverse;
  float dirtSlideSideForceScale;
  float dirtSlideGateScale;
  float dirtSlideForwardForceScale;
  float dirtSlideForwardGateScale;
  CSceneVehicleCarTransmissionTuning transmission;
  CSceneVehicleCarBurnoutTuning burnout;
  CSceneVehicleCarEngineInputTuning input;
};
struct CSceneVehicleCarTuningWater {
  float buoyancyForce;
  float splashHorizontalSpeedThreshold;
  float splashTotalSpeedThreshold;
  CSceneVehicleTuningCurve splashVerticalImpulseCurve;
  CSceneVehicleTuningCurve splashHorizontalImpulseCurve;
  CSceneVehicleTuningCurve frictionFromSpeedCurve;
  float angularLinearDamping;
  float angularSpeedDamping;
};
struct CSceneVehicleCarTuningTurbo {
  float impulseScaleA;
  float impulseScaleB;
  u32 durationA;
  u32 durationB;
};
struct CSceneVehicleCarTuningFeedback {
  CSceneVehicleTuningCurve surfaceCurve;
  float surfaceBaseRate;
  float forceDivisor;
};
