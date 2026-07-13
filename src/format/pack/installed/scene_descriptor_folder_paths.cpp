#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include <ctype.h>
#include <openssl/evp.h>
#include <stddef.h>
#include <stdint.h>
#include <new>

static size_t scene_descriptor_cstr_length(const char *text) {
    size_t length = 0u;
    while (text[length] != '\0') {
        length++;
    }
    return length;
}

static int scene_descriptor_prefix_matches(const char *text,
                                           const char *prefix,
                                           size_t prefixLen) {
    for (size_t i = 0u; i < prefixLen; i++) {
        if (text[i] != prefix[i]) {
            return 0;
        }
    }
    return 1;
}

static char scene_descriptor_low_high_hex_nibble(unsigned int value) {
    value &= 0x0fu;
    return (char)(value < 10u ? ('0' + value) : ('A' + value - 10u));
}

const char *SceneDescriptorFolderPaths::ConstructionBlockInfoPrefix() {
    return "Stadium\\ConstructionBlockInfo\\";
}

const char *SceneDescriptorFolderPaths::StadiumMobilPrefix() {
    return "Stadium\\Mobil\\";
}

const char *SceneDescriptorFolderPaths::RacesMobilPrefix() {
    return "Races\\Mobil\\";
}

const char *SceneDescriptorFolderPaths::StadiumMediaSolidPrefix() {
    return "Stadium\\Media\\Solid\\";
}

const char *SceneDescriptorFolderPaths::StadiumMediaMaterialPrefix() {
    return "Stadium\\Media\\Material\\";
}

int SceneDescriptorFolderPaths::StartsWith(const char *text,
                                           const char *prefix) {
    if (text == nullptr || prefix == nullptr) {
        return 0;
    }
    const size_t prefixLen = scene_descriptor_cstr_length(prefix);
    return scene_descriptor_prefix_matches(text, prefix, prefixLen);
}

int SceneDescriptorFolderPaths::IsConstructionBlockInfoPath(
        const char *plainPath) {
    return StartsWith(plainPath, ConstructionBlockInfoPrefix());
}

int SceneDescriptorFolderPaths::IsStadiumMobilPath(const char *plainPath) {
    return StartsWith(plainPath, StadiumMobilPrefix());
}

int SceneDescriptorFolderPaths::IsRacesMobilPath(const char *plainPath) {
    return StartsWith(plainPath, RacesMobilPrefix());
}

int SceneDescriptorFolderPaths::IsMobilDescriptorPath(
        const char *plainPath) {
    return IsStadiumMobilPath(plainPath) || IsRacesMobilPath(plainPath);
}

int SceneDescriptorFolderPaths::IsStadiumMediaSolidPath(
        const char *plainPath) {
    return StartsWith(plainPath, StadiumMediaSolidPrefix());
}

int SceneDescriptorFolderPaths::IsStadiumMediaMaterialPath(
        const char *plainPath) {
    return StartsWith(plainPath, StadiumMediaMaterialPrefix());
}

int SceneDescriptorFolderPaths::AppendCString(char *dst,
                                              size_t dstSize,
                                              const char *src) {
    if (dst == nullptr || dstSize == 0u || src == nullptr) {
        return 0;
    }
    const size_t dstLen = scene_descriptor_cstr_length(dst);
    const size_t srcLen = scene_descriptor_cstr_length(src);
    if (dstLen + srcLen >= dstSize) {
        return 0;
    }
    for (size_t i = 0u; i <= srcLen; i++) {
        dst[dstLen + i] = src[i];
    }
    return 1;
}

int SceneDescriptorFolderPaths::HashFileNameDescriptorPathWithBase(
        const char *plainPath,
        const char *basePath,
        char *out,
        size_t outSize) {
    if (plainPath == nullptr || basePath == nullptr || out == nullptr ||
        outSize == 0u || !StartsWith(plainPath, basePath)) {
        return 0;
    }
    const char *relative = plainPath + scene_descriptor_cstr_length(basePath);
    const size_t relativeLen = scene_descriptor_cstr_length(relative);
    char lower[160];
    if (relativeLen >= sizeof(lower)) {
        return 0;
    }
    for (size_t i = 0u; i < relativeLen; i++) {
        lower[i] = (char)tolower((unsigned char)relative[i]);
    }
    lower[relativeLen] = '\0';

    unsigned char digest[16];
    unsigned int digestSize = 0u;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        return 0;
    }
    const int digestOk =
            EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) == 1 &&
            EVP_DigestUpdate(ctx, lower, relativeLen) == 1 &&
            EVP_DigestFinal_ex(ctx, digest, &digestSize) == 1 &&
            digestSize == 16u;
    EVP_MD_CTX_free(ctx);
    if (!digestOk) {
        return 0;
    }

    out[0] = '\0';
    if (!AppendCString(out, outSize, basePath)) {
        return 0;
    }
    const size_t prefixLen = scene_descriptor_cstr_length(out);
    if (prefixLen + 34u >= outSize) {
        return 0;
    }
    char *cursor = out + prefixLen;
    const unsigned int lenByte = (unsigned int)(relativeLen & 0xffu);
    *cursor++ = scene_descriptor_low_high_hex_nibble(lenByte);
    *cursor++ = scene_descriptor_low_high_hex_nibble(lenByte >> 4u);
    for (u32 i = 0u; i < 16u; i++) {
        *cursor++ = scene_descriptor_low_high_hex_nibble(digest[i]);
        *cursor++ = scene_descriptor_low_high_hex_nibble(digest[i] >> 4u);
    }
    *cursor = '\0';
    return 1;
}

