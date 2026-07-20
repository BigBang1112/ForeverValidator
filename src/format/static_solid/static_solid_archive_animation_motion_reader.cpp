#include "format/static_solid/static_solid_archive_animation_motion_reader.h"
#include <limits>
#include <new>

#include "format/vehicle_tuning/archive_ids.h"
#include "format/static_solid/static_solid_archive_animation_motion_chunk_ids.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"

void CGameCtnReplayStaticSolidArchiveAnimationMotionState::Reset() {
    skeletonBoneCounts_.clear();
    keysSkeletonNodes_.clear();
    emitterLeaves_.clear();
    managerLeaves_.clear();
}

int CGameCtnReplayStaticSolidArchiveAnimationMotionState::
RememberCFuncSkelBoneCount(
        ArchiveNodeReference skeletonNode,
        u32 boneCount) {
    if (!skeletonNode.IsValid()) {
        return 0;
    }
    try {
        skeletonBoneCounts_.insert_or_assign(skeletonNode.Index(), boneCount);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

int CGameCtnReplayStaticSolidArchiveAnimationMotionState::
RememberCFuncKeysSkelSkeleton(
        ArchiveNodeReference keysNode,
        ArchiveNodeReference skeletonNode) {
    if (!keysNode.IsValid() || !skeletonNode.IsValid()) {
        return 0;
    }
    if (skeletonBoneCounts_.find(skeletonNode.Index()) ==
        skeletonBoneCounts_.end()) {
        return 0;
    }
    try {
        keysSkeletonNodes_.insert_or_assign(keysNode.Index(),
                                            skeletonNode.Index());
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

std::optional<u32> CGameCtnReplayStaticSolidArchiveAnimationMotionState::
CFuncKeysSkelBoneCount(ArchiveNodeReference keysNode) const {
    if (!keysNode.IsValid()) {
        return std::nullopt;
    }
    const auto skeleton = keysSkeletonNodes_.find(keysNode.Index());
    if (skeleton == keysSkeletonNodes_.end()) {
        return std::nullopt;
    }
    const auto boneCount = skeletonBoneCounts_.find(skeleton->second);
    return boneCount != skeletonBoneCounts_.end()
            ? std::optional<u32>(boneCount->second)
            : std::nullopt;
}

int CGameCtnReplayStaticSolidArchiveAnimationMotionState::
RememberCMotionEmitterLeaves(
        ArchiveNodeReference emitterNode,
        const CMotionEmitterLeavesArchiveDefinition &definition) {
    if (!emitterNode.IsValid()) {
        return 0;
    }
    try {
        emitterLeaves_.insert_or_assign(emitterNode.Index(), definition);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

int CGameCtnReplayStaticSolidArchiveAnimationMotionState::
RememberCMotionManagerLeaves(
        ArchiveNodeReference managerNode,
        const CMotionManagerLeavesArchiveDefinition &definition) {
    if (!managerNode.IsValid()) {
        return 0;
    }
    try {
        managerLeaves_.insert_or_assign(managerNode.Index(), definition);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

std::optional<CMotionEmitterLeavesArchiveDefinition>
CGameCtnReplayStaticSolidArchiveAnimationMotionState::
CMotionEmitterLeavesDefinition(
        ArchiveNodeReference emitterNode) const {
    if (!emitterNode.IsValid()) {
        return std::nullopt;
    }
    const auto definition = emitterLeaves_.find(emitterNode.Index());
    return definition != emitterLeaves_.end()
            ? std::optional<CMotionEmitterLeavesArchiveDefinition>(
                      definition->second)
            : std::nullopt;
}

std::optional<CMotionManagerLeavesArchiveDefinition>
CGameCtnReplayStaticSolidArchiveAnimationMotionState::
CMotionManagerLeavesDefinition(
        ArchiveNodeReference managerNode) const {
    if (!managerNode.IsValid()) {
        return std::nullopt;
    }
    const auto definition = managerLeaves_.find(managerNode.Index());
    return definition != managerLeaves_.end()
            ? std::optional<CMotionManagerLeavesArchiveDefinition>(
                      definition->second)
            : std::nullopt;
}

int CAnimationMotionDiscardedRefsPayload::ReadSingle(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    return nodeRefs != nullptr && nodeRefs->ReadNodPtr(nullptr);
}

int CAnimationMotionDiscardedRefsPayload::ReadCountedArray(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    constexpr u32 kMaxRefCount = 0x100000u;
    u32 count = 0u;
    if (!byteStream->ReadU32(&count) ||
        count > kMaxRefCount) {
        return 0;
    }
    for (u32 i = 0u; i < count; i++) {
        if (!ReadSingle(nodeRefs)) {
            return 0;
        }
    }
    return 1;
}

CFuncSkelArchivePayload::CFuncSkelArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveAnimationMotionState *state,
        ArchiveNodeReference currentArchiveNode)
        : cmwIdState_(cmwIdState),
          state_(state),
          currentArchiveNode_(currentArchiveNode) {}

int CFuncSkelArchivePayload::ReadLegacyTable(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        int idsInsteadOfStrings) {
    constexpr u32 kMaxLegacyEntryCount = 0x100000u;
    u32 entryCount = 0u;
    if (!byteStream->ReadU32(&entryCount) ||
        entryCount > kMaxLegacyEntryCount) {
        return 0;
    }
    for (u32 index = 0u; index < entryCount; ++index) {
        const int keyRead = idsInsteadOfStrings
                ? cmwIdState_->ReadSkip(*byteStream)
                : byteStream->ReadStringSkip();
        if (!keyRead || !byteStream->Skip(sizeof(u32))) {
            return 0;
        }
    }
    return 1;
}

int CFuncSkelArchivePayload::ReadBones(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        int hasElementArrays) {
    constexpr u32 kMaxBoneCount = 0x100000u;
    constexpr u32 kMaxElementCount = 0x100000u;
    u32 boneCount = 0u;
    if (!byteStream->ReadU32(&boneCount) || boneCount > kMaxBoneCount) {
        return 0;
    }
    for (u32 index = 0u; index < boneCount; index++) {
        if (!cmwIdState_->ReadSkip(*byteStream)) {
            return 0;
        }
        if (hasElementArrays) {
            u32 elementCount = 0u;
            if (!byteStream->ReadU32(&elementCount) ||
                elementCount > kMaxElementCount ||
                !byteStream->SkipCounted(elementCount, sizeof(u32))) {
                return 0;
            }
        }
    }
    return state_->RememberCFuncSkelBoneCount(currentArchiveNode_, boneCount);
}

int CFuncSkelArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr || state_ == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case ArchiveChunkIdValue(
                CFuncSkelArchiveChunkId::LegacyStringTable):
            return ReadLegacyTable(byteStream, 0) &&
                   ReadBones(byteStream, 1);
        case ArchiveChunkIdValue(CFuncSkelArchiveChunkId::LegacyIdTable):
            return ReadLegacyTable(byteStream, 1) &&
                   ReadBones(byteStream, 1);
        case ArchiveChunkIdValue(CFuncSkelArchiveChunkId::BoneIds):
            return ReadBones(byteStream, 0);
        default:
            return 0;
    }
}

CFuncKeysSkelArchivePayload::CFuncKeysSkelArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CGameCtnReplayStaticSolidArchiveAnimationMotionState *state,
        ArchiveNodeReference currentArchiveNode)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs),
          state_(state),
          currentArchiveNode_(currentArchiveNode) {}

int CFuncKeysSkelArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr ||
        nodeRefs_ == nullptr || state_ == nullptr) {
        return 0;
    }
    CFuncKeysNaturalArchivePayload keysPayload(cmwIdState_);
    if (keysPayload.Read(byteStream, chunkId)) {
        return 1;
    }
    switch (chunkId) {
        case ArchiveChunkIdValue(CFuncKeysSkelArchiveChunkId::SkeletonRef): {
            ArchiveNodeReference skeletonNode = ArchiveNodeReference::Invalid();
            return nodeRefs_->ReadNodeRef(&skeletonNode) &&
                   state_->RememberCFuncKeysSkelSkeleton(
                           currentArchiveNode_, skeletonNode);
        }
        case ArchiveChunkIdValue(CFuncKeysSkelArchiveChunkId::Locations): {
            constexpr u32 kMaxFrameCount = 0x100000u;
            constexpr u32 kLocationByteCount =
                    (4u + 3u) * sizeof(float);
            u32 frameCount = 0u;
            const std::optional<u32> boneCount =
                    state_->CFuncKeysSkelBoneCount(currentArchiveNode_);
            if (!boneCount.has_value() ||
                !byteStream->ReadU32(&frameCount) ||
                frameCount > kMaxFrameCount ||
                (*boneCount != 0u &&
                 frameCount > std::numeric_limits<u32>::max() / *boneCount)) {
                return 0;
            }
            return byteStream->SkipCounted(frameCount * *boneCount,
                                            kLocationByteCount);
        }
        default:
            return 0;
    }
}

CMotionsArchivePayload::CMotionsArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs) {}

int CMotionsArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr ||
        nodeRefs_ == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CLASS_CMotion:
            return cmwIdState_->ReadSkip(*byteStream);
        case ArchiveChunkIdValue(CMotionsArchiveChunkId::Root):
        case ArchiveChunkIdValue(CMotionsArchiveChunkId::MotionRefs):
            return CAnimationMotionDiscardedRefsPayload::ReadCountedArray(
                    byteStream, nodeRefs_);
        default:
            return 0;
    }
}

CMotionSkelArchivePayload::CMotionSkelArchivePayload(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : nodeRefs_(nodeRefs) {}

int CMotionSkelArchivePayload::Read(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CMotionSkelArchiveChunkId::KeysRef) &&
           CAnimationMotionDiscardedRefsPayload::ReadSingle(nodeRefs_);
}

CMotionEmitterLeavesArchivePayload::CMotionEmitterLeavesArchivePayload(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CGameCtnReplayStaticSolidArchiveAnimationMotionState *state,
        ArchiveNodeReference currentArchiveNode)
        : nodeRefs_(nodeRefs),
          state_(state),
          currentArchiveNode_(currentArchiveNode) {}

int CMotionEmitterLeavesArchivePayload::Chunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || nodeRefs_ == nullptr || state_ == nullptr ||
        !currentArchiveNode_.IsValid() ||
        (chunkId != ArchiveChunkIdValue(
                            CMotionEmitterLeavesArchiveChunkId::
                                    PositionAndScalarRadius) &&
         chunkId != ArchiveChunkIdValue(
                            CMotionEmitterLeavesArchiveChunkId::
                                    PositionAndRadius))) {
        return 0;
    }

    CMotionEmitterLeavesArchiveDefinition definition;
    definition.sourceChunkId = chunkId;
    if (!nodeRefs_->ReadNodeRef(&definition.managerModel) ||
        definition.managerModel.IsDeferred()) {
        return 0;
    }
    for (float &component : definition.pos) {
        if (!byteStream->ReadF32(&component)) {
            return 0;
        }
    }
    if (chunkId == ArchiveChunkIdValue(
                           CMotionEmitterLeavesArchiveChunkId::
                                   PositionAndScalarRadius)) {
        float radius = 0.0f;
        if (!byteStream->ReadF32(&radius)) {
            return 0;
        }
        definition.radius.fill(radius);
    } else {
        for (float &component : definition.radius) {
            if (!byteStream->ReadF32(&component)) {
                return 0;
            }
        }
    }
    return state_->RememberCMotionEmitterLeaves(
            currentArchiveNode_, definition);
}

