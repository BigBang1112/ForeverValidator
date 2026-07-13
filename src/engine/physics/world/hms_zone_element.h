#pragma once

#include "engine/core/cmw_nod.h"
#include "engine/core/gm_types.h"
struct CHmsZone;

class CHmsZoneElem : public CMwNod {
public:
    CHmsZoneElem(void);
    ~CHmsZoneElem(void) override;

    virtual const GmIso4 *GetLocation(void) const;

protected:
    CHmsZone *zone = nullptr;
};

class CHmsPoc : public CHmsZoneElem {
public:
    CHmsPoc(void);
    ~CHmsPoc(void) override;

    const GmIso4 *GetLocation(void) const override;
    virtual void SetLocation(const GmIso4 &location);
    virtual void SetZone(CHmsZone *zone, const GmIso4 &location);
    virtual void SwitchOn(void);
    virtual void SwitchOff(void);

protected:
    GmIso4 location{};
    GmVec3 speed{};
    bool isOn = false;
};
