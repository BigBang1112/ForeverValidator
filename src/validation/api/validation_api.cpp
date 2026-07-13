#include <forevervalidator/validation.h>

#include <cstdint>
#include <memory>
#include <new>
#include <utility>

#include "format/assets/replay_asset_repository.h"
#include "format/pack/default_vehicle_pack_archive.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/pack/replay_vehicle_source_bundle.h"
#include "format/replay/replay_file.h"
#include "format/static_solid/default_vehicle_solid_archive.h"
#include "simulation/runtime/replay_deterministic_execution.h"
#include "simulation/runtime/replay_simulation_definition.h"
#include "simulation/runtime/replay_simulation_session.h"
#include "validation/evaluation/replay_validation_session.h"
#include "validation/planning/replay_challenge_map_preload.h"

namespace forevervalidator {

namespace {

struct PreparedAssets {
    AssetBytes stadiumPack;
    AssetBytes packlist;
    ReplayVehicleSourceBundle vehicleSources;
};

struct ValidationState {
    explicit ValidationState(AssetProvider value)
        : provider(std::move(value)) {}

    AssetProvider provider;
    std::unique_ptr<PreparedAssets> prepared;
};

}  // namespace

struct AssetSource::Impl {
    explicit Impl(AssetProvider value) : provider(std::move(value)) {}
    AssetProvider provider;
};

struct ValidationContext::Impl {
    explicit Impl(AssetProvider value) : state(std::move(value)) {}
    ValidationState state;
};

namespace {

ValidationError MakeError(
        ValidationErrorCategory category,
        ValidationErrorCode code,
        ValidationStage stage,
        ValidationFailureReason reason,
        const ReplayIdentity &identity,
        const char *diagnostic) {
    ValidationError error;
    error.category = category;
    error.code = code;
    error.stage = stage;
    error.reason = reason;
    error.replay = identity;
    error.diagnostic = diagnostic == nullptr ? "" : diagnostic;
    return error;
}

ValidationError AllocationError(
        ValidationStage stage,
        const ReplayIdentity &identity,
        const char *diagnostic) {
    return MakeError(
            ValidationErrorCategory::Allocation,
            ValidationErrorCode::AllocationFailed,
            stage,
            ValidationFailureReason::AllocationFailed,
            identity,
            diagnostic);
}

ValidationStatus ToPublicStatus(ReplayValidationStatus status) {
    switch (status) {
    case ReplayValidationStatus::Valid: return ValidationStatus::Valid;
    case ReplayValidationStatus::ValidPrefix:
        return ValidationStatus::ValidPrefix;
    case ReplayValidationStatus::WrongSimulation:
        return ValidationStatus::WrongSimulation;
    case ReplayValidationStatus::IncompleteStandaloneRun:
        return ValidationStatus::IncompleteStandaloneRun;
    case ReplayValidationStatus::RaceCompletionUnavailable:
        return ValidationStatus::RaceCompletionUnavailable;
    case ReplayValidationStatus::ExpectingCompletedRace:
        return ValidationStatus::ExpectingCompletedRace;
    case ReplayValidationStatus::RaceTimeMismatch:
        return ValidationStatus::RaceTimeMismatch;
    case ReplayValidationStatus::StuntsScoreMismatch:
        return ValidationStatus::StuntsScoreMismatch;
    case ReplayValidationStatus::RespawnCountMismatch:
        return ValidationStatus::RespawnCountMismatch;
    case ReplayValidationStatus::RespawnExpectationUnavailable:
        return ValidationStatus::RespawnExpectationUnavailable;
    case ReplayValidationStatus::ObservationError:
        return ValidationStatus::ObservationError;
    }
    return ValidationStatus::ObservationError;
}

ValidationOutcome ToPublicOutcome(ReplayValidationOutcome outcome) {
    switch (outcome) {
    case ReplayValidationOutcome::Invalid: return ValidationOutcome::Invalid;
    case ReplayValidationOutcome::Valid: return ValidationOutcome::Valid;
    case ReplayValidationOutcome::WrongSimulation:
        return ValidationOutcome::WrongSimulation;
    case ReplayValidationOutcome::Unavailable:
        return ValidationOutcome::Unavailable;
    case ReplayValidationOutcome::Error: return ValidationOutcome::Error;
    }
    return ValidationOutcome::Error;
}

Vector3 ToPublicVector(const GmVec3 &value) {
    return {value.x, value.y, value.z};
}

ValidationDeviation ToPublicDeviation(
        const ReplayValidationDeviation &value) {
    ValidationDeviation result;
    result.comparisonOrdinal = value.comparisonOrdinal;
    result.ghostTimeMs = value.ghostTimeMs;
    result.simulationTimeMs = value.simulationTimeMs;
    result.distance = value.distance;
    result.simulatedPosition = ToPublicVector(value.simulatedPosition);
    result.writePosition = ToPublicVector(value.writePosition);
    result.targetPosition = ToPublicVector(value.targetPosition);
    return result;
}

std::optional<ObservationError> ToPublicObservationError(
        const std::optional<ReplayObservationError> &error) {
    if (!error.has_value()) {
        return std::nullopt;
    }
    switch (*error) {
    case ReplayObservationError::NonFiniteDistance:
        return ObservationError::NonFiniteDistance;
    case ReplayObservationError::ReplayMetadataUnavailable:
        return ObservationError::ReplayMetadataUnavailable;
    }
    return ObservationError::ReplayMetadataUnavailable;
}

ValidationReport ToPublicReport(
        const ReplayIdentity &identity,
        const ReplayFileValidationResult &source) {
    ValidationReport report;
    report.replay = identity;
    report.valid = source.validation.status == ReplayValidationStatus::Valid;
    report.status = ToPublicStatus(source.validation.status);
    report.outcome = ToPublicOutcome(source.validation.outcome);
    report.measuredSamples = source.validation.measuredSamples;
    report.expectedSamples = source.validation.expectedSamples;
    report.comparedExactGhostStateCount =
            source.validation.comparedExactGhostStateCount;
    report.wrongSimulation = source.validation.wrongSimulation;
    if (source.validation.firstDivergence.has_value()) {
        report.firstDivergence =
                ToPublicDeviation(*source.validation.firstDivergence);
    }
    if (source.validation.firstExactDeviation.has_value()) {
        report.firstExactDeviation =
                ToPublicDeviation(*source.validation.firstExactDeviation);
    }
    report.maxDeviation = source.validation.maxDeviation;
    report.maxDeviationTimeMs = source.validation.maxDeviationTimeMs;
    report.maxDeviationDistance = source.validation.maxDeviationDistance;
    report.observationError =
            ToPublicObservationError(source.validation.observationError);
    report.metadata.sampleCount = source.metadata.sampleCount;
    report.metadata.samplePeriodMs = source.metadata.samplePeriodMs;
    report.metadata.encodedGhostSampleByteCount =
            source.metadata.encodedGhostSampleByteCount;
    report.metadata.encodedGhostStateByteCount =
            source.metadata.encodedGhostStateByteCount;
    report.metadata.inputDurationMs = source.metadata.inputDurationMs;
    report.metadata.expectedRaceTimeMs = source.metadata.expectedRaceTimeMs;
    report.metadata.expectedRespawns = source.metadata.expectedRespawns;
    report.metadata.requestedSamples = source.metadata.requestedSamples;
    report.metadata.expectedSamples = source.metadata.expectedSamples;
    report.metadata.actionCount = source.metadata.actionCount;
    report.metadata.eventCount = source.metadata.eventCount;
    report.simulation.raceCompleted = source.raceOutcome.raceCompleted;
    report.simulation.raceTimeMs = source.raceOutcome.raceTimeMs;
    report.simulation.stuntsScore = source.raceOutcome.stuntsScore;
    report.simulation.respawnCount = source.raceOutcome.respawnCount;
    return report;
}

ValidationFailureReason ReplayReadReason(ReplayFileReadError error) {
    switch (error) {
    case ReplayFileReadError::Success: return ValidationFailureReason::None;
    case ReplayFileReadError::InvalidRequest:
        return ValidationFailureReason::ReplayInvalidRequest;
    case ReplayFileReadError::FileReadFailed:
        return ValidationFailureReason::ReplayFileReadFailed;
    case ReplayFileReadError::InvalidContainer:
        return ValidationFailureReason::ReplayInvalidContainer;
    case ReplayFileReadError::AllocationFailed:
        return ValidationFailureReason::AllocationFailed;
    case ReplayFileReadError::RootBodyDecompressionFailed:
        return ValidationFailureReason::ReplayRootBodyDecompressionFailed;
    case ReplayFileReadError::MissingGhostBuffer:
        return ValidationFailureReason::ReplayMissingGhostBuffer;
    case ReplayFileReadError::MissingInputStream:
        return ValidationFailureReason::ReplayMissingInputStream;
    case ReplayFileReadError::TooManyInputActions:
        return ValidationFailureReason::ReplayTooManyInputActions;
    case ReplayFileReadError::InvalidGhostMetadata:
        return ValidationFailureReason::ReplayInvalidGhostMetadata;
    case ReplayFileReadError::InvalidInputHeader:
        return ValidationFailureReason::ReplayInvalidInputHeader;
    case ReplayFileReadError::InvalidInputActions:
        return ValidationFailureReason::ReplayInvalidInputActions;
    case ReplayFileReadError::InvalidInputEventHeader:
        return ValidationFailureReason::ReplayInvalidInputEventHeader;
    case ReplayFileReadError::InvalidInputEvents:
        return ValidationFailureReason::ReplayInvalidInputEvents;
    case ReplayFileReadError::InvalidInputTimeline:
        return ValidationFailureReason::ReplayInvalidInputTimeline;
    case ReplayFileReadError::MissingGhostState:
        return ValidationFailureReason::ReplayMissingGhostState;
    case ReplayFileReadError::GhostStateDecompressionFailed:
        return ValidationFailureReason::ReplayGhostStateDecompressionFailed;
    case ReplayFileReadError::InvalidGhostState:
        return ValidationFailureReason::ReplayInvalidGhostState;
    case ReplayFileReadError::GhostTrajectoryAllocationFailed:
        return ValidationFailureReason::ReplayGhostTrajectoryAllocationFailed;
    case ReplayFileReadError::MissingEmbeddedChallenge:
        return ValidationFailureReason::ReplayMissingEmbeddedChallenge;
    case ReplayFileReadError::InvalidEmbeddedChallenge:
        return ValidationFailureReason::ReplayInvalidEmbeddedChallenge;
    case ReplayFileReadError::InvalidMap:
        return ValidationFailureReason::ReplayInvalidMap;
    }
    return ValidationFailureReason::UnexpectedFailure;
}

ValidationError ReplayDecodeError(
        ReplayFileReadError readError,
        const ReplayIdentity &identity) {
    const bool allocation =
            readError == ReplayFileReadError::AllocationFailed ||
            readError == ReplayFileReadError::GhostTrajectoryAllocationFailed;
    ValidationError error = MakeError(
            allocation ? ValidationErrorCategory::Allocation
                       : ValidationErrorCategory::Replay,
            allocation ? ValidationErrorCode::AllocationFailed
                       : ValidationErrorCode::ReplayDecodingFailed,
            ValidationStage::ReplayDecoding,
            ReplayReadReason(readError),
            identity,
            ReplayFileReadErrorName(readError));
    return error;
}

ValidationFailureReason PreloadReason(ReplayChallengePreloadResult result) {
    switch (result) {
    case ReplayChallengePreloadResult::Success:
        return ValidationFailureReason::None;
    case ReplayChallengePreloadResult::InvalidRequest:
        return ValidationFailureReason::ChallengeInvalidRequest;
    case ReplayChallengePreloadResult::AllocationFailed:
        return ValidationFailureReason::AllocationFailed;
    case ReplayChallengePreloadResult::WaterDefinitionFailed:
        return ValidationFailureReason::ChallengeWaterDefinitionFailed;
    case ReplayChallengePreloadResult::SceneDefinitionFailed:
        return ValidationFailureReason::ChallengeSceneDefinitionFailed;
    case ReplayChallengePreloadResult::ChallengeConstructionFailed:
        return ValidationFailureReason::ChallengeConstructionFailed;
    case ReplayChallengePreloadResult::StaticSceneFailed:
        return ValidationFailureReason::ChallengeStaticSceneFailed;
    }
    return ValidationFailureReason::UnexpectedFailure;
}

ValidationError PreloadError(
        ReplayChallengePreloadResult result,
        const ReplayIdentity &identity) {
    const bool allocation =
            result == ReplayChallengePreloadResult::AllocationFailed;
    return MakeError(
            allocation ? ValidationErrorCategory::Allocation
                       : ValidationErrorCategory::Simulation,
            allocation ? ValidationErrorCode::AllocationFailed
                       : ValidationErrorCode::ChallengePreloadFailed,
            result == ReplayChallengePreloadResult::ChallengeConstructionFailed
                    ? ValidationStage::MapConstruction
                    : ValidationStage::ChallengePreload,
            PreloadReason(result),
            identity,
            "replay challenge preload failed");
}

ValidationFailureReason DefinitionReason(
        ReplaySimulationDefinitionBuildResult result) {
    switch (result) {
    case ReplaySimulationDefinitionBuildResult::Success:
        return ValidationFailureReason::None;
    case ReplaySimulationDefinitionBuildResult::MissingVehicleDefinition:
        return ValidationFailureReason::SimulationMissingVehicleDefinition;
    case ReplaySimulationDefinitionBuildResult::InvalidVehiclePhysics:
        return ValidationFailureReason::SimulationInvalidVehiclePhysics;
    case ReplaySimulationDefinitionBuildResult::InvalidInitialParameters:
        return ValidationFailureReason::SimulationInvalidInitialParameters;
    case ReplaySimulationDefinitionBuildResult::AllocationFailure:
        return ValidationFailureReason::AllocationFailed;
    case ReplaySimulationDefinitionBuildResult::InvalidEnvironment:
        return ValidationFailureReason::SimulationInvalidEnvironment;
    case ReplaySimulationDefinitionBuildResult::InvalidMaterials:
        return ValidationFailureReason::SimulationInvalidMaterials;
    }
    return ValidationFailureReason::UnexpectedFailure;
}

ValidationError DefinitionError(
        ReplaySimulationDefinitionBuildResult result,
        const ReplayIdentity &identity) {
    const bool allocation =
            result == ReplaySimulationDefinitionBuildResult::AllocationFailure;
    return MakeError(
            allocation ? ValidationErrorCategory::Allocation
                       : ValidationErrorCategory::Simulation,
            allocation ? ValidationErrorCode::AllocationFailed
                       : ValidationErrorCode::SimulationDefinitionFailed,
            ValidationStage::SimulationStartup,
            DefinitionReason(result),
            identity,
            "simulation definition build failed");
}

ValidationFailureReason ExecutionReason(
        ReplayValidationExecutionResult result) {
    switch (result) {
    case ReplayValidationExecutionResult::Success:
        return ValidationFailureReason::None;
    case ReplayValidationExecutionResult::MissingInput:
        return ValidationFailureReason::SimulationMissingInput;
    case ReplayValidationExecutionResult::InvalidPlan:
        return ValidationFailureReason::SimulationInvalidPlan;
    case ReplayValidationExecutionResult::ControlPlanInvalidRequest:
        return ValidationFailureReason::SimulationControlPlanInvalidRequest;
    case ReplayValidationExecutionResult::ControlTargetAllocationFailed:
        return ValidationFailureReason::SimulationControlTargetAllocationFailed;
    case ReplayValidationExecutionResult::ControlTargetTimeOutOfRange:
        return ValidationFailureReason::SimulationControlTargetTimeOutOfRange;
    case ReplayValidationExecutionResult::ControlTargetNonFinite:
        return ValidationFailureReason::SimulationControlTargetNonFinite;
    case ReplayValidationExecutionResult::ControlTickReservationFailed:
        return ValidationFailureReason::SimulationControlTickReservationFailed;
    case ReplayValidationExecutionResult::ControlTickAllocationFailed:
        return ValidationFailureReason::SimulationControlTickAllocationFailed;
    case ReplayValidationExecutionResult::ControlTargetMissing:
        return ValidationFailureReason::SimulationControlTargetMissing;
    case ReplayValidationExecutionResult::ControlOutputMissing:
        return ValidationFailureReason::SimulationControlOutputMissing;
    case ReplayValidationExecutionResult::InvalidControlPlan:
        return ValidationFailureReason::SimulationInvalidControlPlan;
    case ReplayValidationExecutionResult::PhysicsInputInvalid:
        return ValidationFailureReason::SimulationPhysicsInputInvalid;
    case ReplayValidationExecutionResult::MapStartUnavailable:
        return ValidationFailureReason::SimulationMapStartUnavailable;
    case ReplayValidationExecutionResult::ObservationAllocationFailed:
        return ValidationFailureReason::ObservationAllocationFailed;
    case ReplayValidationExecutionResult::DeterministicExecutionUnavailable:
        return ValidationFailureReason::DeterministicExecutionUnavailable;
    }
    return ValidationFailureReason::UnexpectedFailure;
}

ValidationError ExecutionError(
        ReplayValidationExecutionResult result,
        const ReplayIdentity &identity) {
    ValidationErrorCategory category = ValidationErrorCategory::Simulation;
    ValidationErrorCode code = ValidationErrorCode::SimulationFailed;
    ValidationStage stage = ValidationStage::SimulationStep;
    const char *diagnostic = "replay simulation failed";
    switch (result) {
    case ReplayValidationExecutionResult::ControlTargetAllocationFailed:
    case ReplayValidationExecutionResult::ControlTickReservationFailed:
    case ReplayValidationExecutionResult::ControlTickAllocationFailed:
        category = ValidationErrorCategory::Allocation;
        code = ValidationErrorCode::AllocationFailed;
        break;
    case ReplayValidationExecutionResult::MapStartUnavailable:
        stage = ValidationStage::SimulationStartup;
        break;
    case ReplayValidationExecutionResult::ObservationAllocationFailed:
        category = ValidationErrorCategory::Observation;
        code = ValidationErrorCode::ObservationFailed;
        stage = ValidationStage::Observation;
        break;
    case ReplayValidationExecutionResult::DeterministicExecutionUnavailable:
        code = ValidationErrorCode::DeterministicExecutionUnavailable;
        stage = ValidationStage::SimulationStartup;
        diagnostic = "deterministic execution mode unavailable";
        break;
    case ReplayValidationExecutionResult::Success:
    case ReplayValidationExecutionResult::MissingInput:
    case ReplayValidationExecutionResult::InvalidPlan:
    case ReplayValidationExecutionResult::ControlPlanInvalidRequest:
    case ReplayValidationExecutionResult::ControlTargetTimeOutOfRange:
    case ReplayValidationExecutionResult::ControlTargetNonFinite:
    case ReplayValidationExecutionResult::ControlTargetMissing:
    case ReplayValidationExecutionResult::ControlOutputMissing:
    case ReplayValidationExecutionResult::InvalidControlPlan:
    case ReplayValidationExecutionResult::PhysicsInputInvalid:
        break;
    }
    return MakeError(
            category, code, stage, ExecutionReason(result), identity, diagnostic);
}

Result<AssetBytes> LoadRequiredAsset(
        ValidationState &context,
        const char *identifier,
        ValidationFailureReason missingReason,
        const ReplayIdentity &identity) {
    const AssetRequest request{identifier};
    Result<AssetBytes> loaded = [&]() -> Result<AssetBytes> {
        try {
            return context.provider(request);
        } catch (const std::bad_alloc &) {
            ValidationError error = AllocationError(
                    ValidationStage::AssetLoading,
                    identity,
                    "allocation failed in asset provider");
            error.relatedAsset = identifier;
            return Result<AssetBytes>::Failure(std::move(error));
        } catch (...) {
            ValidationError error = MakeError(
                    ValidationErrorCategory::Asset,
                    ValidationErrorCode::AssetLoadingFailed,
                    ValidationStage::AssetLoading,
                    ValidationFailureReason::AssetProviderFailed,
                    identity,
                    "asset provider threw an unexpected exception");
            error.relatedAsset = identifier;
            return Result<AssetBytes>::Failure(std::move(error));
        }
    }();
    if (!loaded) {
        ValidationError error = std::move(loaded).Error();
        error.stage = ValidationStage::AssetLoading;
        error.replay = identity;
        if (error.category == ValidationErrorCategory::Internal &&
            error.code == ValidationErrorCode::UnexpectedFailure &&
            error.reason == ValidationFailureReason::UnexpectedFailure) {
            error.category = ValidationErrorCategory::Asset;
            error.code = ValidationErrorCode::AssetLoadingFailed;
            error.reason = ValidationFailureReason::AssetProviderFailed;
        }
        if (error.relatedAsset.empty()) {
            error.relatedAsset = identifier;
        }
        return Result<AssetBytes>::Failure(std::move(error));
    }
    AssetBytes bytes = std::move(loaded).Value();
    if (bytes.empty()) {
        ValidationError error = MakeError(
                ValidationErrorCategory::Asset,
                ValidationErrorCode::AssetLoadingFailed,
                ValidationStage::AssetLoading,
                missingReason,
                identity,
                "required installed-pack asset is empty or unavailable");
        error.relatedAsset = identifier;
        return Result<AssetBytes>::Failure(std::move(error));
    }
    return Result<AssetBytes>::Success(std::move(bytes));
}

Result<PreparedAssets *> PrepareAssets(
        ValidationState &context,
        const ReplayIdentity &identity) {
    if (context.prepared != nullptr) {
        return Result<PreparedAssets *>::Success(
                context.prepared.get());
    }

    Result<AssetBytes> packlist = LoadRequiredAsset(
            context, "packlist.dat", ValidationFailureReason::PacklistMissing,
            identity);
    if (!packlist) {
        return Result<PreparedAssets *>::Failure(
                std::move(packlist).Error());
    }
    Result<AssetBytes> stadium = LoadRequiredAsset(
            context, "Stadium.pak", ValidationFailureReason::StadiumPackMissing,
            identity);
    if (!stadium) {
        return Result<PreparedAssets *>::Failure(
                std::move(stadium).Error());
    }

    auto prepared = std::make_unique<PreparedAssets>();
    prepared->packlist = std::move(packlist).Value();
    prepared->stadiumPack = std::move(stadium).Value();

    CPlugFilePack pack;
    if (!pack.OpenFromMemory(
                prepared->stadiumPack.data(), prepared->stadiumPack.size(),
                prepared->packlist.data(), prepared->packlist.size())) {
        ValidationError error = MakeError(
                ValidationErrorCategory::Asset,
                ValidationErrorCode::AssetLoadingFailed,
                ValidationStage::AssetLoading,
                ValidationFailureReason::StadiumPackInvalid,
                identity,
                "could not decode Stadium.pak using packlist.dat");
        error.relatedAsset = "Stadium.pak";
        return Result<PreparedAssets *>::Failure(
                std::move(error));
    }

    std::optional<DefaultVehiclePackData> vehicle =
            DefaultVehiclePackArchive::LoadFromPack(pack);
    std::optional<ReplayVehicleSolidDefinition> solid =
            DefaultVehicleSolidArchive::LoadFromPack(pack);
    if (!vehicle.has_value() || !solid.has_value()) {
        ValidationError error = MakeError(
                ValidationErrorCategory::Asset,
                ValidationErrorCode::AssetLoadingFailed,
                ValidationStage::AssetLoading,
                ValidationFailureReason::DefaultVehicleUnavailable,
                identity,
                "could not load default vehicle definitions from Stadium pack");
        error.relatedAsset = "Stadium.pak";
        return Result<PreparedAssets *>::Failure(
                std::move(error));
    }
    prepared->vehicleSources = ReplayVehicleSourceBundle{
            std::move(*solid),
            std::move(vehicle->tuning),
            std::move(vehicle->vehicle)};
    if (!prepared->vehicleSources.IsComplete()) {
        ValidationError error = MakeError(
                ValidationErrorCategory::Asset,
                ValidationErrorCode::AssetLoadingFailed,
                ValidationStage::AssetLoading,
                ValidationFailureReason::DefaultVehicleUnavailable,
                identity,
                "default vehicle definitions are incomplete");
        error.relatedAsset = "Stadium.pak";
        return Result<PreparedAssets *>::Failure(
                std::move(error));
    }

    context.prepared = std::move(prepared);
    return Result<PreparedAssets *>::Success(
            context.prepared.get());
}

Result<ValidationReport> RunReplayValidation(
        ValidationState &context,
        ByteView replayBytes,
        const ReplayIdentity &identity,
        const ValidationOptions &options) {
    ReplayFile replayFile;
    const ReplayFileReadError readError = ReadReplayBytes(
            reinterpret_cast<const std::uint8_t *>(replayBytes.data),
            replayBytes.size,
            &replayFile);
    if (readError != ReplayFileReadError::Success) {
        return Result<ValidationReport>::Failure(
                ReplayDecodeError(readError, identity));
    }

    Result<PreparedAssets *> preparedResult =
            PrepareAssets(context, identity);
    if (!preparedResult) {
        return Result<ValidationReport>::Failure(
                std::move(preparedResult).Error());
    }
    const PreparedAssets &prepared =
            *preparedResult.Value();

    std::unique_ptr<ReplayAssetRepository> assets = OpenReplayAssetRepository(
            prepared.stadiumPack.data(), prepared.stadiumPack.size(),
            prepared.packlist.data(), prepared.packlist.size());
    if (!assets) {
        ValidationError error = MakeError(
                ValidationErrorCategory::Asset,
                ValidationErrorCode::AssetLoadingFailed,
                ValidationStage::AssetLoading,
                ValidationFailureReason::AssetRepositoryUnavailable,
                identity,
                "could not open replay asset repository");
        error.relatedAsset = "Stadium.pak";
        return Result<ValidationReport>::Failure(std::move(error));
    }

    ReplaySimulationSession simulationSession;
    CGameCtnReplayChallengeMapPreload preload;
    const ReplayChallengePreloadResult preloadResult = preload.Preload(
            replayFile.MapInput(), *assets, simulationSession);
    if (preloadResult != ReplayChallengePreloadResult::Success) {
        return Result<ValidationReport>::Failure(
                PreloadError(preloadResult, identity));
    }

    ReplaySimulationDefinitionBuild definition =
            BuildReplaySimulationDefinition(
                    prepared.vehicleSources, preload.WaterDefinition());
    if (!definition) {
        return Result<ValidationReport>::Failure(
                DefinitionError(definition.Error(), identity));
    }

    simulationSession.ActivateStaticScene();
    const ReplayValidationConfiguration configuration{
            options.requestedSamples,
            options.controlTickMs,
            options.validationPrestartMs,
            {},
            100000u,
    };
    ReplayFileValidationBuild validation = ValidateReplayFile(
            replayFile,
            simulationSession,
            definition.Value(),
            configuration);
    if (!validation) {
        return Result<ValidationReport>::Failure(
                ExecutionError(validation.Error(), identity));
    }
    return Result<ValidationReport>::Success(
            ToPublicReport(identity, validation.Value()));
}

}  // namespace

bool IsNormalizedAssetIdentifier(std::string_view identifier) noexcept {
    if (identifier.empty() || identifier.front() == '/' ||
        identifier.back() == '/' ||
        identifier.find('\\') != std::string_view::npos ||
        identifier.find('\0') != std::string_view::npos) {
        return false;
    }
    const bool asciiDrivePrefix = identifier.size() >= 2u &&
            ((identifier[0] >= 'A' && identifier[0] <= 'Z') ||
             (identifier[0] >= 'a' && identifier[0] <= 'z')) &&
            identifier[1] == ':';
    if (asciiDrivePrefix) {
        return false;
    }
    std::size_t start = 0u;
    while (start < identifier.size()) {
        const std::size_t end = identifier.find('/', start);
        const std::size_t count =
                (end == std::string_view::npos ? identifier.size() : end) - start;
        if (count == 0u ||
            (count == 1u && identifier[start] == '.') ||
            (count == 2u && identifier[start] == '.' &&
             identifier[start + 1u] == '.')) {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1u;
    }
    return true;
}

AssetSource::AssetSource(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}
AssetSource::AssetSource(AssetSource &&) noexcept = default;
AssetSource &AssetSource::operator=(AssetSource &&) noexcept = default;
AssetSource::~AssetSource() = default;

Result<AssetSource> CreateAssetSource(AssetProvider provider) noexcept {
    try {
        if (!provider) {
            return Result<AssetSource>::Failure(MakeError(
                    ValidationErrorCategory::InvalidInput,
                    ValidationErrorCode::InvalidArgument,
                    ValidationStage::ContextCreation,
                    ValidationFailureReason::InvalidAssetProvider,
                    {},
                    "asset provider is empty"));
        }
        return Result<AssetSource>::Success(AssetSource(
                std::make_unique<AssetSource::Impl>(std::move(provider))));
    } catch (const std::bad_alloc &) {
        return Result<AssetSource>::Failure(AllocationError(
                ValidationStage::ContextCreation, {},
                "allocation failed while creating asset source"));
    } catch (...) {
        return Result<AssetSource>::Failure(MakeError(
                ValidationErrorCategory::Internal,
                ValidationErrorCode::UnexpectedFailure,
                ValidationStage::ContextCreation,
                ValidationFailureReason::UnexpectedFailure,
                {},
                "unexpected asset source creation failure"));
    }
}

ValidationContext::ValidationContext(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}
ValidationContext::ValidationContext(ValidationContext &&) noexcept = default;
ValidationContext &ValidationContext::operator=(ValidationContext &&) noexcept =
        default;
ValidationContext::~ValidationContext() = default;

Result<ValidationContext> CreateValidationContext(
        AssetSource source) noexcept {
    try {
        if (source.impl_ == nullptr || !source.impl_->provider) {
            return Result<ValidationContext>::Failure(MakeError(
                    ValidationErrorCategory::InvalidInput,
                    ValidationErrorCode::AssetSourceUnavailable,
                    ValidationStage::ContextCreation,
                    ValidationFailureReason::InvalidAssetSource,
                    {},
                    "asset source is moved-from or invalid"));
        }
        AssetProvider provider = std::move(source.impl_->provider);
        source.impl_.reset();
        return Result<ValidationContext>::Success(ValidationContext(
                std::make_unique<ValidationContext::Impl>(
                        std::move(provider))));
    } catch (const std::bad_alloc &) {
        return Result<ValidationContext>::Failure(AllocationError(
                ValidationStage::ContextCreation, {},
                "allocation failed while creating validation context"));
    } catch (...) {
        return Result<ValidationContext>::Failure(MakeError(
                ValidationErrorCategory::Internal,
                ValidationErrorCode::UnexpectedFailure,
                ValidationStage::ContextCreation,
                ValidationFailureReason::UnexpectedFailure,
                {},
                "unexpected validation context creation failure"));
    }
}

Result<ValidationReport> ValidateReplay(
        ValidationContext &context,
        ByteView replayBytes,
        const ReplayIdentity &identity,
        const ValidationOptions &options) noexcept {
    try {
        if (context.impl_ == nullptr || !context.impl_->state.provider) {
            return Result<ValidationReport>::Failure(MakeError(
                    ValidationErrorCategory::InvalidInput,
                    ValidationErrorCode::InvalidArgument,
                    ValidationStage::ContextCreation,
                    ValidationFailureReason::InvalidValidationContext,
                    identity,
                    "validation context is moved-from or invalid"));
        }
        if (!replayBytes.IsValid() || replayBytes.size == 0u ||
            identity.name.empty() || options.requestedSamples == 0u ||
            options.controlTickMs == 0u ||
            options.validationPrestartMs == 0u) {
            return Result<ValidationReport>::Failure(MakeError(
                    ValidationErrorCategory::InvalidInput,
                    ValidationErrorCode::InvalidArgument,
                    ValidationStage::ContextCreation,
                    ValidationFailureReason::InvalidValidationRequest,
                    identity,
                    "invalid replay validation request"));
        }

        tmnf::simulation::DeterministicExecutionScope deterministicScope;
        if (!deterministicScope.Established()) {
            return Result<ValidationReport>::Failure(MakeError(
                    ValidationErrorCategory::Simulation,
                    ValidationErrorCode::DeterministicExecutionUnavailable,
                    ValidationStage::SimulationStartup,
                    ValidationFailureReason::DeterministicExecutionUnavailable,
                    identity,
                    "deterministic execution mode unavailable"));
        }

        Result<ValidationReport> result = RunReplayValidation(
                context.impl_->state, replayBytes, identity, options);
        if (!deterministicScope.Restore()) {
            return Result<ValidationReport>::Failure(MakeError(
                    ValidationErrorCategory::Simulation,
                    ValidationErrorCode::DeterministicExecutionUnavailable,
                    ValidationStage::SimulationStep,
                    ValidationFailureReason::DeterministicStateRestoreFailed,
                    identity,
                    "deterministic execution state could not be restored"));
        }
        return result;
    } catch (const std::bad_alloc &) {
        return Result<ValidationReport>::Failure(AllocationError(
                ValidationStage::ValidationEvaluation,
                identity,
                "allocation failed during replay validation"));
    } catch (...) {
        return Result<ValidationReport>::Failure(MakeError(
                ValidationErrorCategory::Internal,
                ValidationErrorCode::UnexpectedFailure,
                ValidationStage::ValidationEvaluation,
                ValidationFailureReason::UnexpectedFailure,
                identity,
                "unexpected replay validation failure"));
    }
}

}  // namespace forevervalidator
