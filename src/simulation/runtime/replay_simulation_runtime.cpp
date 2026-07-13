#include "simulation/runtime/replay_simulation_runtime.h"
#include <utility>

#include "engine/physics/dynamics/hms_item.h"
#include "simulation/runtime/replay_environment.h"
#include "simulation/replay/replay_map_scene.h"
#include "simulation/runtime/replay_physics_world.h"
#include "simulation/runtime/replay_vehicle_body.h"
#include "simulation/runtime/replay_vehicle_simulation.h"
#include "simulation/runtime/replay_validation_spawn.h"
#include "engine/game/trackmania_race.h"
namespace {

CHmsItem::Properties ReplayVehicleItemProperties() {
    CHmsItem::Properties properties;
    properties.contactInterest = CHmsItem::EContactInterest_Local;
    properties.collisionGroup = CHmsItem::ECollisionGroup_Dynamic;
    properties.dynamicType = CHmsItem::EDynamicType_Normal;
    properties.active = true;
    properties.shadowTexCastedEnabled = true;
    properties.shadowFakeEnabled = true;
    properties.lightLensFlareEnabled = true;
    properties.shadowTexCastedCount = 1u;
    properties.shadowCasterGroupMask = 2u;
    properties.shadowReceiverGroupMask = 0xff6u;
    return properties;
}

}  // namespace

struct ReplaySimulationRuntime::State {
    explicit State(CTrackManiaRace &race)
        : vehicle(race), race(race) {}

    ReplayPhysicsWorld world;
    ReplayEnvironment environment;
    ReplayVehicleBody body;
    ReplayVehicleSimulation vehicle;
    CTrackManiaRace &race;
    const ReplaySimulationDefinition *definition = nullptr;
    bool staticSceneReady = false;
    bool firstStep = true;
};

ReplaySimulationRuntime::ReplaySimulationRuntime(CTrackManiaRace &race)
    : state_(std::make_unique<State>(race)) {}

ReplaySimulationRuntime::~ReplaySimulationRuntime() = default;

ReplaySimulationRunResult ReplaySimulationRuntime::Start(
        const ReplaySimulationDefinition &definition,
        ReplayMapScene &mapScene,
        const GmIso4 &spawnLocation,
        const ReplayControlTick &firstTick,
        std::uint32_t validationSeed) {
    State &state = *state_;
    state.definition = &definition;
    const ReplayMapSceneResult sceneResult = state.world.ConnectMapScene(
            mapScene, &state.vehicle.Car(), state.race);
    if (sceneResult != ReplayMapSceneResult::Ready) {
        return MapReplaySceneResult(sceneResult);
    }

    state.body.InitializeAtSpawn(
            definition.vehicle.dynaParameters, spawnLocation);
    state.body.ConstructItem(ReplayVehicleItemProperties());
    state.body.BuildCorpus();
    state.body.InstallEmptyCollisionTree();
    state.environment.Build(definition.environment);
    state.environment.InstallWater(
            state.world.Zone(),
            state.world.CollisionZone(),
            definition.environment.water);
    state.world.AddVehicleBody(state.body.Corpus());
    state.staticSceneReady = mapScene.IsActive();

    const ReplayVehiclePreparationResult vehicleResult = state.vehicle.Start(
            definition, firstTick, state.body, state.staticSceneReady);
    if (vehicleResult != ReplayVehiclePreparationResult::Ready) {
        return ReplaySimulationRunResult::VehicleCollisionModelFailed;
    }
    const std::optional<ReplayDynaParameters> parameters =
            state.vehicle.BuildDynaParameters();
    if (parameters.has_value()) {
        state.body.InstallDynaParameters(*parameters);
    }
    state.body.SetSpawnLocation(
            BuildReplayValidationSpawnLocation(
                    spawnLocation, validationSeed));
    state.firstStep = true;
    return ReplaySimulationRunResult::Success;
}

ReplaySimulationStepExecution ReplaySimulationRuntime::Step(
        const ReplayControlTick &tick) {
    State &state = *state_;
    ReplaySimulationStepExecution execution;
    if (state.definition == nullptr) {
        execution.result = ReplaySimulationRunResult::InvalidControlTimeline;
        return execution;
    }

    if (!state.firstStep) {
        state.vehicle.PrepareStep(tick, state.body);
    }

    CSceneVehicleCar &car = state.vehicle.Car();
    car.EnableAbsorbContactCallback(1);
    car.EnablePhysicsUpdates(!tick.actions.suppressVehicleForceCallbacks);
    state.world.InstallEnvironment(
            state.environment,
            state.definition->environment,
            !tick.actions.suppressVehicleForceCallbacks);
    state.world.SetSimulationTime(tick);

    for (std::uint32_t respawnIndex = 0u;
         respawnIndex < tick.actions.respawnAtCheckpointCount;
         ++respawnIndex) {
        if (state.vehicle.Respawn(state.body)) {
            ++execution.respawnExecutedCount;
        }
    }

    state.world.Step();
    execution.simulatedFrame = state.body.CaptureCurrentFrame();
    execution.writeFrame = state.body.CaptureWriteState();
    execution.finishTickMs = state.vehicle.FinishTimeMs();
    state.firstStep = false;
    return execution;
}

std::optional<std::uint32_t> ReplaySimulationRuntime::FinishTimeMs() const {
    return state_->vehicle.FinishTimeMs();
}