CMotionManagerLeavesArchivePayload::CMotionManagerLeavesArchivePayload(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CGameCtnReplayStaticSolidArchiveAnimationMotionState *state,
        ArchiveNodeReference currentArchiveNode)
        : nodeRefs_(nodeRefs),
          state_(state),
          currentArchiveNode_(currentArchiveNode) {}

int CMotionManagerLeavesArchivePayload::Chunk(u32 chunkId) {
    if (nodeRefs_ == nullptr || state_ == nullptr ||
        !currentArchiveNode_.IsValid() ||
        chunkId != ArchiveChunkIdValue(
                           CMotionManagerLeavesArchiveChunkId::MobilModel)) {
        return 0;
    }
    CMotionManagerLeavesArchiveDefinition definition;
    return nodeRefs_->ReadNodeRef(&definition.mobilModel) &&
           !definition.mobilModel.IsDeferred() &&
           state_->RememberCMotionManagerLeaves(
                   currentArchiveNode_, definition);
}

CFuncKeysNaturalArchivePayload::CFuncKeysNaturalArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState)
        : cmwIdState_(cmwIdState) {}

int CFuncKeysNaturalArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CLASS_CFuncKeys:
            return byteStream->ReadStringSkip();
        case ArchiveChunkIdValue(CFuncKeysArchiveChunkId::XValues):
        case TMNF_CLASS_CFuncKeysNatural: {
            constexpr u32 kMaxKeyCount = 0x100000u;
            u32 count = 0u;
            return byteStream->ReadU32(&count) &&
                   count <= kMaxKeyCount &&
                   byteStream->SkipCounted(count, 4u);
        }
        case ArchiveChunkIdValue(
                CFuncKeysArchiveChunkId::TwoRealCompatibilityRange):
            return byteStream->Skip(8u);
        case ArchiveChunkIdValue(CFuncKeysArchiveChunkId::Id):
            return cmwIdState_->ReadSkip(*byteStream);
        default:
            return 0;
    }
}

