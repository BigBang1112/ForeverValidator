// CHmsItem physics methods used by CSceneVehicleCar force hooks and
// static/map item construction.

#include "engine/physics/dynamics/hms_item.h"
#include "engine/core/cmw_nod.h"
#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_dyna.h"
#include "engine/physics/world/hms_zone.h"
#include "engine/scene/plug_physical_object.h"
#include "engine/scene/plug_solid.h"
#include "engine/scene/scene_mobil.h"
CHmsItem::CHmsItem(void) = default;

CHmsItem::CCallback::~CCallback(void) = default;

CHmsItem::~CHmsItem(void) {
    computeForcesCallback = nullptr;
    afterContactsCallback = nullptr;
    absorbContactCallback = nullptr;
    CSceneMobil *owner = sceneMobilOwner;
    sceneMobilOwner = nullptr;
    if (owner != nullptr) {
        owner->OnPhysicsItemDestroyed(*this);
    }
    while (!corpuses.empty()) {
        CHmsCorpus *corpus = corpuses.back();
        if (corpus != nullptr && corpus->Item() == this) {
            corpus->DetachFromWorld();
        } else {
            corpuses.pop_back();
        }
    }
    SetSolid(nullptr);
}

void CHmsItem::CallbackSet(ECallback type, CCallback *callback) {
    switch (type) {
    case ECallback_ComputeForces:
        computeForcesCallback = callback == nullptr
                ? nullptr
                : dynamic_cast<CCallbackComputeForces *>(callback);
        return;
    case ECallback_AfterContacts:
        afterContactsCallback = callback == nullptr
                ? nullptr
                : dynamic_cast<CCallbackAfterContacts *>(callback);
        return;
    case ECallback_AbsorbContact:
        absorbContactCallback = callback == nullptr
                ? nullptr
                : dynamic_cast<CCallbackAbsorbContact *>(callback);
        return;
    case ECallback_Count:
        return;
    }
}

CHmsItem::CCallback *CHmsItem::CallbackGet(ECallback type) const {
    switch (type) {
    case ECallback_ComputeForces:
        return computeForcesCallback;
    case ECallback_AfterContacts:
        return afterContactsCallback;
    case ECallback_AbsorbContact:
        return absorbContactCallback;
    case ECallback_Count:
        return nullptr;
    }
    return nullptr;
}

void CHmsItem::RunComputeForcesCallback(float dt) {
    if (computeForcesCallback != nullptr) {
        computeForcesCallback->ComputeForces(this, dt);
    }
}

void CHmsItem::RunAfterContactsCallback(void) {
    if (afterContactsCallback != nullptr) {
        afterContactsCallback->AfterContacts(this);
    }
}

int CHmsItem::WantsAbsorbContactCallback(void) const {
    return absorbContactCallback != nullptr ? 1 : 0;
}

int CHmsItem::RunAbsorbContactCallback(CHmsPhysicalContact *contact) {
    if (absorbContactCallback == nullptr || contact == nullptr) {
        return 0;
    }
    absorbContactCallback->AbsorbContact(this, *contact);
    return 1;
}

void CHmsItem::SetProperties(const Properties &newProperties) {
    properties = newProperties;
}

const CHmsItem::Properties &CHmsItem::GetProperties(void) const {
    return properties;
}

void CHmsItem::SetSceneMobilOwner(CSceneMobil *owner) {
    sceneMobilOwner = owner;
}

CSceneMobil *CHmsItem::SceneMobilOwner(void) const {
    return sceneMobilOwner;
}

void CHmsItem::ApplyBlockMobilDefaults(void) {
    SetLightEmitter(1);
    SetIsCollisionStatic(1);
    SetIsVisionStatic(1);
    SetCollisionGroup(ECollisionGroup_Static);
    SetCountShadowTexCasted(0u, 1);
    SetShadowCasterGroupMask(2u);
}

void CHmsItem::SetContactInterest(EContactInterest value) {
    properties.contactInterest = value;
}

void CHmsItem::SetIsCollisionStatic(int enabled) {
    properties.collisionStatic = enabled != 0;
}

