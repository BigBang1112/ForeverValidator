#pragma once

#include <cstdint>

enum CSceneVehicleCarEngineControlState : std::uint8_t {
  CSceneVehicleCarEngineControlState_Steady = 0,
  CSceneVehicleCarEngineControlState_GearShift = 1,
  CSceneVehicleCarEngineControlState_ForwardTransition = 2,
  CSceneVehicleCarEngineControlState_ReverseTransition = 3,
  CSceneVehicleCarEngineControlState_BurnoutHold = 4,
};

enum CSceneVehicleCarShiftDirection : std::uint8_t {
  CSceneVehicleCarShiftDirection_Up = 0,
  CSceneVehicleCarShiftDirection_Down = 1,
};

enum CSceneVehicleCarRadiusSteeringPhase : std::uint8_t {
  CSceneVehicleCarRadiusSteeringPhase_Idle = 0,
  CSceneVehicleCarRadiusSteeringPhase_DirectInput = 1,
  CSceneVehicleCarRadiusSteeringPhase_CapturedSlipAngle = 2,
};

enum CSceneVehicleCarImpactState : std::uint8_t {
  CSceneVehicleCarImpactState_None = 0,
  CSceneVehicleCarImpactState_Low = 1,
  CSceneVehicleCarImpactState_High = 2,
};

enum CSceneVehicleCarBurnoutPhase : std::uint8_t {
  CSceneVehicleCarBurnoutPhase_Inactive = 0,
  CSceneVehicleCarBurnoutPhase_TimedSpin = 1,
  CSceneVehicleCarBurnoutPhase_CircularDrift = 2,
  CSceneVehicleCarBurnoutPhase_ExitFade = 3,
};

enum CSceneVehicleCarSpecialContactMode : std::uint8_t {
  CSceneVehicleCarSpecialContactMode_None = 0,
  CSceneVehicleCarSpecialContactMode_ImpulseFromForce = 1,
  CSceneVehicleCarSpecialContactMode_SolidFeedback = 2,
  CSceneVehicleCarSpecialContactMode_Turbo = 3,
};
