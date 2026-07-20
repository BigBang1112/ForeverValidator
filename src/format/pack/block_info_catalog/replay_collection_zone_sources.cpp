#include "format/pack/block_info_catalog/replay_collection_zone_sources.h"
#include <stddef.h>
#include <stdint.h>
#include <cmath>
#include <utility>

#include "engine/resources/block_info_catalog.h"
#include "format/archive/archive_binary32.h"
#include "format/archive/archive_binary.h"
#include "format/archive/fixed_c_string.h"
#include "format/pack/block_info_catalog/blockinfo_archive_stream.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include "format/pack/block_info_catalog/block_info_catalog_identity_reader.h"
#include "format/archive/tmnf_gbx_body_reader.h"
#include "format/archive/archive_class_ids.h"
#include "format/pack/installed/byte_buffer.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/pack/block_info_catalog/replay_collection_archive.h"
#include "format/archive/tmnf_archive_ids.h"
#include "format/pack/block_info_catalog/replay_collection_archive_chunk_ids.h"
#include "format/archive/mw_id_archive_codec.h"
#include <new>
namespace {

constexpr u32 TmufBayCryptedCollectionSurfaceReplacements =
        0x5601d808u;
constexpr u32 TmufStadiumCryptedCollectionSceneRefs =
        0xa803300du;

float DecodeCollectionZoneFloat32(u32 encoded) {
    return TmnfArchiveBinary32::Decode(encoded);
}

int CopyCStringToZoneBuffer(char *dst, size_t dstSize, const char *src) {
    return TmnfFormat::FixedCString::Copy(dst, dstSize, src);
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

int ResolveZoneBlockInfoDescriptor(
        const CPlugFilePack *installedPack,
        const char *collectionName,
        const char *plainPath,
        u32 expectedClassId,
        std::optional<std::string> *identifierOut,
        std::optional<std::string> *selectedPathOut) {
    if (installedPack == nullptr || collectionName == nullptr ||
        identifierOut == nullptr || selectedPathOut == nullptr) {
        return 0;
    }
    identifierOut->reset();
    selectedPathOut->reset();
    if (plainPath == nullptr || plainPath[0] == '\0') {
        return 1;
    }

    char selectedPath[CGameCtnReplayStaticSelectedPathCapacity]{};
    ByteBuffer descriptorBytes;
    BlockInfoCatalogIdentity identity;
    BlockInfoCatalogIdentityReader identityReader;
    u32 classId = 0u;
    u32 bodyOffset = 0u;
    if (!installedPack->SelectedPathForPlainRef(
                plainPath,
                selectedPath,
                sizeof(selectedPath)) ||
        !installedPack->ExtractPath(selectedPath, &descriptorBytes) ||
        descriptorBytes.Empty() || descriptorBytes.Size() > UINT32_MAX ||
        !GbxBodyOffsetReader::TryParse(
                descriptorBytes.Data(),
                static_cast<u32>(descriptorBytes.Size()),
                &classId,
                &bodyOffset) ||
        classId != expectedClassId ||
        bodyOffset >= descriptorBytes.Size() ||
        !identityReader.Parse(
                descriptorBytes.Data(),
                static_cast<u32>(descriptorBytes.Size()),
                &identity) ||
        identity.identifier.empty() || identity.collection != collectionName) {
        return 0;
    }
    *identifierOut = std::move(identity.identifier);
    *selectedPathOut = selectedPath;
    return 1;
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
    int SkipCollectionIdentifierCMwId();
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

int CollectionZoneBufferArchiveStream::SkipCollectionIdentifierCMwId() {
    u32 encodedValue = 0u;
    if (!ReadU32(&encodedValue)) {
        return 0;
    }
    const TmnfFormat::ArchiveIdentifierWord identifier =
            TmnfFormat::CMwIdArchiveCodec::ParseWord(encodedValue);
    if (!identifier.IsNamed()) {
        return 0;
    }
    if (identifier.payload == 0u) {
        return SkipString();
    }

    // Crypted collection values can overwrite the low bytes of the first
    // following CMwId word. Its inline string remains intact in the archive.
    if (offset > byteCount || byteCount - offset < 4u) {
        return 1;
    }
    const u32 length =
            TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset);
    if (length == 0u || length > 128u ||
        length > byteCount - offset - 4u) {
        return 1;
    }
    for (u32 i = 0u; i < length; ++i) {
        const unsigned char c = bytes[offset + 4u + i];
        if (c < 0x20u || c > 0x7eu) {
            return 1;
        }
    }
    return SkipString();
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
    frontierParentIdName[0] = '\0';
    frontierChildIdName[0] = '\0';
    ClearZoneDescriptorPaths(descriptorPath);
    collectionLandZoneHeight.reset();
    zoneHeight = 0u;
    zoneDepth = 0u;
    oldZone = false;
    hasWater = false;
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

char *CGameCtnReplayZoneDescriptorSourceSet::
FrontierParentIdNameArchiveBuffer() {
    return frontierParentIdName;
}

char *CGameCtnReplayZoneDescriptorSourceSet::
FrontierChildIdNameArchiveBuffer() {
    return frontierChildIdName;
}

size_t CGameCtnReplayZoneDescriptorSourceSet::
FrontierIdNameArchiveBufferSize() const {
    return sizeof(frontierParentIdName);
}

const char *CGameCtnReplayZoneDescriptorSourceSet::
FrontierParentIdName() const {
    return frontierParentIdName;
}

const char *CGameCtnReplayZoneDescriptorSourceSet::
FrontierChildIdName() const {
    return frontierChildIdName;
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

const char *CGameCtnReplayZoneDescriptorSourceSet::FirstDescriptorPath()
        const {
    return parsedSourceCount != 0u ? descriptorPath[0] : "";
}

const char *CGameCtnReplayZoneDescriptorSourceSet::DescriptorPath(
        CGameCtnReplayZoneDescriptorRole role) const {
    const u32 slot = static_cast<u32>(role);
    return slot < SourcePathCount && slot < parsedSourceCount
            ? descriptorPath[slot]
            : "";
}

void CGameCtnReplayZoneDescriptorSourceSet::SetCollectionLandZoneHeight(
        u32 height) {
    collectionLandZoneHeight = height;
}

const std::optional<u32> &
CGameCtnReplayZoneDescriptorSourceSet::CollectionLandZoneHeight() const {
    return collectionLandZoneHeight;
}

void CGameCtnReplayZoneDescriptorSourceSet::SetZoneHeight(u32 value) {
    zoneHeight = value;
}

u32 CGameCtnReplayZoneDescriptorSourceSet::ZoneHeight() const {
    return zoneHeight;
}

void CGameCtnReplayZoneDescriptorSourceSet::SetZoneDepth(u32 value) {
    zoneDepth = value;
}

u32 CGameCtnReplayZoneDescriptorSourceSet::ZoneDepth() const {
    return zoneDepth;
}

void CGameCtnReplayZoneDescriptorSourceSet::SetOldZone(bool value) {
    oldZone = value;
}

bool CGameCtnReplayZoneDescriptorSourceSet::IsOldZone() const {
    return oldZone;
}

void CGameCtnReplayZoneDescriptorSourceSet::SetHasWater(bool value) {
    hasWater = value;
}

bool CGameCtnReplayZoneDescriptorSourceSet::HasWater() const {
    return hasWater;
}

int CGameCtnReplayCollectionZoneSourceRef::Configure(
        const CGameCtnReplayCollectionZoneSourceInfo &sourceInfo) {
    source = sourceInfo;
    descriptorSources.Clear();
    hasParsedDescriptorSources = false;
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

const char *CGameCtnReplayCollectionZoneSourceRef::FirstDescriptorPath()
        const {
    return descriptorSources.FirstDescriptorPath();
}

const char *CGameCtnReplayCollectionZoneSourceRef::DescriptorPath(
        CGameCtnReplayZoneDescriptorRole role) const {
    return descriptorSources.DescriptorPath(role);
}

bool CGameCtnReplayCollectionZoneSourceRef::HasParsedDescriptorSources()
        const {
    return hasParsedDescriptorSources;
}

const std::optional<u32> &
CGameCtnReplayCollectionZoneSourceRef::CollectionLandZoneHeight() const {
    return descriptorSources.CollectionLandZoneHeight();
}

u32 CGameCtnReplayCollectionZoneSourceRef::ZoneHeight() const {
    return descriptorSources.ZoneHeight();
}

u32 CGameCtnReplayCollectionZoneSourceRef::ZoneDepth() const {
    return descriptorSources.ZoneDepth();
}

bool CGameCtnReplayCollectionZoneSourceRef::IsOldZone() const {
    return descriptorSources.IsOldZone();
}

bool CGameCtnReplayCollectionZoneSourceRef::HasWater() const {
    return descriptorSources.HasWater();
}

const char *CGameCtnReplayCollectionZoneSourceRef::ZoneIdName() const {
    return descriptorSources.ZoneIdName();
}

const char *CGameCtnReplayCollectionZoneSourceRef::
FrontierParentIdName() const {
    return descriptorSources.FrontierParentIdName();
}

const char *CGameCtnReplayCollectionZoneSourceRef::
FrontierChildIdName() const {
    return descriptorSources.FrontierChildIdName();
}

int CGameCtnReplayCollectionZoneSourceRef::IdentifierFromPlainPath(
        char *out,
        size_t outSize) const {
    return source.IdentifierFromPlainPath(out, outSize);
}

int CGameCtnReplayCollectionZoneSourceRef::ParseZoneDescriptorSources(
        const unsigned char *bytes,
        u32 byteCount,
        const CPlugFilePack *installedPack) {
    descriptorSources.Clear();
    hasParsedDescriptorSources = false;
    if (bytes == nullptr || byteCount < 17u) {
        return 0;
    }
    if (!source.IsFlat() && !source.IsFrontier()) {
        return 0;
    }
    CGameCtnReplayZoneDescriptorSourceSet parsedDescriptorSources;
    parsedDescriptorSources.Clear();
    BlockInfoDescriptorExternalRefs refs;
    const u32 expectedClassId = source.IsFlat()
            ? TMNF_CLASS_CGameCtnZoneFlat
            : TMNF_CLASS_CGameCtnZoneFrontier;
    if (!refs.ParseGbx(bytes, byteCount) ||
        refs.ClassId() != expectedClassId) {
        return 0;
    }
    refs.AttachInstalledPack(installedPack);
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
                        parsedDescriptorSources.ZoneIdNameArchiveBuffer(),
                        parsedDescriptorSources.ZoneIdNameArchiveBufferSize()) ||
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
                        parsedDescriptorSources.ZoneIdNameArchiveBuffer(),
                        parsedDescriptorSources.ZoneIdNameArchiveBufferSize()) ||
                !stream.ReadU32(&zoneType)) {
                return 0;
            }
            break;
        }
        case 0x0305c003u: {
            u32 zoneHeight = 0u;
            char basicZoneName[64];
            if (!stream.ReadU32(&zoneHeight)) {
                return 0;
            }
            parsedDescriptorSources.SetCollectionLandZoneHeight(
                    zoneHeight);
            parsedDescriptorSources.SetZoneHeight(zoneHeight);
            if (!stream.ReadCMwIdText(
                        parsedDescriptorSources.ZoneIdNameArchiveBuffer(),
                        parsedDescriptorSources.ZoneIdNameArchiveBufferSize()) ||
                !stream.ReadCMwIdText(basicZoneName, sizeof(basicZoneName)) ||
                !stream.ReadU32(&zoneType)) {
                return 0;
            }
            break;
        }
        case 0x0305c004u: {
            u32 ignoredMarker = 0u;
            u32 ignoredDeclaredSize = 0u;
            u32 zoneDepth = 0u;
            u32 archivedOldZone = 0u;
            if (!stream.ReadU32(&ignoredMarker) ||
                !stream.ReadU32(&ignoredDeclaredSize) ||
                !stream.ReadU32(&zoneDepth) ||
                !stream.ReadU32(&archivedOldZone)) {
                return 0;
            }
            parsedDescriptorSources.SetZoneDepth(zoneDepth);
            parsedDescriptorSources.SetOldZone(archivedOldZone != 0u);
            break;
        }
        case 0x0305c005u: {
            u32 archivedHasWater = 0u;
            if (!stream.ReadU32(&archivedHasWater)) {
                return 0;
            }
            parsedDescriptorSources.SetHasWater(archivedHasWater != 0u);
            break;
        }
        case TMNF_CLASS_CGameCtnZoneFlat:
        case 0x0305d001u: {
            const u32 refCount =
                    chunkId == TMNF_CLASS_CGameCtnZoneFlat ? 3u : 4u;
            for (u32 i = 0; i < refCount; i++) {
                char *pathBuffer =
                        parsedDescriptorSources.DescriptorPathArchiveBuffer(i);
                if (pathBuffer == nullptr) {
                    return 0;
                }
                if (!stream.ReadBlockInfoSourceRef(
                            pathBuffer,
                            parsedDescriptorSources
                                    .DescriptorPathArchiveBufferSize()) ||
                    !parsedDescriptorSources.NoteDescriptorPathIfPresent(i)) {
                    return 0;
                }
            }
            break;
        }
        case TMNF_CLASS_CGameCtnZoneFrontier:
        case 0x0305e001u: {
            char *pathBuffer =
                    parsedDescriptorSources.DescriptorPathArchiveBuffer(0u);
            if (pathBuffer == nullptr) {
                return 0;
            }
            if (!stream.ReadBlockInfoSourceRef(
                        pathBuffer,
                        parsedDescriptorSources
                                .DescriptorPathArchiveBufferSize()) ||
                !stream.ReadCMwIdText(
                        parsedDescriptorSources
                                .FrontierParentIdNameArchiveBuffer(),
                        parsedDescriptorSources
                                .FrontierIdNameArchiveBufferSize()) ||
                !stream.ReadCMwIdText(
                        parsedDescriptorSources
                                .FrontierChildIdNameArchiveBuffer(),
                        parsedDescriptorSources
                                .FrontierIdNameArchiveBufferSize()) ||
                !parsedDescriptorSources.NoteDescriptorPathIfPresent(0u)) {
                return 0;
            }
            break;
        }
        case 0x0305d002u: {
            u32 word = 0u;
            u32 ignoredBoolA = 0u;
            u32 ignoredBoolB = 0u;
            if (!stream.ReadU32(&word)) {
                return 0;
            }
            if (word == 0x534b4950u) {
                u32 payloadSize = 0u;
                if (!stream.ReadU32(&payloadSize) ||
                    !stream.Skip(payloadSize)) {
                    return 0;
                }
            } else if (!stream.ReadU32(&ignoredBoolA) ||
                       !stream.ReadU32(&ignoredBoolB)) {
                return 0;
            }
            break;
        }
        default:
            return 0;
        }
    }
    if (parsedDescriptorSources.ZoneIdName()[0] == '\0') {
        if (!IdentifierFromPlainPath(
                    parsedDescriptorSources.ZoneIdNameArchiveBuffer(),
                    parsedDescriptorSources.ZoneIdNameArchiveBufferSize())) {
            return 0;
        }
    }
    if (zoneType != (u32)source.Kind() - 1u && zoneType != 0xffffffffu) {
        return 0;
    }
    descriptorSources = std::move(parsedDescriptorSources);
    hasParsedDescriptorSources = true;
    return 1;
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
    squareSize = 0.0f;
    squareHeight = 0.0f;
    waterSurfaceOffset = -1000.0f;
    waterSecondaryOffset = -1000.0f;
    waterRenderCullOffset = -1000.0f;
    defaultWater = true;
    geometryWaterPlanes = false;
    hasWaterDefinition = false;
    defaultBaseIdentifier.clear();
    defaultPylonIdentifier.clear();
}