void CHmsItem::SetIsKinematicOnly(int enabled) {
    properties.kinematicOnly = enabled != 0;
}

void CHmsItem::SetIsVisionStatic(int enabled) {
    const bool visionStatic = enabled != 0;
    if (properties.visionStatic != visionStatic) {
        properties.visionStatic = visionStatic;
        UpdateCorpusCat();
    }
}

void CHmsItem::SetIsForcePointDynamicCollisionResponse(int enabled) {
    properties.forcePointDynamicCollisionResponse = enabled != 0;
}

void CHmsItem::SetShadowCasterGroupMask(unsigned long mask) {
    properties.shadowCasterGroupMask = static_cast<u32>(mask) & 0xfffu;
    if (properties.shadowTexCastedCount != 0u &&
        properties.shadowCasterGroupMask == 0u) {
        properties.shadowCasterGroupMask = 1u;
    }
}

void CHmsItem::SetShadowReceiverGroupMask(unsigned long mask) {
    properties.shadowReceiverGroupMask = static_cast<u32>(mask);
}

void CHmsItem::SetShadowFakeEnable(int enabled) {
    properties.shadowFakeEnabled = enabled != 0;
}

void CHmsItem::SetLightLensFlareEnable(int enabled) {
    properties.lightLensFlareEnabled = enabled != 0;
}

void CHmsItem::SetOccluderForLightMap(int enabled) {
    properties.occluderForLightMap = enabled != 0;
}

void CHmsItem::SetCountShadowTexCasted(uint8_t count, int enabled) {
    const bool shadowEnabled = enabled != 0;
    if (properties.shadowTexCastedCount != count ||
        properties.shadowTexCastedEnabled != shadowEnabled) {
        properties.shadowTexCastedCount = count;
        properties.shadowTexCastedEnabled = shadowEnabled;
        if (count != 0u && properties.shadowCasterGroupMask == 0u) {
            properties.shadowCasterGroupMask = 1u;
        }
        UpdateCorpusCat();
    }
}

void CHmsItem::SetCollisionGroup(ECollisionGroup group) {
    if (group != properties.collisionGroup) {
        const u32 count = static_cast<u32>(this->corpuses.size());
        std::vector<CHmsZone *> corpusZones(count, nullptr);
        for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
            CHmsCorpus *corpus = CorpusAt(corpusIndex);
            if (corpus != nullptr && corpus->OwningZone() != nullptr) {
                corpusZones[corpusIndex] = corpus->OwningZone();
                corpusZones[corpusIndex]->RemoveCorpus(corpus);
            }
        }

        properties.collisionGroup = group;

        for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
            CHmsCorpus *corpus = CorpusAt(corpusIndex);
            if (corpus != nullptr && corpusZones[corpusIndex] != nullptr) {
                corpusZones[corpusIndex]->AddCorpus(corpus);
            }
        }
    }
}

void CHmsItem::SetDynamicType(EDynamicType dynamicType) {
    if (properties.dynamicType != dynamicType) {
        properties.dynamicType = dynamicType;
        UpdateCorpusCat();
    }
}

void CHmsItem::SetIsBackground(int enabled) {
    properties.background = enabled != 0;
    UpdateCorpusCat();
}

EHmsCorpusCat CHmsItem::GetCorpusCat() {
    if (properties.background) {
        properties.shadowTexCastedCount = 0u;
        properties.shadowTexCastedEnabled = true;
        properties.visionStatic = false;
        return EHmsCorpusCat_Background;
    }
    if (properties.active) {
        return properties.visionStatic
                       ? EHmsCorpusCat_StaticVision
                       : EHmsCorpusCat_Dynamic;
    }
    return properties.visionStatic
                   ? EHmsCorpusCat_Static
                   : EHmsCorpusCat_Dormant;
}

CHmsCorpus *CHmsItem::CorpusAt(u32 index) const {
    return corpuses[index];
}

u32 CHmsItem::CorpusCount(void) const {
    return static_cast<u32>(corpuses.size());
}

bool CHmsItem::HasPortals(void) const {
    return !portals.empty();
}

