// Engine input, transmission state, steering integration, and wheel visuals.

#include "engine/scene/scene_vehicle_car_internal.h"
using namespace SceneVehicleCarDynamics;

int CSceneVehicleCar::AreAllWheelsAirborne() {
  u32 wheelCount = WheelGetCount();
  for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
    CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
    if (wheel->realTimeState.contactPresent) {
      return 0;
    }
  }
  return 1;
}

void CSceneVehicleCar::ClampEngineInput() {
  float memory = engine.engineInputMemory;
  float maxInput = engine.engineInputMax;
  float clamped = 0.0f;
  if (!(memory <= 0.0f)) {
    clamped = memory;
    if (!(maxInput > memory)) {
      clamped = maxInput;
    }
  }
  engine.engineInputMemory = clamped;
}

float CSceneVehicleCar::TransmissionWeightedSpeed() {
  float speedSq = gearedDrive.localSpeed.z * gearedDrive.localSpeed.z +
                  LegacyEngineWeightedSpeedXScale * gearedDrive.localSpeed.x *
                      gearedDrive.localSpeed.x +
                  LegacyEngineWeightedSpeedYScale * gearedDrive.localSpeed.y *
                      gearedDrive.localSpeed.y;
  return CIsqrt(speedSq);
}

void CSceneVehicleCar::IntegrateLegacyEngine(CSceneVehicleCarTuning *tuning,
                                             float input, float dt,
                                             int blocked) {
  input = std::fabs(input);

  int groundReady = (!(blocked) && !(0.0f < engine.shiftCooldown));
  const int storedGear = engine.gearIndex;
  int gear = storedGear < 2 ? 0 : storedGear - 1;

  float speed = TransmissionWeightedSpeed();
  float targetInput =
      (speed / (tuning->engineSpeedNorm * LegacyEngineSpeedNormScale)) *
      tuning->GearRatio(gear);

  if (groundReady) {
    input = targetInput;
    if (!engine.useLowSpeedGateB) {
      if (gear == 0) {
        engine.gearIndex = 1;
        engine.shiftCooldown = LegacyEngineGearChangeCooldown;
      } else if (tuning->UpshiftThreshold(gear) < targetInput && gear < 5) {
        engine.gearIndex = gear + 1;
        engine.shiftCooldown = LegacyEngineGearChangeCooldown;
      } else if (targetInput < tuning->DownshiftThreshold(gear) && gear > 1) {
        engine.gearIndex = gear - 1;
        engine.shiftCooldown = LegacyEngineGearChangeCooldown;
      }
    } else if (gear != 0) {
      engine.gearIndex = 0;
      engine.shiftCooldown = LegacyEngineGearChangeCooldown;
    }
  } else if (!(engine.shiftCooldown < 0.0f) &&
             !(dt + dt < engine.shiftCooldown)) {
    engine.engineInputMemory -=
        engine.engineInputMax * dt * LegacyEngineCooldownDecayScale;
  }

  float rate =
      groundReady ? LegacyEngineGroundInputRate : LegacyEngineAirborneInputRate;
  engine.engineInputMemory +=
      (engine.engineInputMax * input - engine.engineInputMemory) * dt * rate;
}

void CSceneVehicleCar::UpdateDirectionalTransitionInput(
    CSceneVehicleCarTuning *tuning, float dt, int inputActive,
    CSceneVehicleCarEngineControlState state) {
  int index =
      state == CSceneVehicleCarEngineControlState_ForwardTransition ? 1 : 0;
  float speedZ = gearedDrive.localSpeed.z;
  int outside;
  if (state == CSceneVehicleCarEngineControlState_ForwardTransition) {
    outside = tuning->gearedDrive.input.forwardTransitionSpeedHigh < speedZ ||
              speedZ < tuning->gearedDrive.input.forwardTransitionSpeedLow;
  } else {
    outside = speedZ < tuning->gearedDrive.input.reverseTransitionSpeedLow ||
              tuning->gearedDrive.input.reverseTransitionSpeedHigh < speedZ;
  }

  gearedDrive.inputWindowExceeded = outside != 0;
  engine.slipRpmScale = 1.0f;
  float absSpeedZ = std::fabs(speedZ);
  engine.targetTransmissionInput = (absSpeedZ * tuning->GearRatio(index) +
                                    tuning->TargetInputBias(index) * 0.0f);

  if (outside || !inputActive) {
    float nextMemory = (engine.engineInputMemory -
                        tuning->gearedDrive.input.groundInputFall * dt);
    engine.engineInputMemory = nextMemory;
    if (nextMemory <= engine.targetTransmissionInput) {
      engine.engineInputMemory = engine.targetTransmissionInput;
      gearedDrive.engineState = CSceneVehicleCarEngineControlState_Steady;
      gearedDrive.inputWindowExceeded = 0;
    }
  } else {
    engine.engineInputMemory =
        (tuning->gearedDrive.input.transitionInputRise * dt +
         engine.engineInputMemory);
  }
}

