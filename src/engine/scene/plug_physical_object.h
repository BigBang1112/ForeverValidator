#pragma once

#include "engine/core/gm_types.h"
struct CPlugTree;

struct CPlugPhysicalParameters {
    float mass = 1.0f;
    GmMat3 impulseInertia{};
    float linearFluidFriction = 0.1f;
    float physicalResponseCoefA = 0.3f;
    float physicalResponseCoefB = 0.3f;
    float vehicleContactFeedbackScale = 1.0f;
    GmVec3 localCenterOfMass{};
};

struct CPlugPhysicalObject {
    CPlugPhysicalObject(void);
    CPlugPhysicalParameters &Parameters(void) { return parameters_; }
    const CPlugPhysicalParameters &Parameters(void) const {
        return parameters_;
    }
    void Configure(const CPlugPhysicalParameters &parameters);
    void BindBoundsTree(CPlugTree *tree);
    void CopyFrom(const CPlugPhysicalObject &source);
    void SetInertiaMatrixSphere(float radius);
    void SetComPos(const GmVec3 &localCenterOfMass);
    void SetInertiaMatrixBox(float mass, const GmVec3 &size);
    void SetComPosAndInertiaMatrixFromTreeBoundingBox(void);
    void ComputeComPos(int includeChildren);
    void ComputeInertiaMatrix(float mass, int includeChildren);
    void ComputeComPosAndInertiaMatrix(float mass, int includeChildren);

private:
    CPlugPhysicalParameters parameters_;
    CPlugTree *boundsTree_ = nullptr;
};
