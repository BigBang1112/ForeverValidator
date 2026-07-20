#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include <stddef.h>
#include <stdint.h>

#include "format/pack/block_info_catalog/replay_collection_zone_sources.h"
#include "format/archive/archive_binary.h"
#include "format/pack/installed/plug_file_pack.h"
#include <new>
static size_t blockinfo_external_refs_cstr_length(const char *text) {
    size_t length = 0u;
    while (text[length] != 0) {
        length++;
    }
    return length;
}

static int blockinfo_external_refs_copy_bytes_to_fixed(
        char *dst,
        size_t dstSize,
        const unsigned char *src,
        size_t byteCount) {
    if (dst == nullptr || dstSize == 0u || src == nullptr ||
        byteCount >= dstSize) {
        return 0;
    }
    for (size_t i = 0u; i < byteCount; i++) {
        dst[i] = static_cast<char>(src[i]);
    }
    dst[byteCount] = 0;
    return 1;
}

static int blockinfo_external_refs_copy_cstr_to_fixed(
        char *dst,
        size_t dstSize,
        const char *src) {
    if (dst == nullptr || dstSize == 0u || src == nullptr) {
        return 0;
    }
    const size_t length = blockinfo_external_refs_cstr_length(src);
    if (length >= dstSize) {
        return 0;
    }
    for (size_t i = 0u; i < length; ++i) {
        dst[i] = src[i];
    }
    dst[length] = '\0';
    return 1;
}

static int blockinfo_external_refs_substring_at(const char *text,
                                                const char *needle) {
    for (size_t i = 0u; needle[i] != 0; i++) {
        if (text[i] != needle[i]) {
            return 0;
        }
    }
    return 1;
}

static int blockinfo_external_refs_contains(const char *text,
                                            const char *needle) {
    if (text == nullptr || needle == nullptr) {
        return 0;
    }
    if (needle[0] == 0) {
        return 1;
    }
    for (size_t i = 0u; text[i] != 0; i++) {
        if (blockinfo_external_refs_substring_at(text + i, needle)) {
            return 1;
        }
    }
    return 0;
}

static int blockinfo_external_refs_contains_folder(const char *text,
                                                   const char *folderName) {
    if (text == nullptr || folderName == nullptr || folderName[0] == 0) {
        return 0;
    }
    const size_t folderLen = blockinfo_external_refs_cstr_length(folderName);
    for (size_t i = 0u; text[i] != 0; i++) {
        if (blockinfo_external_refs_substring_at(text + i, folderName) &&
            text[i + folderLen] == (char)0x5c) {
            return 1;
        }
    }
    return 0;
}

static CGameCtnReplayConstructionZoneKind
blockinfo_external_refs_construction_zone_kind_from_path(
        const char *plainPath) {
    if (plainPath == nullptr || plainPath[0] == 0 ||
        (!blockinfo_external_refs_contains_folder(plainPath,
                                                  "ConstructionZone") &&
         !blockinfo_external_refs_contains_folder(plainPath,
                                                  "TrackManiaZone"))) {
        return CGameCtnReplayConstructionZoneKind::None;
    }
    if (blockinfo_external_refs_contains_folder(plainPath,
                                                "ConstructionZoneFlat") ||
        blockinfo_external_refs_contains_folder(plainPath,
                                                "TrackManiaZoneFlat")) {
        return CGameCtnReplayConstructionZoneKind::Flat;
    }
    if (blockinfo_external_refs_contains_folder(plainPath,
                                                "ConstructionZoneFrontier") ||
        blockinfo_external_refs_contains_folder(plainPath,
                                                "TrackManiaZoneFrontier")) {
        return CGameCtnReplayConstructionZoneKind::Frontier;
    }
    return CGameCtnReplayConstructionZoneKind::Other;
}

struct BlockInfoDescriptorExternalRefParseStream {
    const unsigned char *bytes;
    u32 byteCount;
    u32 offset;

    int ReadU32(u32 *valueOut);
    int ReadStringFixed(char *out, size_t outSize);
};

int BlockInfoDescriptorExternalRefParseStream::ReadU32(u32 *valueOut) {
    if (valueOut == nullptr ||
        offset > byteCount ||
        byteCount - offset < 4u) {
        return 0;
    }
    *valueOut = TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset);
    offset += 4u;
    return 1;
}

int BlockInfoDescriptorExternalRefParseStream::ReadStringFixed(
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
    if (!blockinfo_external_refs_copy_bytes_to_fixed(
                out,
                outSize,
                bytes + offset,
                length)) {
        return 0;
    }
    offset += length;
    return 1;
}

void BlockInfoDescriptorExternalRef::Configure(
        ArchiveNodeReference sourceNode,
        u32 sourceLoadable,
        u32 sourceFolderIndex,
        const char *sourceName) {
    archiveNode = sourceNode;
    loadable = sourceLoadable;
    folderIndex = sourceFolderIndex;
    name[0] = '\0';
    if (sourceName != nullptr) {
        blockinfo_external_refs_copy_cstr_to_fixed(name,
                                                   sizeof(name),
                                                   sourceName);
    }
}

