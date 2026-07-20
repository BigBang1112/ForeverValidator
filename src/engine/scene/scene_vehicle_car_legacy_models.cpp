// Original handling models retained as explicit deterministic force rules.

#include "engine/scene/scene_vehicle_car_internal.h"
using namespace SceneVehicleCarDynamics;

void CSceneVehicleCar::ApplyModel3ContactForces(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning) {
  const u32 wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    SSimulationWheel *wheel = &WheelAt(wheelIndex);
    WheelAddForceToVehicle(*wheel, request.currentForce);

    CSceneVehicleMaterial *material = GetWheelMaterial(*wheel);
    if (!wheel->realTimeState.contactPresent ||
        !(tuning.gearedDrive.lateralForceScale > 0.0f) ||
        tuning.handlingModel != CSceneVehicleCarHandlingModel_Lateral) {
      continue;
    }

    float slipGrip = wheel->realTimeState.slipping
                         ? tuning.gearedDrive.slippingSideFrictionScale
                         : 1.0f;
    float maxSide = material->blendableVals.w * request.slopeAdherenceA *
                    tuning.GetMaxSideFrictionFromSpeed(request.linearSpeed.z) *
                    slipGrip;
    GmVec3 lateral = {
        wheel->realTimeState.accumulatedContactNormal.y,
        -wheel->realTimeState.accumulatedContactNormal.x,
        0.0f,
    };
    lateral = SceneVehicleMath::NormalizeOr(
        lateral, GmVec3{1.0f, 0.0f, 0.0f}, VectorEpsilonSquared);
    if (IsFrontVehicleWheel(wheel->axle)) {
      float visualSteerYawCos = CIcos(request.visualSteerYaw);
      float negVisualSteerYawSin = -CIsin(request.visualSteerYaw);
      lateral = GmVec3{
          visualSteerYawCos * lateral.x,
          visualSteerYawCos * lateral.y,
          negVisualSteerYawSin + visualSteerYawCos * lateral.z,
      };
    }

    float sideForce = -tuning.gearedDrive.lateralForceScale * 0.5f *
                      SceneVehicleMath::Dot(request.linearSpeed, lateral);
    float sideForceAbs = std::fabs(sideForce);
    if (!(maxSide < sideForceAbs)) {
      wheel->realTimeState.slipping = false;
    } else {
      float capped = SignNonNegative(sideForce) * maxSide;
      sideForce = (1.0f - tuning.gearedDrive.sideFrictionSlipBlend) * capped +
                  tuning.gearedDrive.sideFrictionSlipBlend * sideForce;
      wheel->realTimeState.slipping = true;
    }
    if (wheel->realTimeState.slipping) {
      request.outSlipFlag = 1;
    }

    GmVec3 lateralForce = SceneVehicleMath::Scale(lateral, sideForce);
    AddVehicleCentralForce(lateralForce);

    float rollover =
        -tuning.GetRolloverLateralFromSpeed(request.linearSpeed.z) *
        request.slopeAdherenceA *
        tuning.GetRolloverLateralCoefFromAngle(std::fabs(lateral.y));
    GmVec3 rolloverTorque = {
        lateralForce.z * rollover,
        0.0f,
        -rollover * lateralForce.x,
    };
    AddVehicleTorque(rolloverTorque);
  }
}

void CSceneVehicleCar::ApplyModel3SteeringTorques(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    float speedMagnitude) {
  const u32 wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    SSimulationWheel *wheel = &WheelAt(wheelIndex);
    float halfTrack = (IsFrontVehicleWheel(wheel->axle)
                           ? gearedDrive.wheelLongitudinalSpan
                           : -gearedDrive.wheelLongitudinalSpan) *
                      0.5f;
    float steerRamp = 1.0f;
    if (!(tuning.steering.assistFullSpeed < speedMagnitude)) {
      steerRamp = CIsin(static_cast<float>(
          (speedMagnitude / tuning.steering.assistFullSpeed) *
          SceneVehicleMath::HalfPi));
    }
    float maxSide = tuning.GetMaxSideFrictionFromSpeed(request.linearSpeed.z) *
                    request.materialVals.w;
    float wheelSideSpeed =
        request.linearSpeed.x + request.angularSpeed.y * halfTrack;
    float sideForce =
        -tuning.gearedDrive.lateralForceScale * 0.5f * wheelSideSpeed;
    if (maxSide < std::fabs(sideForce)) {
      float blended =
          (1.0f - tuning.gearedDrive.driveSideFrictionSlipBlend) * maxSide +
          tuning.gearedDrive.driveSideFrictionSlipBlend * std::fabs(sideForce);
      sideForce = SignNonNegative(sideForce) * blended;
    }

    float sideTorque = tuning.gearedDrive.sideForceToDriveTorqueScale * sideForce;
    if (IsFrontVehicleWheel(wheel->axle)) {
      float reverseSign = !engine.useLowSpeedGateB ? 1.0f : -1.0f;
      float slipScale = wheel->realTimeState.slipping
                            ? tuning.gearedDrive.slippingSteerTorqueScale
                            : 1.0f;
      float steerTorque =
          tuning.GetSteerDriveTorqueFromSpeed(request.linearSpeed.z);
      const float steerAssist = reverseSign * steerRamp *
                                controls.currentSteering * steerTorque *
                                slipScale;
      sideTorque = sideTorque - steerAssist;
    }
    AddVehicleTorque({0.0f, sideTorque * halfTrack, 0.0f});
  }
}

