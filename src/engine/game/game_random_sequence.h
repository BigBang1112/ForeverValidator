#ifndef TMNF_GAME_RANDOM_SEQUENCE_H
#define TMNF_GAME_RANDOM_SEQUENCE_H

#include <cstdint>

namespace tmnf::simulation {

std::uint32_t CaptureGameRandomState() noexcept;
void RestoreGameRandomState(std::uint32_t state) noexcept;
void ResetGameRandomSequence() noexcept;

}  // namespace tmnf::simulation

#endif
