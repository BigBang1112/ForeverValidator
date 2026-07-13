#pragma once

#include <vector>

#include "engine/core/cmw_nod.h"
#include "engine/core/engine_types.h"
#include "engine/core/func_keys_real.h"
#include "engine/scene/scene_vehicle.h"
#include "engine/scene/scene_vehicle_tuning.h"
#include "engine/game/vehicle_wheel_axle.h"
class CSceneVehicleStruct : public CMwNod {
public:
  struct SSimulationWheel {
    SSurfaceId surfaceId;
    bool killsLateralSpeedOnContact = false;
    VehicleWheelAxle axle = VehicleWheelAxle::Rear;
    SSimulationWheel(void);
    void Reset(void);
  };
  CSceneVehicleStruct(void);
  ~CSceneVehicleStruct(void) override;
  void ResetSimulationState(void);
  void SetSimulationWheelCount(u32 count);
  SSimulationWheel &SimulationWheelAt(u32 index);
  const std::vector<SSimulationWheel> &SimulationWheels(void) const;
  void BindFeedbackCurves(const CSceneVehicleTuningCurve &ramp1,
                          const CSceneVehicleTuningCurve &ramp0,
                          const CSceneVehicleTuningCurve &default30To100);
  CFuncKeysReal &FeedbackRamp1Curve(void) const;
  CFuncKeysReal &FeedbackRamp0Curve(void) const;

private:
  std::vector<SSimulationWheel> simulationWheels;
  CSceneVehicleTuningCurve feedbackRamp1Curve;
  CSceneVehicleTuningCurve feedbackRamp0Curve;
  CSceneVehicleTuningCurve default30To100Curve;
};
