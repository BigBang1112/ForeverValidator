#ifndef TMNF_FORMAT_VEHICLE_TUNING_CHUNK_SCHEMA_H
#define TMNF_FORMAT_VEHICLE_TUNING_CHUNK_SCHEMA_H

#include <stddef.h>
#include <stdint.h>

#include "format/vehicle_tuning/archive_ids.h"
enum class VehicleTuningFieldKind : uint8_t {
    Real,
    Natural,
    Boolean,
    NodRef,
    FloatArray,
    CmwId,
};

struct VehicleTuningChunkSchema {
    VehicleTuningChunkId chunkId;
    const VehicleTuningFieldKind *fields;
    size_t fieldCount;
};


namespace VehicleTuningSchemaStorage {
using F = VehicleTuningFieldKind;

template <size_t N>
constexpr VehicleTuningChunkSchema SchemaFor(
        VehicleTuningChunkId chunkId,
        const VehicleTuningFieldKind (&fields)[N]) {
    return {chunkId, fields, N};
}

inline constexpr VehicleTuningFieldKind Fields_Root[] = {
    F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real,
    F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_Next[] = {
    F::CmwId,
};

inline constexpr VehicleTuningFieldKind Fields_SolidBody[] = {
    F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_LegacyBodyBooleanFlag[] = {
    F::Boolean,
};

inline constexpr VehicleTuningFieldKind Fields_GroundedSolidFeedback[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_SteeringSlewRate[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6ForwardAccelBase[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_SolidInertia[] = {
    F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_SingleMaterialRef[] = {
    F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_WheelForceMode[] = {
    F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_ForceModelAndM6SideForceTorque[] = {
    F::Natural, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_ForceModelTwelveReals[] = {
    F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real,
    F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_ForceModelTwoRealsA[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_ForceModelEightReals[] = {
    F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_AirTorqueResponse[] = {
    F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_ContactImpulseResponse[] = {
    F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_ContactImpulseSingleRealA[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_ContactImpulseSingleRealB[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_AirborneFeedbackAndModel5Torque[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_AirControlMemoryTickWindow[] = {
    F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_AirControlThreeReals[] = {
    F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_AirControlFourReals[] = {
    F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_PointImpulseAndM6SteerAssist[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M5AccelFromSpeedCurve[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_SlopeAdherence[] = {
    F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6SlippingSteerAndFrictionBlend[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_MaxSideFrictionAndWheelAbsorb[] = {
    F::Real, F::NodRef, F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_RolloverLateralAndM6LateralForce[] = {
    F::Real, F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_LateralContactSlowDownCurve[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_Model5SteerSlowDown[] = {
    F::NodRef, F::Real, F::Real, F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_AirControlAndRolloverCoef[] = {
    F::NodRef, F::Real, F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_UpdateParamsConfigB[] = {
    F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_M6SteerSlowDown[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_SteerDriveTorqueCurve[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_M6SpeedLimitForce[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_ImpactFeedbackThresholds[] = {
    F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_PointImpulseAngularSpeedMax[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_PointImpulseLinearSpeedGrowthLimit[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_Model5SlipSlowdownEnabled[] = {
    F::Boolean,
};

inline constexpr VehicleTuningFieldKind Fields_Model4SteerRadiusAndFriction[] = {
    F::Real, F::Real, F::Real, F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_Model4AuxCurveRef[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_Model4MaxFrictionAndSlipping[] = {
    F::Real, F::NodRef, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_Model4QuadraticDampingFriction[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_Model4SteerAngleAndStateRadius[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_Model4StateExitSideSpeedMax[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_Model4SteerAngleInputScale[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M5SlippingAccelFromSpeedCurve[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_Model45LateralSlowDownTicks[] = {
    F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_Model5AuxCurveRefA[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_Model5AuxCurveRefB[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_Model5RolloverTorqueCap[] = {
    F::Real, F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_Model5SteeringMemoryTicks[] = {
    F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_Model5SlipSlowdownTicks[] = {
    F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_M6SlipRatioScale[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_WaterAndSplashResponse[] = {
    F::Real, F::Real, F::Real, F::NodRef, F::NodRef, F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_WaterAngularLinearDamping[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6DamperAbsorbModulationCurve[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_M6CurrentForceTorqueMin[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6CurrentTorqueScale[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6DonutAndBurnoutRadius[] = {
    F::Real, F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_M6BurnoutSideForceFadeScale[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6RolloverLateralSpeedRatioCurve[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_M6BurnoutRadiusCorrectionSpeedScale[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6InputRiseFall[] = {
    F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6FloatBuffer[] = {
    F::FloatArray,
};

inline constexpr VehicleTuningFieldKind Fields_M6GroundInputBrake[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6GroundModeInputHighWindow[] = {
    F::Real, F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6ModeInputLowWindow[] = {
    F::Real, F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_SurfaceFeedback[] = {
    F::NodRef, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6ReverseForwardAccelCaps[] = {
    F::Real, F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6Full[] = {
    F::Real, F::Real, F::NodRef, F::NodRef, F::Real, F::Real, F::Real, F::Real,
    F::Real, F::Real, F::Real, F::Real, F::Real, F::Natural, F::Real, F::Real,
    F::Natural, F::Real, F::Real, F::Real, F::Real, F::NodRef, F::NodRef, F::Real,
    F::Real, F::Real, F::Real, F::Real, F::Real, F::FloatArray, F::FloatArray, F::FloatArray,
    F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_AirControlZScaleCurve[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_WaterAngularSpeedDamping[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6ReverseTurboFull[] = {
    F::Real, F::Real, F::Real, F::Natural, F::Natural,
};

inline constexpr VehicleTuningFieldKind Fields_WheelVisualSteerAngleCurve[] = {
    F::NodRef,
};

inline constexpr VehicleTuningFieldKind Fields_M6Mat6SlideSideForceScale[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6Mat6SlideGateScale[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6Mat6SlideForwardScale[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6Mat6SlideForwardGateScale[] = {
    F::Real,
};

inline constexpr VehicleTuningFieldKind Fields_M6Mat6AuxSlideRealA[] = {
    F::Real,
};


inline constexpr VehicleTuningChunkSchema Schemas[] = {
    SchemaFor(VehicleTuningChunkId::Root, Fields_Root),
    SchemaFor(VehicleTuningChunkId::Next, Fields_Next),
    SchemaFor(VehicleTuningChunkId::SolidBody, Fields_SolidBody),
    SchemaFor(VehicleTuningChunkId::LegacyBodyBooleanFlag, Fields_LegacyBodyBooleanFlag),
    SchemaFor(VehicleTuningChunkId::GroundedSolidFeedback, Fields_GroundedSolidFeedback),
    SchemaFor(VehicleTuningChunkId::SteeringSlewRate, Fields_SteeringSlewRate),
    SchemaFor(VehicleTuningChunkId::M6ForwardAccelBase, Fields_M6ForwardAccelBase),
    SchemaFor(VehicleTuningChunkId::SolidInertia, Fields_SolidInertia),
    SchemaFor(VehicleTuningChunkId::SingleMaterialRef, Fields_SingleMaterialRef),
    SchemaFor(VehicleTuningChunkId::WheelForceMode, Fields_WheelForceMode),
    SchemaFor(VehicleTuningChunkId::ForceModelAndM6SideForceTorque, Fields_ForceModelAndM6SideForceTorque),
    SchemaFor(VehicleTuningChunkId::ForceModelTwelveReals, Fields_ForceModelTwelveReals),
    SchemaFor(VehicleTuningChunkId::ForceModelTwoRealsA, Fields_ForceModelTwoRealsA),
    SchemaFor(VehicleTuningChunkId::ForceModelEightReals, Fields_ForceModelEightReals),
    SchemaFor(VehicleTuningChunkId::AirTorqueResponse, Fields_AirTorqueResponse),
    SchemaFor(VehicleTuningChunkId::ContactImpulseResponse, Fields_ContactImpulseResponse),
    SchemaFor(VehicleTuningChunkId::ContactImpulseSingleRealA, Fields_ContactImpulseSingleRealA),
    SchemaFor(VehicleTuningChunkId::ContactImpulseSingleRealB, Fields_ContactImpulseSingleRealB),
    SchemaFor(VehicleTuningChunkId::AirborneFeedbackAndModel5Torque, Fields_AirborneFeedbackAndModel5Torque),
    SchemaFor(VehicleTuningChunkId::AirControlMemoryTickWindow, Fields_AirControlMemoryTickWindow),
    SchemaFor(VehicleTuningChunkId::AirControlThreeReals, Fields_AirControlThreeReals),
    SchemaFor(VehicleTuningChunkId::AirControlFourReals, Fields_AirControlFourReals),
    SchemaFor(VehicleTuningChunkId::PointImpulseAndM6SteerAssist, Fields_PointImpulseAndM6SteerAssist),
    SchemaFor(VehicleTuningChunkId::M5AccelFromSpeedCurve, Fields_M5AccelFromSpeedCurve),
    SchemaFor(VehicleTuningChunkId::SlopeAdherence, Fields_SlopeAdherence),
    SchemaFor(VehicleTuningChunkId::M6SlippingSteerAndFrictionBlend, Fields_M6SlippingSteerAndFrictionBlend),
    SchemaFor(VehicleTuningChunkId::MaxSideFrictionAndWheelAbsorb, Fields_MaxSideFrictionAndWheelAbsorb),
    SchemaFor(VehicleTuningChunkId::RolloverLateralAndM6LateralForce, Fields_RolloverLateralAndM6LateralForce),
    SchemaFor(VehicleTuningChunkId::LateralContactSlowDownCurve, Fields_LateralContactSlowDownCurve),
    SchemaFor(VehicleTuningChunkId::Model5SteerSlowDown, Fields_Model5SteerSlowDown),
    SchemaFor(VehicleTuningChunkId::AirControlAndRolloverCoef, Fields_AirControlAndRolloverCoef),
    SchemaFor(VehicleTuningChunkId::UpdateParamsCopy08c, Fields_UpdateParamsConfigB),
    SchemaFor(VehicleTuningChunkId::M6SteerSlowDown, Fields_M6SteerSlowDown),
    SchemaFor(VehicleTuningChunkId::SteerDriveTorqueCurve, Fields_SteerDriveTorqueCurve),
    SchemaFor(VehicleTuningChunkId::M6SpeedLimitForce, Fields_M6SpeedLimitForce),
    SchemaFor(VehicleTuningChunkId::ImpactFeedbackThresholds, Fields_ImpactFeedbackThresholds),
    SchemaFor(VehicleTuningChunkId::PointImpulseAngularSpeedMax, Fields_PointImpulseAngularSpeedMax),
    SchemaFor(VehicleTuningChunkId::PointImpulseLinearSpeedGrowthLimit, Fields_PointImpulseLinearSpeedGrowthLimit),
    SchemaFor(VehicleTuningChunkId::Model5SlipSlowdownEnabled, Fields_Model5SlipSlowdownEnabled),
    SchemaFor(VehicleTuningChunkId::Model4SteerRadiusAndFriction, Fields_Model4SteerRadiusAndFriction),
    SchemaFor(VehicleTuningChunkId::Model4AuxCurveRef, Fields_Model4AuxCurveRef),
    SchemaFor(VehicleTuningChunkId::Model4MaxFrictionAndSlipping, Fields_Model4MaxFrictionAndSlipping),
    SchemaFor(VehicleTuningChunkId::Model4QuadraticDampingFriction, Fields_Model4QuadraticDampingFriction),
    SchemaFor(VehicleTuningChunkId::Model4SteerAngleAndStateRadius, Fields_Model4SteerAngleAndStateRadius),
    SchemaFor(VehicleTuningChunkId::Model4StateExitSideSpeedMax, Fields_Model4StateExitSideSpeedMax),
    SchemaFor(VehicleTuningChunkId::Model4SteerAngleInputScale, Fields_Model4SteerAngleInputScale),
    SchemaFor(VehicleTuningChunkId::M5SlippingAccelFromSpeedCurve, Fields_M5SlippingAccelFromSpeedCurve),
    SchemaFor(VehicleTuningChunkId::Model45LateralSlowDownTicks, Fields_Model45LateralSlowDownTicks),
    SchemaFor(VehicleTuningChunkId::Model5AuxCurveRefA, Fields_Model5AuxCurveRefA),
    SchemaFor(VehicleTuningChunkId::Model5AuxCurveRefB, Fields_Model5AuxCurveRefB),
    SchemaFor(VehicleTuningChunkId::Model5RolloverTorqueCap, Fields_Model5RolloverTorqueCap),
    SchemaFor(VehicleTuningChunkId::Model5SteeringMemoryTicks, Fields_Model5SteeringMemoryTicks),
    SchemaFor(VehicleTuningChunkId::Model5SlipSlowdownTicks, Fields_Model5SlipSlowdownTicks),
    SchemaFor(VehicleTuningChunkId::M6SlipRatioScale, Fields_M6SlipRatioScale),
    SchemaFor(VehicleTuningChunkId::WaterAndSplashResponse, Fields_WaterAndSplashResponse),
    SchemaFor(VehicleTuningChunkId::WaterAngularLinearDamping, Fields_WaterAngularLinearDamping),
    SchemaFor(VehicleTuningChunkId::M6DamperAbsorbModulationCurve, Fields_M6DamperAbsorbModulationCurve),
    SchemaFor(VehicleTuningChunkId::M6CurrentForceTorqueMin, Fields_M6CurrentForceTorqueMin),
    SchemaFor(VehicleTuningChunkId::M6CurrentTorqueScale, Fields_M6CurrentTorqueScale),
    SchemaFor(VehicleTuningChunkId::M6DonutAndBurnoutRadius, Fields_M6DonutAndBurnoutRadius),
    SchemaFor(VehicleTuningChunkId::M6BurnoutSideForceFadeScale, Fields_M6BurnoutSideForceFadeScale),
    SchemaFor(VehicleTuningChunkId::M6RolloverLateralSpeedRatioCurve, Fields_M6RolloverLateralSpeedRatioCurve),
    SchemaFor(VehicleTuningChunkId::M6BurnoutRadiusCorrectionSpeedScale, Fields_M6BurnoutRadiusCorrectionSpeedScale),
    SchemaFor(VehicleTuningChunkId::M6InputRiseFall, Fields_M6InputRiseFall),
    SchemaFor(VehicleTuningChunkId::M6FloatBuffer, Fields_M6FloatBuffer),
    SchemaFor(VehicleTuningChunkId::M6GroundInputBrake, Fields_M6GroundInputBrake),
    SchemaFor(VehicleTuningChunkId::M6GroundModeInputHighWindow, Fields_M6GroundModeInputHighWindow),
    SchemaFor(VehicleTuningChunkId::M6ModeInputLowWindow, Fields_M6ModeInputLowWindow),
    SchemaFor(VehicleTuningChunkId::SurfaceFeedback, Fields_SurfaceFeedback),
    SchemaFor(VehicleTuningChunkId::M6ReverseForwardAccelCaps, Fields_M6ReverseForwardAccelCaps),
    SchemaFor(VehicleTuningChunkId::M6Full, Fields_M6Full),
    SchemaFor(VehicleTuningChunkId::AirControlZScaleCurve, Fields_AirControlZScaleCurve),
    SchemaFor(VehicleTuningChunkId::WaterAngularSpeedDamping, Fields_WaterAngularSpeedDamping),
    SchemaFor(VehicleTuningChunkId::M6ReverseTurboFull, Fields_M6ReverseTurboFull),
    SchemaFor(VehicleTuningChunkId::WheelVisualSteerAngleCurve, Fields_WheelVisualSteerAngleCurve),
    SchemaFor(VehicleTuningChunkId::M6Mat6SlideSideForceScale, Fields_M6Mat6SlideSideForceScale),
    SchemaFor(VehicleTuningChunkId::M6Mat6SlideGateScale, Fields_M6Mat6SlideGateScale),
    SchemaFor(VehicleTuningChunkId::M6Mat6SlideForwardScale, Fields_M6Mat6SlideForwardScale),
    SchemaFor(VehicleTuningChunkId::M6Mat6SlideForwardGateScale, Fields_M6Mat6SlideForwardGateScale),
    SchemaFor(VehicleTuningChunkId::M6Mat6AuxSlideRealA, Fields_M6Mat6AuxSlideRealA),
};

inline constexpr size_t SchemaCount = sizeof(Schemas) / sizeof(Schemas[0]);
}  // namespace VehicleTuningSchemaStorage

inline const VehicleTuningChunkSchema *FindVehicleTuningChunkSchema(
        uint32_t chunkId) {
    using namespace VehicleTuningSchemaStorage;
    for (size_t i = 0u; i < SchemaCount; i++) {
        if (ArchiveChunkIdValue(Schemas[i].chunkId) == chunkId) {
            return &Schemas[i];
        }
    }
    return nullptr;
}

inline const VehicleTuningChunkSchema *FindVehicleTuningChunkSchema(
        VehicleTuningChunkId chunkId) {
    return FindVehicleTuningChunkSchema(ArchiveChunkIdValue(chunkId));
}

inline constexpr size_t VehicleTuningChunkSchemaCount =
        VehicleTuningSchemaStorage::SchemaCount;

#endif
