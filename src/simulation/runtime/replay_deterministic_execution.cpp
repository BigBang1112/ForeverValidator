#include "simulation/runtime/replay_deterministic_execution.h"
#include "engine/game/game_random_sequence.h"
#include "engine/core/mw_cmd_buffer_core.h"
#include "engine/resources/system_fid_parameters.h"
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace tmnf::simulation {
namespace {

constexpr unsigned int DeterministicMxcsr = 0x1f80u;
thread_local unsigned int ActiveExecutionScopeCount = 0u;

}  // namespace

DeterministicExecutionScope::DeterministicExecutionScope() {
    if (ActiveExecutionScopeCount != 0u ||
        CSystemFidParameters::ActiveScopeCount() != 0u) {
        return;
    }

    if (std::fegetenv(&savedEnvironment_) != 0) {
        return;
    }
    environmentCaptured_ = true;
    savedCommandBuffer_ = CMwCmdBufferCore::Current();
    savedRandomState_ = CaptureGameRandomState();
#if defined(__i386__) || defined(__x86_64__)
    savedMxcsr_ = _mm_getcsr();
#endif

    if (std::fesetround(FE_TONEAREST) != 0) {
        Restore();
        return;
    }
#if defined(__i386__) || defined(__x86_64__)
    _mm_setcsr(DeterministicMxcsr);
    if (_mm_getcsr() != DeterministicMxcsr) {
        Restore();
        return;
    }
#endif
    if (std::fegetround() != FE_TONEAREST) {
        Restore();
        return;
    }

    ResetGameRandomSequence();
    ++ActiveExecutionScopeCount;
    ownsActiveScope_ = true;
    established_ = true;
}

DeterministicExecutionScope::~DeterministicExecutionScope() {
    Restore();
}

bool DeterministicExecutionScope::Established() const noexcept {
    return established_ && !restored_;
}

bool DeterministicExecutionScope::Restore() noexcept {
    if (restored_) {
        return true;
    }
    restored_ = true;

    if (!environmentCaptured_ && !ownsActiveScope_) {
        return true;
    }

    bool success = true;
    if (CSystemFidParameters::ActiveScopeCount() != 0u) {
        CSystemFidParameters::StaticRelease();
        success = false;
    }

    if (environmentCaptured_) {
        RestoreGameRandomState(savedRandomState_);
        CMwCmdBufferCore::RestoreCurrent(savedCommandBuffer_);
        if (std::fesetenv(&savedEnvironment_) != 0) {
            success = false;
        }
#if defined(__i386__) || defined(__x86_64__)
        _mm_setcsr(savedMxcsr_);
        if (_mm_getcsr() != savedMxcsr_) {
            success = false;
        }
#endif
    }

    if (ownsActiveScope_) {
        if (ActiveExecutionScopeCount == 0u) {
            success = false;
        } else {
            --ActiveExecutionScopeCount;
        }
        ownsActiveScope_ = false;
    }
    established_ = false;
    return success;
}

bool DeterministicExecutionScope::IsActive() noexcept {
    return ActiveExecutionScopeCount != 0u;
}

}  // namespace tmnf::simulation
