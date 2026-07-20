#pragma once

#include <optional>
#include <string_view>

#include "engine/game/game_ctn_types.h"
#include "format/replay/replay_challenge_metadata.h"
#include "validation/evaluation/replay_validation_mode.h"

class ReplayFile;

enum class ReplayAssetRouteResult {
    Success,
    MissingOutput,
    UnsupportedMapEnvironment,
    UnsupportedDecorationEnvironment,
    UnsupportedVehicleIdentifier,
    MissingPlayMode,
    UnsupportedPlayMode,
};

struct ReplayAssetRoute {
    ReplayMapEnvironment mapEnvironment = ReplayMapEnvironment::Unknown;
    ReplayMapEnvironment decorationEnvironment =
            ReplayMapEnvironment::Unknown;
    ReplayVehicleModel vehicleModel = ReplayVehicleModel::Unknown;
    EChallengePlayMode playMode = EChallengePlayMode::Race;
    ReplayValidationMode validationMode = ReplayValidationMode::Race;
    std::string_view mapPackName;
    std::string_view decorationPackName;
    std::string_view vehiclePackName;
};

ReplayAssetRouteResult BuildReplayAssetRoute(
        ReplayMapEnvironment mapEnvironment,
        ReplayMapEnvironment decorationEnvironment,
        ReplayVehicleModel vehicleModel,
        std::optional<EChallengePlayMode> playMode,
        ReplayAssetRoute *out) noexcept;

ReplayAssetRouteResult BuildReplayAssetRoute(
        ReplayMapEnvironment mapEnvironment,
        ReplayVehicleModel vehicleModel,
        std::optional<EChallengePlayMode> playMode,
        ReplayAssetRoute *out) noexcept;

ReplayAssetRouteResult BuildReplayAssetRoute(
        const ReplayFile &replay,
        ReplayAssetRoute *out) noexcept;

const char *ReplayValidationModeName(ReplayValidationMode mode) noexcept;
const char *ReplayAssetRouteResultName(ReplayAssetRouteResult result) noexcept;
