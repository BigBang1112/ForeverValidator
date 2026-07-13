// Geared-drive handling, slip memory, and burnout dynamics.

#include "engine/scene/scene_vehicle_car_internal.h"
using namespace SceneVehicleCarDynamics;

CSceneVehicleMaterial *CSceneVehicleCar::GetWheelMaterial(
    const CSceneVehicleCar::SSimulationWheel &wheel) {
  u32 remappedMaterialIndex =
      MaterialRemapAt(wheel.realTimeState.contactMaterial);
  return MaterialContainer()->MaterialAt(remappedMaterialIndex);
}

int CSceneVehicleCar::AllWheelsContactMaterial(
    EPlugSurfaceMaterialId materialId) {
  u32 wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
    if (!wheel->realTimeState.contactPresent ||
        wheel->realTimeState.contactMaterial != materialId) {
      return 0;
    }
  }
  return 1;
}

void CSceneVehicleCar::CaptureBurnoutReferenceFrame() {
  CHmsCorpus *corpus = HmsItem()->CorpusAt(0u);
  gearedDrive.frameIso = *corpus->GetLocation();
}

int CSceneVehicleCar::AdvanceBurnoutPhases(
    const CSceneVehicleCarTuning *tuning, u32 tick) {
  int stateSlipSeen = 0;

  if (gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_TimedSpin) {
    if (tick < gearedDrive.burnoutStartTick ||
        tick - gearedDrive.burnoutStartTick >=
            tuning->gearedDrive.burnout.durationTicks) {
      gearedDrive.burnoutExitStartTick = tick;
      gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_ExitFade;
    } else {
      stateSlipSeen = 1;
    }
  }

  if (gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_ExitFade) {
    if (tick < gearedDrive.burnoutExitStartTick ||
        tick - gearedDrive.burnoutExitStartTick >=
            tuning->gearedDrive.burnout.exitDurationTicks) {
      gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_Inactive;
      gearedDrive.wheelSpeedOverrideActive = 0;
      return stateSlipSeen;
    }

    u32 wheelCount = WheelGetCount();
    for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
      CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
      wheel->realTimeState.slipping = true;
    }
  }
  return stateSlipSeen;
}

int CSceneVehicleCar::CanApplyDirtSlideForces() {
  const float gate = LowSpeedGateThreshold;
  return controls.lowSpeedGateB < gate || controls.lowSpeedGateB == gate;
}

CSceneVehicleCar::DirtSlideForces CSceneVehicleCar::BuildDirtSlideForces(
    const CSceneVehicleCarTuning *tuning, const GmVec3 &linearSpeed,
    const GmVec3 &unitSpeed) const {
  DirtSlideForces forces;
  GmVec3 &frontSlideForce = forces.front;
  GmVec3 &rearSlideForce = forces.rear;
  // Dirt gives the front wheels a stronger forward slide response.
  float absUnitX = std::fabs(unitSpeed.x);
  float absZ = std::fabs(linearSpeed.z);
  float absXGate = absUnitX * DirtSlideAbsXScale + 1.0f;
  float denomBase = absZ + 1.0f;
  float denom = denomBase * denomBase;
  float sideDenom =
      tuning->gearedDrive.dirtSlideGateScale * controls.lowSpeedGateA + 1.0f;

  frontSlideForce.x =
      (tuning->gearedDrive.dirtSlideSideForceScale * unitSpeed.x) / sideDenom;
  frontSlideForce.y = 0.0f;
  frontSlideForce.z =
      (tuning->gearedDrive.dirtSlideForwardGateScale * controls.lowSpeedGateA *
       DirtSlideFrontRearRatio * absUnitX * absXGate *
       tuning->gearedDrive.dirtSlideForwardForceScale / denom);

  rearSlideForce.x = -frontSlideForce.x;
  rearSlideForce.y = 0.0f;
  rearSlideForce.z = (tuning->gearedDrive.dirtSlideForwardGateScale *
                      controls.lowSpeedGateA * absUnitX * absXGate *
                      tuning->gearedDrive.dirtSlideForwardForceScale / denom);
  return forces;
}

float CSceneVehicleCar::BurnoutPhase(u32 elapsedTicks, u32 durationTicks) {
  float scaledElapsed =
      (Binary32::FromUnsignedInteger(elapsedTicks)) * SceneVehicleMath::Pi;
  return scaledElapsed / Binary32::FromUnsignedInteger(durationTicks);
}

