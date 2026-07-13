#include "format/pack/block_info_catalog/replay_collection_zone_sources.h"
#include <stddef.h>
#include <stdint.h>

#include "format/archive/archive_binary32.h"
#include "format/archive/archive_binary.h"
#include "format/archive/fixed_c_string.h"
#include "format/pack/block_info_catalog/blockinfo_archive_stream.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include "format/pack/installed/byte_buffer.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/pack/block_info_catalog/replay_collection_archive.h"
#include "format/archive/tmnf_archive_ids.h"
#include "format/pack/block_info_catalog/replay_collection_archive_chunk_ids.h"
#include "format/archive/mw_id_archive_codec.h"
#include <new>
namespace {

float DecodeCollectionZoneFloat32(u32 encoded) {
    return TmnfArchiveBinary32::Decode(encoded);
}

int CopyCStringToZoneBuffer(char *dst, size_t dstSize, const char *src) {
    return TmnfFormat::FixedCString::Copy(dst, dstSize, src);
}

int AppendCStringToZoneBuffer(char *dst, size_t dstSize, const char *src) {
    return TmnfFormat::FixedCString::Append(dst, dstSize, src);
}

const char *ZoneLastBackslash(const char *text) {
    const char *last = nullptr;
    for (size_t i = 0u; text[i] != 0; i++) {
        if (text[i] == (char)0x5cu) {
            last = text + i;
        }
    }
    return last;
}

int ZoneSubstringAt(const char *text, const char *needle) {
    for (size_t i = 0u; needle[i] != 0; i++) {
        if (text[i] != needle[i]) {
            return 0;
        }
    }
    return 1;
}

const char *ZoneFindSubstring(const char *text, const char *needle) {
    if (text == nullptr || needle == nullptr) {
        return nullptr;
    }
    if (needle[0] == 0) {
        return text;
    }
    for (size_t i = 0u; text[i] != 0; i++) {
        if (ZoneSubstringAt(text + i, needle)) {
            return text + i;
        }
    }
    return nullptr;
}

void ClearZoneDescriptorPaths(char descriptorPath[][CGameCtnReplayStaticDescriptorPathCapacity]) {
    for (u32 slot = 0u; slot < 4u; slot++) {
        for (u32 index = 0u;
             index < CGameCtnReplayStaticDescriptorPathCapacity;
             index++) {
            descriptorPath[slot][index] = 0;
        }
    }
}

struct CollectionZoneBufferArchiveStream {
    const unsigned char *bytes = nullptr;
    u32 byteCount = 0u;
    u32 offset = 0u;
    u32 cmwIdMode = 0u;
    u32 hasCmwIdMode = 0u;

    int ReadU32(u32 *valueOut);
    int ReadF32(float *valueOut);
    int Skip(u32 skipByteCount);
    int SkipString();
    int SkipCMwIdPayload(u32 encodedValue);
    int SkipCMwId();
};

int CollectionZoneBufferArchiveStream::ReadU32(u32 *valueOut) {
    if (valueOut == nullptr ||
        offset > byteCount ||
        byteCount - offset < 4u) {
        return 0;
    }
    *valueOut = TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset);
    offset += 4u;
    return 1;
}

int CollectionZoneBufferArchiveStream::ReadF32(float *valueOut) {
    if (valueOut == nullptr ||
        offset > byteCount ||
        byteCount - offset < 4u) {
        return 0;
    }
    *valueOut = DecodeCollectionZoneFloat32(
            TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset));
    offset += 4u;
    return 1;
}


int CollectionZoneBufferArchiveStream::Skip(u32 skipByteCount) {
    if (offset > byteCount ||
        byteCount - offset < skipByteCount) {
        return 0;
    }
    offset += skipByteCount;
    return 1;
}

int CollectionZoneBufferArchiveStream::SkipString() {
    u32 length = 0u;
    if (!ReadU32(&length)) {
        return 0;
    }
    return Skip(length);
}

int CollectionZoneBufferArchiveStream::SkipCMwIdPayload(u32 encodedValue) {
    const TmnfFormat::ArchiveIdentifierWord identifier =
            TmnfFormat::CMwIdArchiveCodec::ParseWord(encodedValue);
    if (!identifier.IsNamed()) {
        return 1;
    }
    if (hasCmwIdMode != 0u && cmwIdMode == 2u) {
        return SkipString();
    }
    if (hasCmwIdMode != 0u && cmwIdMode == 3u) {
        return identifier.payload == 0u
                ? SkipString()
                : 1;
    }
    if (identifier.payload == 0u) {
        return SkipString();
    }
    return 1;
}

