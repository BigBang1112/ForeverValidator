#include "format/replay/replay_file.h"
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <zlib.h>

#include "format/archive/archive_binary32.h"
#include "format/compression/lzo1x.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/mw_id_archive_codec.h"
#include <new>
namespace {

using Byte = std::uint8_t;
using Nat32 = std::uint32_t;

constexpr Nat32 GbxVersion6 = 6u;
constexpr Byte GbxBinary = 0x42u;
constexpr Byte GbxUncompressed = 0x55u;
constexpr Byte GbxCompressed = 0x43u;
constexpr Byte GbxReference = 0x52u;
constexpr Nat32 GbxHeaderLengthMask = 0x7fffffffu;
constexpr Nat32 GbxMaximumHeaderChunks = 0x1000u;
constexpr Nat32 ArchiveFacade = 0xfacade01u;
constexpr Nat32 SkipSentinel = 0x534b4950u;

constexpr Nat32 ReplayEmbeddedChallengeChunk = 0x03093002u;
constexpr Nat32 ReplayGhostBufferChunk = 0x03093014u;
constexpr Nat32 ReplayMaximumBodyChunks = 64u;

constexpr std::size_t MaximumChallengeBodyBytes = 0x01000000u;
constexpr std::size_t MaximumMapBlocks = 8192u;
constexpr std::size_t MaximumChallengeIds = 0x1000u;
constexpr std::size_t MaximumSkinNodes = 0x1000u;
constexpr Nat32 NullArchiveNode = 0xffffffffu;
constexpr Nat32 BlockVariantMask = 0x3fu;
constexpr Nat32 BlockMobilSelectionMask = 0x0fc0u;
constexpr Nat32 BlockMobilSelectionShift = 6u;
constexpr Nat32 BlockAutomaticMobilSelection = 0x3fu;
constexpr Nat32 BlockUsesGroundMobilFamily = 0x1000u;
constexpr Nat32 BlockCreatesMobil = 0x2000u;
constexpr Nat32 BlockUsesCustomSize = 0x4000u;
constexpr Nat32 BlockHasSkin = 0x8000u;
constexpr Nat32 BlockUsesSecondarySource = 0x10000u;

constexpr Nat32 ChallengeClassId = 0x03043000u;
constexpr Nat32 ChallengeIdentifierChunk = 0x0304300du;
constexpr Nat32 ChallengeDecorationChunk = 0x03043011u;
constexpr Nat32 ChallengeBlocksChunk = 0x0304301fu;
constexpr Nat32 ChallengePostBlocksChunk = 0x03043021u;

constexpr Nat32 GhostBaseStateChunk = 0x0303f005u;
constexpr Nat32 GhostGenericWrappedChunk = 0x0304f001u;
constexpr Nat32 GhostValidationInputChunk = 0x03092019u;
constexpr Nat32 GhostArchiveCountLimit = 0x10000000u;
constexpr std::size_t GhostInputEventBytes = 9u;
constexpr std::size_t GhostStateDataOffset = 24u;
constexpr Nat32 GhostFixedTimeStep = 1u;
constexpr Nat32 GhostEncodingMode = 0u;
constexpr Nat32 GhostStateVersion = 9u;
constexpr Nat32 GhostEncodedSampleBytes = 61u;
constexpr double InputAxisScale = 1.0 / 65536.0;

struct Bytes {
    const Byte *data = nullptr;
    std::size_t size = 0u;

    bool Empty() const {
        return size == 0u;
    }

    bool Contains(std::size_t offset, std::size_t length) const {
        return offset <= size && length <= size - offset;
    }

    bool ReadU16(std::size_t offset, Nat32 *out) const {
        if (out == nullptr || !Contains(offset, 2u)) {
            return false;
        }
        *out = static_cast<Nat32>(data[offset]) |
               (static_cast<Nat32>(data[offset + 1u]) << 8u);
        return true;
    }

    bool ReadU32(std::size_t offset, Nat32 *out) const {
        if (out == nullptr || !Contains(offset, 4u)) {
            return false;
        }
        *out = static_cast<Nat32>(data[offset]) |
               (static_cast<Nat32>(data[offset + 1u]) << 8u) |
               (static_cast<Nat32>(data[offset + 2u]) << 16u) |
               (static_cast<Nat32>(data[offset + 3u]) << 24u);
        return true;
    }