CFuncTreeSubVisualSequenceArchivePayload::
        CFuncTreeSubVisualSequenceArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodes)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs),
          embeddedNodes_(embeddedNodes) {}

int CFuncTreeSubVisualSequenceArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr) {
        return 0;
    }
    CFuncKeysNaturalArchivePayload keysPayload(cmwIdState_);
    if (keysPayload.Read(byteStream, chunkId)) {
        return 1;
    }
    switch (chunkId) {
        case ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugFloat):
            return byteStream->Skip(4u);
        case ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugFloatAndId):
            return byteStream->Skip(4u) &&
                   cmwIdState_->ReadSkip(*byteStream);
        case ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugVec2):
            return byteStream->Skip(8u);
        case ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugVec3):
            return byteStream->Skip(12u);
        case ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugVec4):
            return byteStream->Skip(16u);
        case ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugVec4AndId):
            return byteStream->Skip(16u) &&
                   cmwIdState_->ReadSkip(*byteStream);
        case TMNF_CLASS_CFuncTreeSubVisualSequence:
            return embeddedNodes_ != nullptr &&
                   embeddedNodes_->ParseEmbeddedNodeArchive(
                           TMNF_CLASS_CFuncKeysNatural);
        case ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::SequenceId):
            return cmwIdState_->ReadSkip(*byteStream);
        case ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::KeyRef):
            return CAnimationMotionDiscardedRefsPayload::ReadSingle(nodeRefs_);
        case ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::InlineIndexRange):
            return byteStream->Skip(12u);
        default:
            return 0;
    }
}

CMotionPlayerArchivePayload::CMotionPlayerArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodes)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs),
          embeddedNodes_(embeddedNodes) {}

int CMotionPlayerArchivePayload::ReadOptionalRef(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    u32 value = 0u;
    if (byteStream == nullptr ||
        !byteStream->ReadBool(&value)) {
        return 0;
    }
    return value == 0u ||
           CAnimationMotionDiscardedRefsPayload::ReadSingle(nodeRefs_);
}

int CMotionPlayerArchivePayload::ReadBaseCommandAndRefArray(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        int hasTrackRef,
        int hasStateBool) {
    if (byteStream == nullptr || embeddedNodes_ == nullptr) {
        return 0;
    }
    u32 ignoredStateBool = 0u;
    if (hasTrackRef &&
        !CAnimationMotionDiscardedRefsPayload::ReadSingle(nodeRefs_)) {
        return 0;
    }
    if (!embeddedNodes_->ParseEmbeddedNodeArchive(TMNF_CLASS_CMotionCmdBase) ||
        !byteStream->Skip(4u)) {
        return 0;
    }
    if (hasStateBool &&
        !byteStream->ReadBool(&ignoredStateBool)) {
        return 0;
    }
    return cmwIdState_ != nullptr &&
           cmwIdState_->ReadSkip(*byteStream) &&
           CAnimationMotionDiscardedRefsPayload::ReadCountedArray(byteStream,
                                                                  nodeRefs_);
}

int CMotionPlayerArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr || nodeRefs_ == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CLASS_CMotion:
            return cmwIdState_->ReadSkip(*byteStream);
        case TMNF_CLASS_CMotionPlayer:
            return ReadOptionalRef(byteStream);
        case ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::ConditionalBaseAndId):
            return ReadOptionalRef(byteStream) &&
                   cmwIdState_->ReadSkip(*byteStream);
        case ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::TrackThenBaseAndRefs):
            return ReadBaseCommandAndRefArray(byteStream, 1, 0);
        case ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::BaseAndRefs):
            return ReadBaseCommandAndRefArray(byteStream, 0, 0);
        case ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::FullStateAndRefs):
            return ReadBaseCommandAndRefArray(byteStream, 0, 1);
        default:
            return 0;
    }
}

CMotionDayTimeArchivePayload::CMotionDayTimeArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs) {}

int CMotionDayTimeArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr || nodeRefs_ == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CLASS_CMotion:
            return cmwIdState_->ReadSkip(*byteStream);
        case TMNF_CLASS_CMotionDayTime:
            return CAnimationMotionDiscardedRefsPayload::ReadSingle(nodeRefs_) &&
                   byteStream->Skip(2u * 4u);
        default:
            return 0;
    }
}

