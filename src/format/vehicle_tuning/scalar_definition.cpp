#include <math.h>
#include <stdio.h>
#include <utility>
#include <vector>

#include "engine/game/replay_vehicle_tuning_definition.h"
#include "format/vehicle_tuning/archive_decoder.h"
#include "format/vehicle_tuning/archive_ids.h"
#include "format/vehicle_tuning/transmission_records.h"
static const char *TransmissionArrayName(TransmissionArrayKind source) {
  switch (source) {
  case TransmissionArrayKind::GearSpeedRatio:
    return "gearSpeedRatio";
  case TransmissionArrayKind::UpshiftThreshold:
    return "upshiftThreshold";
  case TransmissionArrayKind::DownshiftThreshold:
    return "downshiftThreshold";
  case TransmissionArrayKind::RpmWanted:
    return "rpmWanted";
  }
  return "unknown";
}

class VehicleTuningDefinitionAssembler {
public:
  VehicleTuningDefinitionAssembler()
      : tuning(MakeDefaultVehicleTuningDefinition()) {}

  int ApplyArchiveChunk(const VehicleTuningScalarChunk &chunk) {
    const int validationError = ValidateArchiveChunk(chunk);
    if (validationError != 0) {
      return validationError;
    }

    int result = ApplySharedPhysicsChunk(chunk);
    if (result == ChunkNotHandled) {
      result = ApplyLegacyHandlingChunk(chunk);
    }
    if (result == ChunkNotHandled) {
      result = ApplyGearedDriveChunk(chunk);
    }
    if (result == ChunkNotHandled) {
      result = ApplyNaturalControlChunk(chunk);
    }
    if (result != ChunkNotHandled && result != 0) {
      return result;
    }
    ApplyDerivedFieldInvariants(chunk.chunk);
    return 0;
  }

  ReplayVehicleTuningDefinition TakeDefinition() { return std::move(tuning); }

private:
  static constexpr int ChunkNotHandled = -1;

  static int ValidateArchiveChunk(const VehicleTuningScalarChunk &chunk) {
    if (chunk.reals.size() > MaxVehicleTuningChunkRealCount ||
        chunk.naturals.size() > MaxVehicleTuningChunkNaturalCount) {
      fprintf(stderr,
              "bad tuning archive chunk 0x%08x counts real=%u natural=%u\n",
              chunk.chunk.ValueForDiagnostic(),
              static_cast<unsigned>(chunk.reals.size()),
              static_cast<unsigned>(chunk.naturals.size()));
      return 3;
    }
    for (u32 index = 0u; index < chunk.reals.size(); ++index) {
      if (!isfinite(chunk.reals[index])) {
        fprintf(stderr, "tuning archive chunk 0x%08x nonfinite real[%u]\n",
                chunk.chunk.ValueForDiagnostic(), index);
        return 4;
      }
    }
    return 0;
  }

  int ApplySharedPhysicsChunk(const VehicleTuningScalarChunk &chunk) {
    int result = ApplyBodyAndWaterChunk(chunk);
    if (result == ChunkNotHandled) {
      result = ApplyContactAndFeedbackChunk(chunk);
    }
    if (result == ChunkNotHandled) {
      result = ApplyCrossModelChunk(chunk);
    }
    return result;
  }

