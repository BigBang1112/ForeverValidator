#ifndef TMNF_FORMAT_VEHICLE_TUNING_ARCHIVE_IDS_H
#define TMNF_FORMAT_VEHICLE_TUNING_ARCHIVE_IDS_H

#include <cstdint>

#include "format/archive/tmnf_archive_ids.h"
enum class VehicleTuningChunkId : uint32_t {
    Root = 0x0a029000u,
    Next = 0x0a029001u,
    SolidBody = 0x0a029002u,
    LegacyBodyTuningThreeReals = 0x0a029003u,
    LegacyBodyBooleanFlag = 0x0a029004u,
    GroundedSolidFeedback = 0x0a029005u,
    LegacyGroundFeedbackFourReals = 0x0a029006u,
    SteeringSlewRate = 0x0a029007u,
    M6ReverseAndTurboImpulse = 0x0a029008u,
    M6ForwardAccelBase = 0x0a029009u,
    AirborneTorque = 0x0a02900au,
    SolidInertia = 0x0a02900bu,
    LegacySolidInertiaNatural = 0x0a02900cu,
    SingleMaterialRef = 0x0a02900du,
    WheelForceMode = 0x0a02900eu,
    LegacyWheelForceSingleReal = 0x0a02900fu,
    ForceModelAndM6SideForceTorque = 0x0a029010u,
    LegacyForceModelNineReals = 0x0a029011u,
    LegacyForceModelFiveRealsA = 0x0a029012u,
    ForceModelTwelveReals = 0x0a029013u,
    ForceModelTwoRealsA = 0x0a029014u,
    LegacyForceModelFiveRealsB = 0x0a029015u,
    LegacyForceModelTwoRealsB = 0x0a029016u,
    ForceModelEightReals = 0x0a029017u,
    AirTorqueResponse = 0x0a029018u,
    ContactImpulseResponse = 0x0a029019u,
    ContactImpulseSingleRealA = 0x0a02901au,
    ContactImpulseSingleRealB = 0x0a02901bu,
    LegacyContactImpulseSixReals = 0x0a02901cu,
    AirborneFeedbackAndModel5Torque = 0x0a02901du,
    AirControlMemoryTickWindow = 0x0a02901eu,
    AirControlThreeReals = 0x0a02901fu,
    AirControlFourReals = 0x0a029020u,
    LegacyAirControlSingleReal = 0x0a029021u,
    LegacyAirControlTwoReals = 0x0a029022u,
    PointImpulseAndM6SteerAssist = 0x0a029023u,
    M5AccelFromSpeedCurve = 0x0a029024u,
    LegacyM5AccelThreeReals = 0x0a029025u,
    SlopeAdherence = 0x0a029026u,
    M6SlippingSteerAndFrictionBlend = 0x0a029027u,
    MaxSideFrictionAndWheelAbsorb = 0x0a029028u,
    RolloverLateralAndM6LateralForce = 0x0a029029u,
    LateralContactSlowDownCurve = 0x0a02902au,
    Model5SteerSlowDown = 0x0a02902bu,
    AirControlAndRolloverCoef = 0x0a02902cu,
    UpdateParamsCopy08c = 0x0a02902du,
    M6SteerSlowDown = 0x0a02902eu,
    TurboDuration = 0x0a02902fu,
    SteerDriveTorqueCurve = 0x0a029030u,
    M6SpeedLimitForce = 0x0a029031u,
    ImpactFeedbackThresholds = 0x0a029032u,
    PointImpulseAngularSpeedMax = 0x0a029033u,
    PointImpulseLinearSpeedGrowthLimit = 0x0a029034u,
    Model5SlipSlowdownEnabled = 0x0a029035u,
    Model4SteerRadiusAndFriction = 0x0a029036u,
    Model4AuxCurveRef = 0x0a029037u,
    Model4MaxFrictionAndSlipping = 0x0a029038u,
    Model4QuadraticDampingFriction = 0x0a029039u,
    Model4SteerAngleAndStateRadius = 0x0a02903au,
    Model4StateExitSideSpeedMax = 0x0a02903bu,
    Model4SteerAngleInputScale = 0x0a02903cu,
    M5SlippingAccelFromSpeedCurve = 0x0a02903du,
    Model45LateralSlowDownTicks = 0x0a02903eu,
    Model5AuxCurveRefA = 0x0a02903fu,
    Model5AuxCurveRefB = 0x0a029040u,
    Model5RolloverTorqueCap = 0x0a029041u,
    Model5SteeringMemoryTicks = 0x0a029042u,
    Model5SlipSlowdownTicks = 0x0a029043u,
    M6SlipRatioScale = 0x0a029044u,
    LegacyM6SlipFiveReals = 0x0a029045u,
    WaterAndSplashResponse = 0x0a029046u,
    WaterAngularLinearDamping = 0x0a029047u,
    WaterResponseAuxCurveRef = 0x0a029048u,
    M6DamperAbsorbModulationCurve = 0x0a029049u,
    ForceFeedbackDivisorLegacy = 0x0a02904au,
    M6ReverseBurnoutForceThresholdLegacy = 0x0a02904bu,
    M6EmptyLegacyPayload = 0x0a02904cu,
    M6CurrentForceTorqueMin = 0x0a02904du,
    M6CurrentTorqueScale = 0x0a02904eu,
    M6DonutAndBurnoutRadius = 0x0a02904fu,
    M6CompositeBurnoutPayload = 0x0a029050u,
    M6BurnoutSideForceFadeScale = 0x0a029051u,
    M6RolloverLateralSpeedRatioCurve = 0x0a029052u,
    M6BurnoutRadiusCorrectionSpeedScale = 0x0a029053u,
    M6FloatBufferSet = 0x0a029054u,
    M6FloatBufferLegacySingle = 0x0a029055u,
    M6InputRiseFall = 0x0a029056u,
    M6FloatBuffer = 0x0a029057u,
    M6GroundInputBrake = 0x0a029058u,
    M6GroundModeInputHighWindow = 0x0a029059u,
    M6ModeInputLowWindow = 0x0a02905au,
    SurfaceFeedback = 0x0a02905bu,
    M6ReverseForwardAccelCaps = 0x0a02905cu,
    M6Full = 0x0a02905du,
    AirControlZScaleCurve = 0x0a02905eu,
    WaterAngularSpeedDamping = 0x0a02905fu,
    M6ReverseTurboFull = 0x0a029060u,
    WheelVisualSteerAngleCurve = 0x0a029061u,
    M6Mat6SlideSideForceScale = 0x0a029062u,
    M6Mat6SlideGateScale = 0x0a029063u,
    M6Mat6SlideForwardScale = 0x0a029064u,
    M6Mat6SlideForwardGateScale = 0x0a029065u,
    M6Mat6AuxSlideRealA = 0x0a029066u,
    M6Mat6AuxSlideRealB = 0x0a029067u,
    M6Mat6AuxSlideRealC = 0x0a029068u,
    M6Mat6AuxSlideRealD = 0x0a029069u,
};