void CSceneVehicleCar::ApplyModel3DriveForces(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning) {
  float accelBase = tuning.GetAccelFromSpeed(request.linearSpeed.z);
  float sideLimit = tuning.GetMaxSideFrictionFromSpeed(request.linearSpeed.z) *
                    request.materialVals.w;
  float sideSlowdownInput = std::fabs(
      tuning.gearedDrive.lateralForceScale * 0.5f * request.linearSpeed.x);
  if (sideLimit < sideSlowdownInput) {
    sideSlowdownInput = sideLimit;
  }
  float driveScale =
      request.materialVals.y * controls.lowSpeedGateA +
      (engine.useLowSpeedGateB ? -1.0f : 0.0f) * request.materialVals.y *
          controls.lowSpeedGateB +
      (turbo.type != ETurboType_Inactive ? turbo.impulseScale : 0.0f);
  float driveForce =
      (accelBase - tuning.steering.slowDownScale * sideSlowdownInput *
                       std::fabs(controls.currentSteering) *
                       tuning.GetSteerSlowDownFromSpeed(request.linearSpeed.z)) *
      driveScale;
  if (controls.forcedLowSpeedFriction != 0) {
    driveForce = turbo.type == ETurboType_Inactive
                     ? 0.0f
                     : accelBase * turbo.impulseScale;
  }

  float opposingLongitudinalForce = 0.0f;
  if (request.linearSpeed.z > 0.0f) {
    opposingLongitudinalForce =
        (tuning.gearedDrive.forwardAccelBase +
         tuning.gearedDrive.forwardAccelSpeedCoef * request.linearSpeed.z) *
        controls.lowSpeedGateB;
    float cap =
        request.materialVals.z *
        (request.outSlipFlag == 0 ? tuning.gearedDrive.forwardAccelCap
                                  : tuning.gearedDrive.forwardAccelCapWhenSlipping);
    if (cap < opposingLongitudinalForce) {
      opposingLongitudinalForce = cap;
      MarkAllWheelsSlipping();
    }
  }
  if (request.linearSpeed.z < 0.0f &&
      controls.forcedLowSpeedFriction != 0) {
    opposingLongitudinalForce =
        (tuning.gearedDrive.forwardAccelBase -
         tuning.gearedDrive.forwardAccelSpeedCoef * request.linearSpeed.z) *
        controls.lowSpeedGateA;
    float cap =
        request.materialVals.z *
        (request.outSlipFlag == 0 ? tuning.gearedDrive.forwardAccelCap
                                  : tuning.gearedDrive.forwardAccelCapWhenSlipping);
    if (cap < opposingLongitudinalForce) {
      opposingLongitudinalForce = cap;
      MarkAllWheelsSlipping();
    }
    opposingLongitudinalForce = -opposingLongitudinalForce;
  }
  if (controls.forcedLowSpeedFriction != 0 &&
      std::fabs(request.linearSpeed.z) < 1.0f) {
    opposingLongitudinalForce *= std::fabs(request.linearSpeed.z);
  }

  request.outSurfaceFeedback = opposingLongitudinalForce;
  float netLongitudinal = driveForce - opposingLongitudinalForce;
  if (tuning.engineSpeedNorm * request.materialVals.x < request.linearSpeed.z) {
    netLongitudinal = -tuning.gearedDrive.speedLimitForce;
  }
  if (request.linearSpeed.z <
      -(tuning.gearedDrive.transmission.reverseSpeedNorm *
        request.materialVals.x)) {
    netLongitudinal = tuning.gearedDrive.speedLimitForce;
  }

  float longitudinalForceZ = netLongitudinal * request.slopeAdherenceB;
  AddVehicleCentralForce({0.0f, 0.0f, longitudinalForceZ});
  AddVehicleTorque({
      -longitudinalForceZ * tuning.slipResponse.longitudinalTorqueScale,
      0.0f,
      0.0f,
  });
  AddVehicleCentralForce({
      0.0f,
      0.0f,
      (-tuning.gearedDrive.forceZScale * request.currentForce.z) /
          tuning.bodyAirResponse.groundedSolidFeedback1,
  });
}

void CSceneVehicleCar::ComputeForcesModel3(
    float dt, const GmVec3 &currentForce, float slopeAdherenceA,
    float slopeAdherenceB, const GmVec3 &linearSpeed,
    const GmVec3 &angularSpeed, float visualSteerYaw, int hasGroundMaterial,
    CSceneVehicleMaterial::SBlendableVals *materialVals, int &outSlipFlag,
    float &outSurfaceFeedback) {
  LegacyForceRequest request{
      dt, currentForce, slopeAdherenceA, slopeAdherenceB, linearSpeed,
      angularSpeed, visualSteerYaw, hasGroundMaterial != 0, *materialVals,
      outSlipFlag, outSurfaceFeedback,
  };
  const CSceneVehicleCarTuning &tuning = *Tunings()->ActiveTuning();
  ApplyModel3ContactForces(request, tuning);
  if (!request.hasGroundMaterial ||
      tuning.handlingModel != CSceneVehicleCarHandlingModel_Lateral) {
    return;
  }

  float speedMagnitude = CIsqrt(SceneVehicleMath::LengthSquared(linearSpeed));
  if (controls.forcedLowSpeedFriction == 0) {
    if (!(speedMagnitude < reverseGearSpeedThreshold)) {
      if (controls.lowSpeedGateA > LowSpeedGateThreshold) {
        engine.useLowSpeedGateB = false;
      }
    } else if (!(LowSpeedGateThreshold < controls.lowSpeedGateB)) {
      engine.useLowSpeedGateB = false;
    } else {
      engine.useLowSpeedGateB = true;
    }
  }
  ApplyModel3SteeringTorques(request, tuning, speedMagnitude);
  ApplyModel3DriveForces(request, tuning);
}

void CSceneVehicleCar::GetLateralFriction(
    const GmVec3 &linearSpeed, const GmVec3 &direction,
    CSceneVehicleMaterial::SBlendableVals *materialVals, float slopeAdherenceA,
    int alreadySlipping, float &outForce, int &outSlipping) {
  const CSceneVehicleCarTuning &tuning =
      static_cast<const CSceneVehicleCarTuning &>(GetVehicleTuning());
  float sideSpeed = (linearSpeed.y * direction.y + direction.x * linearSpeed.x +
                     linearSpeed.z * direction.z);
  float absSideSpeed = std::fabs(sideSpeed);
  float requestedForce =
      (tuning.radiusSteering.lateralFrictionLinear * -sideSpeed -
       sideSpeed * absSideSpeed *
           tuning.radiusSteering.lateralFrictionQuadratic);
  float slipScale = alreadySlipping != 0
                        ? (tuning.radiusSteering.slippingFrictionScale)
                        : 1.0f;
  float maxFromSpeed = tuning.M4GetMaxFrictionForceFromSpeed(absSideSpeed);
  float maxForce =
      maxFromSpeed * (materialVals->w * slopeAdherenceA) * slipScale;
  float absRequestedForce = std::fabs(requestedForce);

  if (maxForce >= absRequestedForce) {
    outSlipping = 0;
    outForce = requestedForce;
  } else {
    float sign = requestedForce < 0.0f ? -1.0f : 1.0f;
    outSlipping = 1;
    outForce = maxForce * sign;
  }
}

