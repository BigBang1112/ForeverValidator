#pragma once

#include "engine/core/engine_types.h"
#include <cstdint>
#include <vector>

#include "engine/physics/collision/gm_collision_buffer.h"
#include "engine/core/gm_types.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/game/surface_material.h"
struct CHmsCorpus;
struct CPlugTree;

struct SHmsPhysicalCollision : GmCollision {
    int CompareForCollisionResponseSort(const SHmsPhysicalCollision &other) const;
    void NegateContactGeometryForBodyB();
    void BindCollisionActors(
            CHmsCorpus *corpusA,
            CPlugTree *sourceTreeA,
            CHmsCorpus *corpusB,
            CPlugTree *sourceTreeB,
            const CHmsItem::SHmsCollisionGroupPair *groupPair) {
        a = corpusA;
        treeA = sourceTreeA;
        b = corpusB;
        treeB = sourceTreeB;
        collisionGroupPair = groupPair;
    }
    CPlugTree *TreeA() const { return treeA; }
    CPlugTree *TreeB() const { return treeB; }
    CHmsCorpus *CorpusA(void) const { return a; }
    CHmsCorpus *CorpusB(void) const { return b; }
    const CHmsItem::SHmsCollisionGroupPair &CollisionGroupPair(void) const {
        return *collisionGroupPair;
    }

private:
    CHmsCorpus *a = nullptr;
    CPlugTree *treeA = nullptr;
    CHmsCorpus *b = nullptr;
    CPlugTree *treeB = nullptr;
    const CHmsItem::SHmsCollisionGroupPair *collisionGroupPair = nullptr;
};


struct CHmsCollisionBuffer : CGmCollisionBuffer {
    CHmsCollisionBuffer(void);
    ~CHmsCollisionBuffer(void);
    GmCollision &GetCollision(unsigned long index) override;
    GmCollision &AddCollision(void) override;
    unsigned long GetCurrentCount(void) override;

    void Reset(u32 minimumCapacity);
    void Clear(void);
    SHmsPhysicalCollision &AddPhysicalCollision(void);
    void AddPhysicalCollision(const SHmsPhysicalCollision &collision);
    u32 PhysicalCollisionCount() const;
    SHmsPhysicalCollision *PhysicalCollisionAtOrNull(u32 index);
    const SHmsPhysicalCollision *PhysicalCollisionAtOrNull(u32 index) const;
    void SortForCollisionResponse(void);
    void TransformMeshCollisionsToWorldForGmSurf(u32 firstNew, const GmIso4 &meshIso);
    void TransformEllipsoidMeshCollisionsToWorldForGmSurf(u32 firstNew,
                                                          const GmIso4 &unitContactToWorld,
                                                          const GmIso4 &unitNormalToWorld,
                                                          GmVec3 *firstNormalAfterMat3);

private:
    std::vector<SHmsPhysicalCollision> collisions;
};

struct SHmsSphereBufferContact : CHmsCollisionBuffer {
    SHmsSphereBufferContact(void);
    void MergeAndAddToCollisions(
            CHmsCollisionBuffer &target);
    bool queuedForMerge = false;
};



struct CHmsPhysicalContact {
    CHmsCorpus *self = nullptr;
    CPlugTree *contactTree = nullptr;
    EPlugSurfaceMaterialId ownMaterial = EPlugSurfaceMaterialId_Concrete;
    GmVec3 localImpulseNormal{};
    GmVec3 localContactPoint{};
    GmVec3 localRelativeSpeed{};
    GmVec3 replacement{};
    bool accepted = false;
    CHmsCorpus *peer = nullptr;
    CPlugTree *peerCollisionTree = nullptr;
    EPlugSurfaceMaterialId peerMaterial = EPlugSurfaceMaterialId_Concrete;
    void InitForCollisionAResponse(const SHmsPhysicalCollision *collision);
    void InitForCollisionBResponse(const SHmsPhysicalCollision *collision);
    void SetLocalRelativeSpeedForCollisionResponse(CHmsCorpus *corpus, GmVec3 relSpeed);
    CPlugTree *ContactTree() const {
        return contactTree;
    }
    CPlugTree *PeerCollisionTree() const {
        return peerCollisionTree;
    }
};