void CSceneVehicleCar::UpdateTransmissionTransitions(
    CSceneVehicleCarTuning *tuning, int inputActive, int burnoutState) {
  float speedZ = gearedDrive.localSpeed.z;
  if (tuning->gearedDrive.input.forwardTransitionSpeedLow < speedZ &&
      speedZ < tuning->gearedDrive.input.forwardTransitionSpeedHigh &&
      inputActive && !engine.useLowSpeedGateB &&
      gearedDrive.engineState == CSceneVehicleCarEngineControlState_Steady) {
    gearedDrive.engineState =
        CSceneVehicleCarEngineControlState_ForwardTransition;
    gearedDrive.inputWindowExceeded = 0;
    if (engine.gearIndex == 0) {
      engine.shiftCooldown = GearShiftCooldown;
      engine.gearIndex = 1;
    }
  } else if (tuning->gearedDrive.input.reverseTransitionSpeedLow < speedZ &&
             speedZ < tuning->gearedDrive.input.reverseTransitionSpeedHigh &&
             inputActive && engine.useLowSpeedGateB &&
             gearedDrive.engineState ==
                 CSceneVehicleCarEngineControlState_Steady) {
    gearedDrive.engineState =
        CSceneVehicleCarEngineControlState_ReverseTransition;
    gearedDrive.inputWindowExceeded = 0;
    if (engine.gearIndex != 0) {
      engine.gearIndex = 0;
      engine.shiftCooldown = GearShiftCooldown;
    }
  }

  if (gearedDrive.engineState != CSceneVehicleCarEngineControlState_Steady &&
      gearedDrive.engineState != CSceneVehicleCarEngineControlState_GearShift) {
    return;
  }

  if (!engine.useLowSpeedGateB) {
    if (engine.gearIndex == 0) {
      gearedDrive.engineState = CSceneVehicleCarEngineControlState_GearShift;
      gearedDrive.shiftDirection = CSceneVehicleCarShiftDirection_Up;
      if (engine.engineInputMemory < GearShiftInputMemoryThreshold) {
        engine.shiftCooldown = GearShiftCooldown;
        engine.gearIndex = 1;
      }
    }

    int gear = engine.gearIndex;
    if (gear > 0) {
      float upshift = tuning->UpshiftThreshold(gear) * engine.engineInputMax;
      if (upshift < engine.targetTransmissionInput && gear < 5) {
        engine.shiftCooldown = GearShiftCooldown;
        engine.gearIndex = gear + 1;
        gearedDrive.engineState = CSceneVehicleCarEngineControlState_GearShift;
        gearedDrive.shiftDirection = CSceneVehicleCarShiftDirection_Up;
        return;
      }

      float downshift =
          tuning->DownshiftThreshold(gear) * engine.engineInputMax;
      if (engine.targetTransmissionInput < downshift && gear > 1) {
        engine.shiftCooldown = GearShiftCooldown;
        engine.gearIndex = gear - 1;
        gearedDrive.engineState = CSceneVehicleCarEngineControlState_GearShift;
        gearedDrive.shiftDirection = CSceneVehicleCarShiftDirection_Down;
      }
    }
  } else if (engine.gearIndex != 0) {
    gearedDrive.engineState = CSceneVehicleCarEngineControlState_GearShift;
    gearedDrive.shiftDirection = CSceneVehicleCarShiftDirection_Down;
    if (engine.engineInputMemory < GearShiftInputMemoryThreshold) {
      engine.gearIndex = 0;
      engine.shiftCooldown =
          burnoutState ? BurnoutReverseGearShiftCooldown : GearShiftCooldown;
    }
  }
}

