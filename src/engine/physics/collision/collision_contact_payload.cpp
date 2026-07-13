#include "engine/physics/collision/hms_collision.h"
#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_dyna.h"
#include "engine/physics/dynamics/hms_item.h"

GmIso4 CHmsCorpus::LocationForCollisionResponse(void) const {
    if (dyna == 0) {
        return localIso;
    }
    GmIso4 location;
    location.Set(
            dyna->CurrentState().rotation,
            dyna->CurrentState().position);
    return location;
}

GmVec3 CHmsCorpus::TranslationForCollisionResponse(void) const {
    return LocationForCollisionResponse().TranslationForGmSurf();
}

GmVec3 CHmsCorpus::LocalImpulseNormalForCollisionResponse(
        const SHmsPhysicalCollision *collision,
        GmVec3 *beforeTranspose) const {
    if (!item->RequestsLocalContactPayloadForCollisionResponse()) {
        if (beforeTranspose != 0) {
            *beforeTranspose = GmVec3::Zero();
        }
        return GmVec3::Zero();
    }

    GmVec3 local = {
        (collision->impulseNormal.x),
        (collision->impulseNormal.y),
        (collision->impulseNormal.z),
    };
    if (beforeTranspose != 0) {
        *beforeTranspose = local;
    }
    local.MultTranspose(LocationForCollisionResponse().RotationMatrix());
    return local;
}

GmVec3 CHmsCorpus::LocalContactPointForCollisionResponse(
        const SHmsPhysicalCollision *collision,
        GmVec3 *beforeTranspose) const {
    if (!item->RequestsLocalContactPayloadForCollisionResponse()) {
        if (beforeTranspose != 0) {
            *beforeTranspose = GmVec3::Zero();
        }
        return GmVec3::Zero();
    }

    const GmVec3 translation = TranslationForCollisionResponse();
    GmVec3 local = {
        ((collision->contactPoint.x) - ( translation.x)),
        ((collision->contactPoint.y) - ( translation.y)),
        ((collision->contactPoint.z) - ( translation.z)),
    };
    if (beforeTranspose != 0) {
        *beforeTranspose = local;
    }
    local.MultTranspose(LocationForCollisionResponse().RotationMatrix());
    return local;
}

void CHmsPhysicalContact::InitForCollisionAResponse(const SHmsPhysicalCollision *collision) {
    GmVec3 localImpulseBefore;
    GmVec3 localPointBefore;
    self = collision->CorpusA();
    contactTree = collision->TreeA();
    ownMaterial = collision->materialA;
    localImpulseNormal =
        collision->CorpusA()->LocalImpulseNormalForCollisionResponse(
                collision, &localImpulseBefore);
    localContactPoint =
        collision->CorpusA()->LocalContactPointForCollisionResponse(
                collision, &localPointBefore);
    localRelativeSpeed = GmVec3::Zero();
    replacement = GmVec3::Zero();
    accepted = false;
    peer = collision->CorpusB();
    peerCollisionTree = collision->TreeB();
    peerMaterial = collision->materialB;
}

void CHmsPhysicalContact::InitForCollisionBResponse(const SHmsPhysicalCollision *collision) {
    GmVec3 localImpulseBefore;
    GmVec3 localPointBefore;
    self = collision->CorpusB();
    contactTree = collision->TreeB();
    ownMaterial = collision->materialB;
    localImpulseNormal =
        collision->CorpusB()->LocalImpulseNormalForCollisionResponse(
                collision, &localImpulseBefore);
    localContactPoint =
        collision->CorpusB()->LocalContactPointForCollisionResponse(
                collision, &localPointBefore);
    localRelativeSpeed = GmVec3::Zero();
    replacement = GmVec3::Zero();
    accepted = false;
    peer = collision->CorpusA();
    peerCollisionTree = collision->TreeA();
    peerMaterial = collision->materialA;
}

int CHmsCorpus::WantsAbsorbContactForCollisionResponse(void) const {
    return item != nullptr ? item->WantsAbsorbContactCallback() : 0;
}

int CHmsCorpus::RunAbsorbContactForCollisionResponse(
        CHmsPhysicalContact *contact) const {
    return item != nullptr ? item->RunAbsorbContactCallback(contact) : 0;
}

void CHmsPhysicalContact::SetLocalRelativeSpeedForCollisionResponse(
        CHmsCorpus *corpus,
        GmVec3 relSpeed) {
    if (corpus->Item()->RequestsLocalContactPayloadForCollisionResponse()) {
        relSpeed.MultTranspose(
                corpus->LocationForCollisionResponse().RotationMatrix());
        localRelativeSpeed = relSpeed;
    }
}