void CSceneVehicleCar::PrepareModel4GroundForces(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    Model4ForceState &state) {
  state.speedMagnitude =
      CIsqrt(SceneVehicleMath::LengthSquared(request.linearSpeed));
  if (controls.forcedLowSpeedFriction == 0) {
    if (!(state.speedMagnitude < reverseGearSpeedThreshold)) {
      if (controls.lowSpeedGateA > LowSpeedGateThreshold) {
        engine.useLowSpeedGateB = false;
      }
    } else if (!(LowSpeedGateThreshold < controls.lowSpeedGateB)) {
      engine.useLowSpeedGateB = false;
    } else {
      engine.useLowSpeedGateB = true;
    }
  }

  if (radiusSteering.phase == CSceneVehicleCarRadiusSteeringPhase_DirectInput &&
      std::fabs(controls.currentSteering) < ScalarEpsilon) {
    radiusSteering.phase =
        CSceneVehicleCarRadiusSteeringPhase_CapturedSlipAngle;
    radiusSteering.steerAngle =
        CIatan2(request.linearSpeed.x, request.linearSpeed.z);
  }

  if (radiusSteering.phase == CSceneVehicleCarRadiusSteeringPhase_DirectInput) {
    state.steerAngle = -controls.currentSteering *
                       tuning.radiusSteering.steerAngleFromInputScale;
  } else if (radiusSteering.phase ==
             CSceneVehicleCarRadiusSteeringPhase_CapturedSlipAngle) {
    state.steerAngle = radiusSteering.steerAngle;
  }

  state.steerAngleSin = CIsin(state.steerAngle);
  state.steerAngleCos = CIcos(state.steerAngle);
  state.steeredSide =
      GmVec3{state.steerAngleCos, 0.0f, -state.steerAngleSin};
  state.forwardSpeed = request.linearSpeed.x * state.steerAngleSin +
                       request.linearSpeed.z * state.steerAngleCos;
  state.sideSpeedForExit = request.linearSpeed.x;

  float steeredFriction = 0.0f;
  int steeredSlip = 0;
  float xFriction = 0.0f;
  int xSlip = 0;
  const GmVec3 localX = {1.0f, 0.0f, 0.0f};
  GetLateralFriction(request.linearSpeed, state.steeredSide,
                     &request.materialVals, request.slopeAdherenceA,
                     state.wasRadiusSteeringActive, steeredFriction,
                     steeredSlip);
  GetLateralFriction(request.linearSpeed, localX, &request.materialVals,
                     request.slopeAdherenceA,
                     state.wasRadiusSteeringActive, xFriction, xSlip);
  const GmVec3 steeredForce =
      SceneVehicleMath::Scale(state.steeredSide, steeredFriction * 0.5f);
  const GmVec3 xForce = {xFriction * 0.5f, 0.0f, 0.0f};
  AddVehicleCentralForce(steeredForce);
  AddVehicleCentralForce(xForce);

  const GmVec3 dampingTorque = {
      0.0f,
      -request.angularSpeed.y * tuning.radiusSteering.angularDampingLinear -
          std::fabs(request.angularSpeed.y) * request.angularSpeed.y *
              tuning.radiusSteering.angularDampingQuadratic,
      0.0f,
  };
  AddVehicleTorque(dampingTorque);
}

void CSceneVehicleCar::ApplyModel4SteeringTorque(
    const CSceneVehicleCarTuning &tuning, Model4ForceState &state) {
  float steerTorqueY = 0.0f;
  if (radiusSteering.phase ==
          CSceneVehicleCarRadiusSteeringPhase_CapturedSlipAngle &&
      std::fabs(radiusSteering.steerAngle) > ScalarEpsilon &&
      std::fabs(tuning.radiusSteering.capturedAngleRadiusScale) >
          ScalarEpsilon) {
    float reverseSign = !engine.useLowSpeedGateB ? 1.0f : -1.0f;
    float angleSign = radiusSteering.steerAngle > 0.0f ? 1.0f : -1.0f;
    float radius =
        tuning.M4GetSteerRadiusFromSpeed(std::fabs(state.speedMagnitude)) /
        (tuning.radiusSteering.capturedAngleRadiusScale *
         std::fabs(radiusSteering.steerAngle));
    if (radius > ScalarEpsilon) {
      steerTorqueY =
          (-(angleSign * reverseSign) * state.speedMagnitude *
           tuning.radiusSteering.steerTorqueSpeedScale *
           tuning.radiusSteering.angularDampingLinear) /
          radius;
    }
  } else if (std::fabs(controls.currentSteering) > ScalarEpsilon) {
    float reverseSign = !engine.useLowSpeedGateB ? 1.0f : -1.0f;
    float steerSign = controls.currentSteering > 0.0f ? 1.0f : -1.0f;
    float activeScale = state.wasRadiusSteeringActive != 0
                            ? tuning.radiusSteering.inputSteerRadiusScale
                            : 1.0f;
    float radius =
        tuning.M4GetSteerRadiusFromSpeed(std::fabs(state.speedMagnitude)) *
        activeScale;
    if (radius > ScalarEpsilon) {
      steerTorqueY =
          (-(steerSign * reverseSign) * state.speedMagnitude *
           tuning.radiusSteering.steerTorqueSpeedScale *
           tuning.radiusSteering.angularDampingLinear *
           std::fabs(controls.currentSteering)) /
          radius;
    }
  }
  if (steerTorqueY != 0.0f) {
    AddVehicleTorque({0.0f, steerTorqueY, 0.0f});
  }
}

void CSceneVehicleCar::ComputeModel4Drive(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    Model4ForceState &state) {
  float accelBase = tuning.GetAccelFromSpeed(state.forwardSpeed);
  float reverseDriveSign = engine.useLowSpeedGateB ? -1.0f : 0.0f;
  float turboDrive = turbo.type != ETurboType_Inactive ? turbo.impulseScale : 0.0f;
  state.driveForce =
      accelBase * (request.materialVals.y * controls.lowSpeedGateA +
                   request.materialVals.y * reverseDriveSign *
                       controls.lowSpeedGateB +
                   turboDrive);
  if (controls.forcedLowSpeedFriction != 0) {
    state.driveForce = turbo.type == ETurboType_Inactive
                           ? 0.0f
                           : accelBase * turbo.impulseScale;
  }

  if (state.forwardSpeed > 0.0f) {
    state.opposingLongitudinalForce =
        (tuning.gearedDrive.forwardAccelBase +
         tuning.gearedDrive.forwardAccelSpeedCoef * state.forwardSpeed) *
        controls.lowSpeedGateB;
    float cap =
        request.materialVals.z *
        (request.outSlipFlag == 0 ? tuning.gearedDrive.forwardAccelCap
                                  : tuning.gearedDrive.forwardAccelCapWhenSlipping);
    if (cap < state.opposingLongitudinalForce) {
      state.opposingLongitudinalForce = cap;
      state.slipSeen = 1;
    }
  }
  if (state.forwardSpeed < 0.0f && controls.forcedLowSpeedFriction != 0) {
    state.opposingLongitudinalForce =
        (tuning.gearedDrive.forwardAccelBase -
         tuning.gearedDrive.forwardAccelSpeedCoef * state.forwardSpeed) *
        controls.lowSpeedGateA;
    float cap =
        request.materialVals.z *
        (request.outSlipFlag == 0 ? tuning.gearedDrive.forwardAccelCap
                                  : tuning.gearedDrive.forwardAccelCapWhenSlipping);
    if (cap < state.opposingLongitudinalForce) {
      state.opposingLongitudinalForce = cap;
      state.slipSeen = 1;
    }
    state.opposingLongitudinalForce = -state.opposingLongitudinalForce;
  }
  if (controls.forcedLowSpeedFriction != 0 &&
      std::fabs(state.forwardSpeed) < 1.0f) {
    state.opposingLongitudinalForce *= std::fabs(state.forwardSpeed);
  }
}

