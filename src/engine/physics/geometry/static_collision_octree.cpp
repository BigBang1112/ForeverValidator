#include <bitset>
#include <vector>

#include "engine/physics/geometry/gm_octree.h"
#include "engine/physics/collision/hms_collision_manager.h"
namespace {

using Cell = CHmsCollisionManager::SColOctreeCell;

enum class SpatialPartition {
    Overlapping,
    Below,
    Above,
};

struct OctantMembership {
    std::bitset<8> octants;
    u32 sourceIndex = 0u;
};

GmBoxAligned SpanBounds(const std::vector<Cell> &records) {
    GmBoxAligned bounds = records.front().Bounds();
    for (size_t index = 1u; index < records.size(); ++index) {
        bounds.Union(records[index].Bounds());
    }
    return bounds;
}

bool ShouldSplit(const GmBoxAligned &bounds,
                 u32 count,
                 u32 depth,
                 u32 maxDepth,
                 u32 minLeafCount,
                 float volumeThreshold) {
    if (count <= 1u ||
        (maxDepth != 0u && depth >= maxDepth) ||
        (minLeafCount != 0u && count <= minLeafCount)) {
        return false;
    }
    if (volumeThreshold > 1e-5f) {
        const float widthX = bounds.halfExtents.x * 2.0f;
        const float widthY = bounds.halfExtents.y * 2.0f;
        const float widthZ = bounds.halfExtents.z * 2.0f;
        if (widthY * widthX * widthZ < volumeThreshold) {
            return false;
        }
    }
    const float extentSquared =
            bounds.halfExtents.x * bounds.halfExtents.x +
            bounds.halfExtents.y * bounds.halfExtents.y +
            bounds.halfExtents.z * bounds.halfExtents.z;
    return extentSquared != 0.0f;
}

GmBoxAligned OctantBounds(const GmBoxAligned &bounds, u32 octant) {
    const GmVec3 minimum = {
        bounds.center.x - bounds.halfExtents.x,
        bounds.center.y - bounds.halfExtents.y,
        bounds.center.z - bounds.halfExtents.z,
    };
    const GmVec3 maximum = {
        bounds.center.x + bounds.halfExtents.x,
        bounds.center.y + bounds.halfExtents.y,
        bounds.center.z + bounds.halfExtents.z,
    };
    const GmVec3 childMinimum = {
        (octant & 4u) != 0u ? bounds.center.x : minimum.x,
        (octant & 2u) != 0u ? bounds.center.y : minimum.y,
        (octant & 1u) != 0u ? bounds.center.z : minimum.z,
    };
    const GmVec3 childMaximum = {
        (octant & 4u) != 0u ? maximum.x : bounds.center.x,
        (octant & 2u) != 0u ? maximum.y : bounds.center.y,
        (octant & 1u) != 0u ? maximum.z : bounds.center.z,
    };
    return {
        {(childMinimum.x + childMaximum.x) * 0.5f,
         (childMinimum.y + childMaximum.y) * 0.5f,
         (childMinimum.z + childMaximum.z) * 0.5f},
        {(childMaximum.x - childMinimum.x) * 0.5f,
         (childMaximum.y - childMinimum.y) * 0.5f,
         (childMaximum.z - childMinimum.z) * 0.5f},
    };
}

SpatialPartition PartitionRelativeTo(
        const Cell &cell,
        GmAxis axis,
        float splitPlane) {
    const float center = cell.Bounds().center.Component(axis);
    const float halfExtent = cell.Bounds().halfExtents.Component(axis);
    if (center + halfExtent <= splitPlane) {
        return SpatialPartition::Below;
    }
    if (center - halfExtent >= splitPlane) {
        return SpatialPartition::Above;
    }
    return SpatialPartition::Overlapping;
}

void ExtractPartitionByTailRemove(
        std::vector<Cell> &remaining,
        std::vector<SpatialPartition> &partitions,
        SpatialPartition selectedPartition,
        std::vector<Cell> &selected) {
    selected.clear();
    size_t index = 0u;
    while (index < remaining.size()) {
        if (partitions[index] != selectedPartition) {
            ++index;
            continue;
        }
        selected.push_back(remaining[index]);
        remaining[index] = remaining.back();
        partitions[index] = partitions.back();
        remaining.pop_back();
        partitions.pop_back();
    }
}

} // namespace

CHmsCollisionManager::SColOctreeCell
CHmsCollisionManager::SColOctreeCell::Branch(
        const GmBoxAligned &bounds) {
    SColOctreeCell cell;
    cell.bounds_ = bounds;
    cell.subtreeEntryCount_ = 1u;
    cell.staticSurface_.reset();
    return cell;
}

CHmsCollisionManager::SColOctreeCell
CHmsCollisionManager::SColOctreeCell::Surface(
        const GmBoxAligned &bounds,
        const GmIso4 &location,
        CPlugSurface &surface,
        CPlugTree &tree,
        CHmsCorpus &corpus) {
    SColOctreeCell cell;
    cell.bounds_ = bounds;
    cell.subtreeEntryCount_ = 1u;
    cell.staticSurface_ = StaticSurface{location, &surface, &tree, &corpus};
    return cell;
}

