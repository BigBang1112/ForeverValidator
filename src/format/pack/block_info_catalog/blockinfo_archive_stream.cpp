#include "engine/game/game_ctn_block_info.h"
#include "format/pack/block_info_catalog/blockinfo_archive_stream.h"
#include "format/archive/archive_binary32.h"
#include "format/archive/archive_binary.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include "format/pack/block_info_catalog/blockunitinfo_archive_chunk_ids.h"
#include "format/archive/scene_object_archive_chunk_ids.h"
#include "format/archive/scene_object_link_archive_chunk_ids.h"
#include "format/static_solid/static_solid_archive_animation_motion_chunk_ids.h"
#include "format/replay/replay_static_descriptor_limits.h"
#include "format/archive/tmnf_archive_ids.h"
#include "format/archive/mw_id_archive_codec.h"
#include <charconv>
#include <limits>
#include <map>
#include <new>
#include <utility>

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
    if (offset <= byteCount && byteCount - offset >= 4u) {
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

namespace {

struct BlockInfoInlineCMotionsNodeState {
    u32 classId = 0u;
    u32 boneCount = 0u;
    u32 hasBoneCount = 0u;
    ArchiveNodeReference skeleton = ArchiveNodeReference::Invalid();
};

class BlockInfoInlineCMotionsParser {
public:
    explicit BlockInfoInlineCMotionsParser(BlockInfoSizeParseStream *stream)
            : stream_(stream) {}

    int Parse(ArchiveNodeReference rootNode, u32 rootClassId) {
        return stream_ != nullptr &&
               stream_->blockInfoSourceRefs != nullptr &&
               rootNode.IsValid() &&
               ValidateNodeBounds(rootNode) &&
               IsSupportedInlineClass(rootClassId) &&
               RememberNode(rootNode, rootClassId) &&
               ParseNodeArchive(rootNode, rootClassId);
    }

    int ChangesLocations() const {
        return hasSkeletonMotion_ != 0 || hasEmitterLeavesMotion_ != 0;
    }

    int CollisionUsesInitialTransform() const {
        return hasEmitterLeavesMotion_ != 0 && hasSkeletonMotion_ == 0;
    }

private:
    static constexpr u32 MaxCount = 0x100000u;
    static constexpr u32 MaxNestedDepth = 64u;
    static constexpr u32 CFuncKeysLocationByteCount =
            7u * sizeof(float);

    static int IsSupportedInlineClass(u32 classId) {
        return classId == TMNF_CLASS_CMotions ||
               classId == TMNF_CLASS_CMotionPlayer ||
               classId == TMNF_CLASS_CMotionSkel ||
               classId == TMNF_CLASS_CMotionEmitterLeaves ||
               classId == TMNF_CLASS_CMotionManagerLeaves ||
               classId == TMNF_CLASS_CMotionShader ||
               classId == TMNF_CLASS_CFuncKeysSkel ||
               classId == TMNF_CLASS_CFuncSkel ||
               classId == TMNF_CLASS_CFuncKeysNatural;
    }

    int ValidateNodeBounds(ArchiveNodeReference node) const {
        return node.IsValid() &&
               stream_->blockInfoSourceRefs != nullptr &&
               node.Index() <= stream_->blockInfoSourceRefs
                                      ->NodeCountForFormatBounds();
    }

    int RememberNode(ArchiveNodeReference node, u32 classId) {
        if (!ValidateNodeBounds(node)) {
            return 0;
        }
        const auto known = nodes_.find(node.Index());
        if (known != nodes_.end()) {
            return known->second.classId == classId;
        }
        try {
            nodes_.emplace(node.Index(),
                           BlockInfoInlineCMotionsNodeState{classId});
            if (classId == TMNF_CLASS_CMotionSkel) {
                hasSkeletonMotion_ = 1;
            } else if (classId == TMNF_CLASS_CMotionEmitterLeaves) {
                hasEmitterLeavesMotion_ = 1;
            }
            return 1;
        } catch (const std::bad_alloc &) {
            return 0;
        }
    }

    int ReadNodeRef(ArchiveNodeReference *nodeOut = nullptr) {
        ArchiveNodeReference node = ArchiveNodeReference::Invalid();
        if (!stream_->ReadArchiveNodeRef(&node) || node.IsDeferred()) {
            return 0;
        }
        if (nodeOut != nullptr) {
            *nodeOut = node;
        }
        if (node.IsInvalid()) {
            return 1;
        }
        if (!ValidateNodeBounds(node)) {
            return 0;
        }
        if (stream_->blockInfoSourceRefs->FindReference(node) != nullptr) {
            return 1;
        }
        if (nodes_.find(node.Index()) != nodes_.end()) {
            return 1;
        }
        if (stream_->offset > stream_->byteCount ||
            stream_->byteCount - stream_->offset < sizeof(u32)) {
            return 0;
        }
        const u32 classId = TmnfFormat::ArchiveBinary::ReadU32LE(
                stream_->bytes + stream_->offset);
        if (!IsSupportedInlineClass(classId)) {
            return 0;
        }
        u32 consumedClassId = 0u;
        return stream_->ReadU32(&consumedClassId) &&
               RememberNode(node, consumedClassId) &&
               ParseNodeArchive(node, consumedClassId);
    }

    int ReadCountedNodeRefs() {
        u32 count = 0u;
        if (!stream_->ReadU32(&count) || count > MaxCount) {
            return 0;
        }
        for (u32 index = 0u; index < count; ++index) {
            if (!ReadNodeRef()) {
                return 0;
            }
        }
        return 1;
    }

    int ReadCFuncSkelBones(ArchiveNodeReference node,
                           int hasElementArrays) {
        u32 boneCount = 0u;
        if (!stream_->ReadU32(&boneCount) || boneCount > MaxCount) {
            return 0;
        }
        for (u32 bone = 0u; bone < boneCount; ++bone) {
            if (!stream_->SkipCMwId()) {
                return 0;
            }
            if (hasElementArrays) {
                u32 elementCount = 0u;
                if (!stream_->ReadU32(&elementCount) ||
                    elementCount > MaxCount ||
                    elementCount >
                            std::numeric_limits<u32>::max() / sizeof(u32) ||
                    !stream_->Skip(elementCount * sizeof(u32))) {
                    return 0;
                }
            }
        }
        const auto known = nodes_.find(node.Index());
        if (known == nodes_.end() ||
            known->second.classId != TMNF_CLASS_CFuncSkel) {
            return 0;
        }
        known->second.boneCount = boneCount;
        known->second.hasBoneCount = 1u;
        return 1;
    }

    int ReadCFuncSkelLegacyTable(int idsInsteadOfStrings) {
        u32 count = 0u;
        if (!stream_->ReadU32(&count) || count > MaxCount) {
            return 0;
        }
        for (u32 index = 0u; index < count; ++index) {
            const int keyRead = idsInsteadOfStrings
                    ? stream_->SkipCMwId()
                    : stream_->SkipString();
            if (!keyRead || !stream_->Skip(sizeof(u32))) {
                return 0;
            }
        }
        return 1;
    }

    int ParseCFuncSkelChunk(ArchiveNodeReference node, u32 chunkId) {
        switch (chunkId) {
        case ArchiveChunkIdValue(CFuncSkelArchiveChunkId::LegacyStringTable):
            return ReadCFuncSkelLegacyTable(0) &&
                   ReadCFuncSkelBones(node, 1);
        case ArchiveChunkIdValue(CFuncSkelArchiveChunkId::LegacyIdTable):
            return ReadCFuncSkelLegacyTable(1) &&
                   ReadCFuncSkelBones(node, 1);
        case ArchiveChunkIdValue(CFuncSkelArchiveChunkId::BoneIds):
            return ReadCFuncSkelBones(node, 0);
        default:
            return 0;
        }
    }

    int ParseCFuncKeysChunk(u32 chunkId) {
        switch (chunkId) {
        case ArchiveChunkIdValue(CFuncKeysArchiveChunkId::Root):
            return stream_->SkipString();
        case ArchiveChunkIdValue(CFuncKeysArchiveChunkId::XValues):
        case TMNF_CLASS_CFuncKeysNatural: {
            u32 count = 0u;
            return stream_->ReadU32(&count) && count <= MaxCount &&
                   count <= std::numeric_limits<u32>::max() / sizeof(float) &&
                   stream_->Skip(count * sizeof(float));
        }
        case ArchiveChunkIdValue(
                CFuncKeysArchiveChunkId::TwoRealCompatibilityRange):
            return stream_->Skip(2u * sizeof(float));
        case ArchiveChunkIdValue(CFuncKeysArchiveChunkId::Id):
            return stream_->SkipCMwId();
        default:
            return 0;
        }
    }

    int ParseCFuncKeysSkelChunk(ArchiveNodeReference node, u32 chunkId) {
        if (IsCFuncKeysInfo1Chunk(chunkId) ||
            IsCFuncKeysInfo3Chunk(chunkId)) {
            return ParseCFuncKeysChunk(chunkId);
        }
        const auto keys = nodes_.find(node.Index());
        if (keys == nodes_.end() ||
            keys->second.classId != TMNF_CLASS_CFuncKeysSkel) {
            return 0;
        }
        switch (chunkId) {
        case ArchiveChunkIdValue(CFuncKeysSkelArchiveChunkId::SkeletonRef): {
            ArchiveNodeReference skeleton = ArchiveNodeReference::Invalid();
            if (!ReadNodeRef(&skeleton) || !skeleton.IsValid()) {
                return 0;
            }
            const auto parsedSkeleton = nodes_.find(skeleton.Index());
            if (parsedSkeleton == nodes_.end() ||
                parsedSkeleton->second.classId != TMNF_CLASS_CFuncSkel ||
                parsedSkeleton->second.hasBoneCount == 0u) {
                return 0;
            }
            keys->second.skeleton = skeleton;
            return 1;
        }
        case ArchiveChunkIdValue(CFuncKeysSkelArchiveChunkId::Locations): {
            if (!keys->second.skeleton.IsValid()) {
                return 0;
            }
            const auto skeleton = nodes_.find(keys->second.skeleton.Index());
            u32 frameCount = 0u;
            if (skeleton == nodes_.end() ||
                skeleton->second.hasBoneCount == 0u ||
                !stream_->ReadU32(&frameCount) || frameCount > MaxCount ||
                (skeleton->second.boneCount != 0u &&
                 frameCount > std::numeric_limits<u32>::max() /
                                      skeleton->second.boneCount)) {
                return 0;
            }
            const u32 locationCount =
                    frameCount * skeleton->second.boneCount;
            return locationCount == 0u ||
                   (locationCount <= std::numeric_limits<u32>::max() /
                                             CFuncKeysLocationByteCount &&
                    stream_->Skip(locationCount *
                                  CFuncKeysLocationByteCount));
        }
        default:
            return 0;
        }
    }

    int ParseCMotionCmdBaseArchive() {
        for (;;) {
            u32 chunkId = 0u;
            if (!stream_->ReadU32(&chunkId)) {
                return 0;
            }
            if (chunkId == CMwNodArchiveFacadeSentinel) {
                return 1;
            }
            switch (chunkId) {
            case TMNF_CLASS_CMotionCmdBase:
                if (!stream_->Skip(4u * sizeof(float))) return 0;
                break;
            case ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::BaseBool):
                if (!stream_->Skip(5u * sizeof(u32))) return 0;
                break;
            case ArchiveChunkIdValue(
                    CMotionCmdBaseArchiveChunkId::BaseParamsRef):
                if (!stream_->Skip(6u * sizeof(u32))) return 0;
                break;
            case ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::PeriodAndPhase):
                if (!stream_->Skip(2u * sizeof(float))) return 0;
                break;
            case ArchiveChunkIdValue(
                    CMotionCmdBaseArchiveChunkId::PeriodPhaseAndTimeBase):
                if (!stream_->Skip(3u * sizeof(float))) return 0;
                break;
            case ArchiveChunkIdValue(
                    CMotionCmdBaseArchiveChunkId::
                            PeriodPhaseTimeBaseAndLoop):
                if (!stream_->Skip(4u * sizeof(u32))) return 0;
                break;
            case ArchiveChunkIdValue(
                    CMotionCmdBaseArchiveChunkId::
                            PeriodPhaseTimeBaseLoopAndOffset):
                if (!stream_->Skip(5u * sizeof(u32))) return 0;
                break;
            default:
                return 0;
            }
        }
    }

    int ReadCMotionPlayerOptionalRef() {
        u32 hasRef = 0u;
        return stream_->ReadU32(&hasRef) &&
               (hasRef == 0u || ReadNodeRef());
    }

    int ReadCMotionPlayerBaseAndRefs(int hasTrackRef, int hasStateBool) {
        u32 ignoredStateBool = 0u;
        if ((hasTrackRef != 0 && !ReadNodeRef()) ||
            !ParseCMotionCmdBaseArchive() ||
            !stream_->Skip(sizeof(u32)) ||
            (hasStateBool != 0 &&
             !stream_->ReadU32(&ignoredStateBool)) ||
            !stream_->SkipCMwId()) {
            return 0;
        }
        return ReadCountedNodeRefs();
    }

    int ParseCMotionPlayerChunk(u32 chunkId) {
        switch (chunkId) {
        case TMNF_CLASS_CMotion:
            return stream_->SkipCMwId();
        case ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::Root):
            return ReadCMotionPlayerOptionalRef();
        case ArchiveChunkIdValue(
                CMotionPlayerArchiveChunkId::ConditionalBaseAndId):
            return ReadCMotionPlayerOptionalRef() && stream_->SkipCMwId();
        case ArchiveChunkIdValue(
                CMotionPlayerArchiveChunkId::TrackThenBaseAndRefs):
            return ReadCMotionPlayerBaseAndRefs(1, 0);
        case ArchiveChunkIdValue(CMotionPlayerArchiveChunkId::BaseAndRefs):
            return ReadCMotionPlayerBaseAndRefs(0, 0);
        case ArchiveChunkIdValue(
                CMotionPlayerArchiveChunkId::FullStateAndRefs):
            return ReadCMotionPlayerBaseAndRefs(0, 1);
        default:
            return 0;
        }
    }

    int ParseCMotionEmitterLeavesChunk(u32 chunkId) {
        if (chunkId == TMNF_CLASS_CMotion) {
            return stream_->SkipCMwId();
        }
        if (chunkId != ArchiveChunkIdValue(
                               CMotionEmitterLeavesArchiveChunkId::
                                       PositionAndScalarRadius) &&
            chunkId != ArchiveChunkIdValue(
                               CMotionEmitterLeavesArchiveChunkId::
                                       PositionAndRadius)) {
            return 0;
        }
        if (!ReadNodeRef() || !stream_->Skip(3u * sizeof(float))) {
            return 0;
        }
        const u32 radiusCount = chunkId == ArchiveChunkIdValue(
                CMotionEmitterLeavesArchiveChunkId::
                        PositionAndScalarRadius) ? 1u : 3u;
        return stream_->Skip(radiusCount * sizeof(float));
    }

    int ParseCMotionManagerLeavesChunk(u32 chunkId) {
        return chunkId == ArchiveChunkIdValue(
                                  CMotionManagerLeavesArchiveChunkId::
                                          MobilModel) &&
               ReadNodeRef();
    }

    static int IsChunkSupported(u32 classId, u32 chunkId) {
        if (classId == TMNF_CLASS_CMotions) {
            return chunkId == TMNF_CLASS_CMotion ||
                   chunkId == ArchiveChunkIdValue(
                                      CMotionsArchiveChunkId::Root) ||
                   chunkId == ArchiveChunkIdValue(
                                      CMotionsArchiveChunkId::MotionRefs);
        }
        if (classId == TMNF_CLASS_CMotionPlayer) {
            return IsCMotionPlayerInfo1Chunk(chunkId) ||
                   IsCMotionPlayerInfo3Chunk(chunkId);
        }
        if (classId == TMNF_CLASS_CMotionSkel) {
            return IsCMotionSkelInfo3Chunk(chunkId);
        }
        if (classId == TMNF_CLASS_CMotionEmitterLeaves) {
            return IsCMotionEmitterLeavesInfo1Chunk(chunkId) ||
                   IsCMotionEmitterLeavesInfo3Chunk(chunkId);
        }
        if (classId == TMNF_CLASS_CMotionManagerLeaves) {
            return IsCMotionManagerLeavesInfo3Chunk(chunkId);
        }
        if (classId == TMNF_CLASS_CMotionShader) {
            return IsCMotionShaderInfo3Chunk(chunkId);
        }
        if (classId == TMNF_CLASS_CFuncSkel) {
            return IsCFuncSkelInfo1Chunk(chunkId) ||
                   IsCFuncSkelInfo3Chunk(chunkId);
        }
        if (classId == TMNF_CLASS_CFuncKeysSkel) {
            return IsCFuncKeysSkelInfo1Chunk(chunkId) ||
                   IsCFuncKeysSkelInfo3Chunk(chunkId);
        }
        if (classId == TMNF_CLASS_CFuncKeysNatural) {
            return IsCFuncKeysInfo1Chunk(chunkId) ||
                   IsCFuncKeysInfo3Chunk(chunkId) ||
                   IsCFuncKeysNaturalInfo3Chunk(chunkId);
        }
        return 0;
    }

    static int IsOwningSceneMobilChunk(u32 rawChunkId) {
        const u32 chunkId =
                NormalizePackedCSceneObjectOrMobilArchiveChunkId(rawChunkId);
        return IsCSceneObjectInfo1Chunk(chunkId) ||
               IsCSceneObjectInfo3Chunk(chunkId) ||
               IsCSceneMobilInfo1Chunk(chunkId) ||
               IsCSceneMobilInfo3Chunk(chunkId);
    }

    int ParseChunk(ArchiveNodeReference node, u32 classId, u32 chunkId) {
        if (classId == TMNF_CLASS_CMotions) {
            if (chunkId == TMNF_CLASS_CMotion) {
                return stream_->SkipCMwId();
            }
            if (chunkId == ArchiveChunkIdValue(CMotionsArchiveChunkId::Root) ||
                chunkId == ArchiveChunkIdValue(
                                   CMotionsArchiveChunkId::MotionRefs)) {
                return ReadCountedNodeRefs();
            }
            return 0;
        }
        if (classId == TMNF_CLASS_CMotionPlayer) {
            return ParseCMotionPlayerChunk(chunkId);
        }
        if (classId == TMNF_CLASS_CMotionSkel) {
            return chunkId ==
                           ArchiveChunkIdValue(CMotionSkelArchiveChunkId::KeysRef) &&
                   ReadNodeRef();
        }
        if (classId == TMNF_CLASS_CMotionEmitterLeaves) {
            return ParseCMotionEmitterLeavesChunk(chunkId);
        }
        if (classId == TMNF_CLASS_CMotionManagerLeaves) {
            return ParseCMotionManagerLeavesChunk(chunkId);
        }
        if (classId == TMNF_CLASS_CMotionShader) {
            return IsCMotionShaderInfo3Chunk(chunkId) &&
                   ReadNodeRef() && ReadNodeRef() && ReadNodeRef();
        }
        if (classId == TMNF_CLASS_CFuncSkel) {
            return ParseCFuncSkelChunk(node, chunkId);
        }
        if (classId == TMNF_CLASS_CFuncKeysSkel) {
            return ParseCFuncKeysSkelChunk(node, chunkId);
        }
        if (classId == TMNF_CLASS_CFuncKeysNatural) {
            return ParseCFuncKeysChunk(chunkId);
        }
        return 0;
    }

    int ParseNodeArchive(ArchiveNodeReference node, u32 classId) {
        if (depth_ >= MaxNestedDepth) {
            return 0;
        }
        ++depth_;
        for (;;) {
            if (stream_->offset > stream_->byteCount ||
                stream_->byteCount - stream_->offset < sizeof(u32)) {
                --depth_;
                return 0;
            }
            const u32 chunkId = TmnfFormat::ArchiveBinary::ReadU32LE(
                    stream_->bytes + stream_->offset);
            if (chunkId == CMwNodArchiveFacadeSentinel) {
                stream_->offset += sizeof(u32);
                --depth_;
                return 1;
            }
            if (!IsChunkSupported(classId, chunkId)) {
                int ownerBoundary = depth_ == 1u &&
                        IsOwningSceneMobilChunk(chunkId);
                --depth_;
                return ownerBoundary;
            }
            stream_->offset += sizeof(u32);
            if (!ParseChunk(node, classId, chunkId)) {
                --depth_;
                return 0;
            }
        }
    }

    BlockInfoSizeParseStream *stream_ = nullptr;
    std::map<ArchiveNodeReference::IndexType,
             BlockInfoInlineCMotionsNodeState> nodes_;
    u32 depth_ = 0u;
    int hasSkeletonMotion_ = 0;
    int hasEmitterLeavesMotion_ = 0;
};

}  // namespace