void CSceneVehicleCar::UpdateModel4RadiusState(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    Model4ForceState &state) {
  if (state.wasRadiusSteeringActive == 0) {
    if (state.slipSeen != 0) {
      radiusSteering.steerAngle = 0.0f;
      radiusSteering.phase = CSceneVehicleCarRadiusSteeringPhase_DirectInput;
      radiusSteering.previousSteerSign =
          controls.currentSteering > 0.0f ? 1.0f : -1.0f;
    }
    return;
  }

  radiusSteering.steerAngle += tuning.radiusSteering.capturedAngleRate *
                               controls.currentSteering * request.dt;
  if (std::fabs(radiusSteering.steerAngle) >
      tuning.radiusSteering.steerAngleLimit) {
    radiusSteering.steerAngle = SignNonNegative(radiusSteering.steerAngle) *
                                tuning.radiusSteering.steerAngleLimit;
  }
  int crossedZero =
      (radiusSteering.previousSteerSign > 0.0f &&
       radiusSteering.steerAngle < 0.0f) ||
      (radiusSteering.previousSteerSign < 0.0f &&
       radiusSteering.steerAngle > 0.0f);
  if ((std::fabs(state.sideSpeedForExit) <
           tuning.radiusSteering.captureExitSideSpeedMax &&
       std::fabs(controls.currentSteering) < ScalarEpsilon) ||
      crossedZero) {
    state.slipSeen = 0;
    radiusSteering.steerAngle = 0.0f;
    radiusSteering.phase = CSceneVehicleCarRadiusSteeringPhase_Idle;
  } else {
    state.slipSeen = 1;
  }
}

void CSceneVehicleCar::ApplyModel4LongitudinalForce(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    const Model4ForceState &state) {
  request.outSurfaceFeedback = state.opposingLongitudinalForce;
  float netLongitudinal = state.driveForce - state.opposingLongitudinalForce;
  if (tuning.engineSpeedNorm * request.materialVals.x < state.forwardSpeed) {
    netLongitudinal = -tuning.gearedDrive.speedLimitForce;
  }
  if (state.forwardSpeed <
      -(tuning.gearedDrive.transmission.reverseSpeedNorm *
        request.materialVals.x)) {
    netLongitudinal = tuning.gearedDrive.speedLimitForce;
  }

  float longitudinal = netLongitudinal * request.slopeAdherenceB;
  AddVehicleCentralForce({
      longitudinal * state.steerAngleSin,
      0.0f,
      longitudinal * state.steerAngleCos,
  });
  AddVehicleTorque({
      -longitudinal * tuning.slipResponse.longitudinalTorqueScale,
      0.0f,
      0.0f,
  });
}

void CSceneVehicleCar::ComputeForcesModel4(
    float dt, const GmVec3 &currentForce, float slopeAdherenceA,
    float slopeAdherenceB, const GmVec3 &linearSpeed,
    const GmVec3 &angularSpeed, float visualSteerYaw, int hasGroundMaterial,
    CSceneVehicleMaterial::SBlendableVals *materialVals, int &outSlipFlag,
    float &outSurfaceFeedback) {
  LegacyForceRequest request{
      dt, currentForce, slopeAdherenceA, slopeAdherenceB, linearSpeed,
      angularSpeed, visualSteerYaw, hasGroundMaterial != 0, *materialVals,
      outSlipFlag, outSurfaceFeedback,
  };
  const CSceneVehicleCarTuning &tuning = *Tunings()->ActiveTuning();
  for (u32 wheelIndex = 0; wheelIndex < WheelGetCount(); wheelIndex++) {
    WheelAddForceToVehicle(WheelAt(wheelIndex), currentForce);
  }

  Model4ForceState state;
  state.wasRadiusSteeringActive =
      radiusSteering.phase != CSceneVehicleCarRadiusSteeringPhase_Idle;
  if (request.hasGroundMaterial) {
    PrepareModel4GroundForces(request, tuning, state);
    ApplyModel4SteeringTorque(tuning, state);
    ComputeModel4Drive(request, tuning, state);
    UpdateModel4RadiusState(request, tuning, state);
    ApplyModel4LongitudinalForce(request, tuning, state);
  }

  for (u32 wheelIndex = 0; wheelIndex < WheelGetCount(); wheelIndex++) {
    WheelAt(wheelIndex).realTimeState.slipping = state.slipSeen != 0;
  }
  slipMemory.active = state.slipSeen != 0;
}