float CSceneVehicleCar::BurnoutDriveFade(const CSceneVehicleCarTuning *tuning,
                                         u32 tick) {
  if (gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_TimedSpin) {
    float phase = CSceneVehicleCar::BurnoutPhase(
        tick - gearedDrive.burnoutStartTick,
        tuning->gearedDrive.burnout.durationTicks);
    float fade = CIsin(phase);
    return CSceneVehicleCar::BurnoutFadeResult(
        tuning->gearedDrive.burnout.driveFadeScale, fade);
  }

  if (gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_ExitFade) {
    float phase = CSceneVehicleCar::BurnoutPhase(
        tick - gearedDrive.burnoutExitStartTick,
        tuning->gearedDrive.burnout.exitDurationTicks);
    float fade = CIsin(phase);
    return CSceneVehicleCar::BurnoutFadeResult(
        tuning->gearedDrive.burnout.exitAccelFadeScale, fade);
  }

  return 1.0f;
}

float CSceneVehicleCar::BurnoutSideForceFade(const CSceneVehicleCarTuning *tuning,
                                             u32 tick) {
  if (gearedDrive.burnoutPhase != CSceneVehicleCarBurnoutPhase_TimedSpin) {
    return 1.0f;
  }

  float phase = CSceneVehicleCar::BurnoutPhase(
      tick - gearedDrive.burnoutStartTick,
      tuning->gearedDrive.burnout.durationTicks * 2);
  float fade = CIcos(phase);
  return CSceneVehicleCar::BurnoutFadeResult(
      tuning->gearedDrive.burnout.sideForceFadeScale, fade);
}

float CSceneVehicleCar::BurnoutExitAcceleration(const CSceneVehicleCarTuning *tuning,
                                                u32 tick) {
  if (gearedDrive.burnoutPhase != CSceneVehicleCarBurnoutPhase_ExitFade) {
    return 0.0f;
  }

  u32 elapsedQuotient = (tick - gearedDrive.burnoutExitStartTick) /
                        tuning->gearedDrive.burnout.exitDurationTicks;
  float delta = Binary32::FromUnsignedInteger(elapsedQuotient) - 1.0f;
  float deltaSq = delta * delta;
  return deltaSq * tuning->gearedDrive.burnout.exitBonusAccelScale;
}

void CSceneVehicleCar::UpdateGearDirection(const GmVec3 &linearSpeed) {
  const float gate = LowSpeedGateThreshold;
  if (gearedDrive.burnoutPhase != CSceneVehicleCarBurnoutPhase_Inactive) {
    engine.useLowSpeedGateB = false;
    return;
  }

  if (controls.lowSpeedGateB > gate &&
      linearSpeed.z < reverseGearSpeedThreshold &&
      std::fabs(linearSpeed.x) < 2.0f) {
    engine.useLowSpeedGateB = true;
  }
  if (controls.lowSpeedGateA > gate &&
      (linearSpeed.z > 0.0f || std::fabs(linearSpeed.x) > 2.0f)) {
    engine.useLowSpeedGateB = false;
  }
  if (controls.lowSpeedGateA < gate && controls.lowSpeedGateB < gate) {
    if (linearSpeed.z > 0.0f || std::fabs(linearSpeed.z) < 2.0f) {
      engine.useLowSpeedGateB = false;
    } else {
      engine.useLowSpeedGateB = true;
    }
  }
  if (linearSpeed.z > 0.0f && turbo.type != ETurboType_Inactive) {
    engine.useLowSpeedGateB = false;
  }
}

GmVec3 CSceneVehicleCar::BuildBurnoutRadiusSeed(const GmVec3 &bodyCenter) {
  float zero = 0.0f;
  float zExtra = (WaterState().boxLocal.halfExtents.y * zero) +
                 (WaterState().boxLocal.halfExtents.x * zero);
  zExtra += WaterState().boxLocal.halfExtents.z;

  return GmVec3{
      WaterState().boxLocal.center.x - bodyCenter.x,
      WaterState().boxLocal.center.y - bodyCenter.y,
      WaterState().boxLocal.center.z - bodyCenter.z + zExtra,
  };
}