int CollectionZoneBufferArchiveStream::SkipCMwId() {
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

}  // namespace

void CGameCtnReplayCollectionZoneSourceInfo::Clear() {
    sourceNode = ArchiveNodeReference::Invalid();
    kind = CGameCtnReplayConstructionZoneKind::None;
    loadable = 0u;
    plainPath[0] = '\0';
}

int CGameCtnReplayCollectionZoneSourceInfo::Configure(
        ArchiveNodeReference nodeRef,
        CGameCtnReplayConstructionZoneKind sourceKind,
        u32 sourceLoadable,
        const char *sourcePlainPath) {
    sourceNode = nodeRef;
    kind = sourceKind;
    loadable = sourceLoadable;
    return CopyCStringToZoneBuffer(plainPath,
                                   sizeof(plainPath),
                                   sourcePlainPath != nullptr
                                           ? sourcePlainPath
                                           : "");
}

int CGameCtnReplayCollectionZoneSourceInfo::IsConstructionZone() const {
    return kind != CGameCtnReplayConstructionZoneKind::None;
}

int CGameCtnReplayCollectionZoneSourceInfo::IsFlat() const {
    return kind == CGameCtnReplayConstructionZoneKind::Flat;
}

int CGameCtnReplayCollectionZoneSourceInfo::IsFrontier() const {
    return kind == CGameCtnReplayConstructionZoneKind::Frontier;
}

ArchiveNodeReference CGameCtnReplayCollectionZoneSourceInfo::SourceNode() const {
    return sourceNode;
}

CGameCtnReplayConstructionZoneKind
CGameCtnReplayCollectionZoneSourceInfo::Kind() const {
    return kind;
}

u32 CGameCtnReplayCollectionZoneSourceInfo::IsLoadable() const {
    return loadable;
}

const char *CGameCtnReplayCollectionZoneSourceInfo::PlainPath() const {
    return plainPath;
}

int CGameCtnReplayCollectionZoneSourceInfo::IdentifierFromPlainPath(
        char *out,
        size_t outSize) const {
    if (out == nullptr || outSize == 0u || plainPath[0] == '\0') {
        return 0;
    }
    const char *leaf = ZoneLastBackslash(plainPath);
    leaf = leaf != nullptr ? leaf + 1 : plainPath;
    const char *zoneExt = ZoneFindSubstring(leaf, ".TMZone");
    if (zoneExt == nullptr || zoneExt == leaf) {
        return 0;
    }
    const size_t length = (size_t)(zoneExt - leaf);
    if (length >= outSize) {
        return 0;
    }
    return TmnfFormat::FixedCString::CopyBytes(
            out,
            outSize,
            leaf,
            length);
}

void CGameCtnReplayZoneDescriptorSourceSet::Clear() {
    parsedSourceCount = 0u;
    zoneIdName[0] = '\0';
    ClearZoneDescriptorPaths(descriptorPath);
}

int CGameCtnReplayZoneDescriptorSourceSet::SetZoneIdName(const char *name) {
    return CopyCStringToZoneBuffer(zoneIdName, sizeof(zoneIdName), name);
}

char *CGameCtnReplayZoneDescriptorSourceSet::ZoneIdNameArchiveBuffer() {
    return zoneIdName;
}

size_t CGameCtnReplayZoneDescriptorSourceSet::ZoneIdNameArchiveBufferSize()
        const {
    return sizeof(zoneIdName);
}

const char *CGameCtnReplayZoneDescriptorSourceSet::ZoneIdName() const {
    return zoneIdName;
}

char *CGameCtnReplayZoneDescriptorSourceSet::DescriptorPathArchiveBuffer(
        u32 slot) {
    return slot < SourcePathCount ? descriptorPath[slot] : nullptr;
}

size_t CGameCtnReplayZoneDescriptorSourceSet::DescriptorPathArchiveBufferSize()
        const {
    return sizeof(descriptorPath[0]);
}

int CGameCtnReplayZoneDescriptorSourceSet::NoteDescriptorPathIfPresent(
        u32 slot) {
    if (slot >= SourcePathCount) {
        return 0;
    }
    if (descriptorPath[slot][0] != '\0' &&
        parsedSourceCount < slot + 1u) {
        parsedSourceCount = slot + 1u;
    }
    return 1;
}

