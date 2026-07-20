#include "format/archive/tmnf_gbx_body_reader.h"
#include <stdint.h>

#include <new>
#include <string>
#include <utility>

#include "format/archive/archive_binary.h"
using TmnfFormat::ArchiveBinary::ReadU32LE;

bool GbxBodyExternalReference::IsResource(void) const {
    return (flags & 4u) != 0u;
}

int GbxBodyReferenceTable::IsExternalNode(u32 nodeIndex) const {
    for (u32 externalNodeIndex : externalNodeIndices) {
        if (externalNodeIndex == nodeIndex) {
            return 1;
        }
    }
    return 0;
}

namespace {

bool IsReferencePathSegment(std::string_view segment) {
    return !segment.empty() && segment != "." && segment != ".." &&
           segment.find('/') == std::string_view::npos &&
           segment.find('\\') == std::string_view::npos &&
           segment.find('\0') == std::string_view::npos;
}

int SplitReferencePath(
        std::string_view path,
        std::vector<std::string_view> *segments) {
    if (segments == nullptr || path.empty() || path.front() == '\\' ||
        path.back() == '\\') {
        return 0;
    }
    segments->clear();
    std::size_t start = 0u;
    while (start < path.size()) {
        const std::size_t separator = path.find('\\', start);
        const std::size_t end = separator == std::string_view::npos
                ? path.size()
                : separator;
        const std::string_view segment = path.substr(start, end - start);
        if (!IsReferencePathSegment(segment)) {
            segments->clear();
            return 0;
        }
        try {
            segments->push_back(segment);
        } catch (const std::bad_alloc &) {
            segments->clear();
            return 0;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1u;
    }
    return !segments->empty();
}

int AppendFolderPath(
        const std::vector<GbxBodyReferenceFolder> &folders,
        u32 folderIndex,
        u32 depth,
        std::string *out) {
    if (out == nullptr || depth > 64u) {
        return 0;
    }
    if (folderIndex == 0u || folderIndex == 0xffffffffu) {
        return 1;
    }
    if (folderIndex > folders.size()) {
        return 0;
    }
    const GbxBodyReferenceFolder &folder = folders[folderIndex - 1u];
    if (!AppendFolderPath(
                folders, folder.parentFolderIndex, depth + 1u, out)) {
        return 0;
    }
    try {
        if (!out->empty()) {
            out->push_back('\\');
        }
        out->append(folder.name);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    return 1;
}

}  // namespace

int GbxBodyReferenceTable::PlainPathForReference(
        const GbxBodyExternalReference &reference,
        std::string *out) const {
    if (out == nullptr || reference.IsResource() || reference.name.empty()) {
        return 0;
    }
    out->clear();
    if (!AppendFolderPath(folders, reference.folderIndex, 0u, out)) {
        out->clear();
        return 0;
    }
    try {
        if (!out->empty()) {
            out->push_back('\\');
        }
        out->append(reference.name);
    } catch (const std::bad_alloc &) {
        out->clear();
        return 0;
    }
    return 1;
}

int GbxBodyReferenceTable::ResolvePlainPathForReference(
        std::string_view sourceLogicalPath,
        const GbxBodyExternalReference &reference,
        std::string *out) const {
    if (out == nullptr) {
        return 0;
    }
    out->clear();
    std::string declaredPath;
    std::vector<std::string_view> sourceSegments;
    std::vector<std::string_view> declaredSegments;
    if (!PlainPathForReference(reference, &declaredPath) ||
        !SplitReferencePath(sourceLogicalPath, &sourceSegments) ||
        sourceSegments.size() <= ancestorLevel ||
        !SplitReferencePath(declaredPath, &declaredSegments)) {
        return 0;
    }

    sourceSegments.pop_back();
    if (ancestorLevel > sourceSegments.size()) {
        return 0;
    }
    sourceSegments.resize(sourceSegments.size() - ancestorLevel);
    try {
        for (std::string_view segment : sourceSegments) {
            if (!out->empty()) {
                out->push_back('\\');
            }
            out->append(segment.data(), segment.size());
        }
        for (std::string_view segment : declaredSegments) {
            if (!out->empty()) {
                out->push_back('\\');
            }
            out->append(segment.data(), segment.size());
        }
    } catch (const std::bad_alloc &) {
        out->clear();
        return 0;
    }
    return !out->empty();
}

int GbxBodyReferenceTable::HasDeclaredPath(std::string_view path) const {
    if (path.empty()) {
        return 0;
    }
    for (const GbxBodyExternalReference &reference : externalReferences) {
        std::string plainPath;
        if (PlainPathForReference(reference, &plainPath) &&
            plainPath == path) {
            return 1;
        }
    }
    return 0;
}

int GbxBodyOffsetReader::SkipString(u32 *offset) const {
    if (bytes == nullptr || offset == nullptr || *offset > byteCount ||
        byteCount - *offset < 4u) {
        return 0;
    }
    const u32 length = ReadU32LE(bytes + *offset);
    *offset += 4u;
    if (length > 0x0fffffffu || length > byteCount - *offset) {
        return 0;
    }
    *offset += length;
    return 1;
}

int GbxBodyOffsetReader::SkipSubfolders(u32 *offset, u32 depth) const {
    if (bytes == nullptr || offset == nullptr || depth > 64u ||
        *offset > byteCount || byteCount - *offset < 4u) {
        return 0;
    }
    const u32 count = ReadU32LE(bytes + *offset);
    *offset += 4u;
    if (count > 65536u) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        if (!SkipString(offset) ||
            !SkipSubfolders(offset, depth + 1u)) {
            return 0;
        }
    }
    return 1;
}

int GbxBodyOffsetReader::ReadString(
        u32 *offset,
        std::string *out) const {
    if (bytes == nullptr || offset == nullptr || out == nullptr ||
        *offset > byteCount || byteCount - *offset < 4u) {
        return 0;
    }
    const u32 length = ReadU32LE(bytes + *offset);
    *offset += 4u;
    if (length > 0x0fffffffu || length > byteCount - *offset) {
        return 0;
    }
    try {
        out->assign(
                reinterpret_cast<const char *>(bytes + *offset),
                static_cast<std::size_t>(length));
    } catch (const std::bad_alloc &) {
        return 0;
    }
    *offset += length;
    return 1;
}

int GbxBodyOffsetReader::ParseSubfolders(
        u32 *offset,
        u32 depth,
        u32 parentFolderIndex,
        std::vector<GbxBodyReferenceFolder> *folders) const {
    if (bytes == nullptr || offset == nullptr || folders == nullptr ||
        depth > 64u || *offset > byteCount ||
        byteCount - *offset < 4u) {
        return 0;
    }
    const u32 count = ReadU32LE(bytes + *offset);
    *offset += 4u;
    if (count > 65536u || folders->size() > 0x100000u ||
        count > 0x100000u - folders->size()) {
        return 0;
    }
    for (u32 index = 0u; index < count; ++index) {
        GbxBodyReferenceFolder folder;
        folder.parentFolderIndex = parentFolderIndex;
        if (!ReadString(offset, &folder.name) ||
            !IsReferencePathSegment(folder.name)) {
            return 0;
        }
        try {
            folders->push_back(std::move(folder));
        } catch (const std::bad_alloc &) {
            return 0;
        }
        const u32 folderIndex = static_cast<u32>(folders->size());
        if (!ParseSubfolders(
                    offset, depth + 1u, folderIndex, folders)) {
            return 0;
        }
    }
    return 1;
}

int GbxBodyOffsetReader::TryParse(
        const unsigned char *bytes,
        u32 byteCount,
        u32 *classIdOut,
        u32 *bodyOffsetOut) {
    if (bodyOffsetOut == nullptr) {
        return 0;
    }
    GbxBodyReferenceTable references;
    if (!TryParseWithReferences(
                bytes, byteCount, classIdOut, &references)) {
        return 0;
    }
    *bodyOffsetOut = references.bodyOffset;
    return 1;
}

int GbxBodyOffsetReader::TryParseWithReferences(
        const unsigned char *bytes,
        u32 byteCount,
        u32 *classIdOut,
        GbxBodyReferenceTable *referenceTableOut) {
    if (bytes == nullptr || classIdOut == nullptr ||
        referenceTableOut == nullptr ||
        byteCount < 17u ||
        bytes[0] != 'G' || bytes[1] != 'B' || bytes[2] != 'X') {
        return 0;
    }
    const u32 version = (u32)bytes[3] | ((u32)bytes[4] << 8u);
    const u32 classId = ReadU32LE(bytes + 9u);
    const u32 headerSize = ReadU32LE(bytes + 13u);
    if (headerSize > UINT32_MAX - 17u ||
        17u + headerSize > byteCount ||
        byteCount - (17u + headerSize) < 8u) {
        return 0;
    }
    u32 offset = 17u + headerSize;
    const u32 numNodes = ReadU32LE(bytes + offset);
    offset += 4u;
    const u32 externalCount = ReadU32LE(bytes + offset);
    offset += 4u;
    if (numNodes > 0x100000u || externalCount > numNodes) {
        return 0;
    }
    GbxBodyReferenceTable references;
    references.nodeCount = numNodes;
    try {
        references.externalNodeIndices.reserve(externalCount);
        references.externalReferences.reserve(externalCount);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    if (externalCount != 0u) {
        if (byteCount - offset < 4u) {
            return 0;
        }
        references.ancestorLevel = ReadU32LE(bytes + offset);
        offset += 4u;
        GbxBodyOffsetReader reader;
        reader.bytes = bytes;
        reader.byteCount = byteCount;
        if (!reader.ParseSubfolders(
                    &offset, 0u, 0u, &references.folders)) {
            return 0;
        }
        for (u32 i = 0; i < externalCount; i++) {
            if (byteCount - offset < 4u) {
                return 0;
            }
            const u32 flags = ReadU32LE(bytes + offset);
            offset += 4u;
            GbxBodyExternalReference reference;
            reference.flags = flags;
            if ((flags & 4u) != 0u) {
                if (byteCount - offset < 4u) {
                    return 0;
                }
                reference.resourceIndex = ReadU32LE(bytes + offset);
                offset += 4u;
            } else if (!reader.ReadString(&offset, &reference.name) ||
                       !IsReferencePathSegment(reference.name)) {
                return 0;
            }
            if (byteCount - offset < 4u) {
                return 0;
            }
            const u32 nodeIndex = ReadU32LE(bytes + offset);
            offset += 4u;
            if (nodeIndex == 0u || nodeIndex == 0xffffffffu ||
                nodeIndex > numNodes ||
                references.IsExternalNode(nodeIndex)) {
                return 0;
            }
            try {
                references.externalNodeIndices.push_back(nodeIndex);
            } catch (const std::bad_alloc &) {
                return 0;
            }
            int useFile = (flags & 4u) == 0u;
            if (version >= 5u) {
                if (byteCount - offset < 4u) {
                    return 0;
                }
                const u32 archivedUseFile = ReadU32LE(bytes + offset);
                if (archivedUseFile > 1u) {
                    return 0;
                }
                useFile = archivedUseFile != 0u;
                offset += 4u;
            }
            reference.nodeIndex = nodeIndex;
            reference.useFile = useFile != 0;
            if ((flags & 4u) == 0u) {
                if (byteCount - offset < 4u) {
                    return 0;
                }
                reference.folderIndex = ReadU32LE(bytes + offset);
                offset += 4u;
                if (reference.folderIndex != 0xffffffffu &&
                    reference.folderIndex > references.folders.size()) {
                    return 0;
                }
            }
            try {
                references.externalReferences.push_back(
                        std::move(reference));
            } catch (const std::bad_alloc &) {
                return 0;
            }
        }
    }
    references.bodyOffset = offset;
    *classIdOut = classId;
    *referenceTableOut = std::move(references);
    return 1;
}
