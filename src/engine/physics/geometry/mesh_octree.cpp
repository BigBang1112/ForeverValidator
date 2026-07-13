#include "engine/physics/geometry/gm_surface.h"
#include <bitset>
#include <utility>
#include <vector>

namespace {

enum class SpatialPartition {
    Overlapping,
    Below,
    Above,
};

struct OctantMembership {
    std::bitset<8> octants;
    u32 sourceIndex = 0u;
};

std::vector<SMeshOctreeCell> CopyCells(
        u32 count,
        const SMeshOctreeCell *source) {
    return count == 0u
            ? std::vector<SMeshOctreeCell>{}
            : std::vector<SMeshOctreeCell>(source, source + count);
}

GmBoxAligned SpanBounds(const std::vector<SMeshOctreeCell> &cells) {
    GmBoxAligned bounds = cells.front().Bounds();
    for (u32 index = 1u; index < cells.size(); ++index) {
        bounds.Union(cells[index].Bounds());
    }
    return bounds;
}

GmBoxAligned OctantBounds(const GmBoxAligned &bounds, u32 octant) {
    const float minX = bounds.center.x - bounds.halfExtents.x;
    const float minY = bounds.center.y - bounds.halfExtents.y;
    const float minZ = bounds.center.z - bounds.halfExtents.z;
    const float maxX = bounds.center.x + bounds.halfExtents.x;
    const float maxY = bounds.center.y + bounds.halfExtents.y;
    const float maxZ = bounds.center.z + bounds.halfExtents.z;

    const float childMinX = (octant & 4u) != 0u ? bounds.center.x : minX;
    const float childMaxX = (octant & 4u) != 0u ? maxX : bounds.center.x;
    const float childMinY = (octant & 2u) != 0u ? bounds.center.y : minY;
    const float childMaxY = (octant & 2u) != 0u ? maxY : bounds.center.y;
    const float childMinZ = (octant & 1u) != 0u ? bounds.center.z : minZ;
    const float childMaxZ = (octant & 1u) != 0u ? maxZ : bounds.center.z;

    return {
        {(childMinX + childMaxX) * 0.5f,
         (childMinY + childMaxY) * 0.5f,
         (childMinZ + childMaxZ) * 0.5f},
        {(childMaxX - childMinX) * 0.5f,
         (childMaxY - childMinY) * 0.5f,
         (childMaxZ - childMinZ) * 0.5f},
    };
}

bool ShouldSplit(const GmBoxAligned &bounds,
                 u32 sourceCount,
                 u32 depth,
                 u32 maxDepth,
                 u32 minLeafCount,
                 float volumeThreshold) {
    if (maxDepth != 0u && depth >= maxDepth) {
        return false;
    }
    if (minLeafCount != 0u && sourceCount <= minLeafCount) {
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

SpatialPartition PartitionRelativeTo(
        const SMeshOctreeCell &cell,
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
        std::vector<SMeshOctreeCell> &remainingCells,
        std::vector<SpatialPartition> &remainingPartitions,
        SpatialPartition selectedPartition,
        std::vector<SMeshOctreeCell> &selected) {
    selected.clear();
    size_t index = 0u;
    while (index < remainingCells.size()) {
        if (remainingPartitions[index] != selectedPartition) {
            ++index;
            continue;
        }

        selected.push_back(remainingCells[index]);
        if (index + 1u < remainingCells.size()) {
            remainingCells[index] = remainingCells.back();
            remainingPartitions[index] = remainingPartitions.back();
        }
        remainingCells.pop_back();
        remainingPartitions.pop_back();
    }
}

} // namespace

SMeshOctreeCell SMeshOctreeCell::Branch(const GmBoxAligned &bounds) {
    SMeshOctreeCell cell;
    cell.bounds_ = bounds;
    cell.subtreeEntryCount_ = 1u;
    cell.triangleIndex_.reset();
    return cell;
}

SMeshOctreeCell SMeshOctreeCell::Triangle(
        const GmBoxAligned &bounds,
        u32 triangleIndex) {
    SMeshOctreeCell cell;
    cell.bounds_ = bounds;
    cell.subtreeEntryCount_ = 1u;
    cell.triangleIndex_ = triangleIndex;
    return cell;
}

u32 GmOctree<SMeshOctreeCell>::AppendInternal(void) {
    const u32 index = static_cast<u32>(cells.size());
    cells.push_back(SMeshOctreeCell::Branch(
            {{0.0f, 0.0f, 0.0f}, {-1.0f, -1.0f, -1.0f}}));
    return index;
}

void GmOctree<SMeshOctreeCell>::AppendLeaf(
        const SMeshOctreeCell &source) {
    cells.push_back(SMeshOctreeCell::Triangle(
            source.Bounds(), source.TriangleIndex()));
}

u32 GmOctree<SMeshOctreeCell>::BuildOctreeNodes(
        const std::vector<SMeshOctreeCell> &sourceCells) {
    if (sourceCells.empty()) {
        return 0u;
    }

    const GmBoxAligned spanBounds = SpanBounds(sourceCells);
    if (!cells.empty()) {
        cells.back().SetBounds(spanBounds);
    }

    std::vector<OctantMembership> memberships(sourceCells.size());
    for (u32 index = 0u; index < sourceCells.size(); ++index) {
        memberships[index].sourceIndex = index;
    }

    for (u32 octant = 0u; octant < 8u; ++octant) {
        const GmBoxAligned childBounds = OctantBounds(spanBounds, octant);
        for (u32 sourceIndex = 0u;
             sourceIndex < sourceCells.size();
             ++sourceIndex) {
            if (sourceCells[sourceIndex].Bounds().TestInter(childBounds)) {
                memberships[sourceIndex].octants.set(octant);
            }
        }
    }

    std::vector<SMeshOctreeCell> selected;
    u32 emittedSpanCount = 1u;
    for (u32 octant = 0u; octant < 8u; ++octant) {
        selected.clear();

        size_t membershipIndex = 0u;
        while (membershipIndex < memberships.size()) {
            const OctantMembership membership = memberships[membershipIndex];
            if (membership.octants.count() != 1u ||
                !membership.octants.test(octant)) {
                ++membershipIndex;
                continue;
            }

            selected.push_back(sourceCells[membership.sourceIndex]);
            if (membershipIndex + 1u < memberships.size()) {
                memberships[membershipIndex] = memberships.back();
            }
            memberships.pop_back();
        }

        if (!selected.empty()) {
            const u32 internalIndex = AppendInternal();
            const u32 childCount = BuildOctreeNodes(selected);
            cells[internalIndex].SetSubtreeEntryCount(childCount);
            emittedSpanCount += childCount;
        }
    }

    for (const OctantMembership &membership : memberships) {
        AppendLeaf(sourceCells[membership.sourceIndex]);
        ++emittedSpanCount;
    }
    return emittedSpanCount;
}

u32 GmOctree<SMeshOctreeCell>::BuildOctreeRecurse(
        u32 sourceCount,
        SMeshOctreeCell *sourceCells) {
    return BuildOctreeNodes(CopyCells(sourceCount, sourceCells));
}

u32 GmOctree<SMeshOctreeCell>::BuildBintreeNodes(
        const std::vector<SMeshOctreeCell> &sourceCells,
        u32 depth,
        u32 maxDepth,
        u32 minLeafCount,
        float volumeThreshold) {
    if (sourceCells.empty()) {
        return 0u;
    }

    const GmBoxAligned spanBounds = SpanBounds(sourceCells);
    if (!cells.empty()) {
        cells.back().SetBounds(spanBounds);
    }

    if (!ShouldSplit(spanBounds,
                     static_cast<u32>(sourceCells.size()),
                     depth,
                     maxDepth,
                     minLeafCount,
                     volumeThreshold)) {
        for (const SMeshOctreeCell &source : sourceCells) {
            AppendLeaf(source);
        }
        return static_cast<u32>(sourceCells.size()) + 1u;
    }

    const GmAxis axis = spanBounds.LongestAxis();
    const float splitPlane = spanBounds.center.Component(axis);
    std::vector<SMeshOctreeCell> remainingCells = sourceCells;
    std::vector<SpatialPartition> remainingPartitions;
    remainingPartitions.reserve(sourceCells.size());
    for (const SMeshOctreeCell &source : sourceCells) {
        remainingPartitions.push_back(
                PartitionRelativeTo(source, axis, splitPlane));
    }

    const size_t originalCount = remainingCells.size();
    std::vector<SMeshOctreeCell> selected;
    u32 emittedSpanCount = 1u;
    for (const SpatialPartition partition :
         {SpatialPartition::Below, SpatialPartition::Above}) {
        ExtractPartitionByTailRemove(
                remainingCells,
                remainingPartitions,
                partition,
                selected);
        if (selected.size() == 1u) {
            AppendLeaf(selected.front());
            ++emittedSpanCount;
        } else if (selected.size() > 1u) {
            const u32 internalIndex = AppendInternal();
            const u32 childCount = BuildBintreeNodes(
                    selected,
                    depth + 1u,
                    maxDepth,
                    minLeafCount,
                    volumeThreshold);
            cells[internalIndex].SetSubtreeEntryCount(childCount);
            emittedSpanCount += childCount;
        }
    }

    if (remainingCells.size() != originalCount &&
        remainingCells.size() > 1u) {
        const u32 internalIndex = AppendInternal();
        const u32 childCount = BuildBintreeNodes(
                remainingCells,
                depth + 1u,
                maxDepth,
                minLeafCount,
                volumeThreshold);
        cells[internalIndex].SetSubtreeEntryCount(childCount);
        emittedSpanCount += childCount;
    } else {
        for (const SMeshOctreeCell &source : remainingCells) {
            AppendLeaf(source);
        }
        emittedSpanCount += static_cast<u32>(remainingCells.size());
    }
    return emittedSpanCount;
}

u32 GmOctree<SMeshOctreeCell>::BuildBintreeRecurse(
        u32 sourceCount,
        SMeshOctreeCell *sourceCells,
        u32 depth,
        u32 maxDepth,
        u32 minLeafCount,
        float volumeThreshold) {
    return BuildBintreeNodes(
            CopyCells(sourceCount, sourceCells),
            depth,
            maxDepth,
            minLeafCount,
            volumeThreshold);
}

void GmOctree<SMeshOctreeCell>::Build(
        u32 sourceCount,
        SMeshOctreeCell *sourceCells,
        int useBintree,
        u32 maxDepth,
        u32 minLeafCount,
        float volumeThreshold) {
    cells.clear();
    AppendInternal();

    const std::vector<SMeshOctreeCell> sources =
            CopyCells(sourceCount, sourceCells);
    if (useBintree != 0) {
        BuildBintreeNodes(
                sources, 0u, maxDepth, minLeafCount, volumeThreshold);
    } else {
        BuildOctreeNodes(sources);
    }
    cells.front().SetSubtreeEntryCount(static_cast<u32>(cells.size()));
}

const std::vector<SMeshOctreeCell> &
GmOctree<SMeshOctreeCell>::Cells(void) const {
    return cells;
}

std::vector<SMeshOctreeCell>
GmOctree<SMeshOctreeCell>::TakeCells(void) {
    return std::move(cells);
}