int CGameCtnReplayZoneDescriptorSourceSet::SetSingleDescriptorPath(
        const char *path) {
    if (!CopyCStringToZoneBuffer(descriptorPath[0],
                                 sizeof(descriptorPath[0]),
                                 path)) {
        return 0;
    }
    parsedSourceCount = descriptorPath[0][0] != '\0' ? 1u : 0u;
    return 1;
}

int CGameCtnReplayCollectionZoneSourceRef::Configure(
        const CGameCtnReplayCollectionZoneSourceInfo &sourceInfo) {
    source = sourceInfo;
    descriptorSources.Clear();
    return 1;
}

int CGameCtnReplayCollectionZoneSourceRef::IsFlat() const {
    return source.IsFlat();
}

int CGameCtnReplayCollectionZoneSourceRef::IsFrontier() const {
    return source.IsFrontier();
}

ArchiveNodeReference CGameCtnReplayCollectionZoneSourceRef::SourceNode() const {
    return source.SourceNode();
}

CGameCtnReplayConstructionZoneKind
CGameCtnReplayCollectionZoneSourceRef::Kind() const {
    return source.Kind();
}

u32 CGameCtnReplayCollectionZoneSourceRef::IsLoadable() const {
    return source.IsLoadable();
}

const char *CGameCtnReplayCollectionZoneSourceRef::PlainPath() const {
    return source.PlainPath();
}

int CGameCtnReplayCollectionZoneSourceRef::IdentifierFromPlainPath(
        char *out,
        size_t outSize) const {
    return source.IdentifierFromPlainPath(out, outSize);
}

int CGameCtnReplayCollectionZoneSourceRef::FillCatalogDefaultSource() {
    const char *plainPath = source.PlainPath();
    if (plainPath[0] == '\0') {
        return 0;
    }
    char zoneId[64]{};
    if (!IdentifierFromPlainPath(zoneId, sizeof(zoneId))) {
        return 0;
    }
    const char *zoneMarker = nullptr;
    const char *blockInfoFolder = nullptr;
    const char *extension = nullptr;
    if (source.IsFlat()) {
        zoneMarker = "ConstructionZone\\ConstructionZoneFlat\\";
        blockInfoFolder =
                "ConstructionBlockInfo\\ConstructionBlockInfoFlat\\";
        extension = ".TMEDFlat.Gbx";
    } else if (source.IsFrontier()) {
        zoneMarker = "ConstructionZone\\ConstructionZoneFrontier\\";
        blockInfoFolder =
                "ConstructionBlockInfo\\ConstructionBlockInfoFrontier\\";
        extension = ".TMEDFrontier.Gbx";
    } else {
        return 0;
    }
    const char *markerAt = ZoneFindSubstring(plainPath, zoneMarker);
    if (markerAt == nullptr) {
        return 0;
    }
    const size_t prefixLen = (size_t)(markerAt - plainPath);
    char sourcePath[CGameCtnReplayStaticDescriptorPathCapacity];
    if (prefixLen >= sizeof(sourcePath)) {
        return 0;
    }
    if (!TmnfFormat::FixedCString::CopyBytes(sourcePath,
                                             sizeof(sourcePath),
                                             plainPath,
                                             prefixLen)) {
        return 0;
    }
    if (!AppendCStringToZoneBuffer(sourcePath,
                                   sizeof(sourcePath),
                                   blockInfoFolder) ||
        !AppendCStringToZoneBuffer(sourcePath,
                                   sizeof(sourcePath),
                                   zoneId) ||
        !AppendCStringToZoneBuffer(sourcePath,
                                   sizeof(sourcePath),
                                   extension) ||
        !descriptorSources.SetZoneIdName(zoneId) ||
        !descriptorSources.SetSingleDescriptorPath(sourcePath)) {
        return 0;
    }
    return 1;
}

