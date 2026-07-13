#include "engine/physics/dynamics/hms_item.h"

constexpr u32 CHmsItemContactInterest_LocalPayloadThreshold = 1u;
u32 CHmsItem::CollisionGroupForCollisionResponse(void) const {
    return static_cast<u32>(properties.collisionGroup);
}

u32 CHmsItem::ContactInterestForCollisionResponse(void) const {
    return static_cast<u32>(properties.contactInterest);
}

int CHmsItem::RequestsLocalContactPayloadForCollisionResponse(void) const {
    return ContactInterestForCollisionResponse() >
           CHmsItemContactInterest_LocalPayloadThreshold;
}

bool CHmsItem::SHmsCollisionGroupPair::ContactHandlerEnabledForResponse(
        CHmsCollisionContactSide side) const {
    return side == CHmsCollisionContactSide_GroupA ? contactHandlerForGroupA
                                                   : contactHandlerForGroupB;
}

CHmsCollisionContactSide
CHmsItem::SHmsCollisionGroupPair::ContactSideForCollisionResponse(
        u32 itemCollisionGroup) const {
    return itemCollisionGroup != groupA ? CHmsCollisionContactSide_GroupB
                                        : CHmsCollisionContactSide_GroupA;
}

CHmsCollisionContactSide
CHmsItem::SHmsCollisionGroupPair::OppositeContactSideForCollisionResponse(
        CHmsCollisionContactSide side) {
    return side == CHmsCollisionContactSide_GroupA ? CHmsCollisionContactSide_GroupB
                                                   : CHmsCollisionContactSide_GroupA;
}