void CSceneVehicleCar::IntegrateGearedEngine(CSceneVehicleCarTuning *tuning,
                                             float dt, int inputActive,
                                             int blocked) {
  if (blocked) {
    if (inputActive) {
      engine.engineInputMemory =
          (tuning->gearedDrive.input.airborneInputRise * dt +
           engine.engineInputMemory);
    } else {
      engine.engineInputMemory =
          (engine.engineInputMemory -
           tuning->gearedDrive.input.airborneInputFall * dt);
    }
    return;
  }

  int burnoutState =
      gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_TimedSpin ||
      gearedDrive.burnoutPhase == CSceneVehicleCarBurnoutPhase_CircularDrift;
  if (burnoutState && engine.gearIndex != 0) {
    gearedDrive.engineState = CSceneVehicleCarEngineControlState_BurnoutHold;
  } else if (!burnoutState &&
             gearedDrive.engineState ==
                 CSceneVehicleCarEngineControlState_BurnoutHold) {
    gearedDrive.engineState = CSceneVehicleCarEngineControlState_Steady;
  }

  switch (gearedDrive.engineState) {
  case CSceneVehicleCarEngineControlState_ForwardTransition:
  case CSceneVehicleCarEngineControlState_ReverseTransition:
    UpdateDirectionalTransitionInput(tuning, dt, inputActive,
                                     gearedDrive.engineState);
    break;
  case CSceneVehicleCarEngineControlState_BurnoutHold:
    engine.targetTransmissionInput = engine.engineInputMax;
    engine.slipRpmScale = SlipRpmScaleMax;
    if (engine.engineInputMemory < engine.engineInputMax) {
      engine.engineInputMemory =
          (tuning->gearedDrive.input.burnoutHoldInputRise * dt +
           engine.engineInputMemory);
    } else if (engine.engineInputMax < engine.engineInputMemory) {
      engine.engineInputMemory =
          (tuning->gearedDrive.input.airborneInputFall * dt +
           engine.engineInputMemory);
    }
    break;
  default: {
    if (slipMemory.active && inputActive) {
      if (engine.slipRpmScale < SlipRpmScaleMax) {
        engine.slipRpmScale = (SlipRpmScaleMax - engine.slipRpmScale) *
                                  SlipRpmScaleApproachRate * dt +
                              engine.slipRpmScale;
      } else {
        engine.slipRpmScale = SlipRpmScaleMax;
      }
    } else {
      engine.slipRpmScale = 1.0f;
    }

    int gear = engine.gearIndex;
    float slipSpeed = gearedDrive.localSpeed.z * engine.slipRpmScale;
    float absSlipSpeed = std::fabs(slipSpeed);
    engine.targetTransmissionInput = absSlipSpeed * tuning->GearRatio(gear);
    if ((!engine.useLowSpeedGateB && gear == 0) ||
        (engine.useLowSpeedGateB && gear != 0)) {
      engine.targetTransmissionInput = 0.0f;
    }

    if (!(engine.engineInputMemory < engine.targetTransmissionInput)) {
      float fall;
      if (inputActive) {
        if (!slipMemory.active || burnoutState) {
          fall = tuning->gearedDrive.input.groundInputBrake;
        } else {
          fall = tuning->gearedDrive.input.airborneInputFall;
        }
      } else {
        fall = tuning->gearedDrive.input.groundInputFall;
      }
      float nextMemory = engine.engineInputMemory - fall * dt;
      engine.engineInputMemory = nextMemory;
      if (nextMemory < engine.targetTransmissionInput) {
        gearedDrive.engineState = CSceneVehicleCarEngineControlState_Steady;
      }
    } else {
      float nextMemory = (tuning->gearedDrive.input.groundInputRise * dt +
                          engine.engineInputMemory);
      engine.engineInputMemory = nextMemory;
      if (engine.targetTransmissionInput < nextMemory) {
        gearedDrive.engineState = CSceneVehicleCarEngineControlState_Steady;
      }
    }
    break;
  }
  }

  UpdateTransmissionTransitions(tuning, inputActive, burnoutState);
}

void CSceneVehicleCar::EngineIntegrate(float input, float dt) {
  int inputActive = input > EngineInputActiveThreshold;
  int allAirborne = AreAllWheelsAirborne();

  if (0.0f < engine.shiftCooldown) {
    engine.shiftCooldown = engine.shiftCooldown - dt;
  }

  int blocked = allAirborne || (0.0f < engine.shiftCooldown);
  CSceneVehicleCarTuning *tuning = Tunings()->ActiveTuning();

  if (tuning->handlingModel != CSceneVehicleCarHandlingModel_GearedDrive) {
    IntegrateLegacyEngine(tuning, input, dt, blocked);
  } else {
    IntegrateGearedEngine(tuning, dt, inputActive, blocked);
  }

  ClampEngineInput();
}

