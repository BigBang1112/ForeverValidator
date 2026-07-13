#ifndef TMNF_REPLAY_FILE_H
#define TMNF_REPLAY_FILE_H

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "engine/game/replay_challenge_map_input.h"
#include "format/replay/replay_ghost_trajectory.h"
#include "format/replay/replay_input_timeline.h"
enum class ReplayFileReadError {
    Success,
    InvalidRequest,
    FileReadFailed,
    InvalidContainer,
    AllocationFailed,
    RootBodyDecompressionFailed,
    MissingGhostBuffer,
    MissingInputStream,
    TooManyInputActions,
    InvalidGhostMetadata,
    InvalidInputHeader,
    InvalidInputActions,
    InvalidInputEventHeader,
    InvalidInputEvents,
    InvalidInputTimeline,
    MissingGhostState,
    GhostStateDecompressionFailed,
    InvalidGhostState,
    GhostTrajectoryAllocationFailed,
    MissingEmbeddedChallenge,
    InvalidEmbeddedChallenge,
    InvalidMap,
};

struct ReplayGhostArchiveMetadata {
    std::size_t encodedStateByteCount = 0u;
    std::size_t encodedSampleByteCount = 0u;
};

class ReplayFile {
public:
    ReplayFile() = default;
    ReplayFile(const ReplayFile &) = delete;
    ReplayFile &operator=(const ReplayFile &) = delete;
    ReplayFile(ReplayFile &&) = default;
    ReplayFile &operator=(ReplayFile &&) = default;

    const ReplayInputTimeline &InputTimeline() const {
        return inputTimeline_;
    }

    const ReplayGhostTrajectory &GhostTrajectory() const {
        return ghostTrajectory_;
    }

    const CGameCtnReplayMapInput &MapInput() const {
        return mapInput_;
    }

    const ReplayGhostArchiveMetadata &GhostArchiveMetadata() const {
        return ghostArchiveMetadata_;
    }

private:
    friend ReplayFileReadError ReadReplayBytes(
            const std::uint8_t *bytes,
            std::size_t byteCount,
            ReplayFile *out);
    friend ReplayFileReadError ParseReplayStorage(
            const std::vector<std::uint8_t> &fileStorage,
            ReplayFile *out);
    friend ReplayFileReadError ParseReplayStorage(
            const std::vector<std::uint8_t> &fileStorage,
            ReplayFile *out);

    ReplayFile(ReplayInputTimeline inputTimeline,
               ReplayGhostTrajectory ghostTrajectory,
               ReplayGhostArchiveMetadata ghostArchiveMetadata,
               CGameCtnReplayMapInput mapInput)
        : inputTimeline_(std::move(inputTimeline)),
          ghostTrajectory_(std::move(ghostTrajectory)),
          ghostArchiveMetadata_(ghostArchiveMetadata),
          mapInput_(std::move(mapInput)) {}

    ReplayInputTimeline inputTimeline_;
    ReplayGhostTrajectory ghostTrajectory_;
    ReplayGhostArchiveMetadata ghostArchiveMetadata_;
    CGameCtnReplayMapInput mapInput_;
};

ReplayFileReadError ReadReplayBytes(
        const std::uint8_t *bytes,
        std::size_t byteCount,
        ReplayFile *out);
const char *ReplayFileReadErrorName(ReplayFileReadError error);

#endif
