#include "format/archive/hms_item_archive_state.h"
namespace {

constexpr u32 DynamicTypeShift = 11u;
constexpr u32 CollisionGroupShift = 13u;
constexpr u32 ContactInterestShift = 17u;
constexpr u32 ShadowCasterMask = 0x00000fffu;

bool HasBit(u32 value, u32 bit) {
    return (value & (1u << bit)) != 0u;
}

void SetBit(u32 &value, u32 bit, bool enabled) {
    const u32 mask = 1u << bit;
    value = enabled ? value | mask : value & ~mask;
}

}  // namespace

CHmsItem::Properties DecodeHmsItemArchiveState(
        const HmsItemArchiveWords &words) {
    CHmsItem::Properties out;
    out.active = words.physics != 0u;
    out.shadowTexCastedCount = static_cast<uint8_t>(words.physics);
    out.visionStatic = HasBit(words.physics, 8u);
    out.background = HasBit(words.physics, 9u);
    out.dynamicType = static_cast<CHmsItem::EDynamicType>(
            (words.physics >> DynamicTypeShift) & 3u);
    out.collisionGroup = static_cast<CHmsItem::ECollisionGroup>(
            (words.physics >> CollisionGroupShift) & 15u);
    out.contactInterest = static_cast<CHmsItem::EContactInterest>(
            (words.physics >> ContactInterestShift) & 3u);
    out.collisionStatic = HasBit(words.physics, 19u);
    out.kinematicOnly = HasBit(words.physics, 20u);
    out.zombie = HasBit(words.physics, 21u);
    out.occluderForLightMap = HasBit(words.physics, 22u);
    out.shadowTexCastedEnabled = HasBit(words.physics, 23u);
    out.shadowFakeEnabled = HasBit(words.physics, 27u);
    out.lightLensFlareEnabled = HasBit(words.physics, 28u);
    out.shadowCasterGroupMask = words.rendering & ShadowCasterMask;
    out.forcePointDynamicCollisionResponse = HasBit(words.rendering, 12u);
    out.shadowReceiverGroupMask = words.rendering >> 20u;
    out.lightEmitter = HasBit(words.visibility, 0u);
    return out;
}

HmsItemArchiveWords EncodeHmsItemArchiveState(
        const CHmsItem::Properties &properties) {
    HmsItemArchiveWords out;
    if (properties.active) {
        out.physics = 0x11000000u;
    }
    out.physics |= properties.shadowTexCastedCount;
    out.physics |= static_cast<u32>(properties.dynamicType) << DynamicTypeShift;
    out.physics |= static_cast<u32>(properties.collisionGroup)
                   << CollisionGroupShift;
    out.physics |= static_cast<u32>(properties.contactInterest)
                   << ContactInterestShift;
    SetBit(out.physics, 8u, properties.visionStatic);
    SetBit(out.physics, 9u, properties.background);
    SetBit(out.physics, 19u, properties.collisionStatic);
    SetBit(out.physics, 20u, properties.kinematicOnly);
    SetBit(out.physics, 21u, properties.zombie);
    SetBit(out.physics, 22u, properties.occluderForLightMap);
    SetBit(out.physics, 23u, properties.shadowTexCastedEnabled);
    SetBit(out.physics, 27u, properties.shadowFakeEnabled);
    SetBit(out.physics, 28u, properties.lightLensFlareEnabled);

    out.rendering = 0xfff00000u |
                    (properties.shadowCasterGroupMask & ShadowCasterMask) |
                    (properties.shadowReceiverGroupMask << 20u);
    SetBit(out.rendering, 12u,
           properties.forcePointDynamicCollisionResponse);
    SetBit(out.visibility, 0u, properties.lightEmitter);
    return out;
}
