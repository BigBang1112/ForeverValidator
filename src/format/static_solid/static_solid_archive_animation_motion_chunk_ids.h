#ifndef TMNF_STATIC_SOLID_ARCHIVE_ANIMATION_MOTION_CHUNK_IDS_H
#define TMNF_STATIC_SOLID_ARCHIVE_ANIMATION_MOTION_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/tmnf_archive_ids.h"
#include "format/vehicle_tuning/archive_ids.h"
enum class CFuncKeysArchiveChunkId : u32 {
    Root = TMNF_CLASS_CFuncKeys,
    XValues = 0x05002001u,
    TwoRealCompatibilityRange = 0x05002002u,
    Id = 0x05002003u,
};

enum class CFuncTreeSubVisualSequenceArchiveChunkId : u32 {
    Root = TMNF_CLASS_CFuncTreeSubVisualSequence,
    PlugFloat = 0x0500b000u,
    PlugFloatAndId = 0x0500b001u,
    PlugVec2 = 0x0500b002u,
    PlugVec3 = 0x0500b003u,
    PlugVec4 = 0x0500b004u,
    PlugVec4AndId = 0x0500b005u,
    SequenceId = 0x05031001u,
    KeyRef = 0x05031002u,
    InlineIndexRange = 0x05031003u,
};

enum class CMotionPlayerArchiveChunkId : u32 {
    Root = TMNF_CLASS_CMotionPlayer,
    ConditionalBaseAndId = 0x08034001u,
    TrackThenBaseAndRefs = 0x08034002u,
    BaseAndRefs = 0x08034003u,
    FullStateAndRefs = 0x08034004u,
};

enum class CMotionCmdBaseArchiveChunkId : u32 {
    Root = TMNF_CLASS_CMotionCmdBase,
    BaseBool = 0x08029001u,
    BaseParamsRef = 0x08029002u,
    PeriodAndPhase = 0x0802a000u,
    PeriodPhaseAndTimeBase = 0x0802a001u,
    PeriodPhaseTimeBaseAndLoop = 0x0802a002u,
    PeriodPhaseTimeBaseLoopAndOffset = 0x0802a003u,
};

constexpr u32 ArchiveChunkIdValue(CFuncKeysArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CMotionPlayerArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCFuncKeysInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CFuncKeysArchiveChunkId::Root) ||
           chunkId == ArchiveChunkIdValue(CFuncKeysArchiveChunkId::TwoRealCompatibilityRange);
}

constexpr int IsCFuncKeysInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CFuncKeysArchiveChunkId::XValues) ||
           chunkId == ArchiveChunkIdValue(CFuncKeysArchiveChunkId::Id);
}

constexpr int IsCFuncKeysNaturalInfo3Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CFuncKeysNatural;
}

constexpr int IsCFuncTreeSubVisualSequenceInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugFloat) ||
           chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugFloatAndId) ||
           chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugVec2) ||
           chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugVec3) ||
           chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugVec4) ||
           chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::Root);
}

constexpr int IsCFuncTreeSubVisualSequenceInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::PlugVec4AndId) ||
           chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::SequenceId) ||
           chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::KeyRef) ||
           chunkId == ArchiveChunkIdValue(CFuncTreeSubVisualSequenceArchiveChunkId::InlineIndexRange);
}

constexpr int IsCMotionInfo3Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CMotion;
}

constexpr int IsCMotionPlayerInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::Root) ||
           chunkId == ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::ConditionalBaseAndId) ||
           chunkId == ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::TrackThenBaseAndRefs) ||
           chunkId == ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::BaseAndRefs);
}

constexpr int IsCMotionPlayerInfo3Chunk(u32 chunkId) {
    return IsCMotionInfo3Chunk(chunkId) ||
           chunkId == ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::FullStateAndRefs);
}

constexpr int IsCMotionDayTimeInfo3Chunk(u32 chunkId) {
    return IsCMotionInfo3Chunk(chunkId) ||
           chunkId == TMNF_CLASS_CMotionDayTime;
}

constexpr int IsCMotionCmdBaseInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::Root) ||
           chunkId == ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::BaseBool) ||
           chunkId == ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::PeriodAndPhase) ||
           chunkId == ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::PeriodPhaseAndTimeBase) ||
           chunkId == ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::PeriodPhaseTimeBaseLoopAndOffset);
}

constexpr int IsCMotionCmdBaseInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::BaseParamsRef) ||
           chunkId == ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::PeriodPhaseTimeBaseAndLoop);
}

constexpr int IsCMotionShaderInfo3Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CMotionShader;
}

#endif
