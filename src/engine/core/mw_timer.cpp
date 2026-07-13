#include "engine/core/mw_timer.h"
#include <chrono>
#include <cstdint>

const unsigned long &CMwTimer::GetTickTime(void) const {
    return tickTime_;
}

void CMwTimer::Reset(void) {
    tickTime_ = 0ul;
}

void CMwTimer::SetTickTime(unsigned long tickTime) {
    tickTime_ = static_cast<unsigned long>(static_cast<std::uint32_t>(
            tickTime));
}

unsigned long CMwTimerAdapter::GetSchemePeriod(void) const {
    return schemePeriod_;
}

const unsigned long &CMwTimerAdapter::GetTickTime(void) const {
    return timer_.GetTickTime();
}

void CMwTimerAdapter::Reset(void) {
    schemePeriod_ = 0ul;
    timer_.Reset();
}

void CMwTimerAdapter::ConfigureSimulation(unsigned long schemePeriod,
                                          unsigned long tickTime) {
    schemePeriod_ = static_cast<unsigned long>(
            static_cast<std::uint32_t>(schemePeriod));
    timer_.SetTickTime(tickTime);
}

void CMwProfiler_GetTimeStamp_MThreadSafe(long long &outStamp) {
    using Clock = std::chrono::steady_clock;
    using Nanoseconds = std::chrono::nanoseconds;

    outStamp = std::chrono::duration_cast<Nanoseconds>(
            Clock::now().time_since_epoch()).count();
}