CHmsCorpus *CHmsItem::GetCorpus(
        const CHmsZone *zone) const {
    const u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->OwningZone() == zone) {
            return corpus;
        }
    }
    return nullptr;
}

unsigned long CHmsItem::GetCorpusIndex(
        const CHmsZone *zone) const {
    const u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->OwningZone() == zone) {
            return static_cast<unsigned long>(corpusIndex);
        }
    }
    return InvalidEngineIndex;
}

void CHmsCorpus::SetLocation(
        const GmIso4 &location) {
    CHmsCorpus *corpus = this;
    corpus->localIso = location;
    if (corpus->Dynamics() != nullptr) {
        corpus->Dynamics()->SetLocation(location);
    }
}

void CHmsCorpus::SetTranslation(const GmVec3 &translation) {
    localIso.SetTranslation(translation);
    if (dyna != nullptr) {
        dyna->SetTranslation(translation);
    }
}

void CHmsCorpus::RotateOf(const GmMat3 &rotation) {
    if (dyna != nullptr) {
        dyna->RotateOf(rotation);
        return;
    }

    GmMat3 rotated;
    rotated.SetMult(rotation, localIso.RotationMatrix());
    rotated.OrthoNormalize();

    GmIso4 location;
    location.Set(rotated, localIso.Translation());
    SetLocation(location);
}

void CHmsCorpus::RefreshFromSolid(void) {
    CHmsCorpus *corpus = this;
    if (corpus->Dynamics() == nullptr || corpus->Item() == nullptr ||
        corpus->Item()->Solid() == nullptr) {
        return;
    }
    const CPlugPhysicalObject &physical = corpus->Item()->Solid()->Physical();
    CHmsDynaParams &params = corpus->Dynamics()->Parameters();
    params.mass = physical.Parameters().mass;
    params.bodyInertiaLike = physical.Parameters().impulseInertia;
    params.linearDampingScale = physical.Parameters().linearFluidFriction;
    params.angularDampingScale = physical.Parameters().physicalResponseCoefA;
    params.maxStepDistance = physical.Parameters().physicalResponseCoefB;
    params.forceScale = physical.Parameters().vehicleContactFeedbackScale;
    params.localCenterOfMass = physical.Parameters().localCenterOfMass;
}

void CHmsItem::RefreshCorpusesFromSolid(void) {
    for (CHmsCorpus *corpus : corpuses) {
        if (corpus != nullptr) {
            corpus->RefreshFromSolid();
        }
    }
}

void CHmsItem::SetSolid(CPlugSolid *solid) {
    if (solid == this->solid.Get()) {
        return;
    }
    this->solid.MwSetNod(solid);
    RefreshCorpusesFromSolid();
}

CPlugSolid *CHmsItem::Solid(void) const {
    return solid.Get();
}

void CHmsItem::SetLocation(
        const GmIso4 &location,
        const CHmsZone *zone) {
    const unsigned long corpusIndex = GetCorpusIndex(zone);
    const u32 count = static_cast<u32>(corpuses.size());
    CHmsCorpus *primaryCorpus = CorpusAt((u32)corpusIndex);

    if (count > 1u) {
        GmIso4 inversePrimaryLocation;
        GmIso4 relativeLocation;
        const GmIso4 *primaryLocation =
                primaryCorpus->GetLocation();
        inversePrimaryLocation.SetInverse(*primaryLocation);
        relativeLocation.SetMult(location, inversePrimaryLocation);
        GmMat3 relativeRotation = relativeLocation.RotationMatrix();
        relativeRotation.OrthoNormalize();
        const GmVec3 relativeTranslation = relativeLocation.Translation();
        relativeLocation.Set(
                relativeRotation,
                relativeTranslation);

        for (u32 otherCorpusIndex = 0;
             otherCorpusIndex < count;
             otherCorpusIndex++) {
            if (otherCorpusIndex != (u32)corpusIndex) {
                CHmsCorpus *corpus = CorpusAt(otherCorpusIndex);
                GmIso4 corpusLocation = relativeLocation;
                const GmIso4 *currentLocation =
                        corpus->GetLocation();
                corpusLocation.Mult(*currentLocation);
                corpus->SetLocation(corpusLocation);
            }
        }
    }

    primaryCorpus->SetLocation(location);
}

