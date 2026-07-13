#include "engine/game/game_ctn_block_info.h"
#include "format/pack/block_info_catalog/blockinfo_archive_stream.h"
#include "format/archive/archive_binary32.h"
#include "format/archive/archive_binary.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include "format/pack/block_info_catalog/blockunitinfo_archive_chunk_ids.h"
#include "format/archive/scene_object_archive_chunk_ids.h"
#include "format/archive/scene_object_link_archive_chunk_ids.h"
#include "format/replay/replay_static_descriptor_limits.h"
#include "format/archive/tmnf_archive_ids.h"
#include "format/archive/mw_id_archive_codec.h"
#include <charconv>
#include <utility>
#include <new>

static void BlockInfoArchiveStream_CopyBytesToFixed(char *dst,
                                                    size_t dstSize,
                                                    const unsigned char *src,
                                                    size_t byteCount) {
    const size_t copy = byteCount < dstSize ? byteCount : dstSize - 1u;
    for (size_t i = 0u; i < copy; i++) {
        dst[i] = static_cast<char>(src[i]);
    }
    dst[copy] = '\0';
}

static int BlockInfoArchiveStream_CopyStringToFixed(
        char *dst,
        size_t dstSize,
        const std::string &src) {
    if (dst == nullptr || dstSize == 0u) {
        return 0;
    }
    const size_t copy = src.size() < dstSize ? src.size() : dstSize - 1u;
    for (size_t i = 0u; i < copy; ++i) {
        dst[i] = src[i];
    }
    dst[copy] = '\0';
    return 1;
}

static void BlockInfoArchiveStream_ResetUnitDefinition(
        BlockInfoDescriptorUnitDefinition &unit) {
    unit = BlockInfoDescriptorUnitDefinition{};
}

