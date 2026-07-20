#include "format/pack/block_info_catalog/collection_surface_replacement_pairs.h"
#include <stddef.h>
#include <stdint.h>
#include <string>

#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include "format/archive/archive_binary.h"
#include "format/archive/fixed_c_string.h"
#include "format/pack/block_info_catalog/replay_collection_archive.h"
#include "format/pack/block_info_catalog/replay_collection_archive_chunk_ids.h"
#include "format/archive/mw_id_archive_codec.h"
#include <new>
namespace {

int CopyStringToSurfacePairBuffer(
        char *dst,
        size_t dstSize,
        const std::string &src) {
    if (dst == nullptr || dstSize == 0u) {
        return 0;
    }
    TmnfFormat::FixedCString::CopyTruncated(dst, dstSize, src);
    return 1;
}

struct CollectionArchiveLocalCMwIdEntry {
    std::string text;
    u32 encodedValue{};
};

struct CollectionArchiveLocalCMwIdTable {
    std::vector<CollectionArchiveLocalCMwIdEntry> entries;

    int Append(const char *name, u32 encodedValue = 0u);
    const CollectionArchiveLocalCMwIdEntry *FindOneBased(u32 index) const;
};

int CollectionArchiveLocalCMwIdTable::Append(
        const char *name,
        u32 encodedValue) {
    CollectionArchiveLocalCMwIdEntry row;
    row.encodedValue = encodedValue;
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

const CollectionArchiveLocalCMwIdEntry *
CollectionArchiveLocalCMwIdTable::FindOneBased(u32 index) const {
    if (index == 0u || (size_t)(index - 1u) >= entries.size()) {
        return nullptr;
    }
    return &entries[index - 1u];
}

struct CollectionReplacementPairsArchiveStream {
    const unsigned char *bytes = nullptr;
    u32 byteCount = 0u;
    u32 offset = 0u;
    u32 cmwIdMode = 0u;
    u32 hasCmwIdMode = 0u;
    CollectionArchiveLocalCMwIdTable cmwIdTable;

    int ReadU32(u32 *valueOut);
    int ReadStringOwned(std::string *out);
    int ReadStringText(char *out, size_t outSize);
    int ReadCMwIdText(char *out, size_t outSize);
    int ReadCMwIdEncodedText(u32 *encodedOut, char *out, size_t outSize);
};

int CollectionReplacementPairsArchiveStream::ReadU32(u32 *valueOut) {
    if (valueOut == nullptr ||
        offset > byteCount ||
        byteCount - offset < 4u) {
        return 0;
    }
    *valueOut = TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset);
    offset += 4u;
    return 1;
}

int CollectionReplacementPairsArchiveStream::ReadStringOwned(
        std::string *out) {
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

int CollectionReplacementPairsArchiveStream::ReadStringText(
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
        return CopyStringToSurfacePairBuffer(out, outSize, text);
    }
    return 1;
}

int CollectionReplacementPairsArchiveStream::ReadCMwIdText(
        char *out,
        size_t outSize) {
    u32 encodedValue = 0u;
    return ReadCMwIdEncodedText(&encodedValue, out, outSize);
}

int CollectionReplacementPairsArchiveStream::ReadCMwIdEncodedText(
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
            if (!ReadStringOwned(&name) ||
                !cmwIdTable.Append(name.c_str())) {
                return 0;
            }
            if (out != nullptr && outSize != 0u) {
                return CopyStringToSurfacePairBuffer(out, outSize, name);
            }
            return 1;
        }
        if (const CollectionArchiveLocalCMwIdEntry *entry =
                    cmwIdTable.FindOneBased(identifier.payload)) {
            if (out != nullptr && outSize != 0u) {
                return CopyStringToSurfacePairBuffer(out,
                                                     outSize,
                                                     entry->text);
            }
            return 1;
        }
        return 1;
    }
    if (identifier.payload == 0u) {
        return ReadStringText(out, outSize);
    }
    return 1;
}

}  // namespace

void CollectionReplacementPairs::Clear() {
    pairs.clear();
    overflowCount = 0u;
}

