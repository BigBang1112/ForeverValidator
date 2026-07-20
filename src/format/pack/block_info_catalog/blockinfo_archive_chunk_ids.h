#ifndef TMNF_BLOCKINFO_ARCHIVE_CHUNK_IDS_H
#define TMNF_BLOCKINFO_ARCHIVE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "engine/game/game_ctn_block_info.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/tmnf_archive_ids.h"
enum class CGameCtnBlockInfoArchiveChunkId : u32 {
    Root = TMNF_CLASS_CGameCtnBlockInfo,
    BaseWithMobilRoot = 0x0304e001u,
    Id = 0x0304e002u,
    TwoWordsThenId = 0x0304e003u,
    BaseWithNat8NaturalNat16 = 0x0304e004u,
    BaseWithNat8NaturalNat16AndString = 0x0304e005u,
    WordThenId = 0x0304e006u,
    IsoA = 0x0304e007u,
    BaseWithNat8NaturalNat16AndNat16 = 0x0304e008u,
    BoolSurfaceFlag = 0x0304e009u,
    NaturalAndTwoRefs = 0x0304e00au,
    NaturalAndThreeRefs = 0x0304e00bu,
    IsoPair = 0x0304e00cu,
    LegacyBooleanA = 0x0304e00du,
    NaturalAndThreeRefs2 = 0x0304e00eu,
    LegacyBooleanB = 0x0304e00fu,
};

enum class CGameCtnBlockInfoClipArchiveChunkId : u32 {
    Root = TMNF_CLASS_CGameCtnBlockInfoClip,
    Id = 0x03053002u,
};

constexpr u32 ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CGameCtnBlockInfoClipArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCGameCtnBlockInfoArchiveClass(u32 classId) {
    return classId == TMNF_CLASS_CGameCtnBlockInfo ||
           classId == TMNF_CLASS_CGameCtnBlockInfoFlat ||
           classId == TMNF_CLASS_CGameCtnBlockInfoFrontier ||
           classId == TMNF_CLASS_CGameCtnBlockInfoClassic ||
           classId == TMNF_CLASS_CGameCtnBlockInfoRoad ||
           classId == TMNF_CLASS_CGameCtnBlockInfoClip ||
           classId == TMNF_CLASS_CGameCtnBlockInfoSlope ||
           classId == TMNF_CLASS_CGameCtnBlockInfoPylon ||
           classId == TMNF_CLASS_CGameCtnBlockInfoRectAsym ||
           classId == TMNF_CLASS_CGameCtnBlock;
}

constexpr int IsCGameCtnBlockInfoBasePayloadChunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CGameCtnBlockInfo ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::BaseWithMobilRoot) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::BaseWithNat8NaturalNat16) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::
                                      BaseWithNat8NaturalNat16AndString) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::
                                      BaseWithNat8NaturalNat16AndNat16);
}

constexpr int CGameCtnBlockInfoBasePayloadHasNat8NaturalNat16(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::BaseWithNat8NaturalNat16) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::
                                      BaseWithNat8NaturalNat16AndString) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::
                                      BaseWithNat8NaturalNat16AndNat16);
}

constexpr int CGameCtnBlockInfoBasePayloadHasTrailingNat16(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::
                                      BaseWithNat8NaturalNat16AndString) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::
                                      BaseWithNat8NaturalNat16AndNat16);
}

constexpr int CGameCtnBlockInfoBasePayloadHasTrailingString(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::
                                      BaseWithNat8NaturalNat16AndNat16);
}

constexpr int IsCGameCtnBlockInfoInfo1Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CGameCtnBlockInfo ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::BaseWithMobilRoot) ||
           chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::Id) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::TwoWordsThenId) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::BaseWithNat8NaturalNat16) ||
           chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::WordThenId) ||
           chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::IsoA) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::
                                      BaseWithNat8NaturalNat16AndNat16) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::NaturalAndTwoRefs) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::NaturalAndThreeRefs);
}

constexpr int IsCGameCtnBlockInfoInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::
                                      BaseWithNat8NaturalNat16AndString) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::BoolSurfaceFlag) ||
           chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::IsoPair) ||
           chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::LegacyBooleanA) ||
           chunkId == ArchiveChunkIdValue(
                              CGameCtnBlockInfoArchiveChunkId::NaturalAndThreeRefs2) ||
           chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::LegacyBooleanB);
}

constexpr int IsCGameCtnBlockInfoIdOnlyChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::Id) ||
           chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoClipArchiveChunkId::Id);
}

constexpr int IsCGameCtnBlockInfoNaturalOnlyChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::BoolSurfaceFlag) ||
           chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::LegacyBooleanA) ||
           chunkId == ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::LegacyBooleanB);
}

constexpr int IsCGameCtnBlockInfoSubclassRefChunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CGameCtnBlockInfoRoad ||
           chunkId == TMNF_CLASS_CGameCtnBlockInfoPylon;
}

#endif