int ArchiveLocalCMwIdTable::Append(const char *name) {
    if (maxStoredEntries != 0u && entries.size() >= maxStoredEntries) {
        return 1;
    }
    ArchiveLocalCMwIdEntry row;
    try {
        if (name != nullptr) {
            row.text = name;
        }
        entries.push_back(row);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

const ArchiveLocalCMwIdEntry *ArchiveLocalCMwIdTable::FindOneBased(
        u32 index) const {
    if (index == 0u || (size_t)(index - 1u) >= entries.size()) {
        return nullptr;
    }
    return &entries[index - 1u];
}

void BlockInfoSizeParseStream::SetBlockInfoSourceRefs(
        const BlockInfoDescriptorExternalRefs *refs) {
    blockInfoSourceRefs = refs;
}

int BlockInfoSizeParseStream::ReadU32(u32 *valueOut) {
    if (valueOut == nullptr ||
        offset > byteCount ||
        byteCount - offset < 4u) {
        return 0;
    }
    *valueOut = TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset);
    offset += 4u;
    return 1;
}

int BlockInfoSizeParseStream::ReadU8(u32 *valueOut) {
    if (valueOut == nullptr ||
        offset >= byteCount) {
        return 0;
    }
    *valueOut = bytes[offset++];
    return 1;
}

int BlockInfoSizeParseStream::ReadF32(float *valueOut) {
    u32 bits = 0u;
    if (valueOut == nullptr ||
        !ReadU32(&bits)) {
        return 0;
    }
    *valueOut = (TmnfArchiveBinary32::Decode((bits)));
    return 1;
}

int BlockInfoSizeParseStream::ReadIso4(float iso[12]) {
    if (iso == nullptr) {
        return 0;
    }
    for (u32 i = 0; i < 12u; i++) {
        if (!ReadF32(&iso[i])) {
            return 0;
        }
    }
    return 1;
}

int BlockInfoSizeParseStream::ReadU16(u32 *valueOut) {
    if (valueOut == nullptr ||
        offset > byteCount ||
        byteCount - offset < 2u) {
        return 0;
    }
    *valueOut = (u32)bytes[offset] |
                ((u32)bytes[offset + 1u] << 8u);
    offset += 2u;
    return 1;
}

int BlockInfoSizeParseStream::Skip(u32 skipByteCount) {
    if (offset > byteCount ||
        byteCount - offset < skipByteCount) {
        return 0;
    }
    offset += skipByteCount;
    return 1;
}

int BlockInfoSizeParseStream::SkipString() {
    u32 length = 0u;
    if (!ReadU32(&length)) {
        return 0;
    }
    return Skip(length);
}

int BlockInfoSizeParseStream::ReadStringOwned(std::string *out) {
    u32 length = 0u;
    if (out == nullptr) {
        return 0;
    }
    out->clear();
    if (!ReadU32(&length) ||
        offset > byteCount ||
        byteCount - offset < length) {
        return 0;
    }
    try {
        out->assign((const char *)(bytes + offset), (size_t)length);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    offset += length;
    return 1;
}

int BlockInfoSizeParseStream::ReadStringText(
        char *out,
        size_t outSize) {
    std::string text;
    if (out != nullptr && outSize != 0u) {
        out[0] = '\0';
    }
    if (!ReadStringOwned(&text)) {
        return 0;
    }
    if (out != nullptr && outSize != 0u) {
        return BlockInfoArchiveStream_CopyStringToFixed(out, outSize, text);
    }
    return 1;
}

int BlockInfoSizeParseStream::SkipCMwIdPayload(u32 encodedValue) {
    if (hasCmwIdMode != 0u && cmwIdMode == 1u) {
        return encodedValue == 0u ? 1 : SkipString();
    }
    const TmnfFormat::ArchiveIdentifierWord identifier =
            TmnfFormat::CMwIdArchiveCodec::ParseWord(encodedValue);
    if (!identifier.IsNamed()) {
        return 1;
    }
    if (hasCmwIdMode != 0u && cmwIdMode == 2u) {
        return SkipString();
    }
    if (hasCmwIdMode != 0u && cmwIdMode == 3u) {
        if (identifier.payload != 0u) {
            return 1;
        }
        std::string name;
        return ReadStringOwned(&name) && cmwIdTable.Append(name.c_str());
    }
    if (identifier.payload == 0u) {
        return SkipString();
    }
    return 1;
}

int BlockInfoSizeParseStream::SkipCMwId() {
    u32 encodedValue = 0u;
    if (!ReadU32(&encodedValue)) {
        return 0;
    }
    if (hasCmwIdMode == 0u && encodedValue >= 1u && encodedValue <= 3u) {
        cmwIdMode = encodedValue;
        hasCmwIdMode = 1u;
        if (!ReadU32(&encodedValue)) {
            return 0;
        }
    }
    return SkipCMwIdPayload(encodedValue);
}

int BlockInfoSizeParseStream::ReadCMwIdText(
        char *out,
        size_t outSize) {
    u32 encodedValue = 0u;
    return ReadCMwIdEncodedText(
            &encodedValue,
            out,
            outSize);
}

int BlockInfoSizeParseStream::ReadCMwIdEncodedText(
        u32 *encodedOut,
        char *out,
        size_t outSize) {
    u32 encodedValue = 0u;
    if (out != nullptr && outSize != 0u) {
        out[0] = '\0';
    }
    if (!ReadU32(&encodedValue)) {
        return 0;
    }
    if (hasCmwIdMode == 0u &&
        encodedValue >= 1u &&
        encodedValue <= 3u) {
        cmwIdMode = encodedValue;
        hasCmwIdMode = 1u;
        if (!ReadU32(&encodedValue)) {
            return 0;
        }
    }
    if (encodedOut != nullptr) {
        *encodedOut = encodedValue;
    }
    if (hasCmwIdMode != 0u && cmwIdMode == 1u) {
        return encodedValue == 0u ? 1 : ReadStringText(out, outSize);
    }

    const TmnfFormat::ArchiveIdentifierWord identifier =
            TmnfFormat::CMwIdArchiveCodec::ParseWord(encodedValue);
    if (!identifier.IsNamed()) {
        return 1;
    }

    if (hasCmwIdMode != 0u && cmwIdMode == 2u) {
        return ReadStringText(out, outSize);
    }
    if (hasCmwIdMode != 0u && cmwIdMode == 3u) {
        if (identifier.payload == 0u) {
            std::string name;
            if (!ReadStringOwned(&name)) {
                return 0;
            }
            if (!cmwIdTable.Append(name.c_str())) {
                return 0;
            }
            if (out != nullptr && outSize != 0u) {
                return BlockInfoArchiveStream_CopyStringToFixed(
                        out,
                        outSize,
                        name);
            }
            return 1;
        }
        if (const ArchiveLocalCMwIdEntry *entry =
                    cmwIdTable.FindOneBased(identifier.payload)) {
            if (out != nullptr && outSize != 0u) {
                return BlockInfoArchiveStream_CopyStringToFixed(
                        out,
                        outSize,
                        entry->text);
            }
            return 1;
        }
        /*
         * The archive-local CMwId table is shared across the full classic
         * archive. Some block-info subparsers enter mid-stream; unresolved
         * references are CMwId table lookups and consume no payload bytes.
         */
        return 1;
    }
    if (identifier.payload == 0u) {
        return ReadStringText(out, outSize);
    }
    return 1;
}

int BlockInfoSizeParseStream::ReadCMwIdValue(
        TmnfFormat::ArchiveIdentifierValue *valueOut) {
    if (valueOut == nullptr) {
        return 0;
    }

    u32 encodedValue = 0u;
    if (!ReadU32(&encodedValue)) {
        return 0;
    }
    if (hasCmwIdMode == 0u &&
        encodedValue >= 1u && encodedValue <= 3u) {
        cmwIdMode = encodedValue;
        hasCmwIdMode = 1u;
        if (!ReadU32(&encodedValue)) {
            return 0;
        }
    }

    if (hasCmwIdMode != 0u && cmwIdMode == 1u) {
        TmnfFormat::ArchiveIdentifierValue value;
        if (encodedValue == 0u) {
            *valueOut = std::move(value);
            return 1;
        }
        std::string text;
        if (!ReadStringOwned(&text)) {
            return 0;
        }
        if (encodedValue == 1u) {
            value.kind = TmnfFormat::ArchiveIdentifierKind::LocalName;
            value.name = std::move(text);
        } else if (encodedValue == 2u) {
            value.kind = TmnfFormat::ArchiveIdentifierKind::TranslatedName;
            value.name = std::move(text);
        } else if (encodedValue == 3u) {
            value.kind = TmnfFormat::ArchiveIdentifierKind::Number;
            const char *begin = text.data();
            const char *end = begin + text.size();
            const std::from_chars_result parsed =
                    std::from_chars(begin, end, value.number, 10);
            if (parsed.ec != std::errc{} || parsed.ptr != end) {
                return 0;
            }
        } else {
            return 0;
        }
        *valueOut = std::move(value);
        return 1;
    }

    const TmnfFormat::ArchiveIdentifierWord word =
            TmnfFormat::CMwIdArchiveCodec::ParseWord(encodedValue);
    if (!word.IsNamed()) {
        return TmnfFormat::CMwIdArchiveCodec::Resolve(
                word, {}, *valueOut);
    }

    std::string name;
    if (hasCmwIdMode != 0u && cmwIdMode == 3u && word.payload != 0u) {
        const ArchiveLocalCMwIdEntry *entry =
                cmwIdTable.FindOneBased(word.payload);
        if (entry == nullptr) {
            return 0;
        }
        name = entry->text;
    } else {
        if ((hasCmwIdMode == 0u && word.payload != 0u) ||
            !ReadStringOwned(&name)) {
            return 0;
        }
        if (hasCmwIdMode != 0u && cmwIdMode == 3u &&
            !cmwIdTable.Append(name.c_str())) {
            return 0;
        }
    }
    return TmnfFormat::CMwIdArchiveCodec::Resolve(
            word, name, *valueOut);
}

int BlockInfoSizeParseStream::ReadArchiveNodeRef(
        ArchiveNodeReference *refOut) {
    if (refOut == nullptr) {
        return 0;
    }
    u32 archiveIndex = 0u;
    if (!ReadU32(&archiveIndex)) {
        return 0;
    }
    *refOut = archiveIndex == ArchiveNodeReference::InvalidIndex
            ? ArchiveNodeReference::Invalid()
            : ArchiveNodeReference::FromIndex(archiveIndex);
    return 1;
}

int BlockInfoSizeParseStream::SkipNodRefPayload(
        ArchiveNodeReference sourceNode) {
    if (sourceNode.IsInvalid()) {
        return 1;
    }
    if (offset <= byteCount - 4u) {
        const u32 nextWord =
                TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset);
        if (nextWord == TMNF_CLASS_CGameCtnBlockUnitInfo) {
            u32 classId = 0u;
            u32 ignoredOffset[3] = {0u, 0u, 0u};
            return ReadU32(&classId) &&
                   ParseUnitNode(ignoredOffset,
                                                         nullptr);
        }
    }
    return 1;
}

int BlockInfoSizeParseStream::SkipCountedNodRefs() {
    u32 count = 0u;
    if (!ReadU32(&count) ||
        count > 0x100000u) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        if (!SkipNodRef()) {
            return 0;
        }
    }
    return 1;
}