void CSceneVehicleCar::ApplyModel5ContactForces(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning) {
  const u32 wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    SSimulationWheel *wheel = &WheelAt(wheelIndex);
    WheelAddForceToVehicle(*wheel, request.currentForce);

    CSceneVehicleMaterial *material = GetWheelMaterial(*wheel);
    if (!wheel->realTimeState.contactPresent ||
        !(tuning.gearedDrive.lateralForceScale > 0.0f)) {
      continue;
    }

    float slipGrip = wheel->realTimeState.slipping
                         ? tuning.gearedDrive.slippingSideFrictionScale
                         : 1.0f;
    float maxSide = material->blendableVals.w * request.slopeAdherenceA *
                    tuning.GetMaxSideFrictionFromSpeed(request.linearSpeed.z) *
                    slipGrip;
    GmVec3 lateral = {
        wheel->realTimeState.accumulatedContactNormal.y,
        -wheel->realTimeState.accumulatedContactNormal.x,
        0.0f,
    };
    lateral = SceneVehicleMath::NormalizeOr(
        lateral, GmVec3{1.0f, 0.0f, 0.0f}, VectorEpsilonSquared);
    if (IsFrontVehicleWheel(wheel->axle)) {
      float visualSteerYawCos = CIcos(request.visualSteerYaw);
      float negVisualSteerYawSin = -CIsin(request.visualSteerYaw);
      lateral = GmVec3{
          visualSteerYawCos * lateral.x,
          visualSteerYawCos * lateral.y,
          negVisualSteerYawSin + visualSteerYawCos * lateral.z,
      };
    }

    float sideForce = -tuning.gearedDrive.lateralForceScale * 0.5f *
                      SceneVehicleMath::Dot(request.linearSpeed, lateral);
    float sideForceAbs = std::fabs(sideForce);
    if (!(maxSide < sideForceAbs)) {
      wheel->realTimeState.slipping = false;
    } else {
      float capped = SignNonNegative(sideForce) * maxSide;
      sideForce = (1.0f - tuning.gearedDrive.sideFrictionSlipBlend) * capped +
                  tuning.gearedDrive.sideFrictionSlipBlend * sideForce;
      wheel->realTimeState.slipping = true;
    }
    if (wheel->realTimeState.slipping) {
      request.outSlipFlag = 1;
    }

    GmVec3 lateralForce = SceneVehicleMath::Scale(lateral, sideForce);
    AddVehicleCentralForce(lateralForce);
    float rollover =
        -tuning.GetRolloverLateralFromSpeed(request.linearSpeed.z) *
        request.slopeAdherenceA *
        tuning.GetRolloverLateralCoefFromAngle(std::fabs(lateral.y));
    AddVehicleTorque({
        lateralForce.z * rollover,
        0.0f,
        -rollover * lateralForce.x,
    });
  }
}

void CSceneVehicleCar::ApplyModel5SideTorques(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    Model5ForceState &state) {
  const float speedMagnitude =
      CIsqrt(SceneVehicleMath::LengthSquared(request.linearSpeed));
  if (controls.forcedLowSpeedFriction == 0) {
    if (!(speedMagnitude < reverseGearSpeedThreshold)) {
      if (controls.lowSpeedGateA > LowSpeedGateThreshold) {
        engine.useLowSpeedGateB = false;
      }
    } else if (!(LowSpeedGateThreshold < controls.lowSpeedGateB)) {
      engine.useLowSpeedGateB = false;
    } else {
      engine.useLowSpeedGateB = true;
    }
  }

  for (u32 wheelIndex = 0; wheelIndex < 4; wheelIndex++) {
    SSimulationWheel *wheel = &WheelAt(wheelIndex);
    float halfTrack = (IsFrontVehicleWheel(wheel->axle)
                           ? gearedDrive.wheelLongitudinalSpan
                           : -gearedDrive.wheelLongitudinalSpan) *
                      0.5f;
    float steerRamp = 1.0f;
    if (!(tuning.steering.assistFullSpeed < speedMagnitude)) {
      steerRamp = CIsin(static_cast<float>(
          (speedMagnitude / tuning.steering.assistFullSpeed) *
          SceneVehicleMath::HalfPi));
    }
    float maxSide = tuning.GetMaxSideFrictionFromSpeed(request.linearSpeed.z) *
                    request.materialVals.w;
    float wheelSideSpeed =
        request.linearSpeed.x + request.angularSpeed.y * halfTrack;
    float sideForce =
        -tuning.gearedDrive.lateralForceScale * 0.5f * wheelSideSpeed;
    if (std::fabs(sideForce) > maxSide) {
      state.sideForceLimitTotal += maxSide;
      state.requestedSideForceTotal += std::fabs(sideForce);
      float blended =
          (1.0f - tuning.gearedDrive.driveSideFrictionSlipBlend) * maxSide +
          tuning.gearedDrive.driveSideFrictionSlipBlend * std::fabs(sideForce);
      sideForce = SignNonNegative(sideForce) * blended;
      state.slipSeen = 1;
    }

    float sideTorque = tuning.gearedDrive.sideForceToDriveTorqueScale * sideForce;
    if (IsFrontVehicleWheel(wheel->axle)) {
      float reverseSign = !engine.useLowSpeedGateB ? 1.0f : -1.0f;
      float slipScale = wheel->realTimeState.slipping
                            ? tuning.gearedDrive.slippingSteerTorqueScale
                            : 1.0f;
      float steerTorque =
          tuning.GetSteerDriveTorqueFromSpeed(request.linearSpeed.z);
      sideTorque = sideTorque - reverseSign * steerRamp *
                                    controls.currentSteering * steerTorque *
                                    slipScale;
    }
    AddVehicleTorque({0.0f, sideTorque * halfTrack, 0.0f});
  }
}

void CSceneVehicleCar::ComputeModel5DriveForce(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    Model5ForceState &state) {
  state.tick = CMwCmdBufferCore::Current()->Timer().GetTickTime();
  if (state.slipSeen != 0) {
    slipMemory.lastTick = state.tick;
    if (!state.wasSlipping) {
      slipMemory.startTick = state.tick;
    }
    slipMemory.elapsedTicks = state.tick - slipMemory.startTick;
  }

  float slipAccelMix = ComputeSlipAccelerationBlend(
      &tuning, state.tick, state.sideForceLimitTotal,
      state.requestedSideForceTotal);
  float accelSlip = tuning.M5GetSlippingAccelFromSpeed(request.linearSpeed.z);
  state.accelBase = tuning.M5GetAccelFromSpeed(request.linearSpeed.z);
  float accel = (1.0f - slipAccelMix) * accelSlip +
                state.accelBase * slipAccelMix;

  if (std::fabs(controls.steeringControl) > ScalarEpsilon) {
    slipMemory.steeringMemoryTick = state.tick;
    slipMemory.steeringMemorySlip = state.slipSeen != 0;
  }
  float steeringSlowdownGate = 0.0f;
  if (slipMemory.steeringMemoryTick <= state.tick &&
      state.tick - slipMemory.steeringMemoryTick <
          tuning.slipResponse.steeringMemoryTicks) {
    steeringSlowdownGate =
        (slipMemory.steeringMemorySlip == 0 ||
         tuning.slipResponse.slipSlowdownEnabled == 0)
            ? 1.0f
            : 0.0f;
  }
  u32 slipElapsedWindow = slipMemory.elapsedTicks;
  if (tuning.slipResponse.slipSlowdownTicks < slipElapsedWindow) {
    slipElapsedWindow = tuning.slipResponse.slipSlowdownTicks;
  }
  if (tuning.slipResponse.slipSlowdownEnabled != 0 &&
      slipMemory.lastTick <= state.tick &&
      state.tick - slipMemory.lastTick <= slipElapsedWindow) {
    steeringSlowdownGate = 0.0f;
  }

  float reverseDriveSign = engine.useLowSpeedGateB ? -1.0f : 0.0f;
  float turboDrive =
      turbo.type != ETurboType_Inactive ? turbo.impulseScale : 0.0f;
  state.driveForce =
      (request.materialVals.y * controls.lowSpeedGateA +
       request.materialVals.y * reverseDriveSign * controls.lowSpeedGateB) *
          accel +
      state.accelBase * turboDrive -
      tuning.steering.slowDownScale * steeringSlowdownGate *
          tuning.M5GetSteerSlowDownFromSpeed(request.linearSpeed.z);
  if (controls.forcedLowSpeedFriction != 0) {
    state.driveForce = turbo.type == ETurboType_Inactive
                           ? 0.0f
                           : state.accelBase * turbo.impulseScale;
  }
}

