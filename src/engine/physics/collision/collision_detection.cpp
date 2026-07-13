// Dynamic and static collision-tree traversal.

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
void CHmsCollisionManager::SZone::DetectCollisionBetween(
        CHmsCorpus *a,
        CHmsCorpus *b) {
    activeCorpusA = a;
    activeCorpusB = b;
    SPlugTreeLocatedPair pair = {
        a->CollisionTree(),
        a->LocationIso(),
        b->CollisionTree(),
        b->LocationIso(),
    };
    this->ComputeCollision(pair);
}

int CHmsCollisionManager::SZone::ComputeCollisionTree1RootOnly(
        const SPlugTreeLocatedPair &pairRef,
        const GmBoxAligned &boxARef) {
    CHmsCollisionManagerSZone *zone = this;
    const SPlugTreeLocatedPair *pair = &pairRef;
    const GmBoxAligned *boxA = &boxARef;
    CPlugTree *treeA = pair->treeA;
    CPlugTree *treeB = pair->treeB;
    if (!treeB->AllowsSurfaceCollision()) {
        return 0;
    }

    GmBoxAligned boxB;
    treeB->GetTransformedCollisionBox(*pair->isoB, boxB);
    if (!boxB.TestInter(*boxA)) {
        return 0;
    }

    GmIso4 isoB;
    treeB->ComposeCollisionIso(*pair->isoB, isoB);

    int hit = 0;
    CPlugSurface *surfaceA = treeA->Surface();
    CPlugSurface *surfaceB = treeB->Surface();
    if (surfaceA != 0 && surfaceB != 0) {
        hit = zone->ComputeSurfaceCollisionAndTag(treeA, pair->isoA, surfaceA,
                                                  treeB, &isoB, surfaceB);
    }

    u32 childCount = treeB->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        SPlugTreeLocatedPair childPair = {treeA, pair->isoA, treeB->GetChild(childIndex), &isoB};
        if (zone->ComputeCollisionTree1RootOnly(childPair, *boxA)) {
            hit = 1;
        }
    }
    return hit;
}

int CHmsCollisionManager::SZone::ComputeCollisionTree2RootOnly(
        const SPlugTreeLocatedPair &pairRef,
        const GmBoxAligned &boxBRef) {
    CHmsCollisionManagerSZone *zone = this;
    const SPlugTreeLocatedPair *pair = &pairRef;
    const GmBoxAligned *boxB = &boxBRef;
    CPlugTree *treeA = pair->treeA;
    CPlugTree *treeB = pair->treeB;
    if (!treeA->AllowsSurfaceCollision()) {
        return 0;
    }

    GmBoxAligned boxA;
    treeA->GetTransformedCollisionBox(*pair->isoA, boxA);
    if (!boxA.TestInter(*boxB)) {
        return 0;
    }

    GmIso4 isoA;
    treeA->ComposeCollisionIso(*pair->isoA, isoA);

    int hit = 0;
    CPlugSurface *surfaceA = treeA->Surface();
    CPlugSurface *surfaceB = treeB->Surface();
    if (surfaceA != 0 && surfaceB != 0) {
        hit = zone->ComputeSurfaceCollisionAndTag(treeA, &isoA, surfaceA,
                                                  treeB, pair->isoB, surfaceB);
    }

    u32 childCount = treeA->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        SPlugTreeLocatedPair childPair = {treeA->GetChild(childIndex), &isoA, treeB, pair->isoB};
        if (zone->ComputeCollisionTree2RootOnly(childPair, *boxB)) {
            hit = 1;
        }
    }
    return hit;
}

int CHmsCollisionManager::SZone::ComputeCollision(
        const SPlugTreeLocatedPair &pairRef) {
    CHmsCollisionManagerSZone *zone = this;
    const SPlugTreeLocatedPair *pair = &pairRef;
    CPlugTree *treeA = pair->treeA;
    CPlugTree *treeB = pair->treeB;
    if (!treeA->AllowsSurfaceCollision() || !treeB->HasWorldBox()) {
        return 0;
    }

    GmBoxAligned boxA;
    GmBoxAligned boxB;
    treeA->GetTransformedCollisionBox(*pair->isoA, boxA);
    treeB->GetTransformedCollisionBox(*pair->isoB, boxB);
    if (!boxA.TestInter(boxB)) {
        return 0;
    }

    GmIso4 isoA;
    GmIso4 isoB;
    treeA->ComposeCollisionIso(*pair->isoA, isoA);
    treeB->ComposeCollisionIso(*pair->isoB, isoB);

    int hit = 0;
    CPlugSurface *surfaceA = treeA->Surface();
    CPlugSurface *surfaceB = treeB->Surface();
    if (surfaceA != 0 && surfaceB != 0) {
        hit = zone->ComputeSurfaceCollisionAndTag(treeA, &isoA, surfaceA,
                                                  treeB, &isoB, surfaceB);
    }

    u32 childCountB = treeB->GetChildCount();
    if (surfaceA != 0) {
        for (u32 childIndexB = 0; childIndexB < childCountB; childIndexB++) {
            SPlugTreeLocatedPair childPair = {treeA, &isoA, treeB->GetChild(childIndexB), &isoB};
            if (zone->ComputeCollisionTree1RootOnly(childPair, boxA)) {
                hit = 1;
            }
        }
    }

    u32 childCountA = treeA->GetChildCount();
    if (surfaceB != 0) {
        for (u32 childIndexA = 0; childIndexA < childCountA; childIndexA++) {
            SPlugTreeLocatedPair childPair = {treeA->GetChild(childIndexA), &isoA, treeB, &isoB};
            if (zone->ComputeCollisionTree2RootOnly(childPair, boxB)) {
                hit = 1;
            }
        }
    }

    for (u32 childIndexA = 0; childIndexA < childCountA; childIndexA++) {
        for (u32 childIndexB = 0; childIndexB < childCountB; childIndexB++) {
            SPlugTreeLocatedPair childPair = {
                treeA->GetChild(childIndexA),
                &isoA,
                treeB->GetChild(childIndexB),
                &isoB};
            if (zone->ComputeCollision(childPair)) {
                hit = 1;
            }
        }
    }

    return hit;
}

