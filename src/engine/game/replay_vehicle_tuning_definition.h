#ifndef TMNF_REPLAY_VEHICLE_TUNING_DEFINITION_H
#define TMNF_REPLAY_VEHICLE_TUNING_DEFINITION_H

#include <cstdint>
#include <optional>
#include <vector>

#include "engine/core/gm_types.h"
#include "engine/game/surface_material.h"
enum class ReplayVehicleWheelForceMode : std::uint32_t {
    DirectSpring = 0u,
    FollowAbsorb = 1u,
    FollowAbsorbWithImpulse = 2u,
};

enum class ReplayVehicleHandlingModel : std::uint32_t {
    Standard = 0u,
    Lateral = 1u,
    RadiusSteering = 3u,
    SlipResponse = 4u,
    GearedDrive = 5u,
};

enum class ReplayTuningCurveInterpolation {
    Constant,
    Linear,
};

struct ReplayTuningCurveKey {
    float position = 0.0f;
    float value = 0.0f;
};

struct ReplayTuningCurveDefinition {
    ReplayTuningCurveInterpolation interpolation = ReplayTuningCurveInterpolation::Linear;
    std::vector<ReplayTuningCurveKey> keys;
};

struct ReplayVehicleTuningCurves {
    std::optional<ReplayTuningCurveDefinition> lateralContactSlowDownFromSpeed;
    std::optional<ReplayTuningCurveDefinition> maxSideFrictionFromSpeed;
    std::optional<ReplayTuningCurveDefinition> rolloverLateralFromSpeed;
    std::optional<ReplayTuningCurveDefinition> rolloverLateralCoefficientFromAngle;
    std::optional<ReplayTuningCurveDefinition> wheelVisualSteerAngleFromSpeed;
    std::optional<ReplayTuningCurveDefinition> steeringDriveTorqueFromSpeed;
    std::optional<ReplayTuningCurveDefinition> steerSlowDownFromSpeed;
    std::optional<ReplayTuningCurveDefinition> suspensionDamperAbsorbModulation;
    std::optional<ReplayTuningCurveDefinition> airControlZScale;
    std::optional<ReplayTuningCurveDefinition> radiusSteeringRadiusFromSpeed;
    std::optional<ReplayTuningCurveDefinition> radiusSteeringMaxFrictionFromSpeed;
    std::optional<ReplayTuningCurveDefinition> slipResponseAccelFromSpeed;
    std::optional<ReplayTuningCurveDefinition> slipResponseSlippingAccelFromSpeed;
    std::optional<ReplayTuningCurveDefinition> reverseGearAccelFromSpeed;
    std::optional<ReplayTuningCurveDefinition> burnoutRolloverLateralFromSpeedRatio;
    std::optional<ReplayTuningCurveDefinition> burnoutRadiusFromSpeed;
    std::optional<ReplayTuningCurveDefinition> burnoutLateralSpeedFromRadius;
    std::optional<ReplayTuningCurveDefinition> donutRolloverFromSpeed;
    std::optional<ReplayTuningCurveDefinition> burnoutRolloverFromSpeed;
    std::optional<ReplayTuningCurveDefinition> splashVerticalImpulse;
    std::optional<ReplayTuningCurveDefinition> splashHorizontalImpulse;
    std::optional<ReplayTuningCurveDefinition> waterFrictionFromSpeed;
    std::optional<ReplayTuningCurveDefinition> surfaceFeedback;
    std::optional<ReplayTuningCurveDefinition> vehicleFeedbackRamp1;
    std::optional<ReplayTuningCurveDefinition> vehicleFeedbackRamp0;
    std::optional<ReplayTuningCurveDefinition> vehicleDefault30To100;
};

struct ReplayVehicleTuningVisualSettings {
    float wheelSpeedBase = 0.0f;
    float wheelSpeedScale = 0.0f;
};

struct ReplayVehicleTuningSteering {
    float slewRate = 0.0f;
    float assistFullSpeed = 0.0f;
    float slowDownScale = 0.0f;
};

struct ReplayVehicleTuningSuspension {
    float wheelSpringCoef = 0.0f;
    float wheelDamperCoef = 0.0f;
    float damperModulationMaxAbsorb = 0.0f;
    float damperModulationMinAbsorb = 0.0f;
    float wheelRestDamperAbsorb = 0.0f;
    float wheelStaticSpringScale = 0.0f;
    float wheelAbsorbFollowCoef = 0.0f;
};