int SceneDescriptorFolderPaths::HashStadiumMediaSolidPath(
        const char *plainPath,
        char *out,
        size_t outSize) {
    return HashFileNameDescriptorPathWithBase(
            plainPath,
            StadiumMediaSolidPrefix(),
            out,
            outSize);
}

int SceneDescriptorFolderPaths::HashStadiumMediaMaterialPath(
        const char *plainPath,
        char *out,
        size_t outSize) {
    return HashFileNameDescriptorPathWithBase(
            plainPath,
            StadiumMediaMaterialPrefix(),
            out,
            outSize);
}

int SceneDescriptorFolderPaths::HashMobilDescriptorPath(
        const char *plainPath,
        char *out,
        size_t outSize) {
    const char *base = nullptr;
    return FindMobilDescriptorHashBase(plainPath, &base) &&
           HashFileNameDescriptorPathWithBase(plainPath, base, out, outSize);
}

int SceneDescriptorFolderPaths::HashPackSelectedPath(
        const char *plainPath,
        char *out,
        size_t outSize) {
    const char *base = nullptr;
    return FindPackSelectedPathHashBase(plainPath, &base) &&
           HashFileNameDescriptorPathWithBase(plainPath, base, out, outSize);
}

int SceneDescriptorFolderPaths::FindMobilDescriptorHashBase(
        const char *plainPath,
        const char **baseOut) {
    if (baseOut == nullptr) {
        return 0;
    }
    if (IsStadiumMobilPath(plainPath)) {
        *baseOut = StadiumMobilPrefix();
        return 1;
    }
    if (IsRacesMobilPath(plainPath)) {
        *baseOut = RacesMobilPrefix();
        return 1;
    }
    return 0;
}

int SceneDescriptorFolderPaths::FindPackSelectedPathHashBase(
        const char *plainPath,
        const char **baseOut) {
    if (baseOut == nullptr) {
        return 0;
    }
    if (IsStadiumMediaSolidPath(plainPath)) {
        *baseOut = StadiumMediaSolidPrefix();
        return 1;
    }
    if (StartsWith(plainPath, "Stadium\\")) {
        *baseOut = "Stadium\\";
        return 1;
    }
    return 0;
}

char *SceneDescriptorFolderStack::MutableSegment(u32 depth) {
    if (depth >= MaxDepth) {
        return nullptr;
    }
    return segments[depth].data();
}

void SceneDescriptorFolderStack::ClearBelow(u32 depth) {
    if (depth >= MaxDepth) {
        return;
    }
    for (u32 i = depth + 1u; i < MaxDepth; i++) {
        segments[i][0] = '\0';
    }
}

int SceneDescriptorFolderStack::BuildPathToDepth(
        u32 depth,
        char *out,
        size_t outSize) const {
    if (depth >= MaxDepth || out == nullptr || outSize == 0u) {
        return 0;
    }
    out[0] = '\0';
    for (u32 i = 0; i <= depth; i++) {
        const char *segment = segments[i].data();
        if (segment[0] == '\0' ||
            !SceneDescriptorFolderPaths::AppendCString(
                    out, outSize, segment) ||
            !SceneDescriptorFolderPaths::AppendCString(out, outSize, "\\")) {
            return 0;
        }
    }
    return 1;
}

int SceneDescriptorFolderPaths::FindFolderForRef(
        u32 folderIndex,
        const char **folderOut) const {
    if (folderOut == nullptr) {
        return 0;
    }
    if (folderIndex >= 1u &&
        (size_t)(folderIndex - 1u) < paths.size()) {
        *folderOut = paths[folderIndex - 1u].path;
        return 1;
    }
    if ((size_t)folderIndex < paths.size()) {
        *folderOut = paths[folderIndex].path;
        return 1;
    }
    return 0;
}

int SceneDescriptorFolderPaths::AppendFolderPath(
        const SceneDescriptorFolderStack &stack,
        u32 depth) {
    if (paths.size() >= UINT32_MAX) {
        return 0;
    }
    SceneDescriptorFolderPath row;
    if (!stack.BuildPathToDepth(depth, row.path, sizeof(row.path))) {
        return 0;
    }
    try {
        paths.push_back(row);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

int SceneDescriptorFolderPaths::BuildPackRefFullPath(
        const char *folder,
        const char *name,
        char *out,
        size_t outSize) {
    if (folder == nullptr || name == nullptr || out == nullptr || outSize == 0u) {
        return 0;
    }
    out[0] = '\0';
    if (StartsWith(folder, "ConstructionBlockInfo\\") ||
        StartsWith(folder, "ConstructionDecoration") ||
        StartsWith(folder, "ConstructionZone\\") ||
        StartsWith(folder, "DecoSolid\\") ||
        StartsWith(folder, "Media\\") ||
        StartsWith(folder, "Mobil\\")) {
        if (!AppendCString(out, outSize, "Stadium\\")) {
            return 0;
        }
    }
    return AppendCString(out, outSize, folder) &&
           AppendCString(out, outSize, name);
}