int CGameCtnReplayCollectionZoneSources::CopyFrom(
        const CGameCtnReplayCollectionZoneSources &source) {
    try {
        refs = source.refs;
        defaultBaseIdentifier = source.defaultBaseIdentifier;
        defaultPylonIdentifier = source.defaultPylonIdentifier;
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
    squareSize = source.squareSize;
    squareHeight = source.squareHeight;
    waterSurfaceOffset = source.waterSurfaceOffset;
    waterSecondaryOffset = source.waterSecondaryOffset;
    waterRenderCullOffset = source.waterRenderCullOffset;
    defaultWater = source.defaultWater;
    geometryWaterPlanes = source.geometryWaterPlanes;
    hasWaterDefinition = source.hasWaterDefinition;
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
        const BlockInfoDescriptorExternalRefs &externalRefs,
        u32 *nextOffsetOut,
        u32 *cmwIdModeOut,
        u32 *hasCmwIdModeOut) {
    if (bytes == nullptr || nextOffsetOut == nullptr ||
        cmwIdModeOut == nullptr || hasCmwIdModeOut == nullptr ||
        externalRefs.BodyOffsetForFormatParser() > byteCount) {
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
        return 0;
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
    float collectionSquareSize = 0.0f;
    float collectionSquareHeight = 0.0f;
    if (!stream.ReadU32(&newDefaultZoneArchiveIndex) ||
        !stream.ReadU32(&bool92) ||
        !stream.ReadF32(&collectionSquareSize) ||
        !stream.ReadF32(&collectionSquareHeight) ||
        !stream.SkipCollectionIdentifierCMwId() ||
        !stream.SkipCollectionIdentifierCMwId() ||
        !stream.SkipCollectionIdentifierCMwId()) {
        return 0;
    }
    (void)bool92;
    squareSize = collectionSquareSize;
    squareHeight = collectionSquareHeight;
    defaultZoneNode = ArchiveNodeReference::FromIndex(newDefaultZoneArchiveIndex);
    CGameCtnReplayCollectionZoneSourceInfo defaultSource;
    if (!externalRefs.ConstructionZoneSourceInfo(defaultZoneNode,
                                                 &defaultSource)) {
        return 0;
    }
    defaultZoneKind = defaultSource.Kind();
    *nextOffsetOut = stream.offset;
    *cmwIdModeOut = stream.cmwIdMode;
    *hasCmwIdModeOut = stream.hasCmwIdMode;
    return 1;
}

int CGameCtnReplayCollectionZoneSources::ParseWaterDefinitionChunks(
        const unsigned char *bytes,
        u32 byteCount,
        u32 bodyOffset,
        u32 cmwIdMode,
        u32 hasCmwIdMode) {
    if (bytes == nullptr || bodyOffset > byteCount) {
        return 0;
    }

    CollectionZoneBufferArchiveStream stream{};
    stream.bytes = bytes;
    stream.byteCount = byteCount;
    stream.offset = bodyOffset;
    stream.cmwIdMode = cmwIdMode;
    stream.hasCmwIdMode = hasCmwIdMode;

    float parsedWaterSurfaceOffset = waterSurfaceOffset;
    float parsedWaterSecondaryOffset = waterSecondaryOffset;
    float parsedWaterRenderCullOffset = waterRenderCullOffset;
    bool parsedDefaultWater = defaultWater;
    bool parsedGeometryWaterPlanes = geometryWaterPlanes;
    u32 previousChunkIndex = 9u;
    bool sawWaterHeights = false;
    bool sawGeometryPlanes = false;
    for (;;) {
        u32 chunkId = 0u;
        if (!stream.ReadU32(&chunkId)) {
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            if (stream.offset != byteCount) {
                return 0;
            }
            waterSurfaceOffset = parsedWaterSurfaceOffset;
            waterSecondaryOffset = parsedWaterSecondaryOffset;
            waterRenderCullOffset = parsedWaterRenderCullOffset;
            defaultWater = parsedDefaultWater;
            geometryWaterPlanes = parsedGeometryWaterPlanes;
            hasWaterDefinition = sawWaterHeights || sawGeometryPlanes;
            return 1;
        }
        if (chunkId ==
            TmufBayCryptedCollectionSurfaceReplacements) {
            chunkId = 0x0303301du;
        } else if (chunkId ==
                   TmufStadiumCryptedCollectionSceneRefs) {
            chunkId = 0x0303300du;
        }
        if ((chunkId & 0xffffff00u) != TMNF_CLASS_CGameCtnCollection ||
            (chunkId & 0xffu) <= previousChunkIndex ||
            (chunkId & 0xffu) > 0x24u) {
            return 0;
        }
        const u32 chunkIndex = chunkId & 0xffu;
        previousChunkIndex = chunkIndex;
        switch (chunkId) {
        case 0x0303300cu:
            if (!stream.Skip(8u)) {
                return 0;
            }
            break;
        case 0x0303300du: {
            if (!stream.Skip(16u)) {
                return 0;
            }
            break;
        }
        case 0x0303300eu:
        case 0x03033011u:
        case 0x03033019u:
            if (!stream.Skip(4u)) {
                return 0;
            }
            break;
        case 0x0303301au:
            if (!stream.Skip(48u)) {
                return 0;
            }
            break;
        case 0x0303301du: {
            u32 replacementCount = 0u;
            if (!stream.ReadU32(&replacementCount) ||
                replacementCount > 0x100000u) {
                return 0;
            }
            for (u32 i = 0u; i < replacementCount; ++i) {
                if (!stream.SkipCMwId() || !stream.SkipCMwId() ||
                    !stream.Skip(4u)) {
                    return 0;
                }
            }
            if (!stream.Skip(8u)) {
                return 0;
            }
            break;
        }
        case 0x0303301eu: {
            float surface = 0.0f;
            float secondary = 0.0f;
            float renderCull = 0.0f;
            u32 archivedDefaultWater = 0u;
            if (sawWaterHeights ||
                !stream.ReadF32(&surface) ||
                !stream.ReadF32(&secondary) ||
                !stream.ReadF32(&renderCull) ||
                !stream.ReadU32(&archivedDefaultWater) ||
                !std::isfinite(surface) || !std::isfinite(secondary) ||
                !std::isfinite(renderCull) || archivedDefaultWater > 1u) {
                return 0;
            }
            parsedWaterSurfaceOffset = surface;
            parsedWaterSecondaryOffset = secondary;
            parsedWaterRenderCullOffset = renderCull;
            parsedDefaultWater = archivedDefaultWater != 0u;
            sawWaterHeights = true;
            break;
        }
        case 0x0303301fu: {
            u32 seaHouleFileCount = 0u;
            if (!stream.ReadU32(&seaHouleFileCount) ||
                seaHouleFileCount > 0x100000u ||
                seaHouleFileCount > (byteCount - stream.offset) / 4u ||
                !stream.Skip(seaHouleFileCount * 4u)) {
                return 0;
            }
            break;
        }
        case 0x03033020u:
            for (u32 i = 0u; i < 4u; ++i) {
                if (!stream.SkipString()) {
                    return 0;
                }
            }
            break;
        case 0x03033021u:
            if (!stream.SkipString()) {
                return 0;
            }
            break;
        case 0x03033022u: {
            u32 archivedGeometryPlanes = 0u;
            if (sawGeometryPlanes ||
                !stream.ReadU32(&archivedGeometryPlanes) ||
                archivedGeometryPlanes > 1u) {
                return 0;
            }
            parsedGeometryWaterPlanes = archivedGeometryPlanes != 0u;
            sawGeometryPlanes = true;
            break;
        }
        case 0x03033023u: {
            u32 ignoredTerrainModifierFlag = 0u;
            if (!stream.ReadU32(&ignoredTerrainModifierFlag) ||
                ignoredTerrainModifierFlag > 1u) {
                return 0;
            }
            break;
        }
        case 0x03033024u:
            if (!stream.Skip(28u)) {
                return 0;
            }
            break;
        default:
            return 0;
        }
    }
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
        char selectedPath[CGameCtnReplayStaticSelectedPathCapacity]{};
        const int extracted = installedPack->SelectedPathForPlainRef(
                                      entry.PlainPath(),
                                      selectedPath,
                                      sizeof(selectedPath)) &&
                              installedPack->ExtractPath(
                                      selectedPath, &zoneBytes);
        if (!extracted || zoneBytes.Empty() ||
            zoneBytes.Size() > UINT32_MAX ||
            !entry.ParseZoneDescriptorSources(
                    zoneBytes.Data(),
                    static_cast<u32>(zoneBytes.Size()),
                    installedPack)) {
            return 0;
        }
    }
    return 1;
}

int CGameCtnReplayCollectionZoneSources::ResolveDefaultBaseIdentifier(
        const CPlugFilePack *installedPack) {
    defaultBaseIdentifier.clear();
    if (installedPack == nullptr ||
        defaultZoneKind != CGameCtnReplayConstructionZoneKind::Flat) {
        return 1;
    }
    for (const CGameCtnReplayCollectionZoneSourceRef &ref : refs) {
        if (!ref.SourceNode().Matches(defaultZoneNode)) {
            continue;
        }
        const char *path = ref.FirstDescriptorPath();
        ByteBuffer descriptorBytes;
        BlockInfoCatalogIdentity identity;
        BlockInfoCatalogIdentityReader reader;
        if (path == nullptr || *path == '\0' ||
            !installedPack->ExtractPath(path, &descriptorBytes) ||
            descriptorBytes.Empty() || descriptorBytes.Size() > UINT32_MAX ||
            !reader.Parse(descriptorBytes.Data(),
                          static_cast<u32>(descriptorBytes.Size()),
                          &identity)) {
            return 0;
        }
        try {
            defaultBaseIdentifier = std::move(identity.identifier);
        } catch (const std::bad_alloc &) {
            return 0;
        }
        return !defaultBaseIdentifier.empty();
    }
    return 0;
}

int CGameCtnReplayCollectionZoneSources::ResolveDefaultPylonIdentifier(
        const CPlugFilePack *installedPack,
        const char *collectionName) {
    defaultPylonIdentifier.clear();
    if (installedPack == nullptr || collectionName == nullptr ||
        collectionName[0] == '\0' ||
        defaultZoneKind != CGameCtnReplayConstructionZoneKind::Flat) {
        return 1;
    }
    for (const CGameCtnReplayCollectionZoneSourceRef &ref : refs) {
        if (!ref.SourceNode().Matches(defaultZoneNode)) {
            continue;
        }
        const char *path = ref.DescriptorPath(
                CGameCtnReplayZoneDescriptorRole::Pylon);
        if (path == nullptr || path[0] == '\0') {
            return 1;
        }
        ByteBuffer descriptorBytes;
        BlockInfoCatalogIdentity identity;
        BlockInfoCatalogIdentityReader reader;
        u32 classId = 0u;
        u32 bodyOffset = 0u;
        if (!installedPack->ExtractPath(path, &descriptorBytes) ||
            descriptorBytes.Empty() || descriptorBytes.Size() > UINT32_MAX ||
            !GbxBodyOffsetReader::TryParse(
                    descriptorBytes.Data(),
                    static_cast<u32>(descriptorBytes.Size()),
                    &classId,
                    &bodyOffset) ||
            classId != TMNF_CLASS_CGameCtnBlockInfoPylon ||
            bodyOffset >= descriptorBytes.Size() ||
            !reader.Parse(descriptorBytes.Data(),
                          static_cast<u32>(descriptorBytes.Size()),
                          &identity) ||
            identity.collection != collectionName ||
            identity.identifier.empty()) {
            return 0;
        }
        try {
            defaultPylonIdentifier = std::move(identity.identifier);
        } catch (const std::bad_alloc &) {
            return 0;
        }
        return 1;
    }
    return 0;
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
    u32 nextBodyOffset = 0u;
    u32 cmwIdMode = 0u;
    u32 hasCmwIdMode = 0u;
    return ParseZoneBufferRefsChunk(collectionArchive.Bytes(),
                                    collectionArchive.ByteCount(),
                                    externalRefs,
                                    &nextBodyOffset,
                                    &cmwIdMode,
                                    &hasCmwIdMode) &&
           ParseWaterDefinitionChunks(
                   collectionArchive.Bytes(),
                   collectionArchive.ByteCount(),
                   nextBodyOffset,
                   cmwIdMode,
                   hasCmwIdMode) &&
           LoadZoneDescriptors(installedPack) &&
           ResolveDefaultBaseIdentifier(installedPack) &&
           ResolveDefaultPylonIdentifier(installedPack, collectionName) &&
           CountExternalConstructionZones(externalRefs);
}

std::optional<std::string>
CGameCtnReplayCollectionZoneSources::DefaultBaseIdentifier() const {
    if (defaultZoneKind != CGameCtnReplayConstructionZoneKind::Flat ||
        defaultBaseIdentifier.empty()) {
        return std::nullopt;
    }
    return defaultBaseIdentifier;
}

std::optional<std::string>
CGameCtnReplayCollectionZoneSources::DefaultPylonIdentifier() const {
    return defaultPylonIdentifier.empty()
            ? std::nullopt
            : std::optional<std::string>(defaultPylonIdentifier);
}

std::optional<std::string>
CGameCtnReplayCollectionZoneSources::DefaultDescriptorPath(
        CGameCtnReplayZoneDescriptorRole role) const {
    for (const CGameCtnReplayCollectionZoneSourceRef &ref : refs) {
        if (!ref.SourceNode().Matches(defaultZoneNode)) {
            continue;
        }
        const char *path = ref.DescriptorPath(role);
        return path != nullptr && path[0] != '\0'
                ? std::optional<std::string>(path)
                : std::nullopt;
    }
    return std::nullopt;
}

CGameCtnReplayConstructionZoneKind
CGameCtnReplayCollectionZoneSources::DefaultZoneKind() const {
    return defaultZoneKind;
}

float CGameCtnReplayCollectionZoneSources::SquareSize() const {
    return squareSize;
}

float CGameCtnReplayCollectionZoneSources::SquareHeight() const {
    return squareHeight;
}

bool CGameCtnReplayCollectionZoneSources::HasWaterDefinition() const {
    return hasWaterDefinition;
}

int CGameCtnReplayCollectionZoneSources::BuildWaterDefinition(
        const CPlugFilePack *installedPack,
        const char *collectionName,
        CatalogCollectionWaterDefinition *out) const {
    if (installedPack == nullptr || collectionName == nullptr ||
        collectionName[0] == '\0' || out == nullptr ||
        defaultBaseIdentifier.empty()) {
        return 0;
    }

    CatalogCollectionWaterDefinition water;
    water.surfaceOffset = waterSurfaceOffset;
    water.secondaryOffset = waterSecondaryOffset;
    water.renderCullOffset = waterRenderCullOffset;
    water.defaultWater = defaultWater;
    water.geometryWaterPlanes = geometryWaterPlanes;
    bool foundDefaultZone = false;
    bool foundDefaultClip = false;
    try {
        water.zones.reserve(refs.size());
        for (const CGameCtnReplayCollectionZoneSourceRef &ref : refs) {
            if (!ref.IsFlat() && !ref.IsFrontier()) {
                continue;
            }
            if (!ref.HasParsedDescriptorSources()) {
                return 0;
            }
            const char *descriptorPath = ref.FirstDescriptorPath();
            char selectedDescriptorPath[
                    CGameCtnReplayStaticSelectedPathCapacity]{};
            ByteBuffer descriptorBytes;
            BlockInfoCatalogIdentity identity;
            BlockInfoCatalogIdentityReader identityReader;
            if (descriptorPath == nullptr || descriptorPath[0] == '\0' ||
                !installedPack->SelectedPathForPlainRef(
                        descriptorPath,
                        selectedDescriptorPath,
                        sizeof(selectedDescriptorPath)) ||
                !installedPack->ExtractPath(
                        selectedDescriptorPath, &descriptorBytes) ||
                descriptorBytes.Empty() ||
                descriptorBytes.Size() > UINT32_MAX ||
                !identityReader.Parse(
                        descriptorBytes.Data(),
                        static_cast<u32>(descriptorBytes.Size()),
                        &identity) ||
                identity.identifier.empty() ||
                identity.collection != collectionName) {
                return 0;
            }
            CatalogWaterZoneDefinition zone;
            zone.blockInfoIdentifier = std::move(identity.identifier);
            const char *zoneIdName = ref.ZoneIdName();
            if (zoneIdName == nullptr || zoneIdName[0] == '\0') {
                return 0;
            }
            zone.identifier = zoneIdName;
            for (const CatalogWaterZoneDefinition &existing : water.zones) {
                if (existing.identifier == zone.identifier ||
                    existing.blockInfoIdentifier == zone.blockInfoIdentifier) {
                    return 0;
                }
            }
            zone.kind = ref.IsFlat()
                    ? CatalogConstructionZoneKind::Flat
                    : CatalogConstructionZoneKind::Frontier;
            if (ref.IsFlat()) {
                const char *clipDescriptorPath = ref.DescriptorPath(
                        CGameCtnReplayZoneDescriptorRole::Clip);
                const char *pylonDescriptorPath = ref.DescriptorPath(
                        CGameCtnReplayZoneDescriptorRole::Pylon);
                if (!ResolveZoneBlockInfoDescriptor(
                            installedPack,
                            collectionName,
                            clipDescriptorPath,
                            TMNF_CLASS_CGameCtnBlockInfoClip,
                            &zone.clipIdentifier,
                            &zone.selectedClipDescriptorPath) ||
                    !ResolveZoneBlockInfoDescriptor(
                            installedPack,
                            collectionName,
                            pylonDescriptorPath,
                            TMNF_CLASS_CGameCtnBlockInfoPylon,
                            &zone.pylonIdentifier,
                            &zone.selectedPylonDescriptorPath)) {
                    return 0;
                }
            } else {
                const char *parent = ref.FrontierParentIdName();
                const char *child = ref.FrontierChildIdName();
                if (parent == nullptr || parent[0] == '\0' ||
                    child == nullptr || child[0] == '\0') {
                    return 0;
                }
                zone.frontierParentIdentifier = parent;
                zone.frontierChildIdentifier = child;
            }
            zone.height = ref.ZoneHeight();
            zone.depth = ref.ZoneDepth();
            zone.oldZone = ref.IsOldZone();
            zone.hasWater = ref.HasWater();
            if (zone.blockInfoIdentifier == defaultBaseIdentifier) {
                water.defaultZoneHasWater = zone.hasWater;
                foundDefaultZone = true;
                foundDefaultClip = zone.clipIdentifier.has_value() &&
                        zone.selectedClipDescriptorPath.has_value();
            }
            water.zones.push_back(std::move(zone));
        }
    } catch (const std::bad_alloc &) {
        return 0;
    }
    if (!foundDefaultZone || !foundDefaultClip || water.zones.empty()) {
        return 0;
    }
    *out = std::move(water);
    return 1;
}

int CGameCtnReplayCollectionZoneSources::ApplyCollectionLandZoneHeights(
        const CPlugFilePack *installedPack,
        const char *collectionName,
        BlockInfoCatalog *catalog) const {
    if (installedPack == nullptr || collectionName == nullptr ||
        collectionName[0] == '\0' || catalog == nullptr) {
        return 0;
    }
    std::vector<BlockInfoCatalogLandZoneHeightAssignment> assignments;
    try {
        assignments.reserve(refs.size());
        for (const CGameCtnReplayCollectionZoneSourceRef &ref : refs) {
            const std::optional<u32> &height =
                    ref.CollectionLandZoneHeight();
            if (!height.has_value()) {
                continue;
            }
            if (!ref.IsFlat() && !ref.IsFrontier()) {
                return 0;
            }

            BlockInfoCatalogLandZoneHeightAssignment assignment;
            assignment.blockType = ref.IsFlat()
                    ? EBlockType::Flat
                    : EBlockType::Frontier;
            assignment.height = *height;
            const char *descriptorPath = ref.FirstDescriptorPath();
            char selectedDescriptorPath[
                    CGameCtnReplayStaticSelectedPathCapacity]{};
            if (descriptorPath != nullptr && descriptorPath[0] != '\0' &&
                installedPack->SelectedPathForPlainRef(
                        descriptorPath,
                        selectedDescriptorPath,
                        sizeof(selectedDescriptorPath))) {
                ByteBuffer descriptorBytes;
                BlockInfoCatalogIdentity identity;
                BlockInfoCatalogIdentityReader identityReader;
                if (!installedPack->ExtractPath(
                            selectedDescriptorPath, &descriptorBytes) ||
                    descriptorBytes.Empty() ||
                    descriptorBytes.Size() > UINT32_MAX ||
                    !identityReader.Parse(
                            descriptorBytes.Data(),
                            static_cast<u32>(descriptorBytes.Size()),
                            &identity) ||
                    identity.collection != collectionName) {
                    return 0;
                }
                assignment.identifier = std::move(identity.identifier);
                assignment.collection = std::move(identity.collection);
            } else {
                char sourceIdentifier[128]{};
                if (!ref.IdentifierFromPlainPath(
                            sourceIdentifier, sizeof(sourceIdentifier))) {
                    return 0;
                }
                assignment.identifier = sourceIdentifier;
                assignment.alternateIdentifier = collectionName;
                assignment.alternateIdentifier += sourceIdentifier;
                assignment.collection = collectionName;
            }
            assignments.push_back(std::move(assignment));
        }
    } catch (const std::bad_alloc &) {
        return 0;
    }
    return catalog->ApplyCollectionLandZoneHeights(assignments) ==
            BlockInfoCatalogLandZoneHeightResult::Success;
}