void CHmsCollisionManager::SZone::DetectCollisionBetweenTreeAndStaticCollisionTree(
    const GmIso4 &movingIsoRef,
    const CPlugTree &movingTree) {
    CHmsCollisionManagerSZone *zone = this;
    const GmIso4 *movingIso = &movingIsoRef;
    if (!movingTree.HasWorldBox()) {
        return;
    }

    GmIso4 localIso;
    movingTree.ComposeCollisionIso(*movingIso, localIso);

    u32 childCount = movingTree.GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        zone->DetectCollisionBetweenTreeAndStaticCollisionTree(
            localIso,
            *movingTree.GetChild(childIndex));
    }

    CPlugSurface *movingSurface = movingTree.Surface();
    if (movingSurface == 0) {
        return;
    }
    CPlugTree *movingTreeNode = const_cast<CPlugTree *>(&movingTree);

    GmBoxAligned movingBox;
    movingTree.GetTransformedCollisionBox(*movingIso, movingBox);

    GmOctree<CHmsCollisionManagerSColOctreeCell> &staticTrees =
            zone->activeStaticTargetGroup->staticTrees;
    u32 staticTreeCount = staticTrees.GetCount();
    for (u32 staticTreeIndex = 0; staticTreeIndex < staticTreeCount;) {
        CHmsCollisionManagerSColOctreeCell *record = &staticTrees[staticTreeIndex];
        if (!movingBox.TestInter(record->Bounds())) {
            staticTreeIndex += record->SubtreeEntryCount();
            continue;
        }

        if (record->ContainsSurface() &&
            record->SurfaceData().tree->AllowsSurfaceCollision()) {
            const SColOctreeCell::StaticSurface &staticSurface =
                    record->SurfaceData();
            SHmsSphereBufferContact *sphereContact = 0;
            CHmsCollisionBuffer *buffer =
                zone->ChooseCollisionOutputBuffer(
                        movingTreeNode, movingSurface, &sphereContact);
            u32 firstNew = buffer->PhysicalCollisionCount();
            SPlugSurfaceLocatedPair surfacePair = {
                *movingSurface,
                localIso,
                *staticSurface.surface,
                staticSurface.location,
            };

            int surfaceCollisionResult =
                    CPlugSurface::ComputeCollision(surfacePair, *buffer);
            if (surfaceCollisionResult) {
                if (sphereContact != 0) {
                    zone->AddSphereContactOnce(sphereContact);
                }
                zone->TagNewStaticCollisions(
                        buffer, firstNew, movingTreeNode, record);
            }
        }

        staticTreeIndex++;
    }
}

void CHmsCollisionManager::SZone::DetectCollisionsCorpus(
        CHmsCollisionBuffer &collisionBuffer,
        CHmsCorpus *corpus) {
    activeCollisionBuffer = &collisionBuffer;

    u32 groupIndex = corpus->Item()->CollisionGroup();
    CHmsCollisionManagerSGroup *group = &groups[groupIndex - 1];

    for (CHmsCollisionManagerSAgainstGroup &againstEntry : group->againstGroups) {
        CHmsCollisionManagerSAgainstGroup *against = &againstEntry;
        activeCollisionGroupPair = against->collisionGroupPair;

        for (const SGroup::MovingCorpusState &target :
             against->targetGroup->movingCorpuses) {
            CHmsCorpus *other = target.corpus;
            if (against->collisionSchedule.IsEnabled(*corpus, *other)) {
                DetectCollisionBetween(corpus, other);
            }
        }

        activeStaticTargetGroup = against->targetGroup;
        if (against->targetGroup->StaticTreeCount() > 1u) {
            activeCorpusA = corpus;
            DetectCollisionBetweenTreeAndStaticCollisionTree(
                *corpus->LocationIso(),
                *corpus->CollisionTree());
        }
    }

    MergeQueuedSphereContacts(collisionBuffer);
}
