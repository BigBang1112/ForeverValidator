#ifndef TMNF_GBX_BODY_READER_H
#define TMNF_GBX_BODY_READER_H

#include <string>
#include <string_view>
#include <vector>

#include "engine/core/engine_types.h"

struct GbxBodyReferenceFolder {
    u32 parentFolderIndex = 0u;
    std::string name;
};

struct GbxBodyExternalReference {
    u32 flags = 0u;
    u32 nodeIndex = 0u;
    u32 folderIndex = 0xffffffffu;
    u32 resourceIndex = 0xffffffffu;
    bool useFile = false;
    std::string name;

    bool IsResource(void) const;
};

struct GbxBodyReferenceTable {
    u32 nodeCount = 0u;
    u32 bodyOffset = 0u;
    u32 ancestorLevel = 0u;
    std::vector<u32> externalNodeIndices;
    std::vector<GbxBodyReferenceFolder> folders;
    std::vector<GbxBodyExternalReference> externalReferences;

    int IsExternalNode(u32 nodeIndex) const;
    int PlainPathForReference(
            const GbxBodyExternalReference &reference,
            std::string *out) const;
    int ResolvePlainPathForReference(
            std::string_view sourceLogicalPath,
            const GbxBodyExternalReference &reference,
            std::string *out) const;
    int HasDeclaredPath(std::string_view path) const;
};

struct GbxBodyOffsetReader {
    const unsigned char *bytes;
    u32 byteCount;

    int SkipString(u32 *offset) const;
    int SkipSubfolders(u32 *offset, u32 depth) const;
    int ReadString(u32 *offset, std::string *out) const;
    int ParseSubfolders(
            u32 *offset,
            u32 depth,
            u32 parentFolderIndex,
            std::vector<GbxBodyReferenceFolder> *folders) const;
    static int TryParse(
            const unsigned char *bytes,
            u32 byteCount,
            u32 *classIdOut,
            u32 *bodyOffsetOut);
    static int TryParseWithReferences(
            const unsigned char *bytes,
            u32 byteCount,
            u32 *classIdOut,
            GbxBodyReferenceTable *referenceTableOut);
};

#endif