void CSceneVehicleCar::EnterCircularBurnout(const CSceneVehicleCarTuning *tuning,
                                            const GmVec3 &linearSpeed,
                                            float visualSteerYaw) {
  gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_CircularDrift;
  gearedDrive.burnoutDirection = SignNonNegative(visualSteerYaw);

  GmVec3 normalSum = SceneVehicleMath::Zero();
  u32 wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
    if (wheel->realTimeState.contactPresent) {
      normalSum = SceneVehicleMath::Add(
          normalSum, wheel->realTimeState.accumulatedContactNormal);
    }
  }
  float normalSumLenSq = SceneVehicleMath::LengthSquared(normalSum);
  if (VectorEpsilonSquared < normalSumLenSq) {
    gearedDrive.burnoutContactNormal =
        SceneVehicleMath::Scale(normalSum, 1.0f / CIsqrt(normalSumLenSq));
  } else {
    gearedDrive.burnoutContactNormal = GmVec3{0.0f, 1.0f, 0.0f};
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_Inactive;
  }

  GmVec3 bodyCenter =
      HmsItem()->Solid()->Physical().Parameters().localCenterOfMass;
  GmVec3 radiusSeed = BuildBurnoutRadiusSeed(bodyCenter);
  gearedDrive.burnoutBaseRadius =
      CIsqrt(SceneVehicleMath::LengthSquared(radiusSeed));

  GmVec3 tangent = SceneVehicleMath::Scale(
      SceneVehicleMath::Cross(gearedDrive.burnoutContactNormal, linearSpeed),
      gearedDrive.burnoutDirection);
  tangent =
      SceneVehicleMath::NormalizeIfLongEnough(tangent, VectorEpsilonSquared);

  gearedDrive.burnoutContactNormal = TransformDirection(
      gearedDrive.frameIso, gearedDrive.burnoutContactNormal);
  if (gearedDrive.burnoutContactNormal.y < 0.75f) {
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_Inactive;
    gearedDrive.burnoutContactNormal = GmVec3{0.0f, 1.0f, 0.0f};
  }

  GmVec3 forwardAxis = {0.0f, 0.0f, 1.0f};
  float signedAngle = SceneVehicleMath::Angle(forwardAxis, tangent) *
                      SignNonNegative(visualSteerYaw);
  if (signedAngle > tuning->gearedDrive.burnout.angleLimitPositive ||
      signedAngle < -tuning->gearedDrive.burnout.angleLimitNegative) {
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_Inactive;
  } else {
    GmVec3 tangentWorld = TransformDirection(gearedDrive.frameIso, tangent);
    GmVec3 bodyCenterWorld = TransformPoint(gearedDrive.frameIso, bodyCenter);
    float targetRadius = tuning->M6GetBurnoutRadiusFromSpeed(linearSpeed.z) +
                         gearedDrive.burnoutBaseRadius;
    gearedDrive.burnoutTargetRadius = targetRadius;
    gearedDrive.burnoutCenter = SceneVehicleMath::Add(
        bodyCenterWorld, SceneVehicleMath::Scale(tangentWorld, targetRadius));
  }

  gearedDrive.wheelSpeedOverrideActive =
      gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_CircularDrift
          ? 1
          : 0;
}

void CSceneVehicleCar::TryEnterForwardBurnout(const CSceneVehicleCarTuning *tuning,
                                              const GmVec3 &linearSpeed,
                                              float visualSteerYaw,
                                              float frameY, u32 tick,
                                              int waterActive,
                                              int hasGroundMaterial) {
  const float gate = LowSpeedGateThreshold;
  if (controls.forcedLowSpeedFriction != 0 || waterActive != 0 ||
      hasGroundMaterial == 0) {
    return;
  }
  if (!(controls.lowSpeedGateA > gate && controls.lowSpeedGateB > gate)) {
    return;
  }
  if (linearSpeed.z < tuning->gearedDrive.burnout.donutSpeedLow &&
      frameY > 0.75f) {
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_TimedSpin;
    gearedDrive.wheelSpeedOverrideActive = 1;
    gearedDrive.burnoutStartTick = tick;
  }
  if (linearSpeed.z < tuning->gearedDrive.burnout.donutSpeedHigh &&
      linearSpeed.z > tuning->gearedDrive.burnout.donutSpeedLow &&
      !(std::fabs(visualSteerYaw) < ScalarEpsilon)) {
    EnterCircularBurnout(tuning, linearSpeed, visualSteerYaw);
  }
}

