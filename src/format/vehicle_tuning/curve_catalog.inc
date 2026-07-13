enum class VehicleTuningCurveTarget : u32 {
    M5AccelFromSpeedCurve,
    LateralContactSlowDownFromSpeedCurve,
    M5SteerSlowDownFromSpeedCurve,
    SteerDriveTorqueFromSpeedCurve,
    MaxSideFrictionFromSpeedCurve,
    RolloverLateralFromSpeedCurve,
    RolloverLateralCoefFromAngleCurve,
    M4SteerRadiusFromSpeedCurve,
    M4MaxFrictionForceFromSpeedCurve,
    M5SlippingAccelFromSpeedCurve,
    SplashVerticalImpulseCurve,
    SplashHorizontalImpulseCurve,
    WaterFrictionFromSpeedCurve,
    M6DamperAbsorbModulationCurve,
    M6RearGearAccelFromSpeedCurve,
    M6RolloverLateralFromSpeedRatioCurve,
    M6BurnoutRadiusFromSpeedCurve,
    M6LateralSpeedFromBurnoutRadiusCurve,
    M6DonutRolloverFromSpeedCurve,
    M6BurnoutRolloverFromSpeedCurve,
    AirControlZScaleCurve,
    WheelVisualSteerAngleFromSpeedCurve,
    SurfaceFeedbackCurve,
};

struct VehicleTuningCurveBinding {
    VehicleTuningChunkId chunkId;
    u32 nodRefIndex;
    VehicleTuningCurveTarget target;
};

constexpr VehicleTuningCurveBinding VehicleTuningCurveBindings[] = {
    {VehicleTuningChunkId::M5AccelFromSpeedCurve, 0u, VehicleTuningCurveTarget::M5AccelFromSpeedCurve},
    {VehicleTuningChunkId::LateralContactSlowDownCurve, 0u, VehicleTuningCurveTarget::LateralContactSlowDownFromSpeedCurve},
    {VehicleTuningChunkId::Model5SteerSlowDown, 0u, VehicleTuningCurveTarget::M5SteerSlowDownFromSpeedCurve},
    {VehicleTuningChunkId::SteerDriveTorqueCurve, 0u, VehicleTuningCurveTarget::SteerDriveTorqueFromSpeedCurve},
    {VehicleTuningChunkId::MaxSideFrictionAndWheelAbsorb, 0u, VehicleTuningCurveTarget::MaxSideFrictionFromSpeedCurve},
    {VehicleTuningChunkId::RolloverLateralAndM6LateralForce, 0u, VehicleTuningCurveTarget::RolloverLateralFromSpeedCurve},
    {VehicleTuningChunkId::AirControlAndRolloverCoef, 0u, VehicleTuningCurveTarget::RolloverLateralCoefFromAngleCurve},
    {VehicleTuningChunkId::Model4SteerRadiusAndFriction, 0u, VehicleTuningCurveTarget::M4SteerRadiusFromSpeedCurve},
    {VehicleTuningChunkId::Model4MaxFrictionAndSlipping, 0u, VehicleTuningCurveTarget::M4MaxFrictionForceFromSpeedCurve},
    {VehicleTuningChunkId::M5SlippingAccelFromSpeedCurve, 0u, VehicleTuningCurveTarget::M5SlippingAccelFromSpeedCurve},
    {VehicleTuningChunkId::WaterAndSplashResponse, 2u, VehicleTuningCurveTarget::SplashVerticalImpulseCurve},
    {VehicleTuningChunkId::WaterAndSplashResponse, 0u, VehicleTuningCurveTarget::SplashHorizontalImpulseCurve},
    {VehicleTuningChunkId::WaterAndSplashResponse, 1u, VehicleTuningCurveTarget::WaterFrictionFromSpeedCurve},
    {VehicleTuningChunkId::M6DamperAbsorbModulationCurve, 0u, VehicleTuningCurveTarget::M6DamperAbsorbModulationCurve},
    {VehicleTuningChunkId::M6Full, 0u, VehicleTuningCurveTarget::M6RearGearAccelFromSpeedCurve},
    {VehicleTuningChunkId::M6RolloverLateralSpeedRatioCurve, 0u, VehicleTuningCurveTarget::M6RolloverLateralFromSpeedRatioCurve},
    {VehicleTuningChunkId::M6DonutAndBurnoutRadius, 0u, VehicleTuningCurveTarget::M6BurnoutRadiusFromSpeedCurve},
    {VehicleTuningChunkId::M6Full, 1u, VehicleTuningCurveTarget::M6LateralSpeedFromBurnoutRadiusCurve},
    {VehicleTuningChunkId::M6Full, 3u, VehicleTuningCurveTarget::M6DonutRolloverFromSpeedCurve},
    {VehicleTuningChunkId::M6Full, 2u, VehicleTuningCurveTarget::M6BurnoutRolloverFromSpeedCurve},
    {VehicleTuningChunkId::AirControlZScaleCurve, 0u, VehicleTuningCurveTarget::AirControlZScaleCurve},
    {VehicleTuningChunkId::WheelVisualSteerAngleCurve, 0u, VehicleTuningCurveTarget::WheelVisualSteerAngleFromSpeedCurve},
    {VehicleTuningChunkId::SurfaceFeedback, 0u, VehicleTuningCurveTarget::SurfaceFeedbackCurve},
};
