#include "engine/physics/collision/hms_collision_manager.h"
#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_force_field.h"
#include "engine/physics/world/hms_zone.h"
#include "engine/scene/scene_vehicle_water_zone.h"
#include <new>

static const float CHmsZoneDynamic_DefaultDampingCoef = 1.0f;

CHmsZone::CHmsZone(void) = default;

CHmsZone::~CHmsZone(void) {
    while (CHmsCorpus *corpus = FirstCorpusOrNull()) {
        CHmsZone::RemoveCorpus(corpus);
    }
    while (HasForceFields()) {
        RemoveField(LastForceField());
    }
}

CHmsCollisionManagerSZone *CHmsZone::GetCollisionZone(void) {
    return nullptr;
}

CSceneVehicleWaterZone *CHmsZone::WaterZone() {
    CHmsCollisionManagerSZone *collisionZone = GetCollisionZone();
    return collisionZone != nullptr ? &collisionZone->waterZone : nullptr;
}

const CSceneVehicleWaterZone *CHmsZone::WaterZone() const {
    return const_cast<CHmsZone *>(this)->WaterZone();
}

bool CHmsZone::AppendWaterPlaneEq(const GmVec4 &plane) {
    for (const GmVec4 &candidate : waterPlaneEqs_) {
        if (candidate.PlaneEqIsNearlyEqual(plane, 0.0f, 0.0f)) {
            return true;
        }
    }
    try {
        waterPlaneEqs_.push_back(plane);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

std::size_t CHmsZone::WaterPlaneEqCount(void) const {
    return waterPlaneEqs_.size();
}

const GmVec4 *CHmsZone::WaterPlaneEqAt(std::size_t index) const {
    return index < waterPlaneEqs_.size() ? &waterPlaneEqs_[index] : nullptr;
}

CHmsCollisionManagerSZone *CHmsZoneDynamic::GetCollisionZone(void) {
    return collisionManagerZone_;
}

void CHmsZoneDynamic::BindCollisionManager(
        CHmsCollisionManagerSZone &managerZone) {
    collisionManagerZone_ = &managerZone;
}

void CHmsZoneDynamic::UnbindCollisionManager(
        const CHmsCollisionManagerSZone &managerZone) {
    if (collisionManagerZone_ == &managerZone) {
        collisionManagerZone_ = nullptr;
    }
}

CHmsZoneDynamic::CHmsZoneDynamic(void) = default;

CHmsZoneDynamic::~CHmsZoneDynamic(void) {
    while (CHmsCorpus *corpus = FirstCorpusOrNull()) {
        RemoveCorpus(corpus);
    }
    if (collisionManagerZone_ != nullptr) {
        collisionManagerZone_->DetachDynamicZone(*this);
    }
}

void CHmsZoneDynamic::ResetSimulationState(void) {
    while (CHmsCorpus *corpus = FirstCorpusOrNull()) {
        RemoveCorpus(corpus);
    }
    while (HasForceFields()) {
        RemoveField(LastForceField());
    }
    linearDampingCoef_ = CHmsZoneDynamic_DefaultDampingCoef;
    angularDampingCoef_ = CHmsZoneDynamic_DefaultDampingCoef;
    dynamicCorpuses_.clear();
    zombieCorpuses_.clear();
    collisionBuffer_.Reset(0u);
    if (collisionManagerZone_ != nullptr) {
        collisionManagerZone_->DetachDynamicZone(*this);
    }
}

void CHmsZoneDynamic::SetCollisionManagerZone(
        CHmsCollisionManagerSZone &managerZone) {
    if (collisionManagerZone_ == &managerZone) {
        managerZone.AttachDynamicZone(*this);
        return;
    }
    if (collisionManagerZone_ != nullptr && FirstCorpusOrNull() != nullptr) {
        return;
    }
    if (collisionManagerZone_ != nullptr) {
        collisionManagerZone_->DetachDynamicZone(*this);
    }
    managerZone.AttachDynamicZone(*this);
}