ArchiveNodeReference
BlockInfoDescriptorExternalRef::ArchiveNode() const {
    return archiveNode;
}

u32 BlockInfoDescriptorExternalRef::IsLoadable() const {
    return loadable;
}

const char *BlockInfoDescriptorExternalRef::FileName() const {
    return name;
}

int BlockInfoDescriptorExternalRef::HasName() const {
    return name[0] != '\0';
}

int BlockInfoDescriptorExternalRefs::ParseGbx(
        const unsigned char *bytes,
        u32 byteCount) {
    if (bytes == nullptr || byteCount < 17u ||
        bytes[0] != 'G' || bytes[1] != 'B' || bytes[2] != 'X') {
        return 0;
    }
    folders = SceneDescriptorFolderPaths{};
    installedPack = nullptr;
    installedPathRoot[0] = '\0';
    references.clear();
    nodeCount = 0u;
    bodyOffset = 0u;
    classId = 0u;
    version = 0u;
    version = (u32)bytes[3] | ((u32)bytes[4] << 8u);
    classId = TmnfFormat::ArchiveBinary::ReadU32LE(bytes + 9u);
    const u32 headerSize =
            TmnfFormat::ArchiveBinary::ReadU32LE(bytes + 13u);
    if (headerSize > UINT32_MAX - 17u ||
        17u + headerSize > byteCount ||
        byteCount - (17u + headerSize) < 8u) {
        return 0;
    }
    BlockInfoDescriptorExternalRefParseStream stream{};
    stream.bytes = bytes;
    stream.byteCount = byteCount;
    stream.offset = 17u + headerSize;
    u32 externalCount = 0u;
    if (!stream.ReadU32(&nodeCount) ||
        !stream.ReadU32(&externalCount) ||
        nodeCount > 0x100000u ||
        externalCount > 0x100000u) {
        return 0;
    }
    if (externalCount != 0u) {
        try {
            references.reserve(externalCount);
        } catch (const std::bad_alloc &) {
            return 0;
        }
        u32 externalRoot = 0u;
        SceneDescriptorFolderStack stack;
        if (!stream.ReadU32(&externalRoot) ||
            !folders.ParseSubfolders(&stream, stack, 0u)) {
            return 0;
        }
        (void)externalRoot;
        for (u32 i = 0; i < externalCount; i++) {
            char name[CGameCtnReplayStaticMobilModelNameCapacity];
            name[0] = '\0';
            u32 flags = 0u;
            if (!stream.ReadU32(&flags)) {
                return 0;
            }
            if ((flags & 4u) != 0u) {
                u32 id = 0u;
                if (!stream.ReadU32(&id)) {
                    return 0;
                }
                (void)id;
            } else if (!stream.ReadStringFixed(name, sizeof(name))) {
                return 0;
            }
            u32 archiveNodeIndex = 0u;
            if (!stream.ReadU32(&archiveNodeIndex)) {
                return 0;
            }
            u32 loadable = 0u;
            if (version >= 5u &&
                !stream.ReadU32(&loadable)) {
                return 0;
            }
            u32 folderIndex = 0u;
            if ((flags & 4u) == 0u &&
                !stream.ReadU32(&folderIndex)) {
                return 0;
            }
            BlockInfoDescriptorExternalRef ref;
            ref.Configure(ArchiveNodeReference::FromIndex(archiveNodeIndex),
                          loadable,
                          folderIndex,
                          name);
            try {
                references.push_back(ref);
            } catch (const std::bad_alloc &) {
                return 0;
            }
        }
    }
    bodyOffset = stream.offset;
    return 1;
}

void BlockInfoDescriptorExternalRefs::AttachInstalledPack(
        const CPlugFilePack *pack) {
    installedPack = pack;
    installedPathRoot[0] = '\0';
    if (pack != nullptr) {
        (void)blockinfo_external_refs_copy_cstr_to_fixed(
                installedPathRoot,
                sizeof(installedPathRoot),
                pack->PackName().c_str());
    }
}

int BlockInfoDescriptorExternalRefs::AttachInstalledPackRelativeTo(
        const CPlugFilePack *pack,
        const char *parentPlainPath) {
    installedPack = nullptr;
    installedPathRoot[0] = '\0';
    if (pack == nullptr || parentPlainPath == nullptr) {
        return 0;
    }
    size_t rootLength = 0u;
    while (parentPlainPath[rootLength] != '\0' &&
           parentPlainPath[rootLength] != '\\') {
        ++rootLength;
    }
    if (rootLength == 0u || rootLength >= sizeof(installedPathRoot)) {
        return 0;
    }
    for (size_t index = 0u; index < rootLength; ++index) {
        installedPathRoot[index] = parentPlainPath[index];
    }
    installedPathRoot[rootLength] = '\0';
    installedPack = pack;
    return 1;
}