void CSceneVehicleCar::TryEnterReverseBurnout(const CSceneVehicleCarTuning *tuning,
                                              const GmVec3 &linearSpeed,
                                              float drive, float frameY,
                                              u32 tick) {
  if (controls.forcedLowSpeedFriction != 0 || !(linearSpeed.z < 0.0f) ||
      !(controls.lowSpeedGateA > LowSpeedGateThreshold)) {
    return;
  }
  if (tuning->gearedDrive.burnout.reverseForceThreshold <
          -drive * tuning->feedback.forceDivisor * linearSpeed.z &&
      frameY > 0.75f) {
    gearedDrive.burnoutStartTick = tick;
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_TimedSpin;
    gearedDrive.wheelSpeedOverrideActive = 1;
  }
}

void CSceneVehicleCar::ApplyCircularBurnoutForces(
    const CSceneVehicleCarTuning *tuning, const GmVec3 &currentForce,
    const GmVec3 &linearSpeed, const GmVec3 &angularSpeed, float visualSteerYaw,
    int hasGroundMaterial, int waterActive) {
  if (gearedDrive.burnoutPhase != CSceneVehicleCarBurnoutPhase_CircularDrift) {
    return;
  }

  const float gate = LowSpeedGateThreshold;
  if (controls.lowSpeedGateA < gate || controls.lowSpeedGateB < gate ||
      std::fabs(visualSteerYaw) < ScalarEpsilon ||
      gearedDrive.burnoutDirection != SignNonNegative(visualSteerYaw) ||
      contacts.lateralSlowDownContactActive != 0 ||
      contacts.bodyContactPresent != 0 || hasGroundMaterial == 0 ||
      waterActive != 0 || controls.forcedLowSpeedFriction != 0) {
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_Inactive;
    return;
  }

  GmVec3 normalSum = SceneVehicleMath::Zero();
  u32 wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
    normalSum = SceneVehicleMath::Add(
        normalSum, wheel->realTimeState.accumulatedContactNormal);
  }
  GmVec3 averageNormal = SceneVehicleMath::NormalizeOr(
      normalSum, GmVec3{0.0f, 1.0f, 0.0f}, VectorEpsilonSquared);
  GmVec3 worldNormal = TransformDirection(gearedDrive.frameIso, averageNormal);
  float normalDrift = std::fabs(
      SceneVehicleMath::Angle(worldNormal, gearedDrive.burnoutContactNormal));
  if (normalDrift > tuning->gearedDrive.burnout.angleLimit) {
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_Inactive;
    return;
  }

  GmVec3 bodyCenter =
      HmsItem()->Solid()->Physical().Parameters().localCenterOfMass;
  GmVec3 localCenter =
      InverseTransformPoint(gearedDrive.frameIso, gearedDrive.burnoutCenter);
  GmVec3 radial = SceneVehicleMath::Subtract(localCenter, bodyCenter);
  float radiusSq =
      radial.y * radial.y + radial.x * radial.x + radial.z * radial.z;
  float radius = CIsqrt(radiusSq);
  GmVec3 radialDir = radial;
  if (radiusSq > VectorEpsilonSquared) {
    float invRadius = 1.0f / radius;
    radialDir = GmVec3{
        (radial.x * invRadius),
        (radial.y * invRadius),
        (radial.z * invRadius),
    };
  }
  GmVec3 tangent = SceneVehicleMath::Cross(averageNormal, radialDir);
  float tangentSpeed = SceneVehicleMath::Dot(linearSpeed, tangent);
  float radialSpeed = SceneVehicleMath::Dot(linearSpeed, radialDir);
  if (std::fabs(tangentSpeed) > tuning->gearedDrive.burnout.tangentSpeedMax) {
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_Inactive;
  }
  int radiusNeedsCorrection = radius > gearedDrive.burnoutBaseRadius ||
                              radius < tuning->gearedDrive.burnout.radiusMin;
  if (!radiusNeedsCorrection) {
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_Inactive;
  } else {
    float correctionExponent =
        (radius - gearedDrive.burnoutTargetRadius) *
            tuning->gearedDrive.burnout.radiusCorrectionScale -
        radialSpeed * tuning->gearedDrive.burnout.radiusCorrectionSpeedScale;
    double tangentSpeedWide = static_cast<double>(tangentSpeed);
    float expCorrection = CIexp(correctionExponent);
    float tangentSpeedSq = tangentSpeedWide * tangentSpeedWide;
    float correctionMagnitude = (tangentSpeedSq / radius) * expCorrection;
    GmVec3 correction = SceneVehicleMath::Scale(radialDir, correctionMagnitude);
    AddVehicleCentralForce(correction);
  }

  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
    WheelAddForceToVehicle(*wheel, currentForce);
    wheel->realTimeState.slipping = false;
  }

  float lateralSpeed = tuning->M6GetLateralSpeedFromBurnoutRadius(radius);
  float lateralScale =
      tuning->gearedDrive.burnout.lateralCorrectionScale *
      (-SignNonNegative(visualSteerYaw) * lateralSpeed - tangentSpeed);
  GmVec3 lateralForce = SceneVehicleMath::Scale(tangent, lateralScale);
  AddVehicleCentralForce(lateralForce);

  const double pi = static_cast<double>(SceneVehicleMath::Pi);
  GmVec3 forwardAxis = {0.0f, 0.0f, 1.0f};
  float angle = SceneVehicleMath::Angle(forwardAxis, radialDir);
  float angleNorm = angle / pi;
  float signedAngleNorm = gearedDrive.burnoutDirection * angleNorm;
  (void)signedAngleNorm;
  double signedAngle = static_cast<double>(gearedDrive.burnoutDirection) *
                       static_cast<double>(angleNorm) * static_cast<double>(pi);
  if (!std::isfinite(angleNorm) ||
      signedAngle < -static_cast<double>(
                        tuning->gearedDrive.burnout.angleLimitNegative) ||
      signedAngle >
          static_cast<double>(tuning->gearedDrive.burnout.angleLimitPositive)) {
    gearedDrive.burnoutPhase = CSceneVehicleCarBurnoutPhase_Inactive;
  } else {
    float angleReturnTorque = 0.0f;
    float angularY = angularSpeed.y;
    if (angleNorm <= 0.0f) {
      if (!(angularY > 0.0f)) {
        float delta = angleNorm + 1.0f;
        float deltaSq = delta * delta;
        angleReturnTorque = (-tuning->gearedDrive.burnout.angleReturnQuadratic *
                             angularY * deltaSq);
      } else {
        angleReturnTorque =
            -tuning->gearedDrive.burnout.angularDampingLinear * angularY;
      }
    } else if (!(angularY < 0.0f)) {
      float delta = angleNorm - 1.0f;
      float deltaSq = delta * delta;
      angleReturnTorque = (-tuning->gearedDrive.burnout.angleReturnQuadratic *
                           angularY * deltaSq);
    } else {
      angleReturnTorque =
          -tuning->gearedDrive.burnout.angularDampingLinear * angularY;
    }
    float tangentAngularSpeed = tangentSpeed / radius;
    float tangentDampingTorque = 0.0f;
    if ((angleNorm <= 0.0f && tangentAngularSpeed > 0.0f) ||
        (angleNorm > 0.0f && tangentAngularSpeed < 0.0f)) {
      tangentDampingTorque =
          (-tuning->gearedDrive.burnout.tangentAngularDamping *
           tangentAngularSpeed);
    }
    float circleTorqueY =
        (tuning->gearedDrive.burnout.angleTorqueScale * angleNorm +
         angleReturnTorque + tangentDampingTorque);
    GmVec3 circleTorque = {
        0.0f,
        circleTorqueY,
        0.0f,
    };
    AddVehicleTorque(circleTorque);
  }

  GmVec3 angleTorque = {
      0.0f, 0.0f,
      tuning->M6GetDonutRolloverFromSpeed(std::fabs(linearSpeed.x)) *
          -SignNonNegative(linearSpeed.x)};
  AddVehicleTorque(angleTorque);

  GmVec3 burnoutTorque = {
      tuning->M6GetBurnoutRolloverFromSpeed(linearSpeed.z),
      0.0f,
      0.0f,
  };
  AddVehicleTorque(burnoutTorque);
}

