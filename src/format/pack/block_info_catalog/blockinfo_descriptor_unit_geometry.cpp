#include "engine/game/game_ctn_block_info.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_unit_geometry.h"
#include "format/pack/block_info_catalog/blockinfo_archive_chunk_ids.h"
#include "format/pack/block_info_catalog/blockinfo_archive_stream.h"
#include "format/archive/tmnf_archive_ids.h"
#include "format/archive/tmnf_gbx_body_reader.h"
GmInt3 BlockInfoDescriptorUnitDimensions::Size() const {
    return {(int32_t)x, (int32_t)y, (int32_t)z};
}

void BlockInfoDescriptorUnitLayout::Clear() {
    defined = false;
    dimensions = BlockInfoDescriptorUnitDimensions{};
    units.clear();
}

int BlockInfoDescriptorUnitLayout::IsDefined() const {
    return defined;
}

int BlockInfoDescriptorUnitLayout::HasUnits() const {
    return defined && !units.empty();
}

GmInt3 BlockInfoDescriptorUnitLayout::Size() const {
    return defined
            ? dimensions.Size()
            : GmInt3{1, 1, 1};
}

const std::vector<BlockInfoDescriptorUnitPlacement> &
BlockInfoDescriptorUnitLayout::Units() const {
    return units;
}

void BlockInfoDescriptorUnitGeometry::Clear() {
    groundLayout.Clear();
    airLayout.Clear();
    unresolvedUnitRefCount = 0u;
}

int BlockInfoDescriptorUnitGeometry::ParseGbxDescriptorBytes(
        const unsigned char *bytes,
        u32 byteCount) {
    u32 classId = 0u;
    u32 bodyOffset = 0u;
    if (bytes == nullptr ||
        !GbxBodyOffsetReader::TryParse(bytes,
                                       byteCount,
                                       &classId,
                                       &bodyOffset)) {
        return 0;
    }
    Clear();
    BlockInfoSizeParseStream stream{};
    stream.bytes = bytes;
    stream.byteCount = byteCount;
    stream.offset = bodyOffset;
    if (!stream.SkipCommonPrefix()) {
        return 0;
    }
    for (;;) {
        u32 chunkId = 0u;
        if (!stream.ReadU32(&chunkId)) {
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            unresolvedUnitRefCount = stream.unresolvedUnitRefCount;
            return groundLayout.defined && airLayout.defined;
        }
        if (IsCGameCtnBlockInfoBasePayloadChunk(chunkId)) {
            u32 ignored = 0u;
            if (!stream.SkipCMwId() ||
                !stream.Skip(12u) ||
                !stream.Skip(4u) ||
                !stream.Skip(4u) ||
                !stream.ReadU32(&ignored) ||
                !stream.SkipNodRef() ||
                !stream.ReadUnitLayout(&groundLayout, nullptr) ||
                !stream.ReadUnitLayout(&airLayout, nullptr)) {
                return 0;
            }
            unresolvedUnitRefCount = stream.unresolvedUnitRefCount;
            return groundLayout.defined && airLayout.defined;
        }
        if (IsCGameCtnBlockInfoIdOnlyChunk(chunkId)) {
            if (!stream.SkipCMwId()) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(
                                      CGameCtnBlockInfoArchiveChunkId::
                                              TwoWordsThenId)) {
            if (!stream.Skip(8u) || !stream.SkipCMwId()) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(
                                      CGameCtnBlockInfoArchiveChunkId::
                                              WordThenId)) {
            if (!stream.Skip(4u) || !stream.SkipCMwId()) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(
                                      CGameCtnBlockInfoArchiveChunkId::IsoA)) {
            if (!stream.Skip(48u)) {
                return 0;
            }
        } else if (IsCGameCtnBlockInfoNaturalOnlyChunk(chunkId)) {
            if (!stream.Skip(4u)) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(
                                      CGameCtnBlockInfoArchiveChunkId::
                                              NaturalAndTwoRefs)) {
            if (!stream.Skip(4u) ||
                !stream.SkipNodRef() ||
                !stream.SkipNodRef()) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(
                                      CGameCtnBlockInfoArchiveChunkId::
                                              NaturalAndThreeRefs)) {
            if (!stream.Skip(4u) ||
                !stream.SkipNodRef() ||
                !stream.SkipNodRef() ||
                !stream.SkipNodRef()) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(
                                      CGameCtnBlockInfoArchiveChunkId::IsoPair)) {
            if (!stream.Skip(96u)) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(
                                      CGameCtnBlockInfoArchiveChunkId::
                                              NaturalAndThreeRefs2)) {
            if (!stream.Skip(4u) ||
                !stream.SkipNodRef() ||
                !stream.SkipNodRef() ||
                !stream.SkipNodRef()) {
                return 0;
            }
        } else if (IsCGameCtnBlockInfoSubclassRefChunk(chunkId)) {
            if (!stream.SkipNodRef()) {
                return 0;
            }
            if (chunkId == TMNF_CLASS_CGameCtnBlockInfoPylon &&
                (!stream.SkipNodRef() || !stream.SkipNodRef())) {
                return 0;
            }
        } else {
            u32 magic = 0u;
            u32 size = 0u;
            if (!stream.ReadU32(&magic) ||
                magic != 0x534b4950u ||
                !stream.ReadU32(&size) ||
                !stream.Skip(size)) {
                return 0;
            }
        }
    }
}

GmInt3 BlockInfoDescriptorUnitGeometry::SizeWhenGround() const {
    return groundLayout.defined
            ? groundLayout.Size()
            : GmInt3{1, 1, 1};
}

GmInt3 BlockInfoDescriptorUnitGeometry::SizeWhenAir() const {
    return airLayout.defined
            ? airLayout.Size()
            : GmInt3{1, 1, 1};
}

const BlockInfoDescriptorUnitLayout &
BlockInfoDescriptorUnitGeometry::GroundLayout() const {
    return groundLayout;
}

const BlockInfoDescriptorUnitLayout &
BlockInfoDescriptorUnitGeometry::AirLayout() const {
    return airLayout;
}
