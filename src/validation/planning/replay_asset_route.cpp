#include "validation/planning/replay_asset_route.h"

#include "format/replay/replay_file.h"

namespace {

std::string_view MapPackName(ReplayMapEnvironment environment) noexcept {
    switch (environment) {
    case ReplayMapEnvironment::Alpine: return "Alpine";
    case ReplayMapEnvironment::Speed: return "Speed";
    case ReplayMapEnvironment::Rally: return "Rally";
    case ReplayMapEnvironment::Island: return "Island";
    case ReplayMapEnvironment::Coast: return "Coast";
    case ReplayMapEnvironment::Bay: return "Bay";
    case ReplayMapEnvironment::Stadium: return "Stadium";
    case ReplayMapEnvironment::Unknown: return {};
    }
    return {};
}

std::string_view VehiclePackName(ReplayVehicleModel vehicle) noexcept {
    switch (vehicle) {
    case ReplayVehicleModel::SnowCar: return "Alpine";
    case ReplayVehicleModel::DesertCar: return "Speed";
    case ReplayVehicleModel::RallyCar: return "Rally";
    case ReplayVehicleModel::IslandCar: return "Island";
    case ReplayVehicleModel::CoastCar: return "Coast";
    case ReplayVehicleModel::BayCar: return "Bay";
    case ReplayVehicleModel::StadiumCar: return "Stadium";
    case ReplayVehicleModel::Unknown: return {};
    }
    return {};
}

std::optional<ReplayValidationMode> ValidationMode(
        EChallengePlayMode playMode) noexcept {
    switch (playMode) {
    case EChallengePlayMode::Race:
    case EChallengePlayMode::Shortcut:
        return ReplayValidationMode::Race;
    case EChallengePlayMode::Platform:
        return ReplayValidationMode::Platform;
    case EChallengePlayMode::Puzzle:
        return ReplayValidationMode::Puzzle;
    case EChallengePlayMode::Stunts:
        return ReplayValidationMode::Stunts;
    case EChallengePlayMode::Crazy:
        return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace

ReplayAssetRouteResult BuildReplayAssetRoute(
        ReplayMapEnvironment mapEnvironment,
        ReplayMapEnvironment decorationEnvironment,
        ReplayVehicleModel vehicleModel,
        std::optional<EChallengePlayMode> playMode,
        ReplayAssetRoute *out) noexcept {
    if (out == nullptr) {
        return ReplayAssetRouteResult::MissingOutput;
    }
    *out = ReplayAssetRoute{};
    const std::string_view mapPackName = MapPackName(mapEnvironment);
    if (mapPackName.empty()) {
        return ReplayAssetRouteResult::UnsupportedMapEnvironment;
    }
    const std::string_view decorationPackName =
            MapPackName(decorationEnvironment);
    if (decorationPackName.empty()) {
        return ReplayAssetRouteResult::UnsupportedDecorationEnvironment;
    }
    const std::string_view vehiclePackName = VehiclePackName(vehicleModel);
    if (vehiclePackName.empty()) {
        return ReplayAssetRouteResult::UnsupportedVehicleIdentifier;
    }
    if (!playMode.has_value()) {
        return ReplayAssetRouteResult::MissingPlayMode;
    }
    const std::optional<ReplayValidationMode> validationMode =
            ValidationMode(*playMode);
    if (!validationMode.has_value()) {
        return ReplayAssetRouteResult::UnsupportedPlayMode;
    }

    out->mapEnvironment = mapEnvironment;
    out->decorationEnvironment = decorationEnvironment;
    out->vehicleModel = vehicleModel;
    out->playMode = *playMode;
    out->validationMode = *validationMode;
    out->mapPackName = mapPackName;
    out->decorationPackName = decorationPackName;
    out->vehiclePackName = vehiclePackName;
    return ReplayAssetRouteResult::Success;
}

ReplayAssetRouteResult BuildReplayAssetRoute(
        ReplayMapEnvironment mapEnvironment,
        ReplayVehicleModel vehicleModel,
        std::optional<EChallengePlayMode> playMode,
        ReplayAssetRoute *out) noexcept {
    return BuildReplayAssetRoute(mapEnvironment,
                                 mapEnvironment,
                                 vehicleModel,
                                 playMode,
                                 out);
}

ReplayAssetRouteResult BuildReplayAssetRoute(
        const ReplayFile &replay,
        ReplayAssetRoute *out) noexcept {
    return BuildReplayAssetRoute(
            replay.MapEnvironment(),
            DecodeReplayMapEnvironment(
                    replay.MapInput().DecorationCollection().Name()),
            replay.VehicleModel(),
            replay.ChallengeMetadata().playMode,
            out);
}

const char *ReplayValidationModeName(ReplayValidationMode mode) noexcept {
    switch (mode) {
    case ReplayValidationMode::Race: return "Race";
    case ReplayValidationMode::Platform: return "Platform";
    case ReplayValidationMode::Puzzle: return "Puzzle";
    case ReplayValidationMode::Stunts: return "Stunts";
    }
    return "Unknown";
}

const char *ReplayAssetRouteResultName(
        ReplayAssetRouteResult result) noexcept {
    switch (result) {
    case ReplayAssetRouteResult::Success: return "success";
    case ReplayAssetRouteResult::MissingOutput: return "missing output";
    case ReplayAssetRouteResult::UnsupportedMapEnvironment:
        return "unsupported map environment";
    case ReplayAssetRouteResult::UnsupportedDecorationEnvironment:
        return "unsupported decoration environment";
    case ReplayAssetRouteResult::UnsupportedVehicleIdentifier:
        return "unsupported vehicle identifier";
    case ReplayAssetRouteResult::MissingPlayMode: return "missing play mode";
    case ReplayAssetRouteResult::UnsupportedPlayMode:
        return "unsupported play mode";
    }
    return "unknown route result";
}