int BlockInfoSizeParseStream::WordIsSceneObjectLinkChunk(u32 word) {
    return IsCSceneObjectLinkArchiveChunk(word) ||
           word == CMwNodArchiveFacadeSentinel;
}

int BlockInfoSizeParseStream::SkipInlineMotionToSceneChunk() {
    if (offset > byteCount ||
        byteCount - offset < 4u) {
        return 0;
    }
    for (u32 cursor = offset;
         cursor <= byteCount - 4u;
         cursor++) {
        const u32 word =
                TmnfFormat::ArchiveBinary::ReadU32LE(bytes + cursor);
        if (IsCSceneMobilInfo3Chunk(word)) {
            offset = cursor;
            return 1;
        }
    }
    return 0;
}

int BlockInfoSizeParseStream::ReadBlockInfoSourceRef(
        char *descriptorOut,
        size_t descriptorOutSize) {
    ArchiveNodeReference sourceNode =
            ArchiveNodeReference::Invalid();
    if (descriptorOut != nullptr && descriptorOutSize != 0u) {
        descriptorOut[0] = '\0';
    }
    if (!ReadArchiveNodeRef(&sourceNode)) {
        return 0;
    }
    if (sourceNode.IsInvalid()) {
        return 1;
    }
    u32 resolvedExternal = 0u;
    if (blockInfoSourceRefs != nullptr &&
        descriptorOut != nullptr &&
        descriptorOutSize != 0u) {
        if (!blockInfoSourceRefs->ResolveBlockInfoSourceRef(
                    sourceNode,
                    descriptorOut,
                    descriptorOutSize,
                    &resolvedExternal)) {
            return 0;
        }
        if (resolvedExternal != 0u) {
            return 1;
        }
    }
    return SkipNodRefPayload(sourceNode);
}