void CSceneVehicleCar::UpdateWheelVisualState(
    CSceneVehicleCar::SSimulationWheel &wheel, CSceneVehicleCarTuning *tuning,
    float vehicleForwardSpeed, float dt, float visualSpeedDenominator) {
  wheel.realTimeState.visualRotation.Set(wheel.surfaceHandler.RestRotation());

  float wheelVisualSteerAngle = 0.0f;
  if (IsFrontVehicleWheel(wheel.axle)) {
    float yaw = 0.0f;
    if (!(visualSpeedDenominator < 1.0e-5f)) {
      yaw = -controls.currentSteering / visualSpeedDenominator;
    }
    wheel.realTimeState.visualRotation.RotateY(yaw);

    float maxSteerDegrees = WheelVisualDefaultMaxSteerDegrees;
    if (tuning->visual.wheelSteerAngleFromSpeedCurve.IsBound()) {
      unsigned long keyIndex = 0ul;
      float curveInput = (std::fabs(vehicleForwardSpeed) *
                          SceneVehicleMath::SpeedKilometersPerHourScale);
      tuning->visual.wheelSteerAngleFromSpeedCurve.Value().GetValue(
          curveInput, maxSteerDegrees, keyIndex);
    }

    float maxSteerRadians =
        (maxSteerDegrees * SceneVehicleMath::Pi) / DegreesToRadiansDivisor;
    wheelVisualSteerAngle = -controls.currentSteering * maxSteerRadians;
  }

  wheel.realTimeState.targetVisualSteerAngle = wheelVisualSteerAngle;
  WheelUpdateSpeedFromVehicleSpeed(wheel, vehicleForwardSpeed, dt);
  wheel.realTimeState.Integrate(dt);
}

void CSceneVehicleCar::UpdateCurrentSteering(CSceneVehicleCarTuning *tuning,
                                             float dt) {
  if (tuning->steering.slewRate <= 0.0f) {
    controls.currentSteering = controls.steeringControl;
    return;
  }

  float direction = -1.0f;
  if (controls.currentSteering - controls.steeringControl < 0.0f) {
    direction = 1.0f;
  }

  float candidate =
      tuning->steering.slewRate * direction * dt + controls.currentSteering;
  float next = candidate;
  if (controls.steeringControl <= controls.currentSteering) {
    if (controls.steeringControl > candidate) {
      next = controls.steeringControl;
    }
  } else if (controls.steeringControl < candidate) {
    next = controls.steeringControl;
  }

  controls.currentSteering = next;
}

void CSceneVehicleCar::IntegrateVehicle(float dt) {
  GmVec3 linearSpeed;
  HmsItem()->GetLinearSpeed(linearSpeed);
  const float vehicleForwardSpeed = linearSpeed.z;

  CSceneVehicleCarTuning *tuning = Tunings()->ActiveTuning();

  if (integration.updateWheelVisuals) {
    float visualSpeedDenominator =
        (std::fabs(vehicleForwardSpeed) * tuning->visual.wheelSpeedScale +
         tuning->visual.wheelSpeedBase);

    u32 wheelCount = WheelGetCount();
    for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
      CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
      UpdateWheelVisualState(*wheel, tuning, vehicleForwardSpeed, dt,
                             visualSpeedDenominator);
    }
  }

  if (integration.integrateWheels) {
    u32 wheelCount = WheelGetCount();
    for (u32 wheelIndex = 0; wheelIndex < wheelCount; wheelIndex++) {
      CSceneVehicleCar::SSimulationWheel *wheel = &WheelAt(wheelIndex);
      WheelIntegrate(*wheel, dt);
    }
  }

  if (integration.integrateEngine) {
    if (controls.forcedLowSpeedFriction == 0) {
      float input = (!engine.useLowSpeedGateB ? controls.lowSpeedGateA
                                              : controls.lowSpeedGateB);
      EngineIntegrate(input, dt);
    } else {
      engine.engineInputMemory = 0.0f;
    }
  }

  UpdateCurrentSteering(tuning, dt);
  RefreshCollisionTree();
}
