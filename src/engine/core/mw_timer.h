#pragma once

class CMwTimer {
public:
    CMwTimer(void) = default;

    const unsigned long &GetTickTime(void) const;

    void Reset(void);
    void SetTickTime(unsigned long tickTime);

private:
    unsigned long tickTime_ = 0ul;
};

class CMwTimerAdapter {
public:
    CMwTimerAdapter(void) = default;

    unsigned long GetSchemePeriod(void) const;
    const unsigned long &GetTickTime(void) const;

    void Reset(void);
    void ConfigureSimulation(unsigned long schemePeriod,
                             unsigned long tickTime);

private:
    unsigned long schemePeriod_ = 0ul;
    CMwTimer timer_;
};

void CMwProfiler_GetTimeStamp_MThreadSafe(long long &outStamp);
