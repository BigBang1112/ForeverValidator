#pragma once

#include "engine/core/mw_id.h"
struct SGameCtnIdentifier {
    CMwId id;
    CMwId collection;
    CMwId author;

    SGameCtnIdentifier(void);
    SGameCtnIdentifier(const SGameCtnIdentifier &other);
    ~SGameCtnIdentifier(void);
    SGameCtnIdentifier &operator=(const SGameCtnIdentifier &other) = default;
    int operator==(const SGameCtnIdentifier &other) const;
};