int BlockInfoSizeParseStream::SkipMobilRefArray() {
    u32 variantCount = 0u;
    if (!ReadU32(&variantCount) ||
        variantCount > 0x10000u) {
        return 0;
    }
    for (u32 variant = 0u; variant < variantCount; variant++) {
        u32 mobilCount = 0u;
        if (!ReadU32(&mobilCount) ||
            mobilCount > 0x10000u) {
            return 0;
        }
        for (u32 i = 0u; i < mobilCount; i++) {
            if (!SkipNodRef()) {
                return 0;
            }
        }
    }
    return 1;
}

int BlockInfoSizeParseStream::ParseUnitNode(
        u32 outOffset[3],
        BlockInfoDescriptorUnitDefinition *unitOut) {
    if (outOffset == nullptr) {
        return 0;
    }
    if (unitOut != nullptr) {
        BlockInfoArchiveStream_ResetUnitDefinition(*unitOut);
    }
    for (;;) {
        u32 chunkId = 0u;
        if (!ReadU32(&chunkId)) {
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            return 1;
        }
        switch (chunkId) {
        case TMNF_CLASS_CGameCtnBlockUnitInfo: {
            u32 ignored = 0u;
            u32 sourceRefCount = 0u;
            if (!ReadU32(&ignored) ||
                !ReadU32(&ignored) ||
                !ReadU32(&ignored) ||
                !ReadU32(&outOffset[0]) ||
                !ReadU32(&outOffset[1]) ||
                !ReadU32(&outOffset[2]) ||
                !ReadU32(&sourceRefCount) ||
                sourceRefCount > 0x100000u) {
                return 0;
            }
            if (unitOut != nullptr) {
                unitOut->offset.x = (int32_t)outOffset[0];
                unitOut->offset.y = (int32_t)outOffset[1];
                unitOut->offset.z = (int32_t)outOffset[2];
            }
            for (u32 i = 0; i < sourceRefCount; i++) {
                char descriptorPath[CGameCtnReplayStaticDescriptorPathCapacity];
                char *descriptorOut =
                        unitOut != nullptr && sourceRefCount == 4u && i < 4u
                                ? descriptorPath
                                : nullptr;
                if (!ReadBlockInfoSourceRef(
                            descriptorOut,
                            descriptorOut != nullptr
                                    ? sizeof(descriptorPath)
                                    : 0u)) {
                    return 0;
                }
                if (descriptorOut != nullptr) {
                    try {
                        unitOut->sourceDescriptorPaths[i] = descriptorPath;
                    } catch (const std::bad_alloc &) {
                        return 0;
                    }
                }
            }
            break;
        }
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::SurfaceAndOffset): {
            char surfaceIdName[64];
            if (!ReadCMwIdText(unitOut != nullptr ? surfaceIdName : nullptr,
                               unitOut != nullptr ? sizeof(surfaceIdName) : 0u) ||
                !Skip(8u)) {
                return 0;
            }
            if (unitOut != nullptr) {
                try {
                    unitOut->surfaceIdName = surfaceIdName;
                } catch (const std::bad_alloc &) {
                    return 0;
                }
            }
            break;
        }
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::Underground): {
            u32 magic = 0u;
            u32 size = 0u;
            u32 underground = 0u;
            if (!ReadU32(&magic) ||
                magic != 0x534b4950u ||
                !ReadU32(&size) ||
                size < 4u ||
                !ReadU32(&underground) ||
                !Skip(size - 4u)) {
                return 0;
            }
            if (unitOut != nullptr) {
                unitOut->underground = underground != 0u;
            }
            break;
        }
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::ReplacementAndJunction): {
            u32 ignored = 0u;
            if (!SkipNodRef() ||
                !SkipCMwId() ||
                !ReadU32(&ignored) ||
                !ReadU32(&ignored)) {
                return 0;
            }
            break;
        }
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::HelperMask):
            if (!Skip(4u)) {
                return 0;
            }
            break;
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::TerrainModifier): {
            TmnfFormat::ArchiveIdentifierValue ignored;
            if (!ReadCMwIdValue(unitOut != nullptr
                                        ? &unitOut->terrainModifierId
                                        : &ignored)) {
                return 0;
            }
            break;
        }
        default: {
            u32 magic = 0u;
            u32 size = 0u;
            if (!ReadU32(&magic) ||
                magic != 0x534b4950u ||
                !ReadU32(&size) ||
                !Skip(size)) {
                return 0;
            }
            break;
        }
        }
    }
}

