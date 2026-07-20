#include "engine/physics/collision/hms_collision_manager.h"
#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_force_field.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/physics/world/hms_zone.h"
#include <algorithm>

void CHmsZone::CorpusChangeCat(
        unsigned long indexInCategory,
        EHmsCorpusCat oldCategory,
        EHmsCorpusCat newCategory) {
    corpusesByCategory_.ChangeCategory(
            {oldCategory, indexInCategory}, newCategory);
}

void CHmsZone::ChangeCorpusCategory(CHmsCorpus &corpus,
                                    EHmsCorpusCat newCategory) {
    const std::optional<CHmsCorpusCategoryStore::Membership> membership =
            corpusesByCategory_.Find(corpus);
    if (!membership.has_value() || membership->category == newCategory) {
        return;
    }
    CorpusChangeCat(membership->index, membership->category, newCategory);
}

void CHmsZone::CorpusChangeBuild(
        CHmsCorpus *corpus,
        int enabled) {
    if (corpus == nullptr) {
        return;
    }
    auto found = std::find(buildCorpuses_.begin(),
                           buildCorpuses_.end(),
                           corpus);
    if (enabled) {
        if (found == buildCorpuses_.end()) {
            buildCorpuses_.push_back(corpus);
        }
    } else if (found != buildCorpuses_.end()) {
        *found = buildCorpuses_.back();
        buildCorpuses_.pop_back();
    }
}

void CHmsZone::CorpusChangeLightEmitter(
        CHmsCorpus *corpus,
        int enabled) {
    if (corpus == nullptr) {
        return;
    }
    auto found = std::find(lightEmitterCorpuses_.begin(),
                           lightEmitterCorpuses_.end(),
                           corpus);
    if (enabled) {
        if (found == lightEmitterCorpuses_.end()) {
            lightEmitterCorpuses_.push_back(corpus);
        }
    } else if (found != lightEmitterCorpuses_.end()) {
        *found = lightEmitterCorpuses_.back();
        lightEmitterCorpuses_.pop_back();
    }
}

void CHmsZone::AddField(CHmsForceField *field) {
    if (field == nullptr) {
        return;
    }
    if (field->zone_ != nullptr && field->zone_ != this) {
        field->zone_->RemoveField(field);
    }
    if (std::find(forceFields_.begin(), forceFields_.end(), field) ==
        forceFields_.end()) {
        forceFields_.push_back(field);
    }
    field->zone_ = this;
}

void CHmsZone::RemoveField(CHmsForceField *field) {
    if (field == nullptr) {
        return;
    }
    auto found = std::find(forceFields_.begin(),
                           forceFields_.end(),
                           field);
    if (found != forceFields_.end()) {
        *found = forceFields_.back();
        forceFields_.pop_back();
    }
    if (field->zone_ == this) {
        field->zone_ = nullptr;
    }
}

void CHmsZoneDynamic::AddCorpus(CHmsCorpus *corpus) {
    if (corpus == nullptr || corpus->Item() == nullptr ||
        collisionManagerZone_ == nullptr) {
        return;
    }
    if (ContainsCorpus(*corpus)) {
        return;
    }
    if (corpus->OwningZone() != nullptr && corpus->OwningZone() != this) {
        corpus->OwningZone()->RemoveCorpus(corpus);
    }
    collisionManagerZone_->AddCorpus(corpus);

    if (corpus->Item()->IsZombieCorpus()) {
        zombieCorpuses_.push_back(corpus);
    } else if (corpus->Dynamics() != 0) {
        dynamicCorpuses_.push_back(corpus);
    }

    CHmsZone::AddCorpus(corpus);
}

void CHmsZone::AddCorpus(CHmsCorpus *corpus) {
    if (corpus == nullptr || corpus->Item() == nullptr ||
        ContainsCorpus(*corpus)) {
        return;
    }
    if (corpus->zone_ != nullptr && corpus->zone_ != this) {
        corpus->zone_->RemoveCorpus(corpus);
    }
    CHmsItem *item = corpus->Item();
    const EHmsCorpusCat corpusCat = item->GetCorpusCat();
    corpusesByCategory_.Add(*corpus, corpusCat);
    corpus->zone_ = this;
    Zone_UpdateWaterHeights(corpus);

    if (item->HasPortals()) {
        CorpusChangeBuild(corpus, 1);
    }
    if (item->GetProperties().lightEmitter) {
        CorpusChangeLightEmitter(corpus, 1);
    }
}

void CHmsZone::RemoveCorpus(CHmsCorpus *corpus) {
    if (corpus == nullptr) {
        return;
    }
    corpusesByCategory_.Remove(*corpus);

    CorpusChangeBuild(corpus, 0);
    CorpusChangeLightEmitter(corpus, 0);
    if (corpus->zone_ == this) {
        corpus->zone_ = nullptr;
    }
}

void CHmsZoneDynamic::RemoveCorpus(CHmsCorpus *corpus) {
    if (corpus == nullptr) {
        return;
    }
    if (collisionManagerZone_ != nullptr &&
        corpus->RegisteredCollisionManagerZone() ==
                collisionManagerZone_) {
        collisionManagerZone_->RemoveCorpus(corpus);
    }

    auto zombie = std::find(zombieCorpuses_.begin(),
                            zombieCorpuses_.end(),
                            corpus);
    if (zombie != zombieCorpuses_.end()) {
        *zombie = zombieCorpuses_.back();
        zombieCorpuses_.pop_back();
    }
    auto dynamic = std::find(dynamicCorpuses_.begin(),
                             dynamicCorpuses_.end(),
                             corpus);
    if (dynamic != dynamicCorpuses_.end()) {
        *dynamic = dynamicCorpuses_.back();
        dynamicCorpuses_.pop_back();
    }

    CHmsZone::RemoveCorpus(corpus);
}