void CHmsItem::SetLightEmitter(int enabled) {
    const bool lightEmitter = enabled != 0;
    if (properties.lightEmitter != lightEmitter) {
        properties.lightEmitter = lightEmitter;
        const u32 count = static_cast<u32>(this->corpuses.size());
        for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
            CHmsCorpus *corpus = CorpusAt(corpusIndex);
            if (corpus != nullptr && corpus->OwningZone() != nullptr) {
                corpus->OwningZone()->CorpusChangeLightEmitter(corpus, enabled);
            }
        }
    }
}

void CHmsItem::VisibleIdSet(const SPlugVisibleId &visibleId) {
    const u32 value = visibleId.Value();
    SetLightEmitter((value & 1u) != 0u);
}

CHmsDyna *CHmsItem::FirstDyna(void) const {
    if (corpuses.empty()) {
        return 0;
    }

    CHmsCorpus *corpus = CorpusAt(0);
    return corpus->Dynamics();
}

const CHmsZone *CHmsItem::GetZone(unsigned long index) {
    return CorpusAt(static_cast<u32>(index))->OwningZone();
}

void CHmsItem::GetLinearSpeed(GmVec3 &out) const {
    const CHmsItem *item = this;
    CHmsDyna *dyna = item->FirstDyna();
    if (dyna == 0) {
        out = GmVec3::Zero();
        return;
    }
    dyna->GetLocalLinearSpeed(out);
}

void CHmsItem::GetAngularSpeed(GmVec3 &out) const {
    const CHmsItem *item = this;
    CHmsDyna *dyna = item->FirstDyna();
    if (dyna == 0) {
        out = GmVec3::Zero();
        return;
    }
    dyna->GetLocalAngularSpeed(out);
}

void CHmsItem::GetForce(GmVec3 &out) {
    CHmsItem *item = this;
    CHmsDyna *dyna = item->FirstDyna();
    if (dyna == 0) {
        out = GmVec3::Zero();
        return;
    }
    dyna->GetLocalForce(out);
}

void CHmsItem::SetLinearSpeed(const GmVec3 &speed) {
    u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->Dynamics() != 0) {
            corpus->Dynamics()->SetLocalLinearSpeed(speed);
        }
    }
}

void CHmsItem::SetAngularSpeed(const GmVec3 &speed) {
    u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->Dynamics() != 0) {
            corpus->Dynamics()->SetLocalAngularSpeed(speed);
        }
    }
}

void CHmsItem::SetForce(const GmVec3 &force) {
    u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->Dynamics() != 0) {
            corpus->Dynamics()->SetLocalForce(force);
        }
    }
}

void CHmsItem::SetTorque(const GmVec3 &torque) {
    u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->Dynamics() != 0) {
            corpus->Dynamics()->SetLocalTorque(torque);
        }
    }
}

void CHmsItem::AddForce(const GmVec3 &force) {
    u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->Dynamics() != 0) {
            corpus->Dynamics()->AddLocalForce(force);
        }
    }
}

void CHmsItem::AddTorque(const GmVec3 &torque) {
    u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->Dynamics() != 0) {
            corpus->Dynamics()->AddLocalTorque(torque);
        }
    }
}

void CHmsItem::AddForce(const GmVec3 &force,
                                                  const GmVec3 &point) {
    u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->Dynamics() != 0) {
            corpus->Dynamics()->AddLocalForce(force, point);
        }
    }
}

void CHmsItem::AddImpulse(const GmVec3 &impulse,
                                                    const GmVec3 &point) {
    u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->Dynamics() != 0) {
            corpus->Dynamics()->AddLocalImpulse(impulse, point);
        }
    }
}

void CHmsItem::AddImpulse(const GmVec3 &impulse) {
    u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = CorpusAt(corpusIndex);
        if (corpus->Dynamics() != 0) {
            corpus->Dynamics()->AddLocalImpulse(impulse);
        }
    }
}
