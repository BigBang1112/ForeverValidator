#include "format/pack/block_info_catalog/blockinfo_descriptor_parse_state.h"
#include <algorithm>
#include <new>

void BlockInfoParsedMobilCounts::Init() {
    airVariantCount.fill(0u);
    groundVariantCount.fill(0u);
}

u32 BlockInfoParsedMobilCounts::SetVariantCount(
        u32 family,
        u32 variant,
        u32 count) {
    if (family > 1u || variant >= 64u) {
        return 0u;
    }
    (family == 0u ? airVariantCount : groundVariantCount)[variant] = count;
    return 1u;
}

GmInt3 BlockInfoDescriptorParseResult::SizeWhenGround() const {
    return unitGeometry ? unitGeometry->SizeWhenGround() : GmInt3{1, 1, 1};
}

GmInt3 BlockInfoDescriptorParseResult::SizeWhenAir() const {
    return unitGeometry ? unitGeometry->SizeWhenAir() : GmInt3{1, 1, 1};
}

const std::vector<BlockInfoDescriptorUnitDefinition> &
BlockInfoDescriptorParseResult::UnitDefinitionsForFieldUnits(
        bool ground) const {
    return ground ? groundUnitDefinitions : airUnitDefinitions;
}

int BlockInfoParsedMobilCollection::Append(
        const BlockInfoParsedMobil &mobil) {
    try {
        mobils.push_back(mobil);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

u32 BlockInfoParsedMobilCollection::Count() const {
    return static_cast<u32>(mobils.size());
}

const BlockInfoParsedMobil *BlockInfoParsedMobilCollection::ModelAt(
        u32 index) const {
    return index < mobils.size() ? &mobils[index] : nullptr;
}

void BlockInfoParsedMobilCollection::ApplyHmsState(
        u32 firstMobil,
        const BlockInfoParsedHmsState &hmsState) {
    for (u32 index = firstMobil; index < Count(); ++index) {
        BlockInfoParsedMobil &mobil = mobils[index];
        mobil.hasHmsItemState = hmsState.hasItemState;
        mobil.hmsItemPhysicsFlags = hmsState.physicsFlags;
        mobil.hmsItemRenderingFlags = hmsState.renderingFlags;
        mobil.hmsItemVisibilityFlags = hmsState.visibilityFlags;
        mobil.isRaceTriggerMobil = hmsState.isRaceTriggerMobil;
    }
}

void BlockInfoParsedMobilCollection::ApplyHmsState(
        const char *descriptorPath,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const BlockInfoParsedHmsState *hmsState) {
    if (descriptorPath == nullptr || hmsState == nullptr ||
        hmsState->hasItemState == 0u) {
        return;
    }
    for (BlockInfoParsedMobil &mobil : mobils) {
        if (mobil.descriptorPath == descriptorPath &&
            mobil.selectorGroup == selectorGroup &&
            mobil.variantIndex == variantIndex &&
            mobil.mobilIndex == mobilIndex) {
            if (mobil.isRaceTriggerMobil != 0u &&
                hmsState->isRaceTriggerMobil == 0u) {
                continue;
            }
            mobil.hasHmsItemState = 1u;
            mobil.hmsItemPhysicsFlags = hmsState->physicsFlags;
            mobil.hmsItemRenderingFlags = hmsState->renderingFlags;
            mobil.hmsItemVisibilityFlags = hmsState->visibilityFlags;
            mobil.isRaceTriggerMobil = hmsState->isRaceTriggerMobil;
        }
    }
}

void BlockInfoParsedMobilCollection::
        ApplySceneLinkIso4ToAppendedRaceTriggerModels(
                u32 firstMobil,
                const float iso[12]) {
    if (iso == nullptr) {
        return;
    }
    for (u32 index = firstMobil; index < Count(); ++index) {
        BlockInfoParsedMobil &mobil = mobils[index];
        if (mobil.isRaceTriggerMobil == 0u) {
            continue;
        }
        mobil.hasSceneLinkIso4 = 1u;
        std::copy(iso, iso + 12u, mobil.sceneLinkIso4);
    }
}

void BlockInfoParsedMobilCollection::MarkAppendedRaceTrigger(u32 firstMobil) {
    for (u32 index = firstMobil; index < Count(); ++index) {
        mobils[index].isRaceTriggerMobil = 1u;
    }
}

void BlockInfoParsedMobilCollection::MarkAppendedSceneObjectLink(
        u32 firstMobil) {
    for (u32 index = firstMobil; index < Count(); ++index) {
        mobils[index].isSceneObjectLinkMobil = 1u;
    }
}