int BlockInfoSizeParseStream::SkipNodRef() {
    ArchiveNodeReference sourceNode =
            ArchiveNodeReference::Invalid();
    if (!ReadArchiveNodeRef(&sourceNode)) {
        return 0;
    }
    return SkipNodRefPayload(sourceNode);
}

int BlockInfoSizeParseStream::ReadUnitRef(
        u32 outOffset[3],
        u32 *hasOffsetOut,
        BlockInfoDescriptorUnitDefinition *unitOut) {
    ArchiveNodeReference unitNode =
            ArchiveNodeReference::Invalid();
    if (hasOffsetOut != nullptr) {
        *hasOffsetOut = 0u;
    }
    if (unitOut != nullptr) {
        BlockInfoArchiveStream_ResetUnitDefinition(*unitOut);
    }
    if (!ReadArchiveNodeRef(&unitNode)) {
        return 0;
    }
    if (unitNode.IsInvalid()) {
        return 1;
    }
    if (offset <= byteCount - 4u) {
        const u32 nextWord =
                TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset);
        if (nextWord == TMNF_CLASS_CGameCtnBlockUnitInfo) {
            u32 classId = 0u;
            if (!ReadU32(&classId) ||
                !ParseUnitNode(outOffset, unitOut)) {
                return 0;
            }
            if (hasOffsetOut != nullptr) {
                *hasOffsetOut = 1u;
            }
            return 1;
        }
    }
    unresolvedUnitRefCount++;
    return 1;
}

