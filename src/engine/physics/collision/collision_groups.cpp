// Collision-group membership, pair scheduling, and static-tree construction.

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include "engine/core/cmw_nod.h"
#include "engine/physics/geometry/geometry_helpers.h"
#include "engine/physics/collision/gm_collision_buffer.h"
#include "engine/physics/collision/hms_collision.h"
#include "engine/physics/collision/hms_collision_manager.h"
#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_dyna.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/physics/world/hms_zone.h"
#include "engine/physics/geometry/physics_tolerances.h"
#include "engine/scene/plug_solid.h"
#include "engine/physics/geometry/plug_surface.h"
#include "engine/rendering/plug_tree.h"
// The engine starts with five enabled collision group pairs.
static const CHmsItem::SHmsCollisionGroupPair
        k_defaultCollisionGroupPairs[CHmsCollisionManager_GroupCount] = {
    {3u, 1u, 0u, 0u, 1u},
    {2u, 4u, 0u, 1u, 0u},
    {3u, 3u, 1u, 1u, 1u},
    {3u, 4u, 1u, 1u, 1u},
    {1u, 5u, 0u, 1u, 0u},
};

const CHmsItem::SHmsCollisionGroupPair *CHmsItem::DefaultCollisionGroupPairs(
        u32 *countOut) {
    if (countOut != 0) {
        *countOut = CHmsCollisionManager_GroupCount;
    }
    return k_defaultCollisionGroupPairs;
}

CHmsCollisionManager::SGroup::SAgainstGroup::SAgainstGroup(void) = default;

void CHmsCollisionManager::SGroup::SAgainstGroup::Reset(void) {
    targetGroup = nullptr;
    collisionGroupPair = nullptr;
    collisionSchedule.Clear();
}

CHmsCollisionManager::SGroup *
CHmsCollisionManager::SGroup::SAgainstGroup::TargetGroup(void) const {
    return targetGroup;
}

const CHmsItem::SHmsCollisionGroupPair *
CHmsCollisionManager::SGroup::SAgainstGroup::CollisionGroupPair(void) const {
    return collisionGroupPair;
}

int CHmsCollisionManager::SGroup::SAgainstGroup::TargetsGroup(
        const SGroup *group) const {
    return targetGroup == group;
}

u32 CHmsCollisionManager::SGroup::SAgainstGroup::TargetGroupNonStaticCorpusCount(
        void) const {
    return targetGroup != 0 ? targetGroup->NonStaticCorpusCount() : 0u;
}

int CHmsCollisionManager::SGroup::SAgainstGroup::HasStaticTargetTrees(void) const {
    return targetGroup != 0 && targetGroup->StaticTreeCount() > 1u;
}

CHmsCollisionManager::SGroup::SAgainstGroup::~SAgainstGroup(void) = default;

void CHmsCollisionManager::SGroup::Reset(void) {
    allCorpuses.clear();
    movingCorpuses.clear();
    againstGroups.clear();
    staticTrees.Reset();
    groupIndex = 0u;
    disabled = false;
}

CHmsCollisionManager::SGroup::SAgainstGroup &
CHmsCollisionManager::SGroup::AgainstGroupAt(uint32_t index) {
    return againstGroups[index];
}

const CHmsCollisionManager::SGroup::SAgainstGroup &
CHmsCollisionManager::SGroup::AgainstGroupAt(uint32_t index) const {
    return againstGroups[index];
}

CHmsCollisionManager::SGroup::SAgainstGroup *
CHmsCollisionManager::SGroup::FindAgainstGroupTargeting(const SGroup *target) {
    for (SAgainstGroup &against : againstGroups) {
        if (against.TargetsGroup(target)) {
            return &against;
        }
    }
    return nullptr;
}

const CHmsCollisionManager::SGroup::SAgainstGroup *
CHmsCollisionManager::SGroup::FindAgainstGroupTargeting(const SGroup *target) const {
    for (const SAgainstGroup &against : againstGroups) {
        if (against.TargetsGroup(target)) {
            return &against;
        }
    }
    return nullptr;
}

