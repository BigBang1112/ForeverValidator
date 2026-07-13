#include "simulation/runtime/replay_vehicle_assembly.h"
namespace {

void InstallCarWheel(const VehicleWheelDefinition &definition,
                     float restDamperAbsorb,
                     CSceneVehicleCar::SSimulationWheel &wheel) {
  wheel.ResetSimulationState();
  wheel.killsLateralSpeedOnContact = definition.killsLateralSpeedOnContact;
  wheel.axle = definition.axle;
  wheel.rollingRadius = definition.rollingRadius;

  const GmMat3 restRotation = definition.restSurfacePose.RotationMatrix();
  const GmVec3 restPoint = definition.restSurfacePose.Translation();
  GmVec3 currentPoint = restPoint;
  currentPoint.y = (restPoint.y - restDamperAbsorb);
  wheel.surfaceHandler.SetRestPose(restRotation, restPoint);
  wheel.surfaceHandler.SetCurrentPose(restRotation, currentPoint);
  wheel.forceApplicationPoint = definition.forceApplicationPoint;

  wheel.realTimeState = CSceneVehicleCar::SSimulationWheel::SRealTimeState{};
  wheel.realTimeState.damperAbsorb = restDamperAbsorb;
  wheel.realTimeState.contactFrame.SetIdentity();

  wheel.previousPhysicsState = CSceneVehicleCar::SSimulationWheel::SState{};
  wheel.currentPhysicsState = CSceneVehicleCar::SSimulationWheel::SState{};
  wheel.previousAsyncState = CSceneVehicleCar::SSimulationWheel::SState{};
  wheel.asyncState = CSceneVehicleCar::SSimulationWheel::SState{};
  wheel.previousPhysicsState.visualRotation.SetIdentity();
  wheel.currentPhysicsState.visualRotation.SetIdentity();
  wheel.previousAsyncState.visualRotation.SetIdentity();
  wheel.asyncState.visualRotation.SetIdentity();
}

void InstallVehicleStructWheel(const VehicleWheelDefinition &definition,
                               CSceneVehicleStruct::SSimulationWheel &wheel) {
  wheel.Reset();
  wheel.surfaceId = definition.surfaceId;
  wheel.killsLateralSpeedOnContact = definition.killsLateralSpeedOnContact;
  wheel.axle = definition.axle;
}

} // namespace

void InstallReplayVehicleInitialParameters(
    const VehicleInitialParameters &parameters, CSceneVehicleCar &car) {
  car.ConfigureSimulationLimits(parameters.linearSpeedCap,
                                parameters.reverseGearSpeedThreshold);
  car.SetWaterBoxLocal(parameters.waterBoxLocal);
}

void InstallReplayVehicleWheels(const VehicleWheelSetDefinition &definition,
                                CSceneVehicleCar &car,
                                CSceneVehicleStruct &vehicleStruct) {
  vehicleStruct.SetSimulationWheelCount(0u);
  constexpr u32 wheelCount =
      static_cast<u32>(VehicleWheelSetDefinition::OfficialWheelCount);
  car.SetWheelCount(wheelCount);
  vehicleStruct.SetSimulationWheelCount(wheelCount);
  for (u32 index = 0u; index < wheelCount; ++index) {
    InstallCarWheel(definition.wheels[index], definition.restDamperAbsorb,
                    car.WheelAt(index));
    InstallVehicleStructWheel(definition.wheels[index],
                              vehicleStruct.SimulationWheelAt(index));
  }
}