    std::optional<Bytes> Slice(
            std::size_t offset,
            std::size_t length) const {
        if (!Contains(offset, length)) {
            return std::nullopt;
        }
        return Bytes{data + offset, length};
    }
};

ReplayFileReadError DecompressReplayBody(
        const std::vector<Byte> &fileStorage,
        std::vector<Byte> *outBody) {
    if (outBody == nullptr) {
        return ReplayFileReadError::InvalidRequest;
    }
    const Bytes file{fileStorage.data(), fileStorage.size()};
    if (file.size < 29u || file.data[0] != 'G' ||
        file.data[1] != 'B' || file.data[2] != 'X') {
        return ReplayFileReadError::InvalidContainer;
    }

    Nat32 version = 0u;
    Nat32 rootClassId = 0u;
    Nat32 headerSize = 0u;
    Nat32 headerChunkCount = 0u;
    if (!file.ReadU16(3u, &version) ||
        !file.ReadU32(9u, &rootClassId) ||
        !file.ReadU32(13u, &headerSize) ||
        !file.ReadU32(17u, &headerChunkCount)) {
        return ReplayFileReadError::InvalidContainer;
    }
    (void)rootClassId;
    if (version < 3u || version > GbxVersion6 ||
        file.data[5] != GbxBinary ||
        file.data[6] != GbxUncompressed ||
        file.data[7] != GbxCompressed ||
        file.data[8] != GbxReference ||
        headerChunkCount > GbxMaximumHeaderChunks ||
        headerSize < 4u || headerSize > file.size ||
        17u > file.size - headerSize) {
        return ReplayFileReadError::InvalidContainer;
    }

    const std::size_t headerEnd = 17u + headerSize;
    constexpr std::size_t tableOffset = 21u;
    if (headerEnd > file.size || tableOffset > headerEnd ||
        headerChunkCount > (headerEnd - tableOffset) / 8u) {
        return ReplayFileReadError::InvalidContainer;
    }
    std::size_t headerPayload =
            tableOffset + static_cast<std::size_t>(headerChunkCount) * 8u;
    for (Nat32 index = 0u; index < headerChunkCount; ++index) {
        Nat32 chunkId = 0u;
        Nat32 encodedSize = 0u;
        const std::size_t entry = tableOffset + index * 8u;
        if (!file.ReadU32(entry, &chunkId) ||
            !file.ReadU32(entry + 4u, &encodedSize)) {
            return ReplayFileReadError::InvalidContainer;
        }
        (void)chunkId;
        const std::size_t chunkSize = encodedSize & GbxHeaderLengthMask;
        if (headerPayload > headerEnd ||
            chunkSize > headerEnd - headerPayload) {
            return ReplayFileReadError::InvalidContainer;
        }
        headerPayload += chunkSize;
    }

    Nat32 referenceCount = 0u;
    Nat32 locationCount = 0u;
    if (!file.ReadU32(headerEnd, &referenceCount) ||
        !file.ReadU32(headerEnd + 4u, &locationCount) ||
        locationCount != 0u) {
        return ReplayFileReadError::InvalidContainer;
    }
    (void)referenceCount;

    const std::size_t bodyBlock = headerEnd + 8u;
    Nat32 uncompressedSize = 0u;
    Nat32 compressedSize = 0u;
    if (!file.ReadU32(bodyBlock, &uncompressedSize) ||
        !file.ReadU32(bodyBlock + 4u, &compressedSize)) {
        return ReplayFileReadError::InvalidContainer;
    }
    const std::size_t compressedOffset = bodyBlock + 8u;
    if (uncompressedSize == 0u || compressedSize == 0u ||
        !file.Contains(compressedOffset, compressedSize)) {
        return ReplayFileReadError::InvalidContainer;
    }

    try {
        outBody->assign(uncompressedSize, 0u);
    } catch (const std::bad_alloc &) {
        return ReplayFileReadError::AllocationFailed;
    }
    std::size_t decodedSize = outBody->size();
    const int lzoResult = lzo1x_decompress_safe(
            file.data + compressedOffset,
            compressedSize,
            outBody->data(),
            &decodedSize);
    Nat32 firstChunk = 0u;
    const Bytes body{outBody->data(), outBody->size()};
    if (lzoResult != 0 || decodedSize != uncompressedSize ||
        !body.ReadU32(0u, &firstChunk)) {
        outBody->clear();
        return ReplayFileReadError::RootBodyDecompressionFailed;
    }
    return ReplayFileReadError::Success;
}

bool IsReplayWrappedChunk(Nat32 chunkId) {
    switch (chunkId) {
    case 0x03093006u:
    case 0x03093007u:
    case 0x03093008u:
    case 0x0309300fu:
    case 0x03093013u:
        return true;
    default:
        return false;
    }
}

struct ReplayRecordSections {
    std::optional<Bytes> embeddedChallenge;
    Bytes ghostBuffer;
};

ReplayFileReadError FindReplayRecordSections(
        Bytes replayBody,
        ReplayRecordSections *out) {
    if (out == nullptr) {
        return ReplayFileReadError::InvalidRequest;
    }
    *out = ReplayRecordSections{};
    std::size_t offset = 0u;
    for (Nat32 chunkIndex = 0u;
         chunkIndex < ReplayMaximumBodyChunks;
         ++chunkIndex) {
        Nat32 chunkId = 0u;
        if (!replayBody.ReadU32(offset, &chunkId)) {
            return ReplayFileReadError::MissingGhostBuffer;
        }
        if (chunkId == ArchiveFacade) {
            return ReplayFileReadError::MissingGhostBuffer;
        }
        if (chunkId == ReplayGhostBufferChunk) {
            const std::optional<Bytes> ghost = replayBody.Slice(
                    offset, replayBody.size - offset);
            if (!ghost) {
                return ReplayFileReadError::MissingGhostBuffer;
            }
            out->ghostBuffer = *ghost;
            return ReplayFileReadError::Success;
        }
        if (chunkId == ReplayEmbeddedChallengeChunk) {
            Nat32 nestedSize = 0u;
            if (!replayBody.ReadU32(offset + 4u, &nestedSize)) {
                return ReplayFileReadError::MissingGhostBuffer;
            }
            const std::optional<Bytes> nested =
                    replayBody.Slice(offset + 8u, nestedSize);
            if (!nested) {
                return ReplayFileReadError::MissingGhostBuffer;
            }
            out->embeddedChallenge = *nested;
            offset += 8u + nestedSize;
            continue;
        }
        if (IsReplayWrappedChunk(chunkId)) {
            Nat32 sentinel = 0u;
            Nat32 payloadSize = 0u;
            if (!replayBody.ReadU32(offset + 4u, &sentinel) ||
                !replayBody.ReadU32(offset + 8u, &payloadSize) ||
                sentinel != SkipSentinel ||
                !replayBody.Contains(offset + 12u, payloadSize)) {
                return ReplayFileReadError::MissingGhostBuffer;
            }
            offset += 12u + payloadSize;
            continue;
        }
        return ReplayFileReadError::MissingGhostBuffer;
    }
    return ReplayFileReadError::MissingGhostBuffer;
}

class ArchiveCursor {
public:
    explicit ArchiveCursor(Bytes bytes) : bytes_(bytes) {}

    std::size_t Offset() const {
        return offset_;
    }

    void SetOffset(std::size_t offset) {
        offset_ = offset;
    }

    bool ReadU32(Nat32 *out) {
        if (!bytes_.ReadU32(offset_, out)) {
            return false;
        }
        offset_ += 4u;
        return true;
    }

    bool PeekU32(Nat32 *out) const {
        return bytes_.ReadU32(offset_, out);
    }

    bool ReadString(std::string *out) {
        Nat32 length = 0u;
        if (out == nullptr || !ReadU32(&length) ||
            length > 0x00200000u ||
            !bytes_.Contains(offset_, length)) {
            return false;
        }
        try {
            out->assign(
                    reinterpret_cast<const char *>(bytes_.data + offset_),
                    length);
        } catch (const std::bad_alloc &) {
            return false;
        }
        offset_ += length;
        return true;
    }

    bool Advance(std::size_t byteCount) {
        if (!bytes_.Contains(offset_, byteCount)) {
            return false;
        }
        offset_ += byteCount;
        return true;
    }

    Bytes AllBytes() const {
        return bytes_;
    }

private:
    Bytes bytes_;
    std::size_t offset_ = 0u;
};

struct ArchiveIdentifier {
    Nat32 value = 0xffffffffu;
    std::string name;
};

class ChallengeIdentifierTable {
public:
    bool Read(ArchiveCursor *cursor, ArchiveIdentifier *out) {
        if (cursor == nullptr || out == nullptr) {
            return false;
        }
        Nat32 modeOrIdentifier = 0u;
        if (!cursor->PeekU32(&modeOrIdentifier)) {
            return false;
        }
        if (modeOrIdentifier >= 1u && modeOrIdentifier <= 3u) {
            mode_ = modeOrIdentifier;
            if (!cursor->Advance(4u)) {
                return false;
            }
        }
        if (mode_ != 3u) {
            return false;
        }

        Nat32 archiveWord = 0u;
        if (!cursor->ReadU32(&archiveWord)) {
            return false;
        }
        const TmnfFormat::ArchiveIdentifierWord parsed =
                TmnfFormat::CMwIdArchiveCodec::ParseWord(archiveWord);
        if (!parsed.IsNamed()) {
            *out = ArchiveIdentifier{archiveWord, {}};
            return true;
        }
        if (parsed.payload != 0u) {
            const std::size_t index = parsed.payload - 1u;
            if (index >= entries_.size()) {
                return false;
            }
            *out = entries_[index];
            return true;
        }
        if (entries_.size() >= MaximumChallengeIds) {
            return false;
        }

        ArchiveIdentifier identifier;
        if (!cursor->ReadString(&identifier.name)) {
            return false;
        }
        if (!TmnfFormat::CMwIdArchiveCodec::EncodeNameReference(
                    parsed.kind,
                    static_cast<Nat32>(entries_.size() + 1u),
                    identifier.value)) {
            return false;
        }
        try {
            entries_.push_back(identifier);
        } catch (const std::bad_alloc &) {
            return false;
        }
        *out = std::move(identifier);
        return true;
    }