const CPlugFilePack *BlockInfoDescriptorExternalRefs::InstalledPack() const {
    return installedPack;
}

u32 BlockInfoDescriptorExternalRefs::ClassId() const {
    return classId;
}

u32 BlockInfoDescriptorExternalRefs::BodyOffsetForFormatParser() const {
    return bodyOffset;
}

u32 BlockInfoDescriptorExternalRefs::NodeCountForFormatBounds() const {
    return nodeCount;
}

const std::vector<BlockInfoDescriptorExternalRef> &
BlockInfoDescriptorExternalRefs::References() const {
    return references;
}

const BlockInfoDescriptorExternalRef *
BlockInfoDescriptorExternalRefs::FindReference(
        ArchiveNodeReference sourceNode) const {
    for (const BlockInfoDescriptorExternalRef &ref : references) {
        if (ref.ArchiveNode().Matches(sourceNode)) {
            return &ref;
        }
    }
    return nullptr;
}

int BlockInfoDescriptorExternalRefs::PlainPathForReference(
        ArchiveNodeReference sourceNode,
        char *out,
        size_t outSize) const {
    return PlainPathForReference(FindReference(sourceNode), out, outSize);
}

int BlockInfoDescriptorExternalRefs::PlainPathForReference(
        const BlockInfoDescriptorExternalRef *ref,
        char *out,
        size_t outSize) const {
    if (ref == nullptr || out == nullptr || outSize == 0u) {
        return 0;
    }
    out[0] = '\0';
    if (!ref->HasName()) {
        return 1;
    }
    const char *folder = nullptr;
    return folders.FindFolderForRef(ref->FolderIndexForFormatLookup(), &folder) &&
           SceneDescriptorFolderPaths::BuildPackRefFullPath(folder,
                                                            ref->FileName(),
                                                            installedPathRoot[0] != '\0'
                                                                    ? installedPathRoot
                                                                    : "Stadium",
                                                            out,
                                                            outSize);
}

int BlockInfoDescriptorExternalRefs::OptionalPlainPathForReference(
        const BlockInfoDescriptorExternalRef *ref,
        char *out,
        size_t outSize) const {
    if (ref == nullptr || out == nullptr || outSize == 0u) {
        return 0;
    }
    out[0] = '\0';
    if (!ref->HasName()) {
        return 1;
    }
    const char *folder = nullptr;
    if (!folders.FindFolderForRef(ref->FolderIndexForFormatLookup(), &folder)) {
        return 1;
    }
    return SceneDescriptorFolderPaths::BuildPackRefFullPath(folder,
                                                            ref->FileName(),
                                                            installedPathRoot[0] != '\0'
                                                                    ? installedPathRoot
                                                                    : "Stadium",
                                                            out,
                                                            outSize);
}

int BlockInfoDescriptorExternalRefs::ConstructionZoneSourceInfo(
        ArchiveNodeReference sourceNode,
        CGameCtnReplayCollectionZoneSourceInfo *sourceOut) const {
    if (sourceOut == nullptr) {
        return 0;
    }
    sourceOut->Clear();
    const BlockInfoDescriptorExternalRef *ref = FindReference(sourceNode);
    if (ref == nullptr) {
        return 1;
    }
    char plainPath[CGameCtnReplayStaticPackPathCapacity];
    if (!PlainPathForReference(ref, plainPath, sizeof(plainPath))) {
        return 0;
    }
    CGameCtnReplayConstructionZoneKind kind =
            blockinfo_external_refs_construction_zone_kind_from_path(
                    plainPath);
    return sourceOut->Configure(sourceNode, kind, ref->IsLoadable(), plainPath);
}

int BlockInfoDescriptorExternalRefs::ResolveBlockInfoSourceRef(
        ArchiveNodeReference sourceNode,
        char *out,
        size_t outSize,
        u32 *resolvedExternalOut) const {
    if (out == nullptr || outSize == 0u) {
        return 0;
    }
    out[0] = '\0';
    if (resolvedExternalOut != nullptr) {
        *resolvedExternalOut = 0u;
    }
    const BlockInfoDescriptorExternalRef *ref = FindReference(sourceNode);
    if (ref == nullptr || !ref->HasName()) {
        return 1;
    }
    if (resolvedExternalOut != nullptr) {
        *resolvedExternalOut = 1u;
    }
    char plainPath[CGameCtnReplayStaticSelectedPathCapacity];
    if (!PlainPathForReference(ref, plainPath, sizeof(plainPath))) {
        return 0;
    }
    if (!blockinfo_external_refs_contains(plainPath, "ConstructionBlockInfo") &&
        !blockinfo_external_refs_contains(plainPath, "TrackManiaElementDesc")) {
        return 1;
    }
    return blockinfo_external_refs_copy_cstr_to_fixed(out,
                                                     outSize,
                                                     plainPath);
}