void CSceneVehicleCar::ApplyModel5LongitudinalForce(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    Model5ForceState &state) {
  float opposingLongitudinalForce = 0.0f;
  if (request.linearSpeed.z > 0.0f) {
    opposingLongitudinalForce =
        (tuning.gearedDrive.forwardAccelBase +
         tuning.gearedDrive.forwardAccelSpeedCoef * request.linearSpeed.z) *
        controls.lowSpeedGateB;
    float cap =
        request.materialVals.z *
        (request.outSlipFlag == 0 ? tuning.gearedDrive.forwardAccelCap
                                  : tuning.gearedDrive.forwardAccelCapWhenSlipping);
    if (cap < opposingLongitudinalForce) {
      opposingLongitudinalForce = cap;
      MarkAllWheelsSlipping();
      state.slipSeen = 1;
    }
  }
  if (request.linearSpeed.z < 0.0f &&
      controls.forcedLowSpeedFriction != 0) {
    opposingLongitudinalForce =
        (tuning.gearedDrive.forwardAccelBase -
         tuning.gearedDrive.forwardAccelSpeedCoef * request.linearSpeed.z) *
        controls.lowSpeedGateA;
    float cap =
        request.materialVals.z *
        (request.outSlipFlag == 0 ? tuning.gearedDrive.forwardAccelCap
                                  : tuning.gearedDrive.forwardAccelCapWhenSlipping);
    if (cap < opposingLongitudinalForce) {
      opposingLongitudinalForce = cap;
      MarkAllWheelsSlipping();
      state.slipSeen = 1;
    }
    opposingLongitudinalForce = -opposingLongitudinalForce;
  }
  if (controls.forcedLowSpeedFriction != 0 &&
      std::fabs(request.linearSpeed.z) < 1.0f) {
    opposingLongitudinalForce *= std::fabs(request.linearSpeed.z);
  }

  request.outSurfaceFeedback = opposingLongitudinalForce;
  float netLongitudinal = state.driveForce - opposingLongitudinalForce;
  if (tuning.engineSpeedNorm * request.materialVals.x < request.linearSpeed.z) {
    netLongitudinal = -tuning.gearedDrive.speedLimitForce;
  }
  if (request.linearSpeed.z <
      -(tuning.gearedDrive.transmission.reverseSpeedNorm *
        request.materialVals.x)) {
    netLongitudinal = tuning.gearedDrive.speedLimitForce;
  }

  float longitudinalForceZ = netLongitudinal * request.slopeAdherenceB;
  AddVehicleCentralForce({0.0f, 0.0f, longitudinalForceZ});
  if (state.waterActive == 0) {
    float limitedLongitudinal = longitudinalForceZ;
    if (tuning.slipResponse.rolloverTorqueCap <
        std::fabs(limitedLongitudinal)) {
      limitedLongitudinal = SignNonNegative(limitedLongitudinal) *
                            tuning.slipResponse.rolloverTorqueCap;
    }
    AddVehicleTorque({
        -limitedLongitudinal * tuning.slipResponse.longitudinalTorqueScale,
        0.0f,
        0.0f,
    });
  }
  AddVehicleCentralForce({
      0.0f,
      0.0f,
      (-tuning.gearedDrive.forceZScale * request.currentForce.z) /
          tuning.bodyAirResponse.groundedSolidFeedback1,
  });
}

void CSceneVehicleCar::ComputeForcesModel5(
    float dt, const GmVec3 &currentForce, float slopeAdherenceA,
    float slopeAdherenceB, const GmVec3 &linearSpeed,
    const GmVec3 &angularSpeed, float visualSteerYaw, int hasGroundMaterial,
    CSceneVehicleMaterial::SBlendableVals *materialVals, int &outSlipFlag,
    float &outSurfaceFeedback) {
  LegacyForceRequest request{
      dt, currentForce, slopeAdherenceA, slopeAdherenceB, linearSpeed,
      angularSpeed, visualSteerYaw, hasGroundMaterial != 0, *materialVals,
      outSlipFlag, outSurfaceFeedback,
  };
  const CSceneVehicleCarTuning &tuning = *Tunings()->ActiveTuning();
  Model5ForceState state;
  state.waterActive = ApplyWaterForces(currentForce);
  state.wasSlipping = slipMemory.active;
  controls.noGroundFrictionGuard = state.waterActive != 0;

  ApplyModel5ContactForces(request, tuning);
  if (!request.hasGroundMaterial) {
    slipMemory.active = false;
    return;
  }
  ApplyModel5SideTorques(request, tuning, state);
  ComputeModel5DriveForce(request, tuning, state);
  ApplyModel5LongitudinalForce(request, tuning, state);
  slipMemory.active = state.slipSeen != 0;
}

bool CSceneVehicleCar::HandleModel6CircularBurnout(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    const Model6ForceState &state) {
  if (gearedDrive.burnoutPhase != CSceneVehicleCarBurnoutPhase_CircularDrift) {
    return false;
  }
  ApplyCircularBurnoutForces(
      &tuning, request.currentForce, request.linearSpeed, request.angularSpeed,
      request.visualSteerYaw, request.hasGroundMaterial, state.waterActive);
  if (gearedDrive.burnoutPhase !=
      CSceneVehicleCarBurnoutPhase_CircularDrift) {
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_TimedSpin;
    gearedDrive.burnoutStartTick =
        CMwCmdBufferCore::Current()->Timer().GetTickTime();
  }
  gearedDrive.localSpeed = request.linearSpeed;
  return true;
}