CMotionWeatherArchivePayload::CMotionWeatherArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs) {}

int CMotionWeatherArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CLASS_CMotion:
            return cmwIdState_->ReadSkip(*byteStream);
        case ArchiveChunkIdValue(CMotionWeatherArchiveChunkId::Root):
            return CAnimationMotionDiscardedRefsPayload::ReadSingle(
                    nodeRefs_);
        default:
            return 0;
    }
}

int CMotionCmdBaseArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CLASS_CMotionCmdBase:
            return byteStream->Skip(16u);
        case ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::BaseBool):
            return byteStream->Skip(20u);
        case ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::BaseParamsRef):
            return byteStream->Skip(24u);
        case ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::PeriodAndPhase):
            return byteStream->Skip(8u);
        case ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::PeriodPhaseAndTimeBase):
            return byteStream->Skip(12u);
        case ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::PeriodPhaseTimeBaseAndLoop):
            return byteStream->Skip(16u);
        case ArchiveChunkIdValue(
                CMotionCmdBaseArchiveChunkId::
                        PeriodPhaseTimeBaseLoopAndOffset):
            return byteStream->Skip(20u);
        default:
            return 0;
    }
}

CMotionShaderArchivePayload::CMotionShaderArchivePayload(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : nodeRefs_(nodeRefs) {}

int CMotionShaderArchivePayload::Read(u32 chunkId) {
    if (nodeRefs_ == nullptr ||
        chunkId != TMNF_CLASS_CMotionShader) {
        return 0;
    }
    return CAnimationMotionDiscardedRefsPayload::ReadSingle(nodeRefs_) &&
           CAnimationMotionDiscardedRefsPayload::ReadSingle(nodeRefs_) &&
           CAnimationMotionDiscardedRefsPayload::ReadSingle(nodeRefs_);
}

int CGameCtnReplayStaticSolidArchiveAnimationMotionReader::
        ParseAnimationMotionChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 classId,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodes,
        CGameCtnReplayStaticSolidArchiveAnimationMotionState *state,
        ArchiveNodeReference currentArchiveNode) {
    if (classId == TMNF_CLASS_CFuncSkel) {
        CFuncSkelArchivePayload skelPayload(cmwIdState,
                                            state,
                                            currentArchiveNode);
        return skelPayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CFuncKeysSkel) {
        CFuncKeysSkelArchivePayload keysSkelPayload(cmwIdState,
                                                    nodeRefs,
                                                    state,
                                                    currentArchiveNode);
        return keysSkelPayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CFuncKeysNatural) {
        CFuncKeysNaturalArchivePayload keysPayload(cmwIdState);
        return keysPayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CFuncTreeSubVisualSequence) {
        CFuncTreeSubVisualSequenceArchivePayload sequencePayload(cmwIdState,
                                                                 nodeRefs,
                                                                 embeddedNodes);
        return sequencePayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CMotions) {
        CMotionsArchivePayload motionsPayload(cmwIdState, nodeRefs);
        return motionsPayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CMotionPlayer) {
        CMotionPlayerArchivePayload playerPayload(cmwIdState,
                                                  nodeRefs,
                                                  embeddedNodes);
        return playerPayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CMotionSkel) {
        CMotionSkelArchivePayload skelPayload(nodeRefs);
        return skelPayload.Read(chunkId);
    }
    if (classId == TMNF_CLASS_CMotionEmitterLeaves) {
        if (chunkId == TMNF_CLASS_CMotion) {
            return cmwIdState != nullptr &&
                   cmwIdState->ReadSkip(*byteStream);
        }
        CMotionEmitterLeavesArchivePayload emitterPayload(
                nodeRefs, state, currentArchiveNode);
        return emitterPayload.Chunk(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CMotionManagerLeaves) {
        CMotionManagerLeavesArchivePayload managerPayload(
                nodeRefs, state, currentArchiveNode);
        return managerPayload.Chunk(chunkId);
    }
    if (classId == TMNF_CLASS_CMotionDayTime) {
        CMotionDayTimeArchivePayload dayTimePayload(cmwIdState, nodeRefs);
        return dayTimePayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CMotionWeather) {
        CMotionWeatherArchivePayload weatherPayload(cmwIdState, nodeRefs);
        return weatherPayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CMotionCmdBase) {
        CMotionCmdBaseArchivePayload cmdPayload;
        return cmdPayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CMotionShader) {
        CMotionShaderArchivePayload shaderPayload(nodeRefs);
        return shaderPayload.Read(chunkId);
    }
    return 0;
}