int CollectionReplacementPairs::Append(
        const char *sourceSurfaceIdName,
        const char *targetSurfaceIdName) {
    if (sourceSurfaceIdName == nullptr || targetSurfaceIdName == nullptr ||
        sourceSurfaceIdName[0] == '\0' ||
        targetSurfaceIdName[0] == '\0') {
        return 1;
    }
    if (pairs.size() >= MaxStoredPairs) {
        overflowCount++;
        return 1;
    }
    CollectionReplacementPair pair{};
    if (!(TmnfFormat::FixedCString::Copy((pair.sourceSurfaceIdName), (sizeof(pair.sourceSurfaceIdName)), (sourceSurfaceIdName))) ||
        !(TmnfFormat::FixedCString::Copy((pair.targetSurfaceIdName), (sizeof(pair.targetSurfaceIdName)), (targetSurfaceIdName)))) {
        return 0;
    }
    try {
        pairs.push_back(pair);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

int CollectionReplacementPairs::Contains(
        const char *sourceSurfaceIdName,
        const char *targetSurfaceIdName) const {
    if (sourceSurfaceIdName == nullptr || targetSurfaceIdName == nullptr) {
        return 0;
    }
    for (const CollectionReplacementPair &pair : pairs) {
        if ((TmnfFormat::FixedCString::Equals((pair.sourceSurfaceIdName), (sourceSurfaceIdName))) &&
            (TmnfFormat::FixedCString::Equals((pair.targetSurfaceIdName), (targetSurfaceIdName)))) {
            return 1;
        }
    }
    return 0;
}

const std::vector<CollectionReplacementPair> &
CollectionReplacementPairs::Pairs() const {
    return pairs;
}

u32 CollectionReplacementPairs::OverflowCount() const {
    return overflowCount;
}

int CollectionReplacementPairs::ParseSurfaceReplacementPairsChunk(
        const unsigned char *bytes,
        u32 byteCount,
        const BlockInfoDescriptorExternalRefs &refs) {
    const u32 bodyOffset = refs.BodyOffsetForFormatParser();
    if (bytes == nullptr || bodyOffset > byteCount) {
        return 0;
    }
    Clear();
    if (byteCount < 4u || bodyOffset > byteCount - 4u) {
        return 1;
    }
    u32 chunkOffset = UINT32_MAX;
    for (u32 offset = bodyOffset; offset <= byteCount - 4u; offset++) {
        u32 word = TmnfFormat::ArchiveBinary::ReadU32LE(bytes + offset);
        if (word == ArchiveChunkIdValue(
                            CGameCtnCollectionArchiveChunkId::
                                    SurfaceReplacementPairs)) {
            chunkOffset = offset;
            break;
        }
    }
    if (chunkOffset == UINT32_MAX) {
        return 1;
    }

    CollectionReplacementPairsArchiveStream stream{};
    stream.bytes = bytes;
    stream.byteCount = byteCount;
    stream.offset = chunkOffset + 4u;

    u32 count = 0u;
    if (!stream.ReadU32(&count) ||
        count > 0x100000u) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        char source[64];
        char target[64];
        if (!stream.ReadCMwIdText(source, sizeof(source)) ||
            !stream.ReadCMwIdText(target, sizeof(target))) {
            return 0;
        }
        if (source[0] == '\0' || target[0] == '\0') {
            continue;
        }
        if (!Append(source, target)) {
            return 0;
        }
    }
    return 1;
}

int CollectionReplacementPairs::LoadCatalogCollection(
        const CPlugFilePack *installedPack,
        const char *collectionName) {
    Clear();
    if (installedPack == nullptr || collectionName == nullptr ||
        collectionName[0] == '\0') {
        return 1;
    }
    CatalogCollectionArchive collectionArchive;
    return collectionArchive.Load(installedPack, collectionName) &&
           ParseSurfaceReplacementPairsChunk(
                   collectionArchive.Bytes(),
                   collectionArchive.ByteCount(),
                   collectionArchive.ExternalRefs());
}