constexpr uint32_t ArchiveChunkIdValue(VehicleTuningChunkId chunkId) {
    return static_cast<uint32_t>(chunkId);
}

enum class FunctionCurveChunkId : uint32_t {
    Root = 0x0501a000u,
    XValues = 0x05002001u,
    Id = 0x05002003u,
    YValuesAndMode = 0x0501a001u
};

constexpr uint32_t ArchiveChunkIdValue(FunctionCurveChunkId chunkId) {
    return static_cast<uint32_t>(chunkId);
}

enum class VehicleTuningBaseChunkId : uint32_t {
    Base = 0x0a02e000u
};

constexpr uint32_t ArchiveChunkIdValue(
        VehicleTuningBaseChunkId chunkId) {
    return static_cast<uint32_t>(chunkId);
}

enum class VehicleTuningContainerChunkId : uint32_t {
    Root = 0x0a030000u
};

constexpr uint32_t ArchiveChunkIdValue(
        VehicleTuningContainerChunkId chunkId) {
    return static_cast<uint32_t>(chunkId);
}

class VehicleTuningChunkWord {
public:
    static VehicleTuningChunkWord FromArchiveWord(
            uint32_t word) {
        VehicleTuningChunkWord value;
        value.archiveWord = word;
        return value;
    }

    int IsFacadeSentinel() const {
        return archiveWord == CMwNodArchiveFacadeSentinel;
    }

    int Matches(VehicleTuningChunkId chunk) const {
        return archiveWord == ArchiveChunkIdValue(chunk);
    }

    uint32_t ValueForFormatLookup() const {
        return archiveWord;
    }

    uint32_t ValueForDiagnostic() const {
        return archiveWord;
    }

private:
    uint32_t archiveWord = 0u;
};

constexpr uint32_t NodeReferenceArrayArchiveFormatMarker = 10u;

#endif