    bool ReadTriple(
            ArchiveCursor *cursor,
            ArchiveIdentifier (&parts)[3]) {
        return Read(cursor, &parts[0]) &&
               Read(cursor, &parts[1]) &&
               Read(cursor, &parts[2]);
    }

private:
    Nat32 mode_ = 3u;
    std::vector<ArchiveIdentifier> entries_;
};

bool FindFacade(Bytes bytes, std::size_t offset, std::size_t *outOffset) {
    if (outOffset == nullptr) {
        return false;
    }
    while (bytes.Contains(offset, 4u)) {
        Nat32 value = 0u;
        if (!bytes.ReadU32(offset, &value)) {
            return false;
        }
        if (value == ArchiveFacade) {
            *outOffset = offset;
            return true;
        }
        ++offset;
    }
    return false;
}

bool SkipInlineArchiveNode(ArchiveCursor *cursor) {
    if (cursor == nullptr) {
        return false;
    }
    Nat32 nodeIndex = 0u;
    if (!cursor->ReadU32(&nodeIndex)) {
        return false;
    }
    if (nodeIndex == NullArchiveNode) {
        return true;
    }
    Nat32 classId = 0u;
    if (!cursor->ReadU32(&classId)) {
        return false;
    }
    (void)classId;
    std::size_t facadeOffset = 0u;
    if (!FindFacade(cursor->AllBytes(), cursor->Offset(), &facadeOffset)) {
        return false;
    }
    cursor->SetOffset(facadeOffset + 4u);
    return true;
}

bool SkipSkinArchiveNode(
        ArchiveCursor *cursor,
        std::vector<Nat32> *seenNodes) {
    if (cursor == nullptr || seenNodes == nullptr) {
        return false;
    }
    Nat32 nodeIndex = 0u;
    if (!cursor->ReadU32(&nodeIndex)) {
        return false;
    }
    if (nodeIndex == NullArchiveNode) {
        return true;
    }
    for (Nat32 seen : *seenNodes) {
        if (seen == nodeIndex) {
            return true;
        }
    }
    if (seenNodes->size() >= MaximumSkinNodes) {
        return false;
    }
    Nat32 classId = 0u;
    if (!cursor->ReadU32(&classId)) {
        return false;
    }
    (void)classId;
    std::size_t facadeOffset = 0u;
    if (!FindFacade(cursor->AllBytes(), cursor->Offset(), &facadeOffset)) {
        return false;
    }
    try {
        seenNodes->push_back(nodeIndex);
    } catch (const std::bad_alloc &) {
        return false;
    }
    cursor->SetOffset(facadeOffset + 4u);
    return true;
}

bool IsChallengeWrappedChunk(Nat32 chunkId) {
    switch (chunkId) {
    case 0x03043017u:
    case 0x03043018u:
    case 0x03043019u:
    case 0x0304301cu:
    case 0x03043029u:
        return true;
    default:
        return false;
    }
}

bool SkipWrapped(Bytes bytes, std::size_t offset, std::size_t *outNext) {
    Nat32 sentinel = 0u;
    Nat32 payloadSize = 0u;
    if (outNext == nullptr ||
        !bytes.ReadU32(offset + 4u, &sentinel) ||
        !bytes.ReadU32(offset + 8u, &payloadSize) ||
        sentinel != SkipSentinel ||
        !bytes.Contains(offset + 12u, payloadSize)) {
        return false;
    }
    *outNext = offset + 12u + payloadSize;
    return true;
}

CGameCtnReplayMapIdentifier ToMapIdentifier(ArchiveIdentifier identifier) {
    return CGameCtnReplayMapIdentifier(std::move(identifier.name));
}

BlockPlacementState DecodeBlockPlacement(Nat32 flags) {
    const Nat32 archivedMobilSelection =
            (flags & BlockMobilSelectionMask) >> BlockMobilSelectionShift;
    const std::optional<u32> mobilSelection =
            archivedMobilSelection == BlockAutomaticMobilSelection
            ? std::nullopt
            : std::optional<u32>(archivedMobilSelection);
    return BlockPlacementState::Create(
            flags & BlockVariantMask,
            mobilSelection,
            (flags & BlockUsesGroundMobilFamily) != 0u
                    ? BlockMobilFamily::Ground
                    : BlockMobilFamily::Air,
            (flags & BlockCreatesMobil) != 0u,
            (flags & BlockUsesCustomSize) != 0u,
            (flags & BlockUsesSecondarySource) != 0u
                    ? BlockPlacementSource::Secondary
                    : BlockPlacementSource::Primary);
}

bool ReadChallengePreamble(
        Bytes challengeBody,
        ArchiveCursor &cursor,
        ChallengeIdentifierTable &identifiers) {
    Nat32 chunkId = 0u;
    ArchiveIdentifier ignoredChallengeIdentifiers[3];
    Nat32 ignoredDecorationVersion = 0u;
    return cursor.ReadU32(&chunkId) && chunkId == ChallengeIdentifierChunk &&
           identifiers.ReadTriple(&cursor, ignoredChallengeIdentifiers) &&
           cursor.ReadU32(&chunkId) && chunkId == ChallengeDecorationChunk &&
           SkipInlineArchiveNode(&cursor) && SkipInlineArchiveNode(&cursor) &&
           cursor.ReadU32(&ignoredDecorationVersion) &&
           challengeBody.Contains(cursor.Offset(), 0u);
}

bool SeekChallengeBlocks(Bytes bytes, ArchiveCursor &cursor) {
    while (bytes.Contains(cursor.Offset(), 4u)) {
        const std::size_t chunkOffset = cursor.Offset();
        Nat32 chunkId = 0u;
        if (!cursor.ReadU32(&chunkId)) {
            return false;
        }
        if (chunkId == ChallengeBlocksChunk) {
            return true;
        }
        if (!IsChallengeWrappedChunk(chunkId)) {
            return false;
        }
        std::size_t next = 0u;
        if (!SkipWrapped(bytes, chunkOffset, &next)) {
            return false;
        }
        cursor.SetOffset(next);
    }
    return false;
}

struct ChallengeBlockHeader {
    ArchiveIdentifier mapIdentifiers[3];
    ArchiveIdentifier decorationIdentifiers[3];
    GmNat3 size{};
    Nat32 blockVersion = 0u;
    Nat32 blockCount = 0u;
};

std::optional<ChallengeBlockHeader> ReadChallengeBlockHeader(
        ArchiveCursor &cursor,
        ChallengeIdentifierTable &identifiers) {
    ChallengeBlockHeader header;
    std::string ignoredMapName;
    Nat32 ignoredSerializedLightmap = 0u;
    if (!identifiers.ReadTriple(&cursor, header.mapIdentifiers) ||
        !cursor.ReadString(&ignoredMapName) ||
        !identifiers.ReadTriple(&cursor, header.decorationIdentifiers) ||
        !cursor.ReadU32(&header.size.x) ||
        !cursor.ReadU32(&header.size.y) ||
        !cursor.ReadU32(&header.size.z) ||
        !cursor.ReadU32(&ignoredSerializedLightmap) ||
        !cursor.ReadU32(&header.blockVersion) ||
        !cursor.ReadU32(&header.blockCount) ||
        header.blockCount > MaximumMapBlocks) {
        return std::nullopt;
    }
    return header;
}

std::optional<std::vector<CGameCtnReplayMapInputBlock>> ReadChallengeBlocks(
        Bytes bytes,
        ArchiveCursor &cursor,
        ChallengeIdentifierTable &identifiers,
        const ChallengeBlockHeader &header) {
    std::vector<CGameCtnReplayMapInputBlock> blocks;
    std::vector<Nat32> seenSkinNodes;
    try {
        blocks.reserve(header.blockCount);
        seenSkinNodes.reserve(MaximumSkinNodes);
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }

    for (Nat32 blockIndex = 0u; blockIndex < header.blockCount; ++blockIndex) {
        ArchiveIdentifier blockIdentifier;
        if (!identifiers.Read(&cursor, &blockIdentifier) ||
            !bytes.Contains(cursor.Offset(), 8u)) {
            return std::nullopt;
        }
        const Byte direction = bytes.data[cursor.Offset()];
        const GmNat3 coordinate{
            bytes.data[cursor.Offset() + 1u],
            bytes.data[cursor.Offset() + 2u],
            bytes.data[cursor.Offset() + 3u],
        };
        Nat32 flags = 0u;
        if (!cursor.Advance(4u) || !cursor.ReadU32(&flags)) {
            return std::nullopt;
        }
        if ((flags & BlockHasSkin) != 0u) {
            ArchiveIdentifier ignoredSkin;
            if (!identifiers.Read(&cursor, &ignoredSkin) ||
                !SkipSkinArchiveNode(&cursor, &seenSkinNodes)) {
                return std::nullopt;
            }
        }
        try {
            blocks.emplace_back(
                    CGameCtnReplayBlockPlacementId(blockIndex),
                    ToMapIdentifier(std::move(blockIdentifier)),
                    ToMapIdentifier(header.mapIdentifiers[1]),
                    static_cast<ECardinalDir>(direction),
                    coordinate,
                    DecodeBlockPlacement(flags));
        } catch (const std::bad_alloc &) {
            return std::nullopt;
        }
    }
    return blocks;
}

bool HasChallengePostBlocks(Bytes bytes, std::size_t offset) {
    while (bytes.Contains(offset, 4u)) {
        Nat32 chunkId = 0u;
        if (!bytes.ReadU32(offset, &chunkId)) {
            return false;
        }
        if (chunkId == ChallengePostBlocksChunk) {
            return true;
        }
        if (!SkipWrapped(bytes, offset, &offset)) {
            return false;
        }
    }
    return false;
}

ReplayFileReadError ParseChallengeBody(
        Bytes challengeBody,
        CGameCtnReplayMapInput *outMap) {
    if (outMap == nullptr) {
        return ReplayFileReadError::InvalidRequest;
    }
    ArchiveCursor cursor(challengeBody);
    ChallengeIdentifierTable identifiers;
    if (!ReadChallengePreamble(challengeBody, cursor, identifiers) ||
        !SeekChallengeBlocks(challengeBody, cursor)) {
        return ReplayFileReadError::InvalidMap;
    }
    auto header = ReadChallengeBlockHeader(cursor, identifiers);
    if (!header.has_value()) {
        return ReplayFileReadError::InvalidMap;
    }
    auto blocks = ReadChallengeBlocks(
            challengeBody, cursor, identifiers, *header);
    if (!blocks.has_value() ||
        !HasChallengePostBlocks(challengeBody, cursor.Offset())) {
        return ReplayFileReadError::InvalidMap;
    }

    const bool created = CGameCtnReplayMapInput::Create(
            header->size,
            ToMapIdentifier(header->mapIdentifiers[1]),
            ToMapIdentifier(std::move(header->decorationIdentifiers[0])),
            ToMapIdentifier(std::move(header->decorationIdentifiers[1])),
            ToMapIdentifier(std::move(header->decorationIdentifiers[2])),
            std::move(*blocks),
            outMap);
    return created ? ReplayFileReadError::Success
                   : ReplayFileReadError::InvalidMap;
}

ReplayFileReadError ReadEmbeddedChallenge(
        const ReplayRecordSections &sections,
        CGameCtnReplayMapInput *outMap) {
    if (!sections.embeddedChallenge.has_value()) {
        return ReplayFileReadError::MissingEmbeddedChallenge;
    }
    const Bytes nested = *sections.embeddedChallenge;
    if (nested.size < 29u || nested.data[0] != 'G' ||
        nested.data[1] != 'B' || nested.data[2] != 'X') {
        return ReplayFileReadError::InvalidEmbeddedChallenge;
    }

    Nat32 version = 0u;
    Nat32 classId = 0u;
    Nat32 headerSize = 0u;
    Nat32 headerChunkCount = 0u;
    if (!nested.ReadU16(3u, &version) ||
        !nested.ReadU32(9u, &classId) ||
        !nested.ReadU32(13u, &headerSize) ||
        !nested.ReadU32(17u, &headerChunkCount)) {
        return ReplayFileReadError::InvalidEmbeddedChallenge;
    }
    (void)version;
    if (classId != ChallengeClassId ||
        nested.data[5] != GbxBinary ||
        nested.data[6] != GbxUncompressed ||
        nested.data[7] != GbxCompressed ||
        nested.data[8] != GbxReference ||
        headerChunkCount > GbxMaximumHeaderChunks ||
        headerSize < 4u || headerSize > nested.size ||
        17u > nested.size - headerSize) {
        return ReplayFileReadError::InvalidEmbeddedChallenge;
    }

    const std::size_t headerEnd = 17u + headerSize;
    constexpr std::size_t tableOffset = 21u;
    if (headerEnd > nested.size || tableOffset > headerEnd ||
        headerChunkCount > (headerEnd - tableOffset) / 8u) {
        return ReplayFileReadError::InvalidEmbeddedChallenge;
    }
    std::size_t payloadOffset =
            tableOffset + static_cast<std::size_t>(headerChunkCount) * 8u;
    for (Nat32 index = 0u; index < headerChunkCount; ++index) {
        Nat32 chunkId = 0u;
        Nat32 encodedSize = 0u;
        const std::size_t entryOffset = tableOffset + index * 8u;
        if (!nested.ReadU32(entryOffset, &chunkId) ||
            !nested.ReadU32(entryOffset + 4u, &encodedSize)) {
            return ReplayFileReadError::InvalidEmbeddedChallenge;
        }
        (void)chunkId;
        const std::size_t chunkSize = encodedSize & GbxHeaderLengthMask;
        if (payloadOffset > headerEnd ||
            chunkSize > headerEnd - payloadOffset) {
            return ReplayFileReadError::InvalidEmbeddedChallenge;
        }
        payloadOffset += chunkSize;
    }

    Nat32 referenceCount = 0u;
    Nat32 locationCount = 0u;
    Nat32 uncompressedSize = 0u;
    Nat32 compressedSize = 0u;
    if (!nested.ReadU32(headerEnd, &referenceCount) ||
        !nested.ReadU32(headerEnd + 4u, &locationCount) ||
        !nested.ReadU32(headerEnd + 8u, &uncompressedSize) ||
        !nested.ReadU32(headerEnd + 12u, &compressedSize)) {
        return ReplayFileReadError::InvalidEmbeddedChallenge;
    }
    (void)referenceCount;
    const std::size_t compressedOffset = headerEnd + 16u;
    if (locationCount != 0u || uncompressedSize == 0u ||
        compressedSize == 0u ||
        uncompressedSize > MaximumChallengeBodyBytes ||
        !nested.Contains(compressedOffset, compressedSize) ||
        compressedOffset + compressedSize != nested.size) {
        return ReplayFileReadError::InvalidEmbeddedChallenge;
    }

    std::vector<Byte> challengeStorage;
    try {
        challengeStorage.assign(uncompressedSize, 0u);
    } catch (const std::bad_alloc &) {
        return ReplayFileReadError::InvalidEmbeddedChallenge;
    }
    std::size_t decodedSize = challengeStorage.size();
    const int lzoResult = lzo1x_decompress_safe(
            nested.data + compressedOffset,
            compressedSize,
            challengeStorage.data(),
            &decodedSize);
    const Bytes challengeBody{
            challengeStorage.data(), challengeStorage.size()};
    Nat32 firstChunk = 0u;
    if (lzoResult != 0 || decodedSize != uncompressedSize ||
        !challengeBody.ReadU32(0u, &firstChunk)) {
        return ReplayFileReadError::InvalidEmbeddedChallenge;
    }
    return ParseChallengeBody(challengeBody, outMap);
}

bool IsGhostWrappedChunk(Nat32 chunkId) {
    switch (chunkId) {
    case 0x03092004u:
    case 0x03092005u:
    case 0x03092008u:
    case 0x03092009u:
    case 0x0309200au:
    case 0x0309200bu:
    case 0x03092013u:
    case 0x03092014u:
    case 0x03092016u:
    case 0x03092017u:
        return true;
    default:
        return false;
    }
}

bool ReadStringEnd(
        Bytes bytes,
        std::size_t offset,
        std::size_t *outNext,
        std::string *outText = nullptr) {
    Nat32 length = 0u;
    if (outNext == nullptr || !bytes.ReadU32(offset, &length) ||
        !bytes.Contains(offset + 4u, length)) {
        return false;
    }
    if (outText != nullptr) {
        try {
            outText->assign(
                    reinterpret_cast<const char *>(bytes.data + offset + 4u),
                    length);
        } catch (const std::bad_alloc &) {
            return false;
        }
    }
    *outNext = offset + 4u + length;
    return true;
}

bool SkipLooseIdentifier(
        Bytes bytes,
        std::size_t offset,
        std::size_t *outNext,
        std::string *outInlineName = nullptr) {
    Nat32 modeOrIdentifier = 0u;
    if (outNext == nullptr ||
        !bytes.ReadU32(offset, &modeOrIdentifier)) {
        return false;
    }
    std::size_t identifierOffset = offset;
    Nat32 identifier = modeOrIdentifier;
    if (modeOrIdentifier >= 1u && modeOrIdentifier <= 3u) {
        identifierOffset += 4u;
        if (!bytes.ReadU32(identifierOffset, &identifier)) {
            return false;
        }
    }
    const std::size_t next = identifierOffset + 4u;
    const TmnfFormat::ArchiveIdentifierWord parsed =
            TmnfFormat::CMwIdArchiveCodec::ParseWord(identifier);
    if (parsed.IsNamed() && parsed.payload == 0u) {
        return ReadStringEnd(bytes, next, outNext, outInlineName);
    }
    if (outInlineName != nullptr) {
        outInlineName->clear();
    }
    *outNext = next;
    return true;
}

bool ReadWrappedNatural(
        Bytes bytes,
        std::size_t offset,
        Nat32 *outValue,
        std::size_t *outNext) {
    Nat32 sentinel = 0u;
    Nat32 payloadSize = 0u;
    if (outValue == nullptr || outNext == nullptr ||
        !bytes.ReadU32(offset + 4u, &sentinel) ||
        !bytes.ReadU32(offset + 8u, &payloadSize) ||
        sentinel != SkipSentinel || payloadSize != 4u ||
        !bytes.ReadU32(offset + 12u, outValue)) {
        return false;
    }
    *outNext = offset + 16u;
    return true;
}

struct InputArchiveLayout {
    std::size_t inputStoreOffset = 0u;
    std::size_t actionsOffset = 0u;
    std::size_t eventHeaderOffset = 0u;
    std::size_t eventsOffset = 0u;
    Nat32 durationMs = 0u;
    Nat32 version = 0u;
    Nat32 actionCount = 0u;
    Nat32 eventCount = 0u;
    Nat32 capacityOrLimit = 0u;
    Nat32 validationSeed = 0u;
};

bool ParseInputArchiveLayout(
        Bytes ghostBytes,
        std::size_t chunkOffset,
        InputArchiveLayout *out) {
    if (out == nullptr) {
        return false;
    }
    *out = InputArchiveLayout{};
    if (!ghostBytes.ReadU32(chunkOffset + 4u, &out->durationMs)) {
        return false;
    }
    out->inputStoreOffset = chunkOffset + 8u;
    if (out->durationMs == 0u) {
        out->actionsOffset = out->inputStoreOffset + 8u;
        out->eventHeaderOffset = out->actionsOffset;
        out->eventsOffset = out->eventHeaderOffset + 8u;
        return true;
    }

    if (!ghostBytes.ReadU32(out->inputStoreOffset, &out->version) ||
        !ghostBytes.ReadU32(out->inputStoreOffset + 4u, &out->actionCount) ||
        out->actionCount > GhostArchiveCountLimit) {
        return false;
    }
    std::size_t offset = out->inputStoreOffset + 8u;
    out->actionsOffset = offset;
    for (Nat32 actionIndex = 0u;
         actionIndex < out->actionCount;
         ++actionIndex) {
        if (!SkipLooseIdentifier(ghostBytes, offset, &offset)) {
            return false;
        }
    }
    out->eventHeaderOffset = offset;
    if (!ghostBytes.ReadU32(offset, &out->eventCount) ||
        !ghostBytes.ReadU32(offset + 4u, &out->capacityOrLimit) ||
        out->eventCount > GhostArchiveCountLimit) {
        return false;
    }
    offset += 8u;
    out->eventsOffset = offset;
    if (out->eventCount >
        (ghostBytes.size - (offset <= ghostBytes.size ? offset
                                                      : ghostBytes.size)) /
                GhostInputEventBytes ||
        !ghostBytes.Contains(
                offset,
                static_cast<std::size_t>(out->eventCount) *
                        GhostInputEventBytes)) {
        return false;
    }
    offset += static_cast<std::size_t>(out->eventCount) *
            GhostInputEventBytes;

    if (!ReadStringEnd(ghostBytes, offset, &offset) ||
        !ghostBytes.Contains(offset, 12u)) {
        return false;
    }
    offset += 12u;
    if (!ReadStringEnd(ghostBytes, offset, &offset) ||
        !ghostBytes.ReadU32(offset, &out->validationSeed)) {
        return false;
    }
    return true;
}

struct GhostMetadata {
    bool valid = true;
    std::optional<Nat32> raceTime;
    std::optional<Nat32> respawnCount;
    std::optional<Nat32> stuntScore;
};

struct GhostSections {
    InputArchiveLayout input;
    bool hasInput = false;
    std::optional<Bytes> compressedState;
    GhostMetadata metadata;
};

bool ReadGhostArchiveHeader(Bytes bytes) {
    Nat32 chunkId = 0u;
    Nat32 version = 0u;
    Nat32 ghostCount = 0u;
    Nat32 firstGhostNode = 0u;
    Nat32 firstGhostClass = 0u;
    return bytes.ReadU32(0u, &chunkId) && chunkId == ReplayGhostBufferChunk &&
           bytes.ReadU32(4u, &version) &&
           bytes.ReadU32(8u, &ghostCount) && ghostCount != 0u &&
           ghostCount <= GhostArchiveCountLimit &&
           bytes.ReadU32(12u, &firstGhostNode) &&
           bytes.ReadU32(16u, &firstGhostClass);
}

void CaptureGhostMetadata(
        Bytes bytes,
        std::size_t offset,
        Nat32 chunkId,
        GhostMetadata &metadata) {
    Nat32 value = 0u;
    std::size_t ignoredNext = 0u;
    if (!ReadWrappedNatural(bytes, offset, &value, &ignoredNext)) {
        metadata.valid = false;
    } else if (chunkId == 0x03092005u) {
        metadata.raceTime = value;
    } else if (chunkId == 0x03092008u) {
        metadata.respawnCount = value;
    } else if (chunkId == 0x0309200au) {
        metadata.stuntScore = value;
    }
}

std::optional<std::size_t> AdvanceGhostChunk(
        Bytes bytes,
        std::size_t offset,
        Nat32 chunkId,
        Nat32 scanIndex,
        GhostSections &sections) {
    std::size_t next = offset;
    if (chunkId == GhostBaseStateChunk) {
        Nat32 compressedSize = 0u;
        if (!bytes.ReadU32(offset + 8u, &compressedSize)) {
            return std::nullopt;
        }
        auto compressed = bytes.Slice(offset + 12u, compressedSize);
        if (!compressed.has_value()) {
            return std::nullopt;
        }
        if (!sections.compressedState.has_value()) {
            sections.compressedState = *compressed;
        }
        next = offset + 12u + compressedSize;
    } else if (chunkId == GhostGenericWrappedChunk || IsGhostWrappedChunk(chunkId)) {
        if (!SkipWrapped(bytes, offset, &next)) {
            return std::nullopt;
        }
        if (scanIndex < 64u &&
            (chunkId == 0x03092005u ||
             chunkId == 0x03092008u ||
             chunkId == 0x0309200au)) {
            CaptureGhostMetadata(bytes, offset, chunkId, sections.metadata);
        }
    } else if (chunkId == 0x0309200cu) {
        next = offset + 8u;
    } else if (chunkId == 0x0309200eu ||
               chunkId == 0x03092010u ||
               chunkId == 0x03092015u) {
        if (!SkipLooseIdentifier(bytes, offset + 4u, &next)) {
            return std::nullopt;
        }
    } else if (chunkId == 0x0309200fu) {
        if (!ReadStringEnd(bytes, offset + 4u, &next)) {
            return std::nullopt;
        }
    } else if (chunkId == 0x03092012u) {
        next = offset + 24u;
    } else if (chunkId == 0x03092018u) {
        next = offset + 4u;
        for (Nat32 part = 0u; part < 3u; ++part) {
            if (!SkipLooseIdentifier(bytes, next, &next)) {
                return std::nullopt;
            }
        }
    } else {
        return std::nullopt;
    }
    if (next <= offset || next > bytes.size) {
        return std::nullopt;
    }
    return next;
}

bool ReadGhostValidationInput(
        Bytes bytes,
        std::size_t offset,
        Nat32 scanIndex,
        GhostSections &sections) {
    Nat32 wrapperZero = 0u;
    Nat32 wrapperFacade = 0u;
    if (bytes.ReadU32(offset + 4u, &wrapperZero) &&
        bytes.ReadU32(offset + 8u, &wrapperFacade) &&
        wrapperZero == 0u && wrapperFacade == ArchiveFacade) {
        return false;
    }
    if (!ParseInputArchiveLayout(bytes, offset, &sections.input)) {
        return false;
    }
    sections.hasInput = true;
    if (scanIndex >= 64u) {
        sections.metadata.valid = false;
    }
    return true;
}

ReplayFileReadError ScanGhostArchive(
        Bytes ghostBytes,
        GhostSections *out) {
    if (out == nullptr) {
        return ReplayFileReadError::InvalidRequest;
    }
    *out = GhostSections{};
    if (!ReadGhostArchiveHeader(ghostBytes)) {
        return ReplayFileReadError::MissingInputStream;
    }

    std::size_t offset = 20u;
    for (Nat32 scanIndex = 0u; scanIndex < 128u; ++scanIndex) {
        Nat32 chunkId = 0u;
        if (!ghostBytes.ReadU32(offset, &chunkId) || chunkId == ArchiveFacade) {
            return ReplayFileReadError::MissingInputStream;
        }
        if (chunkId == GhostValidationInputChunk) {
            return ReadGhostValidationInput(ghostBytes, offset, scanIndex, *out)
                    ? ReplayFileReadError::Success
                    : ReplayFileReadError::MissingInputStream;
        }
        auto next = AdvanceGhostChunk(
                ghostBytes, offset, chunkId, scanIndex, *out);
        if (!next.has_value()) {
            return ReplayFileReadError::MissingInputStream;
        }
        offset = *next;
    }
    return ReplayFileReadError::MissingInputStream;
}

ReplayInputActionKind DecodeInputAction(std::string_view name) {
    if (name == "Accelerate") return ReplayInputActionKind::Accelerate;
    if (name == "Gas") return ReplayInputActionKind::Gas;
    if (name == "Brake") return ReplayInputActionKind::Brake;
    if (name == "Steer") return ReplayInputActionKind::Steer;
    if (name == "SteerLeft") return ReplayInputActionKind::SteerLeft;
    if (name == "SteerRight") return ReplayInputActionKind::SteerRight;
    if (name == "_FakeIsRaceRunning") return ReplayInputActionKind::RaceRunning;
    if (name == "_FakeFinishLine") return ReplayInputActionKind::FinishLine;
    if (name == "Respawn") return ReplayInputActionKind::Respawn;
    return ReplayInputActionKind::Unmapped;
}

std::int32_t DecodeSigned24(Nat32 encoded) {
    encoded &= 0x00ffffffu;
    if ((encoded & 0x00800000u) != 0u) {
        return static_cast<std::int32_t>(encoded | 0xff000000u);
    }
    return static_cast<std::int32_t>(encoded);
}

ReplayInputActionValue DecodeInputValue(
        ReplayInputActionKind action,
        Nat32 encoded) {
    if (action == ReplayInputActionKind::Gas ||
        action == ReplayInputActionKind::Steer) {
        return ReplayInputActionValue::Analog(static_cast<float>(
                static_cast<double>(DecodeSigned24(encoded)) *
                InputAxisScale));
    }
    if (action == ReplayInputActionKind::Unmapped) {
        return ReplayInputActionValue::None();
    }
    ReplayInputSwitchState state = ReplayInputSwitchState::NonCanonicalActive;
    if (encoded == 0u) {
        state = ReplayInputSwitchState::Released;
    } else if (encoded == 1u) {
        state = ReplayInputSwitchState::Pressed;
    }
    return ReplayInputActionValue::Switch(state);
}

ReplayFileReadError DecodeInputTimeline(
        Bytes ghostBytes,
        const GhostSections &sections,
        ReplayInputTimeline *outTimeline) {
    if (!sections.hasInput) {
        return ReplayFileReadError::MissingInputStream;
    }
    const InputArchiveLayout &layout = sections.input;
    if (layout.actionCount > 256u) {
        return ReplayFileReadError::TooManyInputActions;
    }
    if (!sections.metadata.valid) {
        return ReplayFileReadError::InvalidGhostMetadata;
    }

    Nat32 version = 0u;
    Nat32 actionCount = 0u;
    if (!ghostBytes.ReadU32(layout.inputStoreOffset, &version) ||
        !ghostBytes.ReadU32(layout.inputStoreOffset + 4u, &actionCount) ||
        version != layout.version || actionCount != layout.actionCount) {
        return ReplayFileReadError::InvalidInputHeader;
    }

    std::vector<ReplayInputActionKind> actionKinds;
    std::size_t offset = layout.actionsOffset;
    try {
        actionKinds.reserve(layout.actionCount);
        for (Nat32 index = 0u; index < layout.actionCount; ++index) {
            std::string name;
            if (!SkipLooseIdentifier(
                        ghostBytes, offset, &offset, &name)) {
                return ReplayFileReadError::InvalidInputActions;
            }
            actionKinds.push_back(DecodeInputAction(name));
        }
    } catch (const std::bad_alloc &) {
        return ReplayFileReadError::InvalidInputActions;
    }

    Nat32 eventCount = 0u;
    Nat32 capacityOrLimit = 0u;
    if (!ghostBytes.ReadU32(offset, &eventCount) ||
        !ghostBytes.ReadU32(offset + 4u, &capacityOrLimit) ||
        eventCount != layout.eventCount ||
        capacityOrLimit != layout.capacityOrLimit) {
        return ReplayFileReadError::InvalidInputEventHeader;
    }
    offset += 8u;

    std::vector<ReplayInputEvent> events;
    try {
        events.reserve(eventCount);
        for (Nat32 eventIndex = 0u;
             eventIndex < eventCount;
             ++eventIndex) {
            Nat32 timeMs = 0u;
            Nat32 encodedValue = 0u;
            if (!ghostBytes.ReadU32(offset, &timeMs) ||
                !ghostBytes.ReadU32(offset + 5u, &encodedValue)) {
                return ReplayFileReadError::InvalidInputEvents;
            }
            const Nat32 actionIndex = ghostBytes.data[offset + 4u];
            if (actionIndex >= actionKinds.size()) {
                return ReplayFileReadError::InvalidInputActions;
            }
            const ReplayInputActionKind action = actionKinds[actionIndex];
            events.push_back({
                    timeMs,
                    action,
                    DecodeInputValue(action, encodedValue & 0x00ffffffu)});
            offset += GhostInputEventBytes;
        }
    } catch (const std::bad_alloc &) {
        return ReplayFileReadError::InvalidInputEvents;
    }

    ReplayInputMetadata metadata;
    metadata.durationMs = layout.durationMs;
    metadata.validationSeed = layout.validationSeed;
    if (sections.metadata.raceTime.has_value()) {
        const Nat32 encoded = *sections.metadata.raceTime;
        if (encoded == std::numeric_limits<Nat32>::max()) {
            metadata.raceTimeMs = 0;
        } else if (encoded > static_cast<Nat32>(
                                     std::numeric_limits<std::int32_t>::max())) {
            return ReplayFileReadError::InvalidInputHeader;
        } else {
            metadata.raceTimeMs = static_cast<std::int32_t>(encoded);
        }
    }
    if (sections.metadata.respawnCount.has_value() &&
        *sections.metadata.respawnCount !=
                std::numeric_limits<Nat32>::max()) {
        metadata.respawnCount = *sections.metadata.respawnCount;
    }
    if (sections.metadata.stuntScore.has_value() &&
        *sections.metadata.stuntScore !=
                std::numeric_limits<Nat32>::max()) {
        metadata.stuntScore = *sections.metadata.stuntScore;
    }

    const ReplayInputTimelineCreateResult createResult =
            ReplayInputTimeline::Create(
                    std::move(metadata),
                    std::move(actionKinds),
                    std::move(events),
                    outTimeline);
    return createResult == ReplayInputTimelineCreateResult::Success
            ? ReplayFileReadError::Success
            : ReplayFileReadError::InvalidInputTimeline;
}

bool InflateGhostState(
        Bytes compressed,
        std::vector<Byte> *out) {
    if (out == nullptr) {
        return false;
    }
    out->clear();
    z_stream stream{};
    if (inflateInit(&stream) != Z_OK) {
        return false;
    }

    constexpr std::size_t expansionFactor = 8u;
    constexpr std::size_t capacitySlack = 4096u;
    if (compressed.size >
        (std::numeric_limits<std::size_t>::max() - capacitySlack) /
                expansionFactor) {
        inflateEnd(&stream);
        return false;
    }
    std::size_t capacity = compressed.size * expansionFactor + capacitySlack;
    if (capacity < 65536u) {
        capacity = 65536u;
    }
    std::vector<Byte> decoded;
    try {
        decoded.resize(capacity);
    } catch (const std::bad_alloc &) {
        inflateEnd(&stream);
        return false;
    }

    stream.next_in = const_cast<Bytef *>(compressed.data);
    stream.avail_in = static_cast<uInt>(compressed.size);
    for (;;) {
        if (stream.total_out == capacity) {
            const std::size_t nextCapacity = capacity * 2u;
            if (nextCapacity <= capacity || nextCapacity > 0x7fffffffu) {
                inflateEnd(&stream);
                return false;
            }
            try {
                decoded.resize(nextCapacity);
            } catch (const std::bad_alloc &) {
                inflateEnd(&stream);
                return false;
            }
            capacity = nextCapacity;
        }
        stream.next_out = decoded.data() + stream.total_out;
        stream.avail_out = static_cast<uInt>(capacity - stream.total_out);
        const int result = inflate(&stream, Z_NO_FLUSH);
        if (result == Z_STREAM_END) {
            if (stream.avail_in != 0u ||
                stream.total_in != compressed.size) {
                inflateEnd(&stream);
                return false;
            }
            break;
        }
        if (result != Z_OK) {
            inflateEnd(&stream);
            return false;
        }
    }
    if (stream.total_out > 0x7fffffffu) {
        inflateEnd(&stream);
        return false;
    }
    try {
        decoded.resize(static_cast<std::size_t>(stream.total_out));
        out->swap(decoded);
    } catch (const std::bad_alloc &) {
        inflateEnd(&stream);
        return false;
    }
    inflateEnd(&stream);
    return true;
}

struct GhostStateFormat {
    Nat32 mobileClassId = 0u;
    Nat32 fixedTimeStep = 0u;
    Nat32 encodingMode = 0u;
    Nat32 fixedStepMs = 0u;
    Nat32 stateVersion = 0u;
    Nat32 encodedStateBytes = 0u;
    Nat32 sampleCount = 0u;
    Nat32 encodedSampleBytes = 0u;
};

bool ParseGhostStateFormat(Bytes archive, GhostStateFormat *out) {
    if (out == nullptr || archive.size < 28u) {
        return false;
    }
    *out = GhostStateFormat{};
    if (!archive.ReadU32(0u, &out->mobileClassId) ||
        !archive.ReadU32(4u, &out->fixedTimeStep) ||
        !archive.ReadU32(8u, &out->encodingMode) ||
        !archive.ReadU32(12u, &out->fixedStepMs) ||
        !archive.ReadU32(16u, &out->stateVersion) ||
        !archive.ReadU32(20u, &out->encodedStateBytes)) {
        return false;
    }
    if (out->encodedStateBytes == 0u) {
        Nat32 stateOffsetCount = 0u;
        return archive.size == 28u &&
               archive.ReadU32(GhostStateDataOffset, &stateOffsetCount) &&
               stateOffsetCount == 0u;
    }
    if (!archive.Contains(GhostStateDataOffset, out->encodedStateBytes)) {
        return false;
    }
    const std::size_t tailOffset =
            GhostStateDataOffset + out->encodedStateBytes;
    Nat32 firstStateOffset = 0u;
    if (archive.size - tailOffset != 12u ||
        !archive.ReadU32(tailOffset, &out->sampleCount) ||
        !archive.ReadU32(tailOffset + 4u, &firstStateOffset) ||
        !archive.ReadU32(tailOffset + 8u, &out->encodedSampleBytes) ||
        firstStateOffset != 0u || out->encodedSampleBytes == 0u ||
        out->sampleCount !=
                out->encodedStateBytes / out->encodedSampleBytes ||
        out->encodedStateBytes % out->encodedSampleBytes != 0u) {
        return false;
    }
    return true;
}

bool IsCanonicalInputOnlyGhostState(const GhostStateFormat &format) {
    return format.mobileClassId == 0u &&
           format.fixedTimeStep == GhostFixedTimeStep &&
           format.encodingMode == GhostEncodingMode &&
           format.fixedStepMs == 0u &&
           format.stateVersion == 0u &&
           format.encodedStateBytes == 0u &&
           format.sampleCount == 0u &&
           format.encodedSampleBytes == 0u;
}

ReplayFileReadError DecodeGhostTrajectory(
        const GhostSections &sections,
        ReplayGhostTrajectory *outTrajectory,
        ReplayGhostArchiveMetadata *outMetadata) {
    if (outTrajectory == nullptr || outMetadata == nullptr) {
        return ReplayFileReadError::GhostTrajectoryAllocationFailed;
    }
    if (!sections.compressedState.has_value()) {
        return ReplayFileReadError::MissingGhostState;
    }
    std::vector<Byte> archiveStorage;
    if (!InflateGhostState(*sections.compressedState, &archiveStorage)) {
        return ReplayFileReadError::GhostStateDecompressionFailed;
    }
    if (archiveStorage.size() > 0x7fffffffu) {
        return ReplayFileReadError::InvalidGhostState;
    }
    const Bytes archive{archiveStorage.data(), archiveStorage.size()};
    GhostStateFormat format;
    if (!ParseGhostStateFormat(archive, &format)) {
        return ReplayFileReadError::InvalidGhostState;
    }
    if (format.encodedStateBytes == 0u) {
        if (!IsCanonicalInputOnlyGhostState(format)) {
            return ReplayFileReadError::InvalidGhostState;
        }
        const ReplayGhostTrajectoryCreateResult createResult =
                ReplayGhostTrajectory::CreateFixedStep(
                        0u, {}, outTrajectory);
        if (createResult != ReplayGhostTrajectoryCreateResult::Success) {
            return ReplayFileReadError::GhostTrajectoryAllocationFailed;
        }
        *outMetadata = {};
        return ReplayFileReadError::Success;
    }
    if (format.mobileClassId != TMNF_CLASS_CSceneVehicleCar ||
        format.fixedTimeStep != GhostFixedTimeStep ||
        format.encodingMode != GhostEncodingMode ||
        format.stateVersion != GhostStateVersion ||
        format.encodedSampleBytes != GhostEncodedSampleBytes) {
        return ReplayFileReadError::InvalidGhostState;
    }

    std::vector<GmVec3> positions;
    try {
        positions.reserve(format.sampleCount);
        for (Nat32 sampleIndex = 0u;
             sampleIndex < format.sampleCount;
             ++sampleIndex) {
            const std::size_t offset = GhostStateDataOffset +
                    static_cast<std::size_t>(sampleIndex) *
                            format.encodedSampleBytes;
            const Byte *sample = archive.data + offset;
            positions.push_back({
                    TmnfArchiveBinary32::ReadLittleEndian(sample),
                    TmnfArchiveBinary32::ReadLittleEndian(sample + 4u),
                    TmnfArchiveBinary32::ReadLittleEndian(sample + 8u)});
        }
    } catch (const std::bad_alloc &) {
        return ReplayFileReadError::GhostTrajectoryAllocationFailed;
    }

    const ReplayGhostTrajectoryCreateResult createResult =
            ReplayGhostTrajectory::CreateFixedStep(
                    format.fixedStepMs,
                    std::move(positions),
                    outTrajectory);
    switch (createResult) {
    case ReplayGhostTrajectoryCreateResult::Success:
        *outMetadata = {
                format.encodedStateBytes,
                format.sampleCount == 0u
                        ? 0u
                        : format.encodedSampleBytes,
        };
        return ReplayFileReadError::Success;
    case ReplayGhostTrajectoryCreateResult::InvalidFixedStep:
    case ReplayGhostTrajectoryCreateResult::TimestampOutOfRange:
        return ReplayFileReadError::GhostStateDecompressionFailed;
    case ReplayGhostTrajectoryCreateResult::NonFinitePosition:
        return ReplayFileReadError::InvalidGhostState;
    case ReplayGhostTrajectoryCreateResult::MissingOutput:
        return ReplayFileReadError::GhostTrajectoryAllocationFailed;
    }
    return ReplayFileReadError::GhostTrajectoryAllocationFailed;
}

} // namespace