CSceneVehicleCar::GearedWheelSideForceResult
CSceneVehicleCar::ComputeGearedWheelSideForce(
    const CSceneVehicleCarTuning *tuning,
    CSceneVehicleCar::SSimulationWheel &wheel,
    const CSceneVehicleMaterial::SBlendableVals &materialVals,
    const GmVec3 &linearSpeed, const GmVec3 &angularSpeed, float steerRamp,
    float dt) {
  (void)dt;
  GearedWheelSideForceResult result;
  float signedHalfTrack =
      (IsFrontVehicleWheel(wheel.axle) ? gearedDrive.wheelLongitudinalSpan
                                       : -gearedDrive.wheelLongitudinalSpan) *
      0.5f;
  float wheelSideSpeed = linearSpeed.x + angularSpeed.y * signedHalfTrack;
  float maxStaticSideForce = static_cast<float>(
      tuning->GetMaxSideFrictionFromSpeed(linearSpeed.z) * materialVals.w);
  float requestedSideForce =
      -tuning->gearedDrive.lateralForceScale * 0.5f * wheelSideSpeed;

  if (std::fabs(requestedSideForce) > maxStaticSideForce) {
    float clippedSideForceMagnitude =
        (1.0f - tuning->gearedDrive.driveSideFrictionSlipBlend) *
            maxStaticSideForce +
        tuning->gearedDrive.driveSideFrictionSlipBlend *
            std::fabs(requestedSideForce);
    result.sideLimit = maxStaticSideForce;
    result.sideRequested = std::fabs(requestedSideForce);
    requestedSideForce =
        SignNonNegative(requestedSideForce) * clippedSideForceMagnitude;
    result.slipped = true;
  }

  float driveSideTorque =
      tuning->gearedDrive.sideForceToDriveTorqueScale * requestedSideForce;
  float steerAssistTorque = 0.0f;
  if (IsFrontVehicleWheel(wheel.axle)) {
    float steerTorque = tuning->GetSteerDriveTorqueFromSpeed(linearSpeed.z);
    steerAssistTorque = steerRamp * controls.currentSteering * steerTorque;
    if (engine.useLowSpeedGateB) {
      steerAssistTorque = -steerAssistTorque;
    }
    if (wheel.realTimeState.slipping) {
      steerAssistTorque =
          steerAssistTorque * tuning->gearedDrive.slippingSteerTorqueScale;
    }
    driveSideTorque = driveSideTorque - steerAssistTorque;
  }

  GmVec3 torque = {0.0f, (driveSideTorque * signedHalfTrack), 0.0f};
  AddVehicleTorque(torque);
  result.force = requestedSideForce;
  return result;
}