int CGameCtnReplayCollectionZoneSourceRef::ParseZoneDescriptorSources(
        const unsigned char *bytes,
        u32 byteCount) {
    if (bytes == nullptr || byteCount < 17u) {
        return 0;
    }
    BlockInfoDescriptorExternalRefs refs;
    if (!refs.ParseGbx(bytes, byteCount)) {
        return 0;
    }
    BlockInfoSizeParseStream stream{};
    stream.bytes = bytes;
    stream.byteCount = byteCount;
    stream.offset = refs.BodyOffsetForFormatParser();
    stream.SetBlockInfoSourceRefs(&refs);

    u32 zoneType = 0xffffffffu;
    for (;;) {
        u32 chunkId = 0u;
        if (!stream.ReadU32(&chunkId)) {
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            break;
        }
        switch (chunkId) {
        case TMNF_CLASS_CGameCtnZone: {
            u32 ignored = 0u;
            u32 localZoneNameIndex = 0u;
            if (!stream.ReadU32(&ignored) ||
                !stream.ReadU32(&localZoneNameIndex) ||
                !stream.ReadU32(&zoneType) ||
                !stream.SkipCountedNodRefs()) {
                return 0;
            }
            (void)localZoneNameIndex;
            break;
        }
        case 0x0305c001u: {
            u32 ignored = 0u;
            if (!stream.ReadU32(&ignored) ||
                !stream.ReadCMwIdText(
                        descriptorSources.ZoneIdNameArchiveBuffer(),
                        descriptorSources.ZoneIdNameArchiveBufferSize()) ||
                !stream.ReadU32(&zoneType) ||
                !stream.SkipCountedNodRefs()) {
                return 0;
            }
            break;
        }
        case 0x0305c002u: {
            u32 ignored = 0u;
            if (!stream.ReadU32(&ignored) ||
                !stream.ReadCMwIdText(
                        descriptorSources.ZoneIdNameArchiveBuffer(),
                        descriptorSources.ZoneIdNameArchiveBufferSize()) ||
                !stream.ReadU32(&zoneType)) {
                return 0;
            }
            break;
        }
        case 0x0305c003u: {
            u32 ignored = 0u;
            char basicZoneName[64];
            if (!stream.ReadU32(&ignored) ||
                !stream.ReadCMwIdText(
                        descriptorSources.ZoneIdNameArchiveBuffer(),
                        descriptorSources.ZoneIdNameArchiveBufferSize()) ||
                !stream.ReadCMwIdText(basicZoneName, sizeof(basicZoneName)) ||
                !stream.ReadU32(&zoneType)) {
                return 0;
            }
            break;
        }
        case 0x0305c004u: {
            u32 ignored = 0u;
            u32 ignoredBool = 0u;
            if (!stream.ReadU32(&ignored) ||
                !stream.ReadU32(&ignoredBool)) {
                return 0;
            }
            break;
        }
        case 0x0305c005u: {
            u32 ignoredBool = 0u;
            if (!stream.ReadU32(&ignoredBool)) {
                return 0;
            }
            break;
        }
        case TMNF_CLASS_CGameCtnZoneFlat:
        case 0x0305d001u: {
            const u32 refCount =
                    chunkId == TMNF_CLASS_CGameCtnZoneFlat ? 3u : 4u;
            for (u32 i = 0; i < refCount; i++) {
                char *pathBuffer =
                        descriptorSources.DescriptorPathArchiveBuffer(i);
                if (pathBuffer == nullptr) {
                    return 0;
                }
                if (!stream.ReadBlockInfoSourceRef(
                            pathBuffer,
                            descriptorSources
                                    .DescriptorPathArchiveBufferSize()) ||
                    !descriptorSources.NoteDescriptorPathIfPresent(i)) {
                    return 0;
                }
            }
            break;
        }
        case TMNF_CLASS_CGameCtnZoneFrontier:
        case 0x0305e001u: {
            char *pathBuffer =
                    descriptorSources.DescriptorPathArchiveBuffer(0u);
            if (pathBuffer == nullptr) {
                return 0;
            }
            if (!stream.ReadBlockInfoSourceRef(
                        pathBuffer,
                        descriptorSources.DescriptorPathArchiveBufferSize()) ||
                !stream.ReadCMwIdText(nullptr, 0u) ||
                !stream.ReadCMwIdText(nullptr, 0u) ||
                !descriptorSources.NoteDescriptorPathIfPresent(0u)) {
                return 0;
            }
            break;
        }
        case 0x0305d002u: {
            u32 ignored = 0u;
            u32 ignoredBoolA = 0u;
            u32 ignoredBoolB = 0u;
            if (!stream.ReadU32(&ignored) ||
                !stream.ReadU32(&ignoredBoolA) ||
                !stream.ReadU32(&ignoredBoolB)) {
                return 0;
            }
            break;
        }
        default:
            return 0;
        }
    }
    if (descriptorSources.ZoneIdName()[0] == '\0' &&
        !IdentifierFromPlainPath(
                descriptorSources.ZoneIdNameArchiveBuffer(),
                descriptorSources.ZoneIdNameArchiveBufferSize())) {
        return 0;
    }
    return zoneType == (u32)source.Kind() - 1u || zoneType == 0xffffffffu;
}

