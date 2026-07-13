// CHmsZoneDynamic force accumulation.

#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_force_field.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/physics/world/hms_zone.h"
#include "engine/physics/geometry/physics_tolerances.h"
CHmsForceField::CHmsForceField(void) = default;

CHmsForceField::~CHmsForceField(void) {
    if (zone_ != nullptr) {
        zone_->RemoveField(this);
    }
}

int CHmsForceField::GetValue(
        const GmVec3 &position,
        GmVec3 &out) const {
    (void)position;
    (void)out;
    return 0;
}

void CHmsForceField::SetZone(CHmsZone *zone,
                            const GmIso4 &location) {
    (void)location;
    if (zone_ == zone) {
        return;
    }
    if (zone_ != nullptr) {
        zone_->RemoveField(this);
    }
    if (zone != nullptr) {
        zone->AddField(this);
    }
}

CHmsZone *CHmsForceField::OwningZone(void) const {
    return zone_;
}

CHmsForceFieldUniform::CHmsForceFieldUniform(void)
        : hasValue(false),
          value{0.0f, -9.81f, 0.0f} {
}

CHmsForceFieldUniform::~CHmsForceFieldUniform(void) = default;

void CHmsForceFieldUniform::Configure(
        bool newHasValue,
        const GmVec3 &newValue) {
    hasValue = newHasValue;
    value = newValue;
}

void CHmsForceFieldBall::ComputeBoundingBox(void) {
    this->bounds.center.z = 0.0f;
    this->bounds.center.y = 0.0f;
    this->bounds.center.x = 0.0f;
    this->bounds.halfExtents.x = radius;
    this->bounds.halfExtents.y = radius;
    this->bounds.halfExtents.z = radius;
}

CHmsForceFieldBall::CHmsForceFieldBall(void)
        : center{0.0f, 0.0f, 0.0f},
          radius(2.0f),
          strength(1.0f),
          bounds{} {
    ComputeBoundingBox();
}

CHmsForceFieldBall::~CHmsForceFieldBall(void) = default;

void CHmsForceFieldBall::Configure(
        const GmVec3 &newCenter,
        float newRadius,
        float newStrength) {
    center = newCenter;
    radius = newRadius;
    strength = newStrength;
    ComputeBoundingBox();
}

int CHmsForceFieldUniform::GetValue(
        const GmVec3 &position,
        GmVec3 &out) const {
    (void)position;
    if (!hasValue) {
        return 0;
    }

    out = this->value;
    return 1;
}

void CHmsZoneDynamic::ResetForceFields(
        float linearDamping,
        float angularDamping) {
    while (HasForceFields()) {
        RemoveField(LastForceField());
    }
    linearDampingCoef_ = linearDamping;
    angularDampingCoef_ = angularDamping;
}

int CHmsForceFieldBall::GetValue(
        const GmVec3 &position,
        GmVec3 &out) const {
    GmVec3 delta = {
        center.x - position.x,
        center.y - position.y,
        center.z - position.z,
    };

    float distanceSq =
            (delta.z * delta.z + delta.x * delta.x) + delta.y * delta.y;
    float radiusSq = radius * radius;
    if (!(radiusSq > distanceSq)) {
        return 0;
    }
    if (!(distanceSq > PhysicsTolerance::ForceFieldDistanceSquared)) {
        return 0;
    }

    float ballForceScale = -strength / distanceSq;
    out.x = delta.x * ballForceScale;
    out.y = delta.y * ballForceScale;
    out.z = ballForceScale * delta.z;
    return 1;
}

void CHmsCorpus::ComputeOwnerForces(float dt) {
    if (item != nullptr) {
        item->RunComputeForcesCallback(dt);
    }
}

void CHmsZoneDynamic::ComputeCorpusForces(
        CHmsCorpus *corpus,
        float dt) {
    CHmsDyna *dyna = corpus->Dynamics();
    if (dyna == 0) {
        return;
    }

    CHmsDynaParams *dynaParams = &dyna->Parameters();

    dyna->ValidateDynamicState();

    if (corpus->Item()->GetProperties().kinematicOnly) {
        GmVec3 zero = GmVec3::ZeroForComputeCorpusForces();
        dyna->SetForce(zero);
        dyna->SetTorque(zero);
        return;
    }

    GmVec3 accumulatedForce = GmVec3::ZeroForComputeCorpusForces();

    for (CHmsForceField *field : ForceFields()) {
        GmVec3 fieldValue;
        if (field->GetValue(dyna->CurrentState().position, fieldValue)) {
            accumulatedForce.AddScaledForComputeCorpusForces(fieldValue,
                                dynaParams->forceScale *
                                        dynaParams->mass);
        }
    }

    GmVec3 linearSpeed;
    dyna->GetLinearSpeed(linearSpeed);
    accumulatedForce.AddScaledForComputeCorpusForces(linearSpeed,
                        -linearDampingCoef_ *
                                dynaParams->linearDampingScale);
    dyna->SetForce(accumulatedForce);

    if (dyna->DynamicType() ==
        CHmsDyna::EDynamicType_FullAngularDynamics) {
        GmVec3 angularSpeed;
        dyna->GetAngularSpeed(angularSpeed);
        GmVec3 dampingTorque = angularSpeed;
        dampingTorque.ScaleInPlaceForComputeCorpusForces(
                -angularDampingCoef_ * dynaParams->angularDampingScale);
        dyna->SetTorque(dampingTorque);
    }

    corpus->ComputeOwnerForces(dt);
}