void CSceneVehicleCar::ApplyModel6DirtSlide(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    const Model6ForceState &state) {
  if (!state.dirtSlideSurface || request.linearSpeed.z <= DirtSlideSpeedGate) {
    return;
  }
  if (controls.lowSpeedGateB > LowSpeedGateThreshold) {
    AddVehicleCentralForce({
        -DirtSlideSlowdownScale * request.linearSpeed.x,
        -DirtSlideSlowdownScale * request.linearSpeed.y,
        -DirtSlideSlowdownScale * request.linearSpeed.z,
    });
  }
  if (controls.forcedLowSpeedFriction != 0 ||
      controls.lowSpeedGateA <= LowSpeedGateThreshold ||
      !CanApplyDirtSlideForces()) {
    return;
  }

  GmVec3 unitSpeed = SceneVehicleMath::NormalizeOr(
      request.linearSpeed, GmVec3{0.0f, 0.0f, 1.0f}, VectorEpsilonSquared);
  const DirtSlideForces slideForces =
      BuildDirtSlideForces(&tuning, request.linearSpeed, unitSpeed);
  const GmVec3 &frontSlideForce = slideForces.front;
  const GmVec3 &rearSlideForce = slideForces.rear;
  const u32 slideWheelCount = WheelGetCount();
  for (u32 slideWheelIndex = 0; slideWheelIndex < slideWheelCount;
       slideWheelIndex++) {
    SSimulationWheel *slideWheel = &WheelAt(slideWheelIndex);
    if (!slideWheel->realTimeState.slipping) {
      continue;
    }
    if (slideWheelIndex <= 1) {
      AddVehicleForce(frontSlideForce, slideWheel->forceApplicationPoint);
    }
    if (slideWheelIndex == 2 || slideWheelIndex == 3) {
      AddVehicleForce(rearSlideForce, slideWheel->forceApplicationPoint);
    }
  }
}

void CSceneVehicleCar::ApplyModel6ContactWheel(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    Model6ForceState &state, SSimulationWheel &wheel,
    const GmVec3 &bodyCenter) {
  if (!wheel.realTimeState.contactPresent ||
      !(tuning.gearedDrive.lateralForceScale >= 0.0f)) {
    return;
  }

  CSceneVehicleMaterial *material = GetWheelMaterial(wheel);
  GmVec3 contactLever = SceneVehicleMath::Subtract(
      wheel.realTimeState.latestContactPoint, bodyCenter);
  GmVec3 contactSideAxis = {
      wheel.realTimeState.accumulatedContactNormal.y,
      -wheel.realTimeState.accumulatedContactNormal.x,
      0.0f,
  };
  contactSideAxis = SceneVehicleMath::NormalizeOr(
      contactSideAxis, GmVec3{1.0f, 0.0f, 0.0f}, VectorEpsilonSquared);
  if (IsFrontVehicleWheel(wheel.axle)) {
    float visualSteerYawCos = CIcos(request.visualSteerYaw);
    float negVisualSteerYawSin = -CIsin(request.visualSteerYaw);
    contactSideAxis = GmVec3{
        visualSteerYawCos * contactSideAxis.x,
        visualSteerYawCos * contactSideAxis.y,
        negVisualSteerYawSin + visualSteerYawCos * contactSideAxis.z,
    };
  }

  GmVec3 feedbackTorqueForce = SceneVehicleMath::Scale(
      gearedDrive.scaledCurrentForce, -tuning.feedback.forceDivisor);
  float feedbackTorqueForceLen =
      CIsqrt(SceneVehicleMath::LengthSquared(feedbackTorqueForce));
  if (feedbackTorqueForceLen < tuning.gearedDrive.currentForceTorqueMin) {
    feedbackTorqueForce = SceneVehicleMath::Zero();
  }
  GmVec3 feedbackTorque =
      SceneVehicleMath::Cross(contactLever, feedbackTorqueForce);
  feedbackTorque = SceneVehicleMath::Scale(feedbackTorque, -1.0f);
  feedbackTorque.x *= tuning.gearedDrive.currentTorqueXScale;
  feedbackTorque.y = 0.0f;
  feedbackTorque.z *= tuning.gearedDrive.currentTorqueZScale;
  AddVehicleTorque(feedbackTorque);

  if (gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_TimedSpin) {
    AddVehicleTorque({
        tuning.M6GetBurnoutRolloverFromSpeed(request.linearSpeed.z),
        0.0f,
        0.0f,
    });
  }

  float burnoutSideScale = BurnoutSideForceFade(&tuning, state.tick);
  float damperMod =
      tuning.M6GetModulationFromDamperAbsorbVal(wheel.realTimeState.damperAbsorb);
  bool slipBefore = wheel.realTimeState.slipping;
  float slippingSideGrip = slipBefore != 0
                               ? tuning.gearedDrive.slippingSideFrictionScale
                               : 1.0f;
  float lowSpeedBSlippingGrip =
      (wheel.realTimeState.slipping &&
       controls.lowSpeedGateB > LowSpeedGateThreshold)
          ? tuning.gearedDrive.lowSpeedBSlippingGripScale
          : 1.0f;
  float maxStaticSideForceCurve =
      tuning.GetMaxSideFrictionFromSpeed(request.linearSpeed.z);
  float maxStaticSideForce =
      material->blendableVals.w * request.slopeAdherenceA *
      maxStaticSideForceCurve * slippingSideGrip * lowSpeedBSlippingGrip *
      damperMod;
  float contactSideSpeed =
      SceneVehicleMath::Dot(request.linearSpeed, contactSideAxis);
  float requestedSideForce =
      -tuning.gearedDrive.lateralForceScale * 0.5f * contactSideSpeed *
      burnoutSideScale;
  float requestedSideForceAbs = std::fabs(requestedSideForce);
  if (!(maxStaticSideForce < requestedSideForceAbs)) {
    wheel.realTimeState.slipping = false;
  } else {
    float staticSideForceCap =
        SignNonNegative(requestedSideForce) * maxStaticSideForce;
    requestedSideForce =
        (1.0f - tuning.gearedDrive.sideFrictionSlipBlend) * staticSideForceCap +
        tuning.gearedDrive.sideFrictionSlipBlend * requestedSideForce;
    wheel.realTimeState.slipping = true;
    request.outSlipFlag = 1;
  }

  GmVec3 contactSideForce =
      SceneVehicleMath::Scale(contactSideAxis, requestedSideForce);
  ApplyModel6DirtSlide(request, tuning, state);
  AddVehicleCentralForce(contactSideForce);
}