int BlockInfoSizeParseStream::ReadUnitLayout(
        BlockInfoDescriptorUnitLayout *layoutOut,
        std::vector<BlockInfoDescriptorUnitDefinition> *definitionsOut) {
    u32 count = 0u;
    if (!ReadU32(&count) || count > 0x100000u) {
        return 0;
    }
    if (layoutOut != nullptr) {
        layoutOut->Clear();
        layoutOut->defined = true;
    }
    if (definitionsOut != nullptr) {
        definitionsOut->clear();
    }
    for (u32 i = 0u; i < count; i++) {
        u32 offset[3] = {0u, 0u, 0u};
        u32 hasOffset = 0u;
        BlockInfoDescriptorUnitDefinition unit;
        if (!ReadUnitRef(offset, &hasOffset, &unit)) {
            return 0;
        }
        if (hasOffset != 0u) {
            if (offset[0] == UINT32_MAX ||
                offset[1] == UINT32_MAX ||
                offset[2] == UINT32_MAX) {
                return 0;
            }
            unit.offset.x = (int32_t)offset[0];
            unit.offset.y = (int32_t)offset[1];
            unit.offset.z = (int32_t)offset[2];
            if (layoutOut != nullptr) {
                const u32 xSize = offset[0] + 1u;
                const u32 ySize = offset[1] + 1u;
                const u32 zSize = offset[2] + 1u;
                if (xSize > layoutOut->dimensions.x) {
                    layoutOut->dimensions.x = xSize;
                }
                if (ySize > layoutOut->dimensions.y) {
                    layoutOut->dimensions.y = ySize;
                }
                if (zSize > layoutOut->dimensions.z) {
                    layoutOut->dimensions.z = zSize;
                }
            }
            try {
                if (layoutOut != nullptr) {
                    BlockInfoDescriptorUnitPlacement placement;
                    placement.offset = unit.offset;
                    placement.underground = unit.underground;
                    layoutOut->units.push_back(placement);
                }
                if (definitionsOut != nullptr) {
                    definitionsOut->push_back(std::move(unit));
                }
            } catch (const std::bad_alloc &) {
                if (layoutOut != nullptr) {
                    layoutOut->Clear();
                }
                if (definitionsOut != nullptr) {
                    definitionsOut->clear();
                }
                return 0;
            }
        }
    }
    return 1;
}

int BlockInfoSizeParseStream::SkipCommonPrefix() {
    u32 chunkId = 0u;
    u32 value = 0u;
    if (!ReadU32(&chunkId) ||
        chunkId != TMNF_CGameCtnCollectorChunk_0301A006 ||
        !ReadU32(&value) ||
        !ReadU32(&chunkId) ||
        chunkId != TMNF_CGameCtnCollectorChunk_0301A007 ||
        !Skip(24u) ||
        !ReadU32(&chunkId) ||
        chunkId != TMNF_CGameCtnCollectorChunk_LoadableNodRefAndId ||
        !SkipString() ||
        !ReadU32(&value)) {
        return 0;
    }
    if (value != 0u &&
        !Skip(4u)) {
        return 0;
    }
    if (!SkipCMwId() ||
        !ReadU32(&chunkId) ||
        chunkId != TMNF_CGameCtnCollectorChunk_CMwId0301A00A ||
        !SkipCMwId() ||
        !ReadU32(&chunkId) ||
        chunkId != TMNF_CGameCtnCollectorChunk_SGameCtnIdentifier ||
        !SkipCMwId() ||
        !SkipCMwId() ||
        !SkipCMwId()) {
        return 0;
    }
    return 1;
}

int BlockInfoSizeParseStream::ReadStringFixed(
        char *out,
        size_t outSize) {
    u32 length = 0u;
    if (out == nullptr || outSize == 0u ||
        !ReadU32(&length) ||
        length >= outSize ||
        offset > byteCount ||
        byteCount - offset < length) {
        return 0;
    }
    BlockInfoArchiveStream_CopyBytesToFixed(out,
                                            outSize,
                                            bytes + offset,
                                            length);
    offset += length;
    return 1;
}
