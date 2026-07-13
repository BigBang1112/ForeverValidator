#include "format/static_solid/static_solid_archive_hms_reader.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_hms_chunk_ids.h"
#include "format/static_solid/static_solid_archive_node_graph.h"
int CHmsArchiveDiscardedRefsPayload::ReadSingle(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    return nodeRefs != nullptr && nodeRefs->ReadNodPtr(nullptr);
}

int CHmsArchiveDiscardedRefsPayload::ReadFastBuffer(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    constexpr u32 kMaxArchivedRefCount = 0x100000u;
    u32 reservedWord = 0u;
    u32 count = 0u;
    if (!byteStream->ReadU32(&reservedWord) ||
        !byteStream->ReadU32(&count) ||
        count > kMaxArchivedRefCount) {
        return 0;
    }
    for (u32 i = 0u; i < count; i++) {
        if (!ReadSingle(nodeRefs)) {
            return 0;
        }
    }
    return 1;
}

int CHmsReflectGroundArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    if (byteStream == nullptr) {
        return 0;
    }
    u32 enabled = 0u;
    if (!byteStream->ReadBool(&enabled)) {
        return 0;
    }
    return enabled == 0u || byteStream->Skip(6u * 4u);
}

CHmsItemSolidArchivePayload::CHmsItemSolidArchivePayload(u32 hmsNodeIndex)
        : hmsNode_(ArchiveNodeReference::FromIndex(hmsNodeIndex)),
          solidNode_(
                  ArchiveNodeReference::Invalid()) {}

int CHmsItemSolidArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (nodeRefs == nullptr || !nodeRefs->ReadNodeRef(&solidNode_)) {
        return 0;
    }
    return archiveNodeGraph == nullptr ||
           archiveNodeGraph->RememberMobilSolidLink(
                   hmsNode_,
                   solidNode_);
}

int CHmsSoundSourceArchivePayload::ReadSoundRef(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    return CHmsArchiveDiscardedRefsPayload::ReadSingle(nodeRefs);
}

int CHmsSoundSourceArchivePayload::ReadSoundRefRealBool(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    return ReadSoundRef(nodeRefs) &&
           byteStream != nullptr &&
           byteStream->Skip(8u);
}

int CHmsLightArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }

    u32 discardedRefCount = 0u;
    switch (chunkId) {
        case ArchiveChunkIdValue(CHmsLightArchiveChunkId::Root):
        case ArchiveChunkIdValue(CHmsLightArchiveChunkId::GxLight):
            discardedRefCount = 1u;
            break;
        case ArchiveChunkIdValue(CHmsLightArchiveChunkId::ProjectorBitmap):
            discardedRefCount = 2u;
            break;
        case ArchiveChunkIdValue(CHmsLightArchiveChunkId::RefBuffer):
            discardedRefCount = 3u;
            break;
        default:
            return 0;
    }

    if (!byteStream->Skip(4u)) {
        return 0;
    }
    while (discardedRefCount-- != 0u) {
        if (!CHmsArchiveDiscardedRefsPayload::ReadSingle(nodeRefs)) {
            return 0;
        }
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveHmsReader::ParseCHmsZoneChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case ArchiveChunkIdValue(CHmsZoneArchiveChunkId::Root):
            return byteStream->Skip(4u);
        case ArchiveChunkIdValue(CHmsZoneArchiveChunkId::EmptyOld):
            return 1;
        case ArchiveChunkIdValue(CHmsZoneArchiveChunkId::MaterialsAndFog):
            return CHmsArchiveDiscardedRefsPayload::ReadFastBuffer(byteStream,
                                                                   nodeRefs) &&
                   byteStream->Skip(28u);
        case ArchiveChunkIdValue(CHmsZoneArchiveChunkId::ReflectGround):
            return CHmsReflectGroundArchivePayload().Read(byteStream);
        case ArchiveChunkIdValue(CHmsZoneArchiveChunkId::NodRef):
            return CHmsArchiveDiscardedRefsPayload::ReadSingle(nodeRefs);
        case ArchiveChunkIdValue(CHmsZoneArchiveChunkId::MaterialBuffer):
            return CHmsArchiveDiscardedRefsPayload::ReadFastBuffer(byteStream,
                                                                   nodeRefs);
        case ArchiveChunkIdValue(CHmsZoneArchiveChunkId::RefBuffer):
            return CHmsArchiveDiscardedRefsPayload::ReadSingle(nodeRefs);
        default:
            return 0;
    }
}

int CGameCtnReplayStaticSolidArchiveHmsReader::ParseCHmsItemChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        u32 hmsNodeIndex,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::Root):
            return byteStream->Skip(16u);
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::Solid): {
            CHmsItemSolidArchivePayload solidPayload(hmsNodeIndex);
            return solidPayload.Read(archiveNodeGraph, nodeRefs);
        }
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::CorpusArray):
            return CHmsArchiveDiscardedRefsPayload::ReadFastBuffer(byteStream,
                                                                   nodeRefs);
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags20):
            return byteStream->Skip(20u);
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags17):
            return byteStream->Skip(17u);
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags21):
            return byteStream->Skip(21u);
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags25A):
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags25B):
            return byteStream->Skip(25u);
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::Flags12):
            return byteStream->Skip(12u);
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::Flags4):
            return byteStream->Skip(4u);
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsLegacy):
            return byteStream->Skip(8u);
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16A):
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16B):
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16C):
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16D):
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16E):
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16F):
        case ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16G):
            return byteStream->Skip(10u);
        default:
            return 0;
    }
}

int CGameCtnReplayStaticSolidArchiveHmsReader::ParseCHmsSoundSourceChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::Root):
            return 1;
        case ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::SoundRef):
            return CHmsSoundSourceArchivePayload::ReadSoundRef(nodeRefs);
        case ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::VolumeAndPitch):
            return byteStream->Skip(8u);
        case ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::VolumePitchAndPan):
        case ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::SoundRefIntegerBool):
            return byteStream->Skip(12u);
        case ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::SoundRefRealBool):
            return CHmsSoundSourceArchivePayload::ReadSoundRefRealBool(
                    byteStream,
                    nodeRefs);
        case ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::PocEmitter):
            return byteStream->Skip(4u);
        default:
            return 0;
    }
}

int CGameCtnReplayStaticSolidArchiveHmsReader::ParseHmsChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        u32 hmsNodeIndex,
        u32 classId,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (classId == TMNF_CLASS_CHmsZone ||
        classId == TMNF_CLASS_CHmsZoneDynamic) {
        return ParseCHmsZoneChunk(byteStream,
                                  chunkId,
                                  nodeRefs);
    }
    if (classId == TMNF_CLASS_CHmsItem) {
        return ParseCHmsItemChunk(byteStream,
                                  archiveNodeGraph,
                                  hmsNodeIndex,
                                  chunkId,
                                  nodeRefs);
    }
    if (classId == TMNF_CLASS_CHmsSoundSource) {
        return ParseCHmsSoundSourceChunk(byteStream,
                                         chunkId,
                                         nodeRefs);
    }
    if (classId == TMNF_CLASS_CHmsLight) {
        CHmsLightArchivePayload lightPayload;
        return lightPayload.Read(byteStream, chunkId, nodeRefs);
    }
    return 0;
}