ReplayFileReadError ParseReplayStorage(
        const std::vector<Byte> &fileStorage,
        ReplayFile *out) {
    std::vector<Byte> replayBodyStorage;
    ReplayFileReadError error =
            DecompressReplayBody(fileStorage, &replayBodyStorage);
    if (error != ReplayFileReadError::Success) {
        return error;
    }

    const Bytes replayBody{
            replayBodyStorage.data(), replayBodyStorage.size()};
    ReplayRecordSections sections;
    error = FindReplayRecordSections(replayBody, &sections);
    if (error != ReplayFileReadError::Success) {
        return error;
    }

    CGameCtnReplayMapInput mapInput;
    error = ReadEmbeddedChallenge(sections, &mapInput);
    if (error != ReplayFileReadError::Success) {
        return error;
    }

    GhostSections ghostSections;
    error = ScanGhostArchive(sections.ghostBuffer, &ghostSections);
    if (error != ReplayFileReadError::Success) {
        return error;
    }
    ReplayInputTimeline inputTimeline;
    error = DecodeInputTimeline(
            sections.ghostBuffer,
            ghostSections,
            &inputTimeline);
    if (error != ReplayFileReadError::Success) {
        return error;
    }
    ReplayGhostTrajectory ghostTrajectory;
    ReplayGhostArchiveMetadata ghostArchiveMetadata;
    error = DecodeGhostTrajectory(ghostSections,
                                  &ghostTrajectory,
                                  &ghostArchiveMetadata);
    if (error != ReplayFileReadError::Success) {
        return error;
    }

    *out = ReplayFile(
            std::move(inputTimeline),
            std::move(ghostTrajectory),
            ghostArchiveMetadata,
            std::move(mapInput));
    return ReplayFileReadError::Success;
}

