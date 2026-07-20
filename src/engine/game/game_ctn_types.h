#pragma once

#include <cstdint>

enum class EChallengePlayMode : std::uint32_t {
    Race = 0u,
    Platform = 1u,
    Puzzle = 2u,
    Crazy = 3u,
    Shortcut = 4u,
    Stunts = 5u,
};

// Original engine-wide track geometry unit retained by its authoritative
// symbol name.
extern const float SQUARE_HEIGHT;
