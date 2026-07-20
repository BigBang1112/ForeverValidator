#include "engine/game/game_ctn_block_info.h"
#include "format/static_solid/static_solid_archive_blockinfo_reader.h"
#include "format/pack/block_info_catalog/blockinfo_archive_chunk_ids.h"
#include "format/pack/block_info_catalog/blockunitinfo_archive_chunk_ids.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/archive/tmnf_archive_ids.h"
int CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    return nodeRefs != nullptr && nodeRefs->ReadNodPtr(nullptr);
}

int CGameCtnBlockInfoDiscardedRefsPayload::ReadUnitArray(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    constexpr u32 kMaxUnitCount = 0x100000u;
    u32 count = 0u;
    if (!byteStream->ReadU32(&count) ||
        count > kMaxUnitCount) {
        return 0;
    }
    for (u32 i = 0u; i < count; i++) {
        if (!ReadSingle(nodeRefs)) {
            return 0;
        }
    }
    return 1;
}

int CGameCtnBlockInfoDiscardedRefsPayload::ReadMobilVariants(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    constexpr u32 kMaxVariantCount = 0x100000u;
    constexpr u32 kMaxMobilCount = 0x100000u;
    u32 variantCount = 0u;
    if (!byteStream->ReadU32(&variantCount) ||
        variantCount > kMaxVariantCount) {
        return 0;
    }
    for (u32 variant = 0u; variant < variantCount; variant++) {
        u32 mobilCount = 0u;
        if (!byteStream->ReadU32(&mobilCount) ||
            mobilCount > kMaxMobilCount) {
            return 0;
        }
        for (u32 mobil = 0u; mobil < mobilCount; mobil++) {
            if (!ReadSingle(nodeRefs)) {
                return 0;
            }
        }
    }
    return 1;
}

CGameCtnBlockInfoCommonArchivePayload::
        CGameCtnBlockInfoCommonArchivePayload(u32 chunkId)
        : chunkId_(chunkId) {}

int CGameCtnBlockInfoCommonArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || cmwIdState == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    if (!cmwIdState->ReadSkip(*byteStream) ||
        !byteStream->Skip(20u) ||
        !byteStream->Skip(4u) ||
        !CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs) ||
        !CGameCtnBlockInfoDiscardedRefsPayload::ReadUnitArray(byteStream,
                                                              nodeRefs) ||
        !CGameCtnBlockInfoDiscardedRefsPayload::ReadUnitArray(byteStream,
                                                              nodeRefs) ||
        !CGameCtnBlockInfoDiscardedRefsPayload::ReadMobilVariants(byteStream,
                                                                  nodeRefs) ||
        !CGameCtnBlockInfoDiscardedRefsPayload::ReadMobilVariants(byteStream,
                                                                  nodeRefs)) {
        return 0;
    }
    if (CGameCtnBlockInfoBasePayloadHasNat8NaturalNat16(chunkId_) &&
        !byteStream->Skip(7u)) {
        return 0;
    }
    if (CGameCtnBlockInfoBasePayloadHasTrailingNat16(chunkId_) &&
        !byteStream->Skip(2u)) {
        return 0;
    }
    if (CGameCtnBlockInfoBasePayloadHasTrailingString(chunkId_) &&
        !byteStream->ReadStringSkip()) {
        return 0;
    }
    return 1;
}

CGameCtnBlockUnitInfoArchivePayload::CGameCtnBlockUnitInfoArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs) {}

int CGameCtnBlockUnitInfoArchivePayload::ReadSourceRefs(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    if (byteStream == nullptr) {
        return 0;
    }
    constexpr u32 kMaxSourceRefCount = 0x100000u;
    u32 sourceRefCount = 0u;
    if (!byteStream->Skip(24u) ||
        !byteStream->ReadU32(&sourceRefCount) ||
        sourceRefCount > kMaxSourceRefCount) {
        return 0;
    }
    for (u32 i = 0u; i < sourceRefCount; i++) {
        if (!CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_)) {
            return 0;
        }
    }
    return 1;
}

int CGameCtnBlockUnitInfoArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr || nodeRefs_ == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CLASS_CGameCtnBlockUnitInfo:
            return ReadSourceRefs(byteStream);
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::SurfaceAndOffset):
            return cmwIdState_->ReadSkip(*byteStream) &&
                   byteStream->Skip(8u);
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::Underground):
            return byteStream->Skip(4u);
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::ReplacementAndJunction):
            return CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_) &&
                   cmwIdState_->ReadSkip(*byteStream) &&
                   byteStream->Skip(8u);
        case ArchiveChunkIdValue(CGameCtnBlockUnitInfoArchiveChunkId::HelperMask):
            return byteStream->Skip(4u);
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::TerrainModifier):
            return cmwIdState_->ReadSkip(*byteStream);
        default:
            return 0;
    }
}