void CSceneVehicleCar::ApplyModel6ContactForces(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    Model6ForceState &state) {
  const GmVec3 bodyCenter =
      HmsItem()->Solid()->Physical().Parameters().localCenterOfMass;
  state.wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < state.wheelCount; wheelIndex++) {
    SSimulationWheel &wheel = WheelAt(wheelIndex);
    WheelAddForceToVehicle(wheel, request.currentForce);
    ApplyModel6ContactWheel(request, tuning, state, wheel, bodyCenter);
  }
}

void CSceneVehicleCar::ApplyModel6GroundForces(
    const LegacyForceRequest &request, const CSceneVehicleCarTuning &tuning,
    Model6ForceState &state) {
  if (controls.forcedLowSpeedFriction == 0 &&
      controls.lowSpeedGateB > LowSpeedGateThreshold &&
      controls.lowSpeedGateA < LowSpeedGateThreshold &&
      gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_TimedSpin) {
    gearedDrive.burnoutExitStartTick = state.tick;
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_ExitFade;
  }

  TryEnterForwardBurnout(
      &tuning, request.linearSpeed, request.visualSteerYaw, state.frameY,
      state.tick, request.hasGroundMaterial);
  UpdateGearDirection(request.linearSpeed);

  float rolloverInput =
      (request.linearSpeed.x * request.linearSpeed.x) /
      (std::fabs(request.linearSpeed.z) + 1.0f);
  AddVehicleTorque({
      0.0f,
      0.0f,
      -SignNonNegative(request.linearSpeed.x) *
          tuning.M6GetRolloverLateralFromSpeedRatio(rolloverInput),
  });

  float steerAssistRamp = tuning.GearedSteerAssistRamp(request.linearSpeed);
  for (u32 wheelIndex = 0; wheelIndex < state.wheelCount; wheelIndex++) {
    SSimulationWheel *wheel = &WheelAt(wheelIndex);
    const GearedWheelSideForceResult wheelForce =
        ComputeGearedWheelSideForce(
            &tuning, *wheel, request.materialVals, request.linearSpeed,
            request.angularSpeed, steerAssistRamp, request.dt);
    state.sideForceLimitTotal += wheelForce.sideLimit;
    state.requestedSideForceTotal += wheelForce.sideRequested;
    state.slipSeen |= wheelForce.slipped;
  }

  UpdateSlipMemory(state.tick, state.slipSeen);
  float slipAccelMix = ComputeSlipAccelerationBlend(
      &tuning, state.tick, state.sideForceLimitTotal,
      state.requestedSideForceTotal);
  float driveForce = ComputeGearedDriveForce(
      &tuning, request.materialVals, request.linearSpeed, state.tick,
      state.waterActive, slipAccelMix);
  const OpposingLongitudinalResult opposing =
      ComputeOpposingLongitudinalForce(
          &tuning, request.materialVals, request.linearSpeed, driveForce,
          state.frameY, state.tick, request.outSlipFlag);
  const float opposingLongitudinalForce = opposing.force;
  state.slipSeen |= opposing.slipped;
  request.outSurfaceFeedback = opposingLongitudinalForce;

  float netLongitudinal =
      driveForce - SignNonNegative(request.linearSpeed.z) *
                       opposingLongitudinalForce;
  if (request.linearSpeed.z > tuning.engineSpeedNorm * request.materialVals.x) {
    netLongitudinal = !(netLongitudinal < 0.0f)
                          ? -tuning.gearedDrive.speedLimitForce
                          : netLongitudinal - tuning.gearedDrive.speedLimitForce;
  }
  if (request.linearSpeed.z <
      -tuning.gearedDrive.transmission.reverseSpeedNorm *
          request.materialVals.x) {
    netLongitudinal = !(netLongitudinal > 0.0f)
                          ? tuning.gearedDrive.speedLimitForce
                          : netLongitudinal + tuning.gearedDrive.speedLimitForce;
  }

  AddVehicleCentralForce(
      {0.0f, 0.0f, netLongitudinal * request.slopeAdherenceB});
  AddVehicleCentralForce({
      0.0f,
      0.0f,
      (-tuning.gearedDrive.forceZScale * request.currentForce.z) /
          tuning.bodyAirResponse.groundedSolidFeedback1,
  });
  slipMemory.active = state.slipSeen != 0;
  gearedDrive.localSpeed = request.linearSpeed;
}

void CSceneVehicleCar::ComputeForcesModel6(
    float dt, const GmVec3 &currentForce, float slopeAdherenceA,
    float slopeAdherenceB, const GmVec3 &linearSpeed,
    const GmVec3 &angularSpeed, float visualSteerYaw, int hasGroundMaterial,
    CSceneVehicleMaterial::SBlendableVals *materialVals, int &outSlipFlag,
    float &outSurfaceFeedback) {
  LegacyForceRequest request{
      dt, currentForce, slopeAdherenceA, slopeAdherenceB, linearSpeed,
      angularSpeed, visualSteerYaw, hasGroundMaterial != 0, *materialVals,
      outSlipFlag, outSurfaceFeedback,
  };
  const CSceneVehicleCarTuning &tuning = *Tunings()->ActiveTuning();
  CaptureBurnoutReferenceFrame();

  Model6ForceState state;
  state.frameY = gearedDrive.frameIso.rotation.basisY.y;
  state.waterActive = ApplyWaterForces(currentForce);
  controls.noGroundFrictionGuard = state.waterActive != 0;
  state.dirtSlideSurface = AllWheelsContactMaterial(DirtSlideMaterial) != 0;
  if (HandleModel6CircularBurnout(request, tuning, state)) {
    return;
  }

  state.tick = CMwCmdBufferCore::Current()->Timer().GetTickTime();
  state.slipSeen = AdvanceBurnoutPhases(&tuning, state.tick);
  ApplyModel6ContactForces(request, tuning, state);
  if (!request.hasGroundMaterial) {
    if (gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_TimedSpin) {
      gearedDrive.burnoutExitStartTick = state.tick;
      gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_ExitFade;
    }
    slipMemory.active = state.slipSeen != 0;
    gearedDrive.localSpeed = linearSpeed;
    return;
  }
  ApplyModel6GroundForces(request, tuning, state);
}