ReplayFileReadError ReadReplayBytes(
        const std::uint8_t *bytes,
        std::size_t byteCount,
        ReplayFile *out) {
    if (out == nullptr || bytes == nullptr || byteCount == 0u) {
        return ReplayFileReadError::InvalidRequest;
    }
    try {
        const std::vector<Byte> fileStorage(bytes, bytes + byteCount);
        return ParseReplayStorage(fileStorage, out);
    } catch (const std::bad_alloc &) {
        return ReplayFileReadError::AllocationFailed;
    }
}

const char *ReplayFileReadErrorName(ReplayFileReadError error) {
    switch (error) {
    case ReplayFileReadError::Success: return "success";
    case ReplayFileReadError::InvalidRequest: return "invalid request";
    case ReplayFileReadError::FileReadFailed: return "file read failed";
    case ReplayFileReadError::InvalidContainer: return "invalid replay container";
    case ReplayFileReadError::AllocationFailed: return "allocation failed";
    case ReplayFileReadError::RootBodyDecompressionFailed:
        return "replay body decompression failed";
    case ReplayFileReadError::MissingGhostBuffer: return "missing ghost buffer";
    case ReplayFileReadError::MissingInputStream: return "missing input stream";
    case ReplayFileReadError::TooManyInputActions: return "too many input actions";
    case ReplayFileReadError::InvalidGhostMetadata: return "invalid ghost metadata";
    case ReplayFileReadError::InvalidInputHeader: return "invalid input header";
    case ReplayFileReadError::InvalidInputActions: return "invalid input actions";
    case ReplayFileReadError::InvalidInputEventHeader:
        return "invalid input event header";
    case ReplayFileReadError::InvalidInputEvents: return "invalid input events";
    case ReplayFileReadError::InvalidInputTimeline: return "invalid input timeline";
    case ReplayFileReadError::MissingGhostState: return "missing ghost state";
    case ReplayFileReadError::GhostStateDecompressionFailed:
        return "ghost state decompression failed";
    case ReplayFileReadError::InvalidGhostState: return "invalid ghost state";
    case ReplayFileReadError::GhostTrajectoryAllocationFailed:
        return "ghost trajectory allocation failed";
    case ReplayFileReadError::MissingEmbeddedChallenge:
        return "missing embedded challenge";
    case ReplayFileReadError::InvalidEmbeddedChallenge:
        return "invalid embedded challenge";
    case ReplayFileReadError::InvalidMap: return "invalid replay map";
    }
    return "unknown replay error";
}