int BlockInfoSizeParseStream::ParseInlineCMotionsArchive(
        ArchiveNodeReference rootNode,
        int *changesLocationsOut,
        int *collisionUsesInitialTransformOut) {
    return ParseInlineMotionArchive(
            rootNode,
            TMNF_CLASS_CMotions,
            changesLocationsOut,
            collisionUsesInitialTransformOut);
}

int BlockInfoSizeParseStream::ParseInlineMotionArchive(
        ArchiveNodeReference rootNode,
        u32 rootClassId,
        int *changesLocationsOut,
        int *collisionUsesInitialTransformOut) {
    if (changesLocationsOut != nullptr) {
        *changesLocationsOut = 0;
    }
    if (collisionUsesInitialTransformOut != nullptr) {
        *collisionUsesInitialTransformOut = 0;
    }
    if (blockInfoSourceRefs == nullptr) {
        return 0;
    }
    BlockInfoSizeParseStream candidate{};
    try {
        candidate = *this;
    } catch (const std::bad_alloc &) {
        return 0;
    }
    BlockInfoInlineCMotionsParser parser(&candidate);
    if (!parser.Parse(rootNode, rootClassId)) {
        return 0;
    }
    *this = std::move(candidate);
    if (changesLocationsOut != nullptr) {
        *changesLocationsOut = parser.ChangesLocations();
    }
    if (collisionUsesInitialTransformOut != nullptr) {
        *collisionUsesInitialTransformOut =
                parser.CollisionUsesInitialTransform();
    }
    return 1;
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
    u32 parsedOffset[3] = {0u, 0u, 0u};
    BlockInfoDescriptorUnitDefinition parsedUnit;
    for (;;) {
        u32 chunkId = 0u;
        if (!ReadU32(&chunkId)) {
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            outOffset[0] = parsedOffset[0];
            outOffset[1] = parsedOffset[1];
            outOffset[2] = parsedOffset[2];
            if (unitOut != nullptr) {
                *unitOut = std::move(parsedUnit);
            }
            return 1;
        }
        switch (chunkId) {
        case TMNF_CLASS_CGameCtnBlockUnitInfo: {
            u32 junctionMask = 0u;
            u32 helper = 0u;
            u32 ignored = 0u;
            u32 sourceRefCount = 0u;
            if (!ReadU32(&junctionMask) ||
                !ReadU32(&helper) ||
                !ReadU32(&ignored) ||
                !ReadU32(&parsedOffset[0]) ||
                !ReadU32(&parsedOffset[1]) ||
                !ReadU32(&parsedOffset[2]) ||
                !ReadU32(&sourceRefCount) ||
                sourceRefCount > 0x100000u) {
                return 0;
            }
            parsedUnit.junctionMask = junctionMask;
            parsedUnit.helperMask = helper != 0u ? 0xffu : 0u;
            parsedUnit.hasJunctionMask = true;
            parsedUnit.hasHelperMask = true;
            parsedUnit.offset.x = (int32_t)parsedOffset[0];
            parsedUnit.offset.y = (int32_t)parsedOffset[1];
            parsedUnit.offset.z = (int32_t)parsedOffset[2];
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
                        parsedUnit.sourceDescriptorPaths[i] = descriptorPath;
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
                    parsedUnit.surfaceIdName = surfaceIdName;
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
            parsedUnit.underground = underground != 0u;
            parsedUnit.hasUnderground = true;
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
                CGameCtnBlockUnitInfoArchiveChunkId::HelperMask): {
            u32 helperMask = 0u;
            if (!ReadU32(&helperMask)) {
                return 0;
            }
            parsedUnit.helperMask = helperMask;
            parsedUnit.hasHelperMask = true;
            break;
        }
        case ArchiveChunkIdValue(
                CGameCtnBlockUnitInfoArchiveChunkId::TerrainModifier): {
            TmnfFormat::ArchiveIdentifierValue ignored;
            if (!ReadCMwIdValue(unitOut != nullptr
                                        ? &parsedUnit.terrainModifierId
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
    if (outOffset == nullptr) {
        return 0;
    }
    ArchiveNodeReference unitNode =
            ArchiveNodeReference::Invalid();
    if (!ReadArchiveNodeRef(&unitNode)) {
        return 0;
    }
    if (unitNode.IsInvalid()) {
        if (hasOffsetOut != nullptr) {
            *hasOffsetOut = 0u;
        }
        if (unitOut != nullptr) {
            BlockInfoArchiveStream_ResetUnitDefinition(*unitOut);
        }
        return 1;
    }
    if (offset <= byteCount && byteCount - offset >= 4u) {
        const u32 nextWord =
                TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset);
        if (nextWord == TMNF_CLASS_CGameCtnBlockUnitInfo) {
            u32 classId = 0u;
            u32 parsedOffset[3] = {0u, 0u, 0u};
            BlockInfoDescriptorUnitDefinition parsedUnit;
            if (!ReadU32(&classId) ||
                !ParseUnitNode(parsedOffset, &parsedUnit)) {
                return 0;
            }
            outOffset[0] = parsedOffset[0];
            outOffset[1] = parsedOffset[1];
            outOffset[2] = parsedOffset[2];
            if (hasOffsetOut != nullptr) {
                *hasOffsetOut = 1u;
            }
            if (unitOut != nullptr) {
                *unitOut = std::move(parsedUnit);
            }
            return 1;
        }
    }
    unresolvedUnitRefCount++;
    if (hasOffsetOut != nullptr) {
        *hasOffsetOut = 0u;
    }
    if (unitOut != nullptr) {
        BlockInfoArchiveStream_ResetUnitDefinition(*unitOut);
    }
    return 1;
}

int BlockInfoSizeParseStream::ReadUnitLayout(
        BlockInfoDescriptorUnitLayout *layoutOut,
        std::vector<BlockInfoDescriptorUnitDefinition> *definitionsOut) {
    u32 count = 0u;
    if (!ReadU32(&count) || count > 0x100000u) {
        return 0;
    }
    BlockInfoDescriptorUnitLayout parsedLayout;
    std::vector<BlockInfoDescriptorUnitDefinition> parsedDefinitions;
    if (layoutOut != nullptr) {
        parsedLayout.defined = true;
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
                if (xSize > parsedLayout.dimensions.x) {
                    parsedLayout.dimensions.x = xSize;
                }
                if (ySize > parsedLayout.dimensions.y) {
                    parsedLayout.dimensions.y = ySize;
                }
                if (zSize > parsedLayout.dimensions.z) {
                    parsedLayout.dimensions.z = zSize;
                }
            }
            try {
                if (layoutOut != nullptr) {
                    BlockInfoDescriptorUnitPlacement placement;
                    placement.offset = unit.offset;
                    placement.underground = unit.underground;
                    parsedLayout.units.push_back(placement);
                }
                if (definitionsOut != nullptr) {
                    parsedDefinitions.push_back(std::move(unit));
                }
            } catch (const std::bad_alloc &) {
                return 0;
            }
        }
    }
    if (layoutOut != nullptr) {
        *layoutOut = std::move(parsedLayout);
    }
    if (definitionsOut != nullptr) {
        *definitionsOut = std::move(parsedDefinitions);
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
