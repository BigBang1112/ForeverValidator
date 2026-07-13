#pragma once

#include <functional>
#include <optional>

#include "engine/scene/scene_vehicle_car_tuning_types.h"
class CSceneVehicleCar;

class CSceneVehicleCarTuning : public CSceneVehicleTuning {
public:
  float engineSpeedNorm;
  float lowSpeedFrictionMagnitude;
  float lowSpeedLinearDamping;
  CSceneVehicleTuningCurve lateralContactSlowDownFromSpeedCurve;
  CSceneVehicleTuningCurve maxSideFrictionFromSpeedCurve;
  CSceneVehicleTuningCurve rolloverLateralFromSpeedCurve;
  CSceneVehicleTuningCurve rolloverLateralCoefFromAngleCurve;
  CSceneVehicleCarWheelForceMode wheelForceMode;
  CSceneVehicleCarHandlingModel handlingModel;
  CSceneVehicleCarTuningVisualSettings visual;
  CSceneVehicleCarTuningSteering steering;
  CSceneVehicleCarTuningSuspension suspension;
  CSceneVehicleCarTuningContactResponse contactResponse;
  CSceneVehicleCarTuningBodyAirResponse bodyAirResponse;
  CSceneVehicleCarRadiusSteeringTuning radiusSteering;
  CSceneVehicleCarSlipResponseTuning slipResponse;
  CSceneVehicleCarGearedDriveTuning gearedDrive;
  CSceneVehicleCarTuningWater water;
  CSceneVehicleCarTuningTurbo turbo;
  CSceneVehicleCarTuningFeedback feedback;
  CSceneVehicleCarTuning(void);
  ~CSceneVehicleCarTuning(void) override;
  float GetMaxSideFrictionFromSpeed(float speed) const;
  float GetAccelFromSpeed(float speed) const;
  float GetRolloverLateralFromSpeed(float speed) const;
  float GetRolloverLateralCoefFromAngle(float angle) const;
  float GetSteerDriveTorqueFromSpeed(float speed) const;
  float GetLateralContactSlowDownFromSpeed(float speed) const;
  float GetSteerSlowDownFromSpeed(float speed) const;
  float GetWaterFrictionFromSpeed(float speed) const;
  float M4GetSteerRadiusFromSpeed(float speed) const;
  float M4GetMaxFrictionForceFromSpeed(float speed) const;
  float M5GetAccelFromSpeed(float speed) const;
  float M5GetSlippingAccelFromSpeed(float speed) const;
  float M5GetSteerSlowDownFromSpeed(float speed) const;
  float M5GetLateralContactSlowDownFromSpeed(float speed) const;
  float M6GetModulationFromDamperAbsorbVal(float damperAbsorb) const;
  float M6GetRearGearAccelFromSpeed(float speed) const;
  float M6GetBurnoutRadiusFromSpeed(float speed) const;
  float M6GetLateralSpeedFromBurnoutRadius(float radius) const;
  float M6GetBurnoutRolloverFromSpeed(float speed) const;
  float M6GetDonutRolloverFromSpeed(float speed) const;
  float M6GetRolloverLateralFromSpeedRatio(float speedRatio) const;
  void M6InitRpmDelta(void);
  void M6CheckRpmWantedConstistensy(void);
  void OnNodLoaded(void) override;
  void ResetToDefaults(void);

private:
  friend class CSceneVehicleCar;

  float GearedSteerAssistRamp(const GmVec3 &linearSpeed) const;
  float GearRatio(int gear) const {
    return gearedDrive.transmission.gearSpeedRatio[static_cast<uint32_t>(gear)];
  }
  float UpshiftThreshold(int gear) const {
    return gearedDrive.transmission.upshiftThreshold[static_cast<uint32_t>(gear)];
  }
  float DownshiftThreshold(int gear) const {
    return gearedDrive.transmission
        .downshiftThreshold[static_cast<uint32_t>(gear)];
  }
  float TargetInputBias(int gear) const {
    return gearedDrive.transmission.targetInputBias[static_cast<uint32_t>(gear)];
  }
  float EvaluateCurve(const CSceneVehicleTuningCurve &curve, float x) const;
  float EvaluateSpeedCurve(const CSceneVehicleTuningCurve &curve,
                           float speed) const;
  float EvaluateLinearSpeedCurve(const CSceneVehicleTuningCurve &curve,
                                 float speed) const;
};
class CSceneVehicleTunings : public CMwNod {
public:
  CSceneVehicleTunings(void);
  ~CSceneVehicleTunings(void) override;
  void SetActiveTuning(CSceneVehicleCarTuning &activeTuning);
  CSceneVehicleCarTuning *ActiveTuning(void) const;

private:
  std::optional<std::reference_wrapper<CSceneVehicleCarTuning>> activeTuning_;
};
