#include "format/static_solid/static_solid_archive_animation_motion_reader.h"
#include "format/vehicle_tuning/archive_ids.h"
#include "format/static_solid/static_solid_archive_animation_motion_chunk_ids.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
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
        CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodes) {
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
    if (classId == TMNF_CLASS_CMotionPlayer) {
        CMotionPlayerArchivePayload playerPayload(cmwIdState,
                                                  nodeRefs,
                                                  embeddedNodes);
        return playerPayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CMotionDayTime) {
        CMotionDayTimeArchivePayload dayTimePayload(cmwIdState, nodeRefs);
        return dayTimePayload.Read(byteStream, chunkId);
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