void CHmsCollisionManager::SGroup::RegisterCollisionPairsFor(
        CHmsCorpus &corpus) {
    for (SAgainstGroup &against : againstGroups) {
        SGroup *target = against.TargetGroup();
        if (target == nullptr) {
            continue;
        }
        for (const MovingCorpusState &targetState : target->movingCorpuses) {
            against.collisionSchedule.Include(corpus, *targetState.corpus);
        }

        SAgainstGroup *reciprocal = target->FindAgainstGroupTargeting(this);
        if (reciprocal == nullptr) {
            continue;
        }
        for (const MovingCorpusState &sourceState : target->movingCorpuses) {
            reciprocal->collisionSchedule.Include(*sourceState.corpus, corpus);
        }
    }
}

void CHmsCollisionManager::SGroup::UnregisterCollisionPairsFor(
        CHmsCorpus &corpus) {
    for (SAgainstGroup &against : againstGroups) {
        against.collisionSchedule.RemoveSource(corpus);
        SGroup *target = against.TargetGroup();
        if (target == nullptr) {
            continue;
        }
        if (SAgainstGroup *reciprocal =
                    target->FindAgainstGroupTargeting(this)) {
            reciprocal->collisionSchedule.RemoveTarget(corpus);
        }
    }
}

u32 CHmsCollisionManager::SGroup::AllCorpusCount(void) const {
    return static_cast<u32>(allCorpuses.size());
}

u32 CHmsCollisionManager::SGroup::NonStaticCorpusCount(void) const {
    return static_cast<u32>(movingCorpuses.size());
}

CHmsCorpus *CHmsCollisionManager::SGroup::NonStaticCorpusAt(
        u32 index) const {
    return movingCorpuses[index].corpus;
}

u32 CHmsCollisionManager::SGroup::AgainstGroupCount(void) const {
    return static_cast<u32>(againstGroups.size());
}

u32 CHmsCollisionManager::SGroup::StaticTreeCount(void) const {
    return staticTrees.GetCount();
}

int CHmsCollisionManager::SGroup::HasAgainstGroups(void) const {
    return !againstGroups.empty();
}

void CHmsCollisionManager::SGroup::ComputeNonStaticCorpusInfos() {
    if (disabled != 0) {
        return;
    }

    for (MovingCorpusState &state : movingCorpuses) {
        GmVec3 speed = {0.0f, 0.0f, 0.0f};
        if (state.corpus->Dynamics() != nullptr) {
            state.corpus->Dynamics()->GetLinearSpeed(speed);
        }
        state.linearSpeedSquared = speed.LengthSquaredForCollision();
    }
}

void CHmsCollisionManager::SGroup::AddNonStaticCorpus(CHmsCorpus *corpus) {
    movingCorpuses.push_back({corpus, 0.0f});
    RegisterCollisionPairsFor(*corpus);
}

void CHmsCollisionManager::SGroup::AddCorpus(CHmsCorpus *corpus) {
    allCorpuses.push_back(corpus);
    AddNonStaticCorpus(corpus);
}


void CHmsCollisionManager::SGroup::ClearAllStatic() {
    for (CHmsCorpus *corpus : allCorpuses) {
        const auto moving = std::find_if(
                movingCorpuses.begin(),
                movingCorpuses.end(),
                [corpus](const MovingCorpusState &state) {
                    return state.corpus == corpus;
                });
        if (moving == movingCorpuses.end()) {
            AddNonStaticCorpus(corpus);
        }
    }
    staticTrees.Reset();
}

void CHmsCollisionManager::SGroup::AddStaticSurfacesFromTree(
        CHmsCorpus *corpus,
        CPlugTree *tree,
        const GmIso4 &parentIso,
        std::vector<SColOctreeCell> &staticSurfaceRecords) {
    if (!tree->HasWorldBox()) {
        return;
    }

    GmIso4 iso;
    if (tree->HasLocalTransform()) {
        iso.SetMult(tree->LocalIso(), parentIso);
    } else {
        iso = parentIso;
    }

    u32 childCount = tree->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        AddStaticSurfacesFromTree(
            corpus,
            tree->GetChild(childIndex),
            iso,
            staticSurfaceRecords);
    }

    CPlugSurface *surface = tree->Surface();
    if (surface == 0) {
        return;
    }
    if (!surface->AllowsStaticCollisionRecordAppend()) {
        return;
    }

    GmBoxAligned bounds;
    bounds.SetMult(surface->GeomBox(), iso);
    staticSurfaceRecords.push_back(SColOctreeCell::Surface(
            bounds, iso, *surface, *tree, *corpus));
}