  int ApplyBodyAndWaterChunk(const VehicleTuningScalarChunk &chunk) {
    switch (chunk.chunk.ValueForFormatLookup()) {
    case ArchiveChunkIdValue(VehicleTuningChunkId::Root):
      if (!ApplyReal(chunk, 1u, tuning.visual.wheelSpeedBase))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.visual.wheelSpeedScale))
        return 5;
      if (!ApplyReal(chunk, 3u, tuning.engineSpeedNorm))
        return 5;
      if (!ApplyReal(chunk, 4u, tuning.suspension.wheelSpringCoef))
        return 5;
      if (!ApplyReal(chunk, 5u, tuning.suspension.wheelDamperCoef))
        return 5;
      if (!ApplyReal(chunk, 6u, tuning.suspension.damperModulationMaxAbsorb))
        return 5;
      if (!ApplyReal(chunk, 7u, tuning.suspension.damperModulationMinAbsorb))
        return 5;
      if (!ApplyReal(chunk, 8u, tuning.suspension.wheelRestDamperAbsorb))
        return 5;
      if (!ApplyReal(chunk, 11u, tuning.suspension.wheelStaticSpringScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::SolidBody):
      if (!ApplyReal(chunk, 0u, tuning.bodyAirResponse.solidPhysicalMass))
        return 5;
      if (!ApplyReal(chunk, 1u,
                     tuning.bodyAirResponse.solidCenterZHalfExtentScale))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.bodyAirResponse.solidCenterYOffset))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::GroundedSolidFeedback):
      if (!ApplyReal(chunk, 0u, tuning.bodyAirResponse.groundedSolidFeedback1))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::AirborneTorque):
      if (!ApplyReal(chunk, 0u, tuning.bodyAirResponse.airborneSolidFeedback0))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.bodyAirResponse.airTorqueLinearCoef))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::SolidInertia):
      if (!ApplyReal(chunk, 0u, tuning.bodyAirResponse.solidInertiaMass))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.bodyAirResponse.solidInertiaBoxSize.x))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.bodyAirResponse.solidInertiaBoxSize.y))
        return 5;
      if (!ApplyReal(chunk, 3u, tuning.bodyAirResponse.solidInertiaBoxSize.z))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::AirTorqueResponse):
      if (!ApplyReal(chunk, 0u, tuning.bodyAirResponse.airborneSolidFeedback0))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.bodyAirResponse.airTorqueLinearCoef))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.bodyAirResponse.airTorqueQuadraticCoef))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::SlopeAdherence):
      if (!ApplyReal(chunk, 0u, tuning.bodyAirResponse.slopeAdherence1Min))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.bodyAirResponse.slopeAdherence1Max))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.bodyAirResponse.slopeAdherence2Min))
        return 5;
      if (!ApplyReal(chunk, 3u, tuning.bodyAirResponse.slopeAdherence2Max))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::WaterAndSplashResponse):
      if (!ApplyReal(chunk, 0u, tuning.water.buoyancyForce))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.water.splashHorizontalSpeedThreshold))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.water.splashTotalSpeedThreshold))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::WaterAngularLinearDamping):
      if (!ApplyReal(chunk, 0u, tuning.water.angularLinearDamping))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::WaterAngularSpeedDamping):
      if (!ApplyReal(chunk, 0u, tuning.water.angularSpeedDamping))
        return 5;
      break;
    default:
      return ChunkNotHandled;
    }
    return 0;
  }

  int ApplyContactAndFeedbackChunk(const VehicleTuningScalarChunk &chunk) {
    switch (chunk.chunk.ValueForFormatLookup()) {
    case ArchiveChunkIdValue(VehicleTuningChunkId::SteeringSlewRate):
      if (!ApplyReal(chunk, 0u, tuning.steering.slewRate))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::ContactImpulseResponse):
      if (!ApplyReal(chunk, 0u,
                     tuning.contactResponse.bodyContactTangentLimitOther))
        return 5;
      if (!ApplyReal(chunk, 1u,
                     tuning.contactResponse.bodyContactTangentLimitMetal))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.contactResponse.bodyContactImpulseMetal))
        return 5;
      if (!ApplyReal(chunk, 3u, tuning.contactResponse.bodyContactImpulseOther))
        return 5;
      if (!ApplyReal(chunk, 5u,
                     tuning.contactResponse.wheelContactImpulseOther))
        return 5;
      if (!ApplyReal(chunk, 7u,
                     tuning.contactResponse.wheelContactImpulseMetal))
        return 5;
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::PointImpulseAndM6SteerAssist):
      if (!ApplyReal(chunk, 0u,
                     tuning.contactResponse.pointImpulseAngularScale))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.steering.assistFullSpeed))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::ImpactFeedbackThresholds):
      if (!ApplyReal(chunk, 4u,
                     tuning.contactResponse.wheelImpactFeedbackHighThreshold))
        return 5;
      if (!ApplyReal(chunk, 5u,
                     tuning.contactResponse.wheelImpactFeedbackLowThreshold))
        return 5;
      if (!ApplyReal(chunk, 6u,
                     tuning.contactResponse.bodyImpactFeedbackHighThreshold))
        return 5;
      if (!ApplyReal(chunk, 7u,
                     tuning.contactResponse.bodyImpactFeedbackLowThreshold))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::PointImpulseAngularSpeedMax):
      if (!ApplyReal(chunk, 0u,
                     tuning.contactResponse.pointImpulseAngularSpeedMax))
        return 5;
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::PointImpulseLinearSpeedGrowthLimit):
      if (!ApplyReal(
              chunk, 0u,
              tuning.contactResponse.pointImpulseLinearSpeedGrowthLimitSq))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::ForceFeedbackDivisorLegacy):
      if (!ApplyReal(chunk, 1u, tuning.feedback.forceDivisor))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::SurfaceFeedback):
      if (!ApplyReal(chunk, 0u, tuning.feedback.surfaceBaseRate))
        return 5;
      break;
    default:
      return ChunkNotHandled;
    }
    return 0;
  }

  int ApplyCrossModelChunk(const VehicleTuningScalarChunk &chunk) {
    switch (chunk.chunk.ValueForFormatLookup()) {
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::ForceModelAndM6SideForceTorque):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.sideForceToDriveTorqueScale))
        return 5;
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.handlingModel =
          static_cast<ReplayVehicleHandlingModel>(chunk.naturals[0u]);
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::AirborneFeedbackAndModel5Torque):
      if (!ApplyReal(chunk, 0u, tuning.bodyAirResponse.airborneSolidFeedback1))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.slipResponse.longitudinalTorqueScale))
        return 5;
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::MaxSideFrictionAndWheelAbsorb):
      if (!ApplyReal(chunk, 1u, tuning.gearedDrive.slippingSideFrictionScale))
        return 5;
      if (!ApplyReal(chunk, 3u, tuning.lowSpeedFrictionMagnitude))
        return 5;
      if (!ApplyReal(chunk, 4u, tuning.suspension.wheelAbsorbFollowCoef))
        return 5;
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::RolloverLateralAndM6LateralForce):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.lateralForceScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::AirControlAndRolloverCoef):
      if (!ApplyReal(chunk, 0u,
                     tuning.bodyAirResponse.airControlYSwitchThreshold))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.gearedDrive.forwardAccelSpeedCoef))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.gearedDrive.forwardAccelCapWhenSlipping))
        return 5;
      if (!ApplyReal(chunk, 3u, tuning.gearedDrive.forwardAccelCap))
        return 5;
      if (!ApplyReal(chunk, 4u, tuning.lowSpeedLinearDamping))
        return 5;
      break;
    default:
      return ChunkNotHandled;
    }
    return 0;
  }

  int ApplyLegacyHandlingChunk(const VehicleTuningScalarChunk &chunk) {
    switch (chunk.chunk.ValueForFormatLookup()) {
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::Model4SteerRadiusAndFriction):
      if (!ApplyReal(chunk, 0u, tuning.radiusSteering.steerTorqueSpeedScale))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.radiusSteering.angularDampingLinear))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.radiusSteering.lateralFrictionLinear))
        return 5;
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::Model4MaxFrictionAndSlipping):
      if (!ApplyReal(chunk, 1u, tuning.radiusSteering.slippingFrictionScale))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.radiusSteering.inputSteerRadiusScale))
        return 5;
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::Model4QuadraticDampingFriction):
      if (!ApplyReal(chunk, 0u, tuning.radiusSteering.angularDampingQuadratic))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.radiusSteering.lateralFrictionQuadratic))
        return 5;
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::Model4SteerAngleAndStateRadius):
      if (!ApplyReal(chunk, 0u, tuning.radiusSteering.capturedAngleRate))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.radiusSteering.capturedAngleRadiusScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::Model4StateExitSideSpeedMax):
      if (!ApplyReal(chunk, 0u, tuning.radiusSteering.captureExitSideSpeedMax))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::Model4SteerAngleInputScale):
      if (!ApplyReal(chunk, 0u, tuning.radiusSteering.steerAngleFromInputScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::Model5RolloverTorqueCap):
      if (!ApplyReal(chunk, 0u, tuning.slipResponse.rolloverTorqueCap))
        return 5;
      break;
    default:
      return ChunkNotHandled;
    }
    return 0;
  }

  int ApplyGearedDriveChunk(const VehicleTuningScalarChunk &chunk) {
    int result = ApplyGearedDriveDynamicsChunk(chunk);
    if (result == ChunkNotHandled) {
      result = ApplyGearedDriveBurnoutChunk(chunk);
    }
    if (result == ChunkNotHandled) {
      result = ApplyGearedDriveInputChunk(chunk);
    }
    return result;
  }

  int ApplyGearedDriveDynamicsChunk(const VehicleTuningScalarChunk &chunk) {
    switch (chunk.chunk.ValueForFormatLookup()) {
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6ReverseAndTurboImpulse):
      if (!ApplyReal(chunk, 0u,
                     tuning.gearedDrive.transmission.reverseSpeedNorm))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.turbo.impulseScaleA))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6ForwardAccelBase):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.forwardAccelBase))
        return 5;
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::M6SlippingSteerAndFrictionBlend):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.slippingSteerTorqueScale))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.gearedDrive.sideFrictionSlipBlend))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6SteerSlowDown):
      if (!ApplyReal(chunk, 0u, tuning.steering.slowDownScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6SpeedLimitForce):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.speedLimitForce))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.gearedDrive.forceZScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6SlipRatioScale):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.slipRatioScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6CurrentForceTorqueMin):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.currentForceTorqueMin))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6CurrentTorqueScale):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.currentTorqueXScale))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.gearedDrive.currentTorqueZScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6ReverseForwardAccelCaps):
      if (!ApplyReal(chunk, 0u,
                     tuning.gearedDrive.forwardAccelCapWhenSlippingReverse))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.gearedDrive.forwardAccelCapReverse))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6ReverseTurboFull):
      if (!ApplyReal(chunk, 0u,
                     tuning.gearedDrive.transmission.reverseSpeedNorm))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.turbo.impulseScaleA))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.turbo.impulseScaleB))
        return 5;
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.turbo.durationA = chunk.naturals[0u];
      if (!HasNatural(chunk, 1u))
        return 6;
      tuning.turbo.durationB = chunk.naturals[1u];
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6Mat6SlideSideForceScale):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.dirtSlideSideForceScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6Mat6SlideGateScale):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.dirtSlideGateScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6Mat6SlideForwardScale):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.dirtSlideForwardForceScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6Mat6SlideForwardGateScale):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.dirtSlideForwardGateScale))
        return 5;
      break;
    default:
      return ChunkNotHandled;
    }
    return 0;
  }

  int ApplyGearedDriveBurnoutChunk(const VehicleTuningScalarChunk &chunk) {
    switch (chunk.chunk.ValueForFormatLookup()) {
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::M6ReverseBurnoutForceThresholdLegacy):
      if (!ApplyReal(chunk, 1u,
                     tuning.gearedDrive.burnout.reverseForceThreshold))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6DonutAndBurnoutRadius):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.burnout.donutSpeedHigh))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6BurnoutSideForceFadeScale):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.burnout.sideForceFadeScale))
        return 5;
      break;
    case ArchiveChunkIdValue(
        VehicleTuningChunkId::M6BurnoutRadiusCorrectionSpeedScale):
      if (!ApplyReal(chunk, 0u,
                     tuning.gearedDrive.burnout.radiusCorrectionSpeedScale))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6Full):
      if (!ApplyReal(chunk, 0u, tuning.feedback.forceDivisor))
        return 5;
      if (!ApplyReal(chunk, 1u,
                     tuning.gearedDrive.burnout.reverseForceThreshold))
        return 5;
      if (!ApplyReal(chunk, 2u,
                     tuning.gearedDrive.burnout.lateralCorrectionScale))
        return 5;
      if (!ApplyReal(chunk, 3u, tuning.gearedDrive.burnout.donutSpeedLow))
        return 5;
      if (!ApplyReal(chunk, 4u, tuning.gearedDrive.burnout.angleTorqueScale))
        return 5;
      if (!ApplyReal(chunk, 5u,
                     tuning.gearedDrive.burnout.radiusCorrectionScale))
        return 5;
      if (!ApplyReal(chunk, 6u, tuning.gearedDrive.burnout.radiusMin))
        return 5;
      if (!ApplyReal(chunk, 7u, tuning.gearedDrive.burnout.tangentSpeedMax))
        return 5;
      if (!ApplyReal(chunk, 8u,
                     tuning.gearedDrive.burnout.angularDampingLinear))
        return 5;
      if (!ApplyReal(chunk, 9u,
                     tuning.gearedDrive.burnout.angleReturnQuadratic))
        return 5;
      if (!ApplyReal(chunk, 10u, tuning.gearedDrive.perSlippingWheelAccelScale))
        return 5;
      if (!ApplyReal(chunk, 11u, tuning.gearedDrive.burnout.driveFadeScale))
        return 5;
      if (!ApplyReal(chunk, 12u, tuning.gearedDrive.burnout.exitAccelFadeScale))
        return 5;
      if (!ApplyReal(chunk, 13u, tuning.gearedDrive.lowSpeedBSlippingGripScale))
        return 5;
      if (!ApplyReal(chunk, 14u,
                     tuning.gearedDrive.burnout.exitSideFrictionScale))
        return 5;
      if (!ApplyReal(chunk, 15u, tuning.gearedDrive.burnout.exitSteerGripScale))
        return 5;
      if (!ApplyReal(chunk, 16u, tuning.gearedDrive.burnout.exitMinSpeed))
        return 5;
      if (!ApplyReal(chunk, 17u, tuning.gearedDrive.burnout.angleLimit))
        return 5;
      if (!ApplyReal(chunk, 18u,
                     tuning.gearedDrive.burnout.wheelAngularSpeedOverride))
        return 5;
      if (!ApplyReal(chunk, 19u, tuning.gearedDrive.burnout.angleLimitPositive))
        return 5;
      if (!ApplyReal(chunk, 20u, tuning.gearedDrive.burnout.angleLimitNegative))
        return 5;
      if (!ApplyReal(chunk, 21u,
                     tuning.gearedDrive.burnout.exitBonusAccelScale))
        return 5;
      if (!ApplyReal(chunk, 22u, tuning.gearedDrive.input.engineInputMaximum))
        return 5;
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.gearedDrive.burnout.durationTicks = chunk.naturals[0u];
      if (!HasNatural(chunk, 1u))
        return 6;
      tuning.gearedDrive.burnout.exitDurationTicks = chunk.naturals[1u];
      if (!HasNatural(chunk, 2u))
        return 6;
      break;
    default:
      return ChunkNotHandled;
    }
    return 0;
  }

  int ApplyGearedDriveInputChunk(const VehicleTuningScalarChunk &chunk) {
    switch (chunk.chunk.ValueForFormatLookup()) {
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6InputRiseFall):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.input.burnoutHoldInputRise))
        return 5;
      if (!ApplyReal(chunk, 1u, tuning.gearedDrive.input.airborneInputRise))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.gearedDrive.input.airborneInputFall))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6GroundInputBrake):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.input.groundInputBrake))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6GroundModeInputHighWindow):
      if (!ApplyReal(chunk, 0u, tuning.gearedDrive.input.groundInputRise))
        return 5;
      if (!ApplyReal(chunk, 1u,
                     tuning.gearedDrive.input.forwardTransitionSpeedHigh))
        return 5;
      if (!ApplyReal(chunk, 2u, tuning.gearedDrive.input.transitionInputRise))
        return 5;
      if (!ApplyReal(chunk, 3u, tuning.gearedDrive.input.groundInputFall))
        return 5;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::M6ModeInputLowWindow):
      if (!ApplyReal(chunk, 0u,
                     tuning.gearedDrive.input.reverseTransitionSpeedHigh))
        return 5;
      if (!ApplyReal(chunk, 1u,
                     tuning.gearedDrive.input.forwardTransitionSpeedLow))
        return 5;
      if (!ApplyReal(chunk, 2u,
                     tuning.gearedDrive.input.reverseTransitionSpeedLow))
        return 5;
      break;
    default:
      return ChunkNotHandled;
    }
    return 0;
  }

  int ApplyNaturalControlChunk(const VehicleTuningScalarChunk &chunk) {
    switch (chunk.chunk.ValueForFormatLookup()) {
    case ArchiveChunkIdValue(VehicleTuningChunkId::Model5SlipSlowdownEnabled):
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.slipResponse.slipSlowdownEnabled = chunk.naturals[0u] != 0u;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::SingleMaterialRef):
      if (!HasNatural(chunk, 0u))
        return 6;
      if (chunk.naturals[0u] >= EPlugSurfaceMaterialId_Count) {
        fprintf(stderr,
                "tuning archive chunk 0x%08x invalid surface material %u\n",
                chunk.chunk.ValueForDiagnostic(), chunk.naturals[0u]);
        return 7;
      }
      tuning.contactResponse.singleMaterial =
          static_cast<EPlugSurfaceMaterialId>(chunk.naturals[0u]);
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::WheelForceMode):
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.wheelForceMode =
          static_cast<ReplayVehicleWheelForceMode>(chunk.naturals[0u]);
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::AirControlMemoryTickWindow):
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.bodyAirResponse.airControlMemoryTickWindow = chunk.naturals[0u];
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::Model5SteerSlowDown):
      if (!HasNatural(chunk, 0u))
        return 6;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::UpdateParamsCopy08c):
      if (!HasNatural(chunk, 0u))
        return 6;
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::TurboDuration):
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.turbo.durationA = chunk.naturals[0u];
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::Model45LateralSlowDownTicks):
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.slipResponse.lateralSlowDownTickWindow = chunk.naturals[0u];
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::Model5SteeringMemoryTicks):
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.slipResponse.steeringMemoryTicks = chunk.naturals[0u];
      break;
    case ArchiveChunkIdValue(VehicleTuningChunkId::Model5SlipSlowdownTicks):
      if (!HasNatural(chunk, 0u))
        return 6;
      tuning.slipResponse.slipSlowdownTicks = chunk.naturals[0u];
      break;
    default:
      return ChunkNotHandled;
    }
    return 0;
  }

  static bool ApplyReal(const VehicleTuningScalarChunk &chunk, u32 index,
                        float &target) {
    if (index >= chunk.reals.size()) {
      fprintf(stderr, "tuning archive chunk 0x%08x missing real[%u]\n",
              chunk.chunk.ValueForDiagnostic(), index);
      return false;
    }
    target = (chunk.reals[index]);
    return true;
  }

  static bool HasNatural(const VehicleTuningScalarChunk &chunk, u32 index) {
    if (index < chunk.naturals.size()) {
      return true;
    }
    fprintf(stderr, "tuning archive chunk 0x%08x missing natural[%u]\n",
            chunk.chunk.ValueForDiagnostic(), index);
    return false;
  }

  void ApplyDerivedFieldInvariants(VehicleTuningChunkWord chunk) {
    if (chunk.Matches(VehicleTuningChunkId::GroundedSolidFeedback)) {
      tuning.bodyAirResponse.airborneSolidFeedback1 =
          tuning.bodyAirResponse.groundedSolidFeedback1;
    } else if (chunk.Matches(VehicleTuningChunkId::M6ReverseAndTurboImpulse)) {
      tuning.turbo.impulseScaleB = tuning.turbo.impulseScaleA;
    } else if (chunk.Matches(VehicleTuningChunkId::AirborneTorque)) {
      tuning.bodyAirResponse.airTorqueQuadraticCoef = 0.0f;
    } else if (chunk.Matches(VehicleTuningChunkId::TurboDuration)) {
      tuning.turbo.durationB = tuning.turbo.durationA;
    }
  }

  ReplayVehicleTuningDefinition tuning;
};