float CSceneVehicleCarTuning::GearedSteerAssistRamp(
    const GmVec3 &linearSpeed) const {
  float speed =
      CIsqrt(linearSpeed.x * linearSpeed.x + linearSpeed.y * linearSpeed.y +
             linearSpeed.z * linearSpeed.z);
  if (speed < 0.7f) {
    return 0.0f;
  }
  if (speed > steering.assistFullSpeed) {
    return 1.0f;
  }
  float angle80 = (speed / steering.assistFullSpeed) * SceneVehicleMath::HalfPi;
  float angle = angle80;
  return CIsin(angle);
}

void CSceneVehicleCar::UpdateSlipMemory(u32 tick, int slipSeen) {
  if (slipSeen == 0) {
    return;
  }

  slipMemory.lastTick = tick;
  if (!slipMemory.active) {
    slipMemory.startTick = tick;
  }
  slipMemory.elapsedTicks = tick - slipMemory.startTick;
}

float CSceneVehicleCar::ComputeSlipAccelerationBlend(
    const CSceneVehicleCarTuning *tuning, u32 tick, float accumulatedSideForceLimit,
    float accumulatedRequestedSideForce) {
  if (tick != slipMemory.lastTick ||
      !(accumulatedSideForceLimit > ScalarEpsilon)) {
    return 1.0f;
  }

  float slip = (accumulatedRequestedSideForce - accumulatedSideForceLimit) /
               accumulatedSideForceLimit / tuning->gearedDrive.slipRatioScale;
  return 1.0f - ClampZeroOne(slip);
}

float CSceneVehicleCar::ComputeSlippingWheelDriveScale(
    const CSceneVehicleCarTuning *tuning) {
  float penalty = 1.0f;
  u32 wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
    if (wheel->realTimeState.slipping) {
      penalty *= tuning->gearedDrive.perSlippingWheelAccelScale;
    }
  }
  return penalty;
}

