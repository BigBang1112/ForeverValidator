// Collision contact storage and sphere-contact consolidation.

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
void CHmsCollisionBuffer::Reset(u32 minimumCapacity) {
    collisions.clear();
    collisions.reserve(minimumCapacity);
}

void CHmsCollisionBuffer::Clear(void) {
    collisions.clear();
}

GmCollision &CHmsCollisionBuffer::GetCollision(unsigned long index) {
    return collisions[index];
}

GmCollision &CHmsCollisionBuffer::AddCollision(void) {
    return AddPhysicalCollision();
}

unsigned long CHmsCollisionBuffer::GetCurrentCount(void) {
    return static_cast<unsigned long>(PhysicalCollisionCount());
}

SHmsPhysicalCollision &CHmsCollisionBuffer::AddPhysicalCollision(void) {
    collisions.emplace_back();
    return collisions.back();
}

void CHmsCollisionBuffer::AddPhysicalCollision(
        const SHmsPhysicalCollision &collision) {
    collisions.push_back(collision);
}

u32 CHmsCollisionBuffer::PhysicalCollisionCount() const {
    return static_cast<u32>(collisions.size());
}

SHmsPhysicalCollision *CHmsCollisionBuffer::PhysicalCollisionAtOrNull(
        u32 index) {
    if (index >= collisions.size()) {
        return nullptr;
    }
    return &collisions[index];
}

const SHmsPhysicalCollision *CHmsCollisionBuffer::PhysicalCollisionAtOrNull(
        u32 index) const {
    if (index >= collisions.size()) {
        return nullptr;
    }
    return &collisions[index];
}

CHmsCollisionBuffer::CHmsCollisionBuffer(void) {
    collisions.reserve(0x32u);
}

CHmsCollisionBuffer::~CHmsCollisionBuffer(void) = default;

SHmsSphereBufferContact::SHmsSphereBufferContact(void) = default;

void SHmsSphereBufferContact::MergeAndAddToCollisions(
        CHmsCollisionBuffer &target) {
    CHmsCollisionBuffer &source = *this;
    u32 sourceCount = source.PhysicalCollisionCount();
    u32 firstTargetCollision = target.PhysicalCollisionCount();

    for (u32 sourceIndex = 0; sourceIndex < sourceCount; sourceIndex++) {
        SHmsPhysicalCollision *collision =
                source.PhysicalCollisionAtOrNull(sourceIndex);
        if (collision != nullptr && collision->sphereMergePrimary != 0) {
            target.AddPhysicalCollision(*collision);
        }
    }

    u32 targetCountAfterFlagged = target.PhysicalCollisionCount();
    for (u32 sourceIndex = 0; sourceIndex < sourceCount; sourceIndex++) {
        SHmsPhysicalCollision *collision =
                source.PhysicalCollisionAtOrNull(sourceIndex);
        if (collision == nullptr || collision->sphereMergePrimary != 0) {
            continue;
        }

        u32 targetIndex = firstTargetCollision;
        for (; targetIndex < targetCountAfterFlagged; targetIndex++) {
            SHmsPhysicalCollision *targetCollision =
                    target.PhysicalCollisionAtOrNull(targetIndex);
            if (targetCollision == nullptr ||
                collision->extraNegated.IsNearlyEqual(targetCollision->extraNegated) ||
                PhysicsTolerance::SphereContactNormalAlignment <
                    collision->impulseNormal.Dot(targetCollision->impulseNormal)) {
                break;
            }
        }

        if (targetIndex == targetCountAfterFlagged) {
            target.AddPhysicalCollision(*collision);
        }
    }

    this->queuedForMerge = false;
    source.Clear();
}