int ApplyVehicleTuningScalars(
    const std::vector<VehicleTuningScalarChunk> &chunks,
    ReplayVehicleTuningDefinition &definition) {
  if (chunks.size() > MaxVehicleTuningChunkCount) {
    fprintf(stderr, "bad tuning archive scalar chunk count=%zu\n",
            chunks.size());
    return 2;
  }

  VehicleTuningDefinitionAssembler archiveTuning;
  for (const VehicleTuningScalarChunk &chunk : chunks) {
    const int error = archiveTuning.ApplyArchiveChunk(chunk);
    if (error != 0) {
      return error;
    }
  }

  definition = archiveTuning.TakeDefinition();
  return 0;
}

class VehicleTransmissionAssembler {
public:
  int LoadFromArchiveRecords(const std::vector<TransmissionArray> &records) {
    return sources.Load(records, &count);
  }

  int BuildDerivedBuffers(float engineInputMax) {
    if (!isfinite(engineInputMax)) {
      return 1;
    }
    targetInputBias.assign(count, 0.0f);
    rpmDelta.assign(count, 0.0f);

    for (std::size_t i = 2u; i < count; i++) {
      const float ratio =
          (sources.GearSpeedRatio()[i] / sources.GearSpeedRatio()[i - 1u]);
      const float offset = ((sources.RpmWanted()[i] -
                             (sources.UpshiftThreshold()[i - 1u] * ratio)) *
                            engineInputMax);
      targetInputBias[i] = ((ratio * targetInputBias[i - 1u]) + offset);
    }
    if (count >= 2u) {
      const std::size_t last = count - 1u;
      for (std::size_t i = 1u; i < last; i++) {
        if (engineInputMax == 0.0f) {
          rpmDelta[i] = 0.0f;
        } else {
          const float value =
              ((sources.DownshiftThreshold()[i + 1u] -
                (targetInputBias[i + 1u] / engineInputMax)) *
               sources.GearSpeedRatio()[i] / sources.GearSpeedRatio()[i + 1u]) +
              (targetInputBias[i] / engineInputMax);
          rpmDelta[i] = (value);
        }
      }
    }
    return 0;
  }