void CGameCtnReplayCollectionZoneSources::Clear() {
    refs.clear();
    externalRefCount = 0u;
    flatExternalRefCount = 0u;
    frontierExternalRefCount = 0u;
    loadableExternalRefCount = 0u;
    nonLoadableExternalRefCount = 0u;
    bufferArchiveTag = 0u;
    bufferRefCount = 0u;
    bufferConstructionRefCount = 0u;
    bufferFlatRefCount = 0u;
    bufferFrontierRefCount = 0u;
    defaultZoneNode = ArchiveNodeReference::Invalid();
    defaultZoneKind = CGameCtnReplayConstructionZoneKind::None;
}

int CGameCtnReplayCollectionZoneSources::CopyFrom(
        const CGameCtnReplayCollectionZoneSources &source) {
    try {
        refs = source.refs;
    } catch (const std::bad_alloc &) {
        Clear();
        return 0;
    }
    externalRefCount = source.externalRefCount;
    flatExternalRefCount = source.flatExternalRefCount;
    frontierExternalRefCount = source.frontierExternalRefCount;
    loadableExternalRefCount = source.loadableExternalRefCount;
    nonLoadableExternalRefCount = source.nonLoadableExternalRefCount;
    bufferArchiveTag = source.bufferArchiveTag;
    bufferRefCount = source.bufferRefCount;
    bufferConstructionRefCount = source.bufferConstructionRefCount;
    bufferFlatRefCount = source.bufferFlatRefCount;
    bufferFrontierRefCount = source.bufferFrontierRefCount;
    defaultZoneNode = source.defaultZoneNode;
    defaultZoneKind = source.defaultZoneKind;
    return 1;
}