struct ReplayVehicleTuningContactResponse {
    float bodyImpactFeedbackHighThreshold = 0.0f;
    float bodyImpactFeedbackLowThreshold = 0.0f;
    float wheelImpactFeedbackHighThreshold = 0.0f;
    float wheelImpactFeedbackLowThreshold = 0.0f;
    float bodyContactTangentLimitOther = 0.0f;
    float bodyContactTangentLimitMetal = 0.0f;
    float bodyContactImpulseMetal = 0.0f;
    float bodyContactImpulseOther = 0.0f;
    float wheelContactImpulseOther = 0.0f;
    float wheelContactImpulseMetal = 0.0f;
    float pointImpulseAngularYScale = 0.0f;
    float pointImpulseAngularScale = 0.0f;
    float pointImpulseAngularSpeedMax = 0.0f;
    float pointImpulseLinearSpeedGrowthLimitSq = 0.0f;
    float specialContactImpulseMagnitude = 0.0f;
    float specialSolidFeedbackValue = 0.0f;
    EPlugSurfaceMaterialId singleMaterial =
            EPlugSurfaceMaterialId_Concrete;
};

struct ReplayVehicleTuningBodyAirResponse {
    float solidPhysicalMass = 0.0f;
    float solidInertiaMass = 0.0f;
    GmVec3 solidInertiaBoxSize{};
    float solidCenterZHalfExtentScale = 0.0f;
    float solidCenterYOffset = 0.0f;
    float solidPhysicalResponseCoefB = 0.0f;
    float airborneSolidFeedback0 = 0.0f;
    float airTorqueLinearCoef = 0.0f;
    float airTorqueQuadraticCoef = 0.0f;
    float groundedSolidFeedback1 = 0.0f;
    float airborneSolidFeedback1 = 0.0f;
    float slopeAdherence1Min = 0.0f;
    float slopeAdherence1Max = 0.0f;
    float slopeAdherence2Min = 0.0f;
    float slopeAdherence2Max = 0.0f;
    std::uint32_t airControlMemoryTickWindow = 0u;
    float airControlYSwitchThreshold = 0.0f;
};

struct ReplayVehicleRadiusSteeringTuning {
    float steerTorqueSpeedScale = 0.0f;
    float angularDampingLinear = 0.0f;
    float angularDampingQuadratic = 0.0f;
    float lateralFrictionLinear = 0.0f;
    float lateralFrictionQuadratic = 0.0f;
    float captureExitSideSpeedMax = 0.0f;
    float inputSteerRadiusScale = 0.0f;
    float slippingFrictionScale = 0.0f;
    float capturedAngleRate = 0.0f;
    float capturedAngleRadiusScale = 0.0f;
    float steerAngleLimit = 0.0f;
    float steerAngleFromInputScale = 0.0f;
};

struct ReplayVehicleSlipResponseTuning {
    float slippingAccelScale = 0.0f;
    std::uint32_t lateralSlowDownTickWindow = 0u;
    float rolloverTorqueCap = 0.0f;
    std::uint32_t steeringMemoryTicks = 0u;
    bool slipSlowdownEnabled = false;
    std::uint32_t slipSlowdownTicks = 0u;
    float longitudinalTorqueScale = 0.0f;
};

struct ReplayVehicleTransmissionTuning {
    float reverseSpeedNorm = 0.0f;
    std::vector<float> gearSpeedRatio;
    std::vector<float> upshiftThreshold;
    std::vector<float> downshiftThreshold;
    std::vector<float> rpmWanted;
    std::vector<float> targetInputBias;
    std::vector<float> rpmDelta;
};

struct ReplayVehicleBurnoutTuning {
    float reverseForceThreshold = 0.0f;
    float donutSpeedHigh = 0.0f;
    float donutSpeedLow = 0.0f;
    float lateralCorrectionScale = 0.0f;
    float angleTorqueScale = 0.0f;
    float angularDampingLinear = 0.0f;
    float angleReturnQuadratic = 0.0f;
    float tangentAngularDamping = 0.0f;
    float radiusCorrectionScale = 0.0f;
    float radiusCorrectionSpeedScale = 0.0f;
    float radiusMin = 0.0f;
    float tangentSpeedMax = 0.0f;
    float angleLimit = 0.0f;
    float angleLimitPositive = 0.0f;
    float angleLimitNegative = 0.0f;
    std::uint32_t durationTicks = 0u;
    float driveFadeScale = 0.0f;
    float sideForceFadeScale = 0.0f;
    std::uint32_t exitDurationTicks = 0u;
    float exitAccelFadeScale = 0.0f;
    float exitMinSpeed = 0.0f;
    float exitSteerGripScale = 0.0f;
    float exitBonusAccelScale = 0.0f;
    float wheelAngularSpeedOverride = 0.0f;
    float exitSideFrictionScale = 0.0f;
};

