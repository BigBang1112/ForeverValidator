#pragma once

#include "engine/core/engine_types.h"

class StaticSolidArchiveId {
public:
    StaticSolidArchiveId()
            : index_(UINT32_MAX) {
    }

    static StaticSolidArchiveId Invalid() {
        return StaticSolidArchiveId(UINT32_MAX);
    }

    static StaticSolidArchiveId FromIndex(
            u32 index) {
        return StaticSolidArchiveId(index);
    }

    int IsValid() const {
        return index_ != UINT32_MAX;
    }

    int MatchesIndex(u32 index) const {
        return index_ == index;
    }

    int Matches(StaticSolidArchiveId other) const {
        return other.MatchesIndex(index_);
    }

    int FitsWithin(u32 selectedPayloadCount) const {
        return IsValid() && index_ < selectedPayloadCount;
    }

    u32 Index() const {
        return index_;
    }

private:
    explicit StaticSolidArchiveId(u32 index)
            : index_(index) {
    }

    u32 index_;
};