  int Install(ReplayVehicleTuningDefinition &definition) const {
    ReplayVehicleTransmissionTuning &transmission =
        definition.gearedDrive.transmission;
    transmission.gearSpeedRatio = sources.GearSpeedRatio();
    transmission.upshiftThreshold = sources.UpshiftThreshold();
    transmission.downshiftThreshold = sources.DownshiftThreshold();
    transmission.targetInputBias = targetInputBias;
    transmission.rpmDelta = rpmDelta;
    return 0;
  }

private:
  class SourceSet {
  public:
    int Load(const std::vector<TransmissionArray> &records,
             std::size_t *outCount) {
      for (const TransmissionArray &record : records) {
        Select(record.source) = &record;
      }
      if (!gearSpeedRatio || !upshiftThreshold || !downshiftThreshold ||
          !rpmWanted) {
        fprintf(stderr, "missing required M6 archive float-buffer source\n");
        return 1;
      }
      const std::size_t sourceCount = gearSpeedRatio->values.size();
      if (upshiftThreshold->values.size() != sourceCount ||
          downshiftThreshold->values.size() != sourceCount ||
          rpmWanted->values.size() != sourceCount) {
        fprintf(stderr, "M6 archive float-buffer counts differ\n");
        return 2;
      }
      *outCount = sourceCount;
      return 0;
    }

