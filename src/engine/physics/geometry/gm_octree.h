#pragma once

#include <vector>

#include "engine/core/engine_types.h"
#include "engine/core/gm_types.h"
template <typename Entry>
class GmOctree {
public:
    using Storage = std::vector<Entry>;

    u32 GetCount() const { return static_cast<u32>(entries_.size()); }
    bool Empty() const { return entries_.empty(); }
    Entry &operator[](u32 index) { return entries_[index]; }
    const Entry &operator[](u32 index) const { return entries_[index]; }
    const Storage &Entries() const { return entries_; }
    void Reset() { entries_.clear(); }

    void Build(const Storage &source,
               int useBintree,
               u32 maxDepth,
               u32 minLeafCount,
               float volumeThreshold);
    u32 BuildOctreeRecurse(const Storage &source);
    u32 BuildBintreeRecurse(const Storage &source,
                            u32 depth,
                            u32 maxDepth,
                            u32 minLeafCount,
                            float volumeThreshold);

private:
    u32 AppendInternal(const GmBoxAligned &box);
    void AppendLeaf(const Entry &source);

    Storage entries_;
};
