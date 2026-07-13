#include "engine/scene/scene_vehicle_struct.h"
CSceneVehicleStruct::SSimulationWheel::SSimulationWheel(void) = default;

void CSceneVehicleStruct::SSimulationWheel::Reset(void) {
  surfaceId.Reset();
  killsLateralSpeedOnContact = false;
  axle = VehicleWheelAxle::Rear;
}

CSceneVehicleStruct::CSceneVehicleStruct(void) = default;

CSceneVehicleStruct::~CSceneVehicleStruct(void) = default;

void CSceneVehicleStruct::ResetSimulationState(void) {
  simulationWheels.clear();
  feedbackRamp1Curve.Reset();
  feedbackRamp0Curve.Reset();
  default30To100Curve.Reset();
}

void CSceneVehicleStruct::SetSimulationWheelCount(u32 count) {
  simulationWheels.resize(count);
}

CSceneVehicleStruct::SSimulationWheel &
CSceneVehicleStruct::SimulationWheelAt(u32 index) {
  return simulationWheels[index];
}

const std::vector<CSceneVehicleStruct::SSimulationWheel> &
CSceneVehicleStruct::SimulationWheels(void) const {
  return simulationWheels;
}

void CSceneVehicleStruct::BindFeedbackCurves(
    const CSceneVehicleTuningCurve &ramp1,
    const CSceneVehicleTuningCurve &ramp0,
    const CSceneVehicleTuningCurve &default30To100) {
  feedbackRamp1Curve = ramp1;
  feedbackRamp0Curve = ramp0;
  default30To100Curve = default30To100;
}

CFuncKeysReal &CSceneVehicleStruct::FeedbackRamp1Curve(void) const {
  return feedbackRamp1Curve.Value();
}

CFuncKeysReal &CSceneVehicleStruct::FeedbackRamp0Curve(void) const {
  return feedbackRamp0Curve.Value();
}