    const std::vector<float> &GearSpeedRatio() const {
      return gearSpeedRatio->values;
    }

    const std::vector<float> &UpshiftThreshold() const {
      return upshiftThreshold->values;
    }

    const std::vector<float> &DownshiftThreshold() const {
      return downshiftThreshold->values;
    }

    const std::vector<float> &RpmWanted() const { return rpmWanted->values; }

  private:
    const TransmissionArray *&Select(TransmissionArrayKind source) {
      switch (source) {
      case TransmissionArrayKind::GearSpeedRatio:
        return gearSpeedRatio;
      case TransmissionArrayKind::UpshiftThreshold:
        return upshiftThreshold;
      case TransmissionArrayKind::DownshiftThreshold:
        return downshiftThreshold;
      case TransmissionArrayKind::RpmWanted:
        return rpmWanted;
      }
      return rpmWanted;
    }

    const TransmissionArray *gearSpeedRatio = nullptr;
    const TransmissionArray *upshiftThreshold = nullptr;
    const TransmissionArray *downshiftThreshold = nullptr;
    const TransmissionArray *rpmWanted = nullptr;
  };

  SourceSet sources;
  std::size_t count = 0u;
  std::vector<float> targetInputBias;
  std::vector<float> rpmDelta;
};

int ApplyVehicleTransmissionArrays(
    const std::vector<TransmissionArray> &records,
    ReplayVehicleTuningDefinition &definition) {
  for (std::size_t recordIndex = 0; recordIndex < records.size();
       recordIndex++) {
    const TransmissionArray &record = records[recordIndex];
    for (float value : record.values) {
      if (!isfinite(value)) {
        fprintf(stderr,
                "invalid tuning archive float-buffer record %zu source=%s "
                "count=%zu\n",
                recordIndex, TransmissionArrayName(record.source),
                record.values.size());
        return 2;
      }
    }
  }

  VehicleTransmissionAssembler transmissionBuffers;
  int error = transmissionBuffers.LoadFromArchiveRecords(records);
  if (error != 0) {
    return 3;
  }
  const float engineInputMax = definition.gearedDrive.input.engineInputMaximum;
  error = transmissionBuffers.BuildDerivedBuffers(engineInputMax);
  if (error != 0) {
    fprintf(stderr, "M6 archive float-buffer build failed %d\n", error);
    return 4;
  }
  error = transmissionBuffers.Install(definition);
  if (error != 0) {
    fprintf(stderr, "M6 archive float-buffer install failed %d\n", error);
    return 5;
  }
  return 0;
}
