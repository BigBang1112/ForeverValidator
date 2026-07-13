#pragma once

#include "engine/core/gm_types.h"
struct CHmsZone;

struct CHmsForceField {
    CHmsForceField(void);
    virtual ~CHmsForceField(void);
    virtual int GetValue(const GmVec3 &position, GmVec3 &out) const;
    virtual void SetZone(
            CHmsZone *zone,
            const GmIso4 &location);
    CHmsZone *OwningZone(void) const;

private:
    CHmsZone *zone_ = nullptr;

    friend struct CHmsZone;
};

struct CHmsForceFieldUniform : CHmsForceField {
    bool hasValue;
    GmVec3 value;
    CHmsForceFieldUniform(void);
    ~CHmsForceFieldUniform(void) override;
    int GetValue(
            const GmVec3 &position,
            GmVec3 &out) const override;
    void Configure(bool newHasValue, const GmVec3 &newValue);
};

struct CHmsForceFieldBall : CHmsForceField {
    GmVec3 center;
    float radius;
    float strength;
    GmBoxAligned bounds;
    CHmsForceFieldBall(void);
    ~CHmsForceFieldBall(void) override;
    int GetValue(
            const GmVec3 &position,
            GmVec3 &out) const override;
    void ComputeBoundingBox(void);
    void Configure(const GmVec3 &newCenter,
                   float newRadius,
                   float newStrength);
};