GmAxis GmBoxAligned::LongestAxis(void) const {
    GmAxis axis = GmAxis::X;
    float longest = halfExtents.x;
    if (halfExtents.y > longest) {
        axis = GmAxis::Y;
        longest = halfExtents.y;
    }
    if (halfExtents.z > longest) {
        axis = GmAxis::Z;
    }
    return axis;
}

void CHmsCollisionManager::SGroup::RemoveNonStaticCorpus(CHmsCorpus *corpus) {
    const auto found = std::find_if(
            movingCorpuses.begin(),
            movingCorpuses.end(),
            [corpus](const MovingCorpusState &state) {
                return state.corpus == corpus;
            });
    if (found == movingCorpuses.end()) {
        return;
    }

    UnregisterCollisionPairsFor(*corpus);
    *found = movingCorpuses.back();
    movingCorpuses.pop_back();
}

void CHmsCollisionManager::SGroup::RemoveCorpus(CHmsCorpus *corpus) {
    if (corpus == nullptr) {
        return;
    }
    const auto corpusIt = std::find(allCorpuses.begin(), allCorpuses.end(), corpus);
    if (corpusIt == allCorpuses.end()) {
        return;
    }
    allCorpuses.erase(corpusIt);

    const auto moving = std::find_if(
            movingCorpuses.begin(),
            movingCorpuses.end(),
            [corpus](const MovingCorpusState &state) {
                return state.corpus == corpus;
            });
    if (moving != movingCorpuses.end()) {
        RemoveNonStaticCorpus(corpus);
    } else {
        UpdateStaticCollisionTrees(1);
    }
}


void CHmsCollisionManager::SGroup::UpdateStaticCollisionTrees(int mode) {
    (void)mode;

    ClearAllStatic();

    std::vector<SColOctreeCell> staticSurfaceRecords;
    for (CHmsCorpus *corpus : allCorpuses) {
        if (!corpus->Item()->HasStaticCollisionTree()) {
            continue;
        }

        RemoveNonStaticCorpus(corpus);

        GmIso4 corpusIso = *corpus->VirtualLocationIso();
        AddStaticSurfacesFromTree(
            corpus,
            corpus->CollisionTree(),
            corpusIso,
            staticSurfaceRecords);
    }

    staticTrees.Build(staticSurfaceRecords, 1, 0u, 0u, 0.0f);
}


void CHmsCollisionManager::SGroup::ComputeIsToPerformCollisions() {
    if (disabled != 0) {
        return;
    }

    for (SAgainstGroup &against : againstGroups) {
        SGroup &other = *against.targetGroup;
        for (u32 sourceCorpusIndex = 0u;
             sourceCorpusIndex < movingCorpuses.size();
             ++sourceCorpusIndex) {
            MovingCorpusState &source = movingCorpuses[sourceCorpusIndex];
            for (u32 targetCorpusIndex = 0u;
                 targetCorpusIndex < other.movingCorpuses.size();
                 ++targetCorpusIndex) {
                const MovingCorpusState &target =
                        other.movingCorpuses[targetCorpusIndex];
                int perform = 1;
                if (!other.disabled) {
                    const float diff = source.linearSpeedSquared -
                                       target.linearSpeedSquared;
                    float epsilon = PhysicsTolerance::CollisionSpeedSquared;
                    perform = 0;
                    if (diff > epsilon) {
                        perform = 1;
                    } else if (diff <= epsilon) {
                        if ((groupIndex == other.groupIndex &&
                             targetCorpusIndex != sourceCorpusIndex) ||
                            (groupIndex < other.groupIndex)) {
                            perform = 1;
                        }
                    }
                }
                against.collisionSchedule.SetEnabled(
                        *source.corpus, *target.corpus, perform != 0);
            }
        }
    }
}

CHmsCollisionManager::SGroup::SGroup(void) = default;

CHmsCollisionManager::SGroup::~SGroup(void) = default;