void CSceneVehicleCar::MarkAllWheelsSlipping() {
  u32 wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
    wheel->realTimeState.slipping = true;
  }
}

CSceneVehicleCar::OpposingLongitudinalResult
CSceneVehicleCar::ComputeOpposingLongitudinalForce(
    const CSceneVehicleCarTuning *tuning,
    const CSceneVehicleMaterial::SBlendableVals &materialVals,
    const GmVec3 &linearSpeed, float driveForce, float frameY, u32 tick,
    int slipFlag) {
  OpposingLongitudinalResult result;
  float &opposingLongitudinalForce = result.force;
  if (linearSpeed.z > 0.0f) {
    float penalty = ComputeSlippingWheelDriveScale(tuning);
    opposingLongitudinalForce =
        (tuning->gearedDrive.forwardAccelBase +
         tuning->gearedDrive.forwardAccelSpeedCoef * linearSpeed.z) *
        controls.lowSpeedGateB * penalty;
    float cap = materialVals.z *
                (slipFlag != 0 ? tuning->gearedDrive.forwardAccelCapWhenSlipping
                               : tuning->gearedDrive.forwardAccelCap);
    if (cap < opposingLongitudinalForce) {
      opposingLongitudinalForce = cap;
      MarkAllWheelsSlipping();
      result.slipped = true;
    }
  } else if (linearSpeed.z < 0.0f &&
             controls.lowSpeedGateA > LowSpeedGateThreshold) {
    TryEnterReverseBurnout(tuning, linearSpeed, driveForce, frameY, tick);
    float penalty = ComputeSlippingWheelDriveScale(tuning);
    opposingLongitudinalForce =
        (tuning->gearedDrive.forwardAccelBase -
         tuning->gearedDrive.forwardAccelSpeedCoef * linearSpeed.z) *
        controls.lowSpeedGateA * penalty;
    float cap =
        materialVals.z *
        (slipFlag != 0 ? tuning->gearedDrive.forwardAccelCapWhenSlippingReverse
                       : tuning->gearedDrive.forwardAccelCapReverse);
    if (cap < opposingLongitudinalForce) {
      opposingLongitudinalForce = cap;
      MarkAllWheelsSlipping();
      result.slipped = true;
    }
  }
  return result;
}

float CSceneVehicleCar::ComputeGearedDriveForce(
    const CSceneVehicleCarTuning *tuning,
    const CSceneVehicleMaterial::SBlendableVals &materialVals,
    const GmVec3 &linearSpeed, u32 tick, int waterActive, float slipAccelMix) {
  float slippingAccelCurve = tuning->M5GetSlippingAccelFromSpeed(linearSpeed.z);
  float gearAccelCurve =
      engine.useLowSpeedGateB
          ? tuning->M6GetRearGearAccelFromSpeed(linearSpeed.z)
          : tuning->M5GetAccelFromSpeed(linearSpeed.z);
  float blendedAccelCurve =
      gearedDrive.engineState == CSceneVehicleCarEngineControlState_GearShift
          ? 0.0f
          : (1.0f - slipAccelMix) * slippingAccelCurve +
                gearAccelCurve * slipAccelMix;

  float turboDriveOverride = turbo.type != ETurboType_Inactive
                                 ? gearAccelCurve * turbo.impulseScale
                                 : 0.0f;
  float rearGearMaterialSign = engine.useLowSpeedGateB ? -1.0f : 0.0f;
  float steeringSlowdownForce =
      tuning->steering.slowDownScale * std::fabs(controls.currentSteering) *
      tuning->M5GetSteerSlowDownFromSpeed(linearSpeed.z) *
      (engine.useLowSpeedGateB ? -1.0f : 1.0f);
  float driveForce =
      BurnoutExitAcceleration(tuning, tick) +
      BurnoutDriveFade(tuning, tick) *
          (turboDriveOverride +
           (controls.lowSpeedGateA * materialVals.y +
            rearGearMaterialSign * materialVals.y * controls.lowSpeedGateB) *
               blendedAccelCurve) -
      steeringSlowdownForce;

  if (waterActive != 0) {
    driveForce *= 0.5f;
  }
  if (controls.forcedLowSpeedFriction != 0) {
    driveForce = turbo.type != ETurboType_Inactive
                     ? gearAccelCurve * turbo.impulseScale
                     : 0.0f;
  }
  return driveForce;
}