int CGameCtnReplayCollectionZoneSources::AppendRef(
        const CGameCtnReplayCollectionZoneSourceInfo &source) {
    if (refs.size() >= UINT32_MAX) {
        return 0;
    }
    CGameCtnReplayCollectionZoneSourceRef entry;
    if (!entry.Configure(source)) {
        return 0;
    }
    try {
        refs.push_back(entry);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

int CGameCtnReplayCollectionZoneSources::ParseZoneBufferRefsChunk(
        const unsigned char *bytes,
        u32 byteCount,
        const BlockInfoDescriptorExternalRefs &externalRefs) {
    if (bytes == nullptr || externalRefs.BodyOffsetForFormatParser() > byteCount) {
        return 0;
    }
    CollectionZoneBufferArchiveStream stream{};
    stream.bytes = bytes;
    stream.byteCount = byteCount;
    stream.offset = externalRefs.BodyOffsetForFormatParser();

    u32 chunkId = 0u;
    if (!stream.ReadU32(&chunkId)) {
        return 0;
    }
    if (chunkId != ArchiveChunkIdValue(
                           CGameCtnCollectionArchiveChunkId::ZoneBufferRefs)) {
        return 1;
    }

    if (!stream.SkipCMwId()) {
        return 0;
    }

    u32 archiveTag = 0u;
    u32 zoneRefCount = 0u;
    if (!stream.ReadU32(&archiveTag) ||
        !stream.ReadU32(&zoneRefCount) ||
        zoneRefCount > externalRefs.NodeCountForFormatBounds() ||
        zoneRefCount > 0x10000u) {
        return 0;
    }
    bufferArchiveTag = archiveTag;
    bufferRefCount = zoneRefCount;

    u32 constructionZoneRefCount = 0u;
    u32 flatZoneRefCount = 0u;
    u32 frontierZoneRefCount = 0u;
    for (u32 i = 0; i < zoneRefCount; i++) {
        u32 archiveNodeIndex = 0u;
        if (!stream.ReadU32(&archiveNodeIndex)) {
            return 0;
        }
        const ArchiveNodeReference sourceNode =
                ArchiveNodeReference::FromIndex(archiveNodeIndex);
        CGameCtnReplayCollectionZoneSourceInfo source;
        if (!externalRefs.ConstructionZoneSourceInfo(sourceNode,
                                                     &source) ||
            !AppendRef(source)) {
            return 0;
        }
        if (source.IsConstructionZone()) {
            constructionZoneRefCount++;
        }
        if (source.IsFlat()) {
            flatZoneRefCount++;
        } else if (source.IsFrontier()) {
            frontierZoneRefCount++;
        }
    }
    bufferConstructionRefCount = constructionZoneRefCount;
    bufferFlatRefCount = flatZoneRefCount;
    bufferFrontierRefCount = frontierZoneRefCount;

    u32 newDefaultZoneArchiveIndex = 0u;
    u32 bool92 = 0u;
    float real31 = 0.0f;
    float real32 = 0.0f;
    if (!stream.ReadU32(&newDefaultZoneArchiveIndex) ||
        !stream.ReadU32(&bool92) ||
        !stream.ReadF32(&real31) ||
        !stream.ReadF32(&real32)) {
        return 0;
    }
    (void)bool92;
    (void)real31;
    (void)real32;
    defaultZoneNode = ArchiveNodeReference::FromIndex(newDefaultZoneArchiveIndex);
    CGameCtnReplayCollectionZoneSourceInfo defaultSource;
    if (!externalRefs.ConstructionZoneSourceInfo(defaultZoneNode,
                                                 &defaultSource)) {
        return 0;
    }
    defaultZoneKind = defaultSource.Kind();
    return 1;
}

int CGameCtnReplayCollectionZoneSources::LoadZoneDescriptors(
        const CPlugFilePack *installedPack) {
    if (installedPack == nullptr) {
        return 0;
    }
    for (CGameCtnReplayCollectionZoneSourceRef &entry : refs) {
        if ((!entry.IsFlat() && !entry.IsFrontier()) ||
            entry.PlainPath()[0] == '\0') {
            continue;
        }
        ByteBuffer zoneBytes{};
        const int extracted =
                installedPack->ExtractPath(entry.PlainPath(), &zoneBytes);
        if (!extracted) {
            if (!entry.FillCatalogDefaultSource()) {
                return 0;
            }
            continue;
        }
        if (zoneBytes.Empty() || zoneBytes.Size() > UINT32_MAX ||
            !entry.ParseZoneDescriptorSources(
                    zoneBytes.Data(), static_cast<u32>(zoneBytes.Size()))) {
            return 0;
        }
    }
    return 1;
}

int CGameCtnReplayCollectionZoneSources::CountExternalConstructionZones(
        const BlockInfoDescriptorExternalRefs &externalRefs) {
    for (const BlockInfoDescriptorExternalRef &ref :
         externalRefs.References()) {
        CGameCtnReplayCollectionZoneSourceInfo source;
        if (!externalRefs.ConstructionZoneSourceInfo(ref.ArchiveNode(),
                                                     &source)) {
            return 0;
        }
        if (!source.IsConstructionZone()) {
            continue;
        }
        externalRefCount++;
        if (source.IsLoadable()) {
            loadableExternalRefCount++;
        } else {
            nonLoadableExternalRefCount++;
        }
        if (source.IsFlat()) {
            flatExternalRefCount++;
        } else if (source.IsFrontier()) {
            frontierExternalRefCount++;
        }
    }
    return 1;
}

int CGameCtnReplayCollectionZoneSources::LoadFromCatalogCollection(
        const CPlugFilePack *installedPack,
        const char *collectionName) {
    Clear();
    if (installedPack == nullptr || collectionName == nullptr ||
        collectionName[0] == '\0') {
        return 1;
    }
    CatalogCollectionArchive collectionArchive;
    if (!collectionArchive.Load(installedPack, collectionName)) {
        return 0;
    }
    const BlockInfoDescriptorExternalRefs &externalRefs =
            collectionArchive.ExternalRefs();
    return ParseZoneBufferRefsChunk(collectionArchive.Bytes(),
                                    collectionArchive.ByteCount(),
                                    externalRefs) &&
           LoadZoneDescriptors(installedPack) &&
           CountExternalConstructionZones(externalRefs);
}

std::optional<std::string>
CGameCtnReplayCollectionZoneSources::DefaultBaseIdentifier() const {
    if (defaultZoneKind != CGameCtnReplayConstructionZoneKind::Flat) {
        return std::nullopt;
    }
    for (const CGameCtnReplayCollectionZoneSourceRef &ref : refs) {
        if (ref.SourceNode().Matches(defaultZoneNode)) {
            char identifier[64]{};
            if (ref.IdentifierFromPlainPath(identifier, sizeof(identifier))) {
                return std::string(identifier);
            }
            return std::nullopt;
        }
    }
    return std::nullopt;
}

CGameCtnReplayConstructionZoneKind
CGameCtnReplayCollectionZoneSources::DefaultZoneKind() const {
    return defaultZoneKind;
}
