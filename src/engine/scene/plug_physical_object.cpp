#include "engine/scene/plug_physical_object.h"
#include "engine/rendering/plug_tree.h"
CPlugPhysicalObject::CPlugPhysicalObject() {
    SetInertiaMatrixSphere(1.0f);
}

void CPlugPhysicalObject::Configure(
        const CPlugPhysicalParameters &parameters) {
    parameters_ = parameters;
}

void CPlugPhysicalObject::BindBoundsTree(CPlugTree *tree) {
    boundsTree_ = tree;
}

void CPlugPhysicalObject::CopyFrom(const CPlugPhysicalObject &source) {
    parameters_ = source.parameters_;
    boundsTree_ = source.boundsTree_;
}

void CPlugPhysicalObject::SetInertiaMatrixSphere(float radius) {
    constexpr float Pi = 3.1415927f;
    parameters_.impulseInertia.SetIdentity();
    const float radiusScale =
            3.0f / (radius * ((4.0f * Pi) * radius * radius));
    parameters_.impulseInertia.Mult(radiusScale);
    parameters_.impulseInertia.Mult(1.0f / parameters_.mass);
}

void CPlugPhysicalObject::SetComPos(
        const GmVec3 &localCenterOfMass) {
    parameters_.localCenterOfMass = localCenterOfMass;
}

void CPlugPhysicalObject::SetInertiaMatrixBox(
        float mass,
        const GmVec3 &size) {
    const float scaledX = size.x * 2.0f;
    const float scaledY = size.y * 2.0f;
    const float scaledZ = 2.0f * size.z;
    const float inverseMass = 1.0f / mass;
    const float inverseMassScale = inverseMass * 12.0f;

    parameters_.impulseInertia.SetIdentity();
    const float scaledZSq = scaledZ * scaledZ;
    const float scaledYSq = scaledY * scaledY;
    parameters_.impulseInertia.Element(GmAxis::X, GmAxis::X) =
            inverseMassScale / (scaledYSq + scaledZSq);

    const float scaledXSq = scaledX * scaledX;
    parameters_.impulseInertia.Element(GmAxis::Y, GmAxis::Y) =
            inverseMassScale / (scaledZSq + scaledXSq);
    parameters_.impulseInertia.Element(GmAxis::Z, GmAxis::Z) =
            inverseMassScale / (scaledXSq + scaledYSq);
}

void CPlugPhysicalObject::SetComPosAndInertiaMatrixFromTreeBoundingBox(void) {
    ComputeComPosAndInertiaMatrix(parameters_.mass, 1);
}

void CPlugPhysicalObject::ComputeComPos(int includeChildren) {
    if (boundsTree_ == nullptr) {
        parameters_.localCenterOfMass = {};
        return;
    }
    (void)boundsTree_->UpdateBoundingBox(includeChildren);
    SetComPos(boundsTree_->Box().center);
}

void CPlugPhysicalObject::ComputeInertiaMatrix(float objectMass,
                                                int includeChildren) {
    if (boundsTree_ == nullptr) {
        SetInertiaMatrixSphere(1.0f);
        return;
    }
    (void)boundsTree_->UpdateBoundingBox(includeChildren);
    SetInertiaMatrixBox(objectMass, boundsTree_->Box().halfExtents);
}

void CPlugPhysicalObject::ComputeComPosAndInertiaMatrix(float objectMass,
                                                        int includeChildren) {
    ComputeComPos(includeChildren);
    ComputeInertiaMatrix(objectMass, includeChildren);
}
