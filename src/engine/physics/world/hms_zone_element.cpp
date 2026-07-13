#include "engine/physics/world/hms_zone_element.h"
CHmsZoneElem::CHmsZoneElem(void) = default;

CHmsZoneElem::~CHmsZoneElem(void) = default;

const GmIso4 *CHmsZoneElem::GetLocation(void) const {
    return nullptr;
}

CHmsPoc::CHmsPoc(void) {
    location.SetIdentity();
}

CHmsPoc::~CHmsPoc(void) = default;

const GmIso4 *CHmsPoc::GetLocation(void) const {
    return &location;
}

void CHmsPoc::SetLocation(const GmIso4 &newLocation) {
    location = newLocation;
}

void CHmsPoc::SetZone(CHmsZone *newZone, const GmIso4 &newLocation) {
    zone = newZone;
    location = newLocation;
}

void CHmsPoc::SwitchOn(void) {
    isOn = true;
}

void CHmsPoc::SwitchOff(void) {
    isOn = false;
}