CGameCtnBlockInfoFamilyArchivePayload::CGameCtnBlockInfoFamilyArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs) {}

int CGameCtnBlockInfoFamilyArchivePayload::ReadCollectorLoadableNodRefAndId(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    if (byteStream == nullptr) {
        return 0;
    }
    u32 hasLoadableRef = 0u;
    if (!byteStream->ReadStringSkip() ||
        !byteStream->ReadBool(&hasLoadableRef)) {
        return 0;
    }
    if (hasLoadableRef != 0u &&
        !CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_)) {
        return 0;
    }
    return cmwIdState_ != nullptr && cmwIdState_->ReadSkip(*byteStream);
}

int CGameCtnBlockInfoFamilyArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState_ == nullptr || nodeRefs_ == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CGameCtnCollectorChunk_0301A006:
            return byteStream->Skip(4u);
        case TMNF_CGameCtnCollectorChunk_0301A007:
            return byteStream->Skip(24u);
        case TMNF_CGameCtnCollectorChunk_LoadableNodRefAndId:
            return ReadCollectorLoadableNodRefAndId(byteStream);
        case TMNF_CGameCtnCollectorChunk_CMwId0301A00A:
            return cmwIdState_->ReadSkip(*byteStream);
        case TMNF_CGameCtnCollectorChunk_SGameCtnIdentifier:
            return cmwIdState_->ReadSkip(*byteStream) &&
                   cmwIdState_->ReadSkip(*byteStream) &&
                   cmwIdState_->ReadSkip(*byteStream);
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::Root):
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::BaseWithMobilRoot):
        case ArchiveChunkIdValue(
                CGameCtnBlockInfoArchiveChunkId::BaseWithNat8NaturalNat16):
        case ArchiveChunkIdValue(
                CGameCtnBlockInfoArchiveChunkId::
                        BaseWithNat8NaturalNat16AndString):
        case ArchiveChunkIdValue(
                CGameCtnBlockInfoArchiveChunkId::
                        BaseWithNat8NaturalNat16AndNat16): {
            CGameCtnBlockInfoCommonArchivePayload commonPayload(chunkId);
            return commonPayload.Read(byteStream, cmwIdState_, nodeRefs_);
        }
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::Id):
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::TwoWordsThenId):
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::WordThenId):
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::IsoA):
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::NaturalAndTwoRefs):
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::NaturalAndThreeRefs):
            return 1;
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::BoolSurfaceFlag):
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::LegacyBooleanA):
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::LegacyBooleanB):
            return byteStream->Skip(4u);
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::IsoPair):
            return byteStream->Skip(96u);
        case ArchiveChunkIdValue(CGameCtnBlockInfoArchiveChunkId::NaturalAndThreeRefs2):
            return byteStream->Skip(4u) && (CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_) &&
           CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_) &&
           CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_));
        case TMNF_CLASS_CGameCtnBlockInfoRoad:
            return CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_);
        case ArchiveChunkIdValue(CGameCtnBlockInfoClipArchiveChunkId::Id):
            return cmwIdState_->ReadSkip(*byteStream);
        case TMNF_CLASS_CGameCtnBlockInfoPylon:
            return (CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_) &&
           CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_) &&
           CGameCtnBlockInfoDiscardedRefsPayload::ReadSingle(nodeRefs_));
        default:
            return 0;
    }
}

int CGameCtnReplayStaticSolidArchiveBlockInfoReader::ParseBlockInfoChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 classId,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (classId == TMNF_CLASS_CGameCtnBlockUnitInfo) {
        CGameCtnBlockUnitInfoArchivePayload unitPayload(cmwIdState, nodeRefs);
        return unitPayload.Read(byteStream, chunkId);
    }
    if (classId == TMNF_CLASS_CGameCtnBlockInfo ||
        classId == TMNF_CLASS_CGameCtnBlockInfoFlat ||
        classId == TMNF_CLASS_CGameCtnBlockInfoFrontier ||
        classId == TMNF_CLASS_CGameCtnBlockInfoClassic ||
        classId == TMNF_CLASS_CGameCtnBlockInfoRoad ||
        classId == TMNF_CLASS_CGameCtnBlockInfoClip ||
        classId == TMNF_CLASS_CGameCtnBlockInfoSlope ||
        classId == TMNF_CLASS_CGameCtnBlockInfoPylon ||
        classId == TMNF_CLASS_CGameCtnBlockInfoRectAsym ||
        classId == TMNF_CLASS_CGameCtnBlock) {
        CGameCtnBlockInfoFamilyArchivePayload blockInfoPayload(cmwIdState,
                                                               nodeRefs);
        return blockInfoPayload.Read(byteStream, chunkId);
    }
    return 0;
}
