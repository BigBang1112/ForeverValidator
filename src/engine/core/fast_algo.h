#pragma once

struct CFastAlgo {
    static unsigned long ComputeHashSize(unsigned long value);
    static unsigned long ComputeHashVal(
            const char *text,
            unsigned long *lengthOut);
    static unsigned long ComputeHashVal(
            const wchar_t *text,
            unsigned long *lengthOut);
};