struct ReplayVehicleEngineInputTuning {
    float engineInputMaximum = 0.0f;
    float burnoutHoldInputRise = 0.0f;
    float airborneInputRise = 0.0f;
    float airborneInputFall = 0.0f;
    float groundInputBrake = 0.0f;
    float groundInputRise = 0.0f;
    float transitionInputRise = 0.0f;
    float groundInputFall = 0.0f;
    float forwardTransitionSpeedHigh = 0.0f;
    float forwardTransitionSpeedLow = 0.0f;
    float reverseTransitionSpeedHigh = 0.0f;
    float reverseTransitionSpeedLow = 0.0f;
};

struct ReplayVehicleGearedDriveTuning {
    float forwardAccelBase = 0.0f;
    float forwardAccelSpeedCoef = 0.0f;
    float forwardAccelCapWhenSlipping = 0.0f;
    float forwardAccelCap = 0.0f;
    float speedLimitForce = 0.0f;
    float forceZScale = 0.0f;
    float sideForceToDriveTorqueScale = 0.0f;
    float slippingSteerTorqueScale = 0.0f;
    float lateralForceScale = 0.0f;
    float slippingSideFrictionScale = 0.0f;
    float sideFrictionSlipBlend = 0.0f;
    float driveSideFrictionSlipBlend = 0.0f;
    float slipRatioScale = 0.0f;
    float currentForceTorqueMin = 0.0f;
    float currentTorqueXScale = 0.0f;
    float currentTorqueZScale = 0.0f;
    float perSlippingWheelAccelScale = 0.0f;
    float lowSpeedBSlippingGripScale = 0.0f;
    float forwardAccelCapWhenSlippingReverse = 0.0f;
    float forwardAccelCapReverse = 0.0f;
    float dirtSlideSideForceScale = 0.0f;
    float dirtSlideGateScale = 0.0f;
    float dirtSlideForwardForceScale = 0.0f;
    float dirtSlideForwardGateScale = 0.0f;
    ReplayVehicleTransmissionTuning transmission;
    ReplayVehicleBurnoutTuning burnout;
    ReplayVehicleEngineInputTuning input;
};

struct ReplayVehicleTuningWater {
    float buoyancyForce = 0.0f;
    float splashHorizontalSpeedThreshold = 0.0f;
    float splashTotalSpeedThreshold = 0.0f;
    float angularLinearDamping = 0.0f;
    float angularSpeedDamping = 0.0f;
};

struct ReplayVehicleTuningTurbo {
    float impulseScaleA = 0.0f;
    float impulseScaleB = 0.0f;
    std::uint32_t durationA = 0u;
    std::uint32_t durationB = 0u;
};

struct ReplayVehicleTuningFeedback {
    float surfaceBaseRate = 0.0f;
    float forceDivisor = 0.0f;
};

struct ReplayVehicleTuningDefinition {
    float engineSpeedNorm = 0.0f;
    float lowSpeedFrictionMagnitude = 0.0f;
    float lowSpeedLinearDamping = 0.0f;
    ReplayVehicleWheelForceMode wheelForceMode = ReplayVehicleWheelForceMode::DirectSpring;
    ReplayVehicleHandlingModel handlingModel = ReplayVehicleHandlingModel::Standard;
    ReplayVehicleTuningVisualSettings visual;
    ReplayVehicleTuningSteering steering;
    ReplayVehicleTuningSuspension suspension;
    ReplayVehicleTuningContactResponse contactResponse;
    ReplayVehicleTuningBodyAirResponse bodyAirResponse;
    ReplayVehicleRadiusSteeringTuning radiusSteering;
    ReplayVehicleSlipResponseTuning slipResponse;
    ReplayVehicleGearedDriveTuning gearedDrive;
    ReplayVehicleTuningWater water;
    ReplayVehicleTuningTurbo turbo;
    ReplayVehicleTuningFeedback feedback;
    ReplayVehicleTuningCurves curves;
};

ReplayVehicleTuningDefinition MakeDefaultVehicleTuningDefinition();

#endif // TMNF_REPLAY_VEHICLE_TUNING_DEFINITION_H
