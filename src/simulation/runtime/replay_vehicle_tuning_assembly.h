#ifndef TMNF_REPLAY_VEHICLE_TUNING_ASSEMBLY_H
#define TMNF_REPLAY_VEHICLE_TUNING_ASSEMBLY_H

#include "engine/game/replay_vehicle_tuning_definition.h"
#include "engine/core/func_keys_real.h"
#include "engine/scene/scene_vehicle_car_tuning.h"
#include "engine/scene/scene_vehicle_struct.h"
class ReplayVehicleTuningInstallation {
public:
  void Reset();
  void Install(const ReplayVehicleTuningDefinition &definition,
               CSceneVehicleCarTuning &tuning,
               CSceneVehicleStruct &vehicleStruct);

private:
  static void
  InstallCurve(const std::optional<ReplayTuningCurveDefinition> &definition,
               CFuncKeysReal &storage, CSceneVehicleTuningCurve &target);

  struct OwnedCurves {
    CFuncKeysReal lateralContactSlowDownFromSpeed;
    CFuncKeysReal maxSideFrictionFromSpeed;
    CFuncKeysReal rolloverLateralFromSpeed;
    CFuncKeysReal rolloverLateralCoefficientFromAngle;
    CFuncKeysReal wheelVisualSteerAngleFromSpeed;
    CFuncKeysReal steeringDriveTorqueFromSpeed;
    CFuncKeysReal steerSlowDownFromSpeed;
    CFuncKeysReal suspensionDamperAbsorbModulation;
    CFuncKeysReal airControlZScale;
    CFuncKeysReal radiusSteeringRadiusFromSpeed;
    CFuncKeysReal radiusSteeringMaxFrictionFromSpeed;
    CFuncKeysReal slipResponseAccelFromSpeed;
    CFuncKeysReal slipResponseSlippingAccelFromSpeed;
    CFuncKeysReal reverseGearAccelFromSpeed;
    CFuncKeysReal burnoutRolloverLateralFromSpeedRatio;
    CFuncKeysReal burnoutRadiusFromSpeed;
    CFuncKeysReal burnoutLateralSpeedFromRadius;
    CFuncKeysReal donutRolloverFromSpeed;
    CFuncKeysReal burnoutRolloverFromSpeed;
    CFuncKeysReal splashVerticalImpulse;
    CFuncKeysReal splashHorizontalImpulse;
    CFuncKeysReal waterFrictionFromSpeed;
    CFuncKeysReal surfaceFeedback;
    CFuncKeysReal vehicleFeedbackRamp1;
    CFuncKeysReal vehicleFeedbackRamp0;
    CFuncKeysReal vehicleDefault30To100;
  } curves;
};

#endif // TMNF_REPLAY_VEHICLE_TUNING_ASSEMBLY_H