template <> u32 GmOctree<Cell>::AppendInternal(const GmBoxAligned &bounds) {
    const u32 index = static_cast<u32>(entries_.size());
    entries_.push_back(Cell::Branch(bounds));
    return index;
}

template <> void GmOctree<Cell>::AppendLeaf(const Cell &source) {
    entries_.push_back(source);
    entries_.back().SetSubtreeEntryCount(1u);
}

template <>
u32 GmOctree<Cell>::BuildBintreeRecurse(
        const Storage &source,
        u32 depth,
        u32 maxDepth,
        u32 minLeafCount,
        float volumeThreshold) {
    if (source.empty()) {
        return 0u;
    }

    const GmBoxAligned spanBounds = SpanBounds(source);
    entries_.back().SetBounds(spanBounds);
    if (!ShouldSplit(spanBounds,
                     static_cast<u32>(source.size()),
                     depth,
                     maxDepth,
                     minLeafCount,
                     volumeThreshold)) {
        for (const Cell &record : source) {
            AppendLeaf(record);
        }
        return static_cast<u32>(source.size() + 1u);
    }

    const GmAxis axis = spanBounds.LongestAxis();
    const float splitPlane = spanBounds.center.Component(axis);
    Storage remaining = source;
    std::vector<SpatialPartition> partitions;
    partitions.reserve(source.size());
    for (const Cell &record : source) {
        partitions.push_back(PartitionRelativeTo(record, axis, splitPlane));
    }

    const size_t originalCount = remaining.size();
    Storage selected;
    u32 emittedSpanCount = 1u;
    for (const SpatialPartition partition :
         {SpatialPartition::Below, SpatialPartition::Above}) {
        ExtractPartitionByTailRemove(
                remaining, partitions, partition, selected);
        if (selected.size() == 1u) {
            AppendLeaf(selected.front());
            ++emittedSpanCount;
        } else if (selected.size() > 1u) {
            const u32 internalIndex = AppendInternal(SpanBounds(selected));
            const u32 childCount = BuildBintreeRecurse(
                    selected,
                    depth + 1u,
                    maxDepth,
                    minLeafCount,
                    volumeThreshold);
            entries_[internalIndex].SetSubtreeEntryCount(childCount);
            emittedSpanCount += childCount;
        }
    }

    if (remaining.size() != originalCount && remaining.size() > 1u) {
        const u32 internalIndex = AppendInternal(SpanBounds(remaining));
        const u32 childCount = BuildBintreeRecurse(
                remaining,
                depth + 1u,
                maxDepth,
                minLeafCount,
                volumeThreshold);
        entries_[internalIndex].SetSubtreeEntryCount(childCount);
        emittedSpanCount += childCount;
    } else {
        for (const Cell &record : remaining) {
            AppendLeaf(record);
        }
        emittedSpanCount += static_cast<u32>(remaining.size());
    }
    return emittedSpanCount;
}

template <> u32 GmOctree<Cell>::BuildOctreeRecurse(const Storage &source) {
    if (source.empty()) {
        return 0u;
    }

    const GmBoxAligned spanBounds = SpanBounds(source);
    entries_.back().SetBounds(spanBounds);
    std::vector<OctantMembership> memberships(source.size());
    for (u32 index = 0u; index < source.size(); ++index) {
        memberships[index].sourceIndex = index;
    }
    for (u32 octant = 0u; octant < 8u; ++octant) {
        const GmBoxAligned childBounds = OctantBounds(spanBounds, octant);
        for (u32 sourceIndex = 0u;
             sourceIndex < source.size();
             ++sourceIndex) {
            if (source[sourceIndex].Bounds().TestInter(childBounds)) {
                memberships[sourceIndex].octants.set(octant);
            }
        }
    }

    u32 emittedSpanCount = 1u;
    for (u32 octant = 0u; octant < 8u; ++octant) {
        Storage selected;
        size_t membershipIndex = 0u;
        while (membershipIndex < memberships.size()) {
            const OctantMembership membership = memberships[membershipIndex];
            if (membership.octants.count() != 1u ||
                !membership.octants.test(octant)) {
                ++membershipIndex;
                continue;
            }
            selected.push_back(source[membership.sourceIndex]);
            memberships[membershipIndex] = memberships.back();
            memberships.pop_back();
        }
        if (!selected.empty()) {
            const u32 internalIndex = AppendInternal(
                    OctantBounds(spanBounds, octant));
            const u32 childCount = BuildOctreeRecurse(selected);
            entries_[internalIndex].SetSubtreeEntryCount(childCount);
            emittedSpanCount += childCount;
        }
    }
    for (const OctantMembership &membership : memberships) {
        AppendLeaf(source[membership.sourceIndex]);
        ++emittedSpanCount;
    }
    return emittedSpanCount;
}

template <>
void GmOctree<Cell>::Build(
        const Storage &source,
        int useBintree,
        u32 maxDepth,
        u32 minLeafCount,
        float volumeThreshold) {
    entries_.clear();
    entries_.push_back(Cell::Branch({}));
    if (!source.empty()) {
        if (useBintree != 0) {
            BuildBintreeRecurse(
                    source, 0u, maxDepth, minLeafCount, volumeThreshold);
        } else {
            BuildOctreeRecurse(source);
        }
    }
    entries_.front().SetSubtreeEntryCount(
            static_cast<u32>(entries_.size()));
}
