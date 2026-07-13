#include "format/pack/installed/plug_file_pack.h"
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <openssl/evp.h>
#include <zlib.h>

#include "format/archive/classic_buffer_crypted.h"
#include "format/archive/archive_binary.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include "format/pack/installed/plug_file_pack_crypto_constants.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_payload_decoder.h"
#include <new>

static size_t pack_cstring_length(const char *text) {
    size_t length = 0u;
    while (text[length] != 0) {
        length++;
    }
    return length;
}

static int pack_strings_equal(const char *lhs, const char *rhs) {
    if (lhs == nullptr || rhs == nullptr) {
        return lhs == rhs;
    }
    size_t index = 0u;
    for (;;) {
        if (lhs[index] != rhs[index]) {
            return 0;
        }
        if (lhs[index] == 0) {
            return 1;
        }
        index++;
    }
}

static void copy_pack_bytes(unsigned char *dst,
                            const unsigned char *src,
                            size_t count) {
    for (size_t i = 0u; i < count; i++) {
        dst[i] = src[i];
    }
}

static void copy_pack_chars(char *dst, const char *src, size_t count) {
    for (size_t i = 0u; i < count; i++) {
        dst[i] = src[i];
    }
}

static int copy_cstr_to_fixed(char *dst, size_t dstSize, const char *src) {
    if (dst == nullptr || dstSize == 0u || src == nullptr) {
        return 0;
    }
    const size_t len = pack_cstring_length(src);
    if (len >= dstSize) {
        return 0;
    }
    copy_pack_chars(dst, src, len + 1u);
    return 1;
}

int CPlugFileFidContainer_SFileDesc::IsCompressed() const {
    return (flags & CompressionFlagsMask) != 0u;
}

int CPlugFileFidContainer_SFileDesc::IsPublicFile() const {
    return (flags & PublicFileFlag) != 0u;
}

int CPlugFileFidContainer_SFileDesc::ForceNoCrypt() const {
    return (flags & ForceNoCryptFlag) != 0u;
}

int CPlugFileFidContainer_SFileDesc::IsEncryptedPayload() const {
    return !(IsPublicFile() || ForceNoCrypt());
}

u32 CPlugFileFidContainer_SFileDesc::PackedPayloadByteCount() const {
    return IsCompressed() ? compressedSize : uncompressedSize;
}

int CPlugFileFidContainer_SFileDesc::IsDefaultVehicleTuningsPath(
        const char *selectedPath) const {
    return selectedPath != nullptr &&
           pack_strings_equal(selectedPath, TmnfDefaultVehicleTuningsPackPath);
}

int CPlugFileFidContainer_SFileDesc::ShouldTryStreamFeedbackExtraction(
        const char *selectedPath) const {
    const int isDefaultVehicleTunings =
            IsDefaultVehicleTuningsPath(selectedPath);
    const int streamFeedbackDescriptor =
            selectedPath != nullptr &&
            (SceneDescriptorFolderPaths::IsConstructionBlockInfoPath(
                     selectedPath) ||
             SceneDescriptorFolderPaths::IsStadiumMobilPath(selectedPath) ||
             SceneDescriptorFolderPaths::IsRacesMobilPath(selectedPath) ||
             SceneDescriptorFolderPaths::IsStadiumMediaSolidPath(
                     selectedPath) ||
             isDefaultVehicleTunings);
    return (IsEncryptedPayload() || isDefaultVehicleTunings) &&
           streamFeedbackDescriptor;
}

struct CPlugFilePackLoadHeadersReader {
    std::unique_ptr<CClassicBufferCrypted> crypted;

    int Read(void *out, size_t byteCount);
    int ReadU32(u32 *out);
    int ReadU64(uint64_t *out);
    int ReadString(std::string *out);
};

static int Md5Bytes(const unsigned char *data,
                           size_t byteCount,
                           unsigned char digest[16]) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned int digestSize = 0u;
    if (ctx == nullptr) {
        return 0;
    }
    const int ok =
            EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) == 1 &&
            EVP_DigestUpdate(ctx, data, byteCount) == 1 &&
            EVP_DigestFinal_ex(ctx, digest, &digestSize) == 1 &&
            digestSize == 16u;
    EVP_MD_CTX_free(ctx);
    return ok;
}

static int Md5CString(const char *text, unsigned char digest[16]) {
    return text != nullptr &&
           Md5Bytes((const unsigned char *)text, pack_cstring_length(text), digest);
}

static int derive_pack_key_from_packlist_string(const char *packKey,
                                                SNat128 &key) {
    char seed[128];
    unsigned char digest[16];
    if (packKey == nullptr ||
        snprintf(seed, sizeof(seed), "%sNadeoPak", packKey) >=
                (int)sizeof(seed) ||
        !Md5CString(seed, digest)) {
        return 0;
    }
    for (u32 wordIndex = 0u; wordIndex < key.words.size(); ++wordIndex) {
        const u32 offset = wordIndex * 4u;
        key.words[wordIndex] =
                (static_cast<u32>(digest[offset]) << 24u) |
                (static_cast<u32>(digest[offset + 1u]) << 16u) |
                (static_cast<u32>(digest[offset + 2u]) << 8u) |
                static_cast<u32>(digest[offset + 3u]);
    }
    return 1;
}

static int ReadPacklistStadiumKey(
        const unsigned char *data,
        size_t size,
        char keyOut[33]) {
    if (data == nullptr || size < 10u) {
        return 0;
    }
    const u32 count = data[1];
    const u32 seedNatural = TmnfFormat::ArchiveBinary::ReadU32LE(data + 6u);
    char seedText[32];
    char nameSeed[128];
    unsigned char nameDigest[16];
    snprintf(seedText, sizeof(seedText), "%u", seedNatural);
    if (snprintf(nameSeed, sizeof(nameSeed), "%s%s",
                 TmnfPackListNameSalt, seedText) >= (int)sizeof(nameSeed) ||
        !Md5CString(nameSeed, nameDigest)) {
        return 0;
    }
    size_t offset = 10u;
    for (u32 index = 0; index < count; index++) {
        if (offset + 2u > size) {
            return 0;
        }
        const unsigned char flags = data[offset];
        const unsigned char nameLen = data[offset + 1u];
        if (nameLen > 0x1fu || offset + 2u + nameLen + 0x20u > size) {
            return 0;
        }
        char name[64];
        for (u32 i = 0; i < nameLen; i++) {
            name[i] = (char)(data[offset + 2u + i] ^ nameDigest[i & 0x0fu]);
        }
        name[nameLen] = '\0';
        const size_t keyOffset = offset + 2u + nameLen;
        char keySeed[160];
        if ((flags & 1u) != 0u) {
            if (snprintf(keySeed, sizeof(keySeed), "%s%s%s",
                         name, seedText, TmnfPackKeySaltOdd) >=
                    (int)sizeof(keySeed)) {
                return 0;
            }
        } else if (snprintf(keySeed, sizeof(keySeed), "%s%s%s",
                            name, seedText, TmnfPackKeySaltEven) >=
                   (int)sizeof(keySeed)) {
            return 0;
        }
        unsigned char keyDigest[16];
        if (!Md5CString(keySeed, keyDigest)) {
            return 0;
        }
        char decodedKey[33];
        for (u32 i = 0; i < 0x20u; i++) {
            decodedKey[i] = (char)(data[keyOffset + i] ^ keyDigest[i & 0x0fu]);
        }
        decodedKey[32] = '\0';
        if (pack_strings_equal(name, "stadium")) {
            copy_pack_chars(keyOut, decodedKey, 33u);
            return 1;
        }
        offset = keyOffset + 0x20u;
    }
    return 0;
}

static int inflate_static_solid_part(const unsigned char *input,
                                     u32 inputByteCount,
                                     unsigned char *output,
                                     u32 outputByteCount) {
    z_stream stream{};
    stream.next_in = (Bytef *)input;
    stream.avail_in = inputByteCount;
    stream.next_out = output;
    stream.avail_out = outputByteCount;
    if (inflateInit(&stream) != Z_OK) {
        return 0;
    }
    const int status = inflate(&stream, Z_NO_FLUSH);
    const int ok = stream.total_out == outputByteCount &&
                   (status == Z_OK ||
                    status == Z_STREAM_END ||
                    stream.avail_out == 0u);
    inflateEnd(&stream);
    return ok;
}

CPlugFilePack::CPlugFilePack() {
    key = {};
    dataStart = 0u;
}

CPlugFilePack::~CPlugFilePack() {
    FreeLoadedPack();
}

int CPlugFilePack::IsCrypted(void) const {
    return !key.IsNull();
}

int CPlugFilePackLoadHeadersReader::Read(void *out, size_t requestedByteCount) {
    return crypted != nullptr &&
           requestedByteCount <= UINT32_MAX &&
           crypted->Read(out,
                         static_cast<unsigned long>(requestedByteCount)) ==
                   requestedByteCount;
}

int CPlugFilePackLoadHeadersReader::ReadU32(u32 *out) {
    unsigned char wordBytes[4];
    if (out == nullptr || !Read(wordBytes, sizeof(wordBytes))) {
        return 0;
    }
    *out = TmnfFormat::ArchiveBinary::ReadU32LE(wordBytes);
    return 1;
}

int CPlugFilePackLoadHeadersReader::ReadU64(uint64_t *out) {
    unsigned char qwordBytes[8];
    if (out == nullptr || !Read(qwordBytes, sizeof(qwordBytes))) {
        return 0;
    }
    *out = TmnfFormat::ArchiveBinary::ReadU64LE(qwordBytes);
    return 1;
}

int CPlugFilePackLoadHeadersReader::ReadString(std::string *out) {
    u32 byteCount = 0u;
    if (out == nullptr ||
        !ReadU32(&byteCount) ||
        byteCount > 0x0fffffffu) {
        return 0;
    }
    try {
        out->assign(byteCount, '\0');
    } catch (const std::bad_alloc &) {
        return 0;
    }
    if (byteCount != 0u &&
        !Read(&(*out)[0], byteCount)) {
        return 0;
    }
    return 1;
}

void CPlugFilePack::FreeLoadedPack() {
    folders.clear();
    files.clear();
    bytes.Clear();
    key = {};
    dataStart = 0u;
}

int CPlugFilePack::LoadHeadersFromMemory() {
    if (bytes.Size() < 20u || bytes.Size() > UINT32_MAX) {
        return 0;
    }
    CPlugFilePackLoadHeadersReader reader;
    reader.crypted = CClassicBufferCrypted::CreateBlowfishReaderForMemory(
            bytes.Data(),
            static_cast<unsigned long>(bytes.Size()),
            20ul,
            key);
    if (reader.crypted == nullptr) {
        return 0;
    }
    unsigned char discard16[16];
    u32 discard = 0u;
    u32 loadedFolderCount = 0u;
    if (!reader.Read(discard16, sizeof(discard16)) ||
        !reader.ReadU32(&discard) ||
        !reader.ReadU32(&dataStart) ||
        !reader.ReadU32(&discard) ||
        !reader.ReadU32(&discard) ||
        !reader.Read(discard16, sizeof(discard16)) ||
        !reader.ReadU32(&discard) ||
        !reader.ReadU32(&loadedFolderCount) ||
        loadedFolderCount > 4096u) {
        return 0;
    }
    try {
        folders.clear();
        folders.resize(loadedFolderCount);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    for (u32 i = 0u; i < loadedFolderCount; i++) {
        if (!reader.ReadU32(&folders[i].parent) ||
            !reader.ReadString(&folders[i].name)) {
            return 0;
        }
    }
    if (loadedFolderCount > 2u && folders[2].name.size() > 4u) {
        unsigned char utf16[8] = {0};
        for (u32 i = 0u; i < 4u; i++) {
            utf16[i * 2u] = (unsigned char)folders[2].name[i + 2u];
        }
        reader.crypted->MixPageFeedback(utf16, 4ul);
    }
    u32 loadedFileCount = 0u;
    if (!reader.ReadU32(&loadedFileCount) ||
        loadedFileCount > 200000u) {
        return 0;
    }
    try {
        files.clear();
        files.resize(loadedFileCount);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    for (u32 i = 0u; i < loadedFileCount; i++) {
        if (!reader.ReadU32(&files[i].folderIndex) ||
            !reader.ReadString(&files[i].name) ||
            !reader.ReadU32(&discard) ||
            !reader.ReadU32(&files[i].uncompressedSize) ||
            !reader.ReadU32(&files[i].compressedSize) ||
            !reader.ReadU32(&files[i].offsetRel) ||
            !reader.ReadU32(&files[i].classId) ||
            !reader.ReadU64(&files[i].flags)) {
            return 0;
        }
    }
    return 1;
}

int CPlugFilePack::OpenFromMemory(
        const void *pakBytes,
        size_t pakByteCount,
        const void *packlistBytes,
        size_t packlistByteCount) {
    if ((pakBytes == nullptr && pakByteCount != 0u) ||
        (packlistBytes == nullptr && packlistByteCount != 0u)) {
        return 0;
    }
    FreeLoadedPack();
    char packlistKey[33]{};
    if (!bytes.Append(pakBytes, pakByteCount) ||
        bytes.Size() < 20u ||
        !ReadPacklistStadiumKey(
                static_cast<const unsigned char *>(packlistBytes),
                packlistByteCount,
                packlistKey) ||
        !derive_pack_key_from_packlist_string(packlistKey, key) ||
        !LoadHeadersFromMemory()) {
        FreeLoadedPack();
        return 0;
    }
    return 1;
}

int CPlugFilePack::FolderPathRecursive(
        u32 folderIndex,
        char *out,
        size_t outSize) const {
    if (out == nullptr || outSize == 0u ||
        (size_t)folderIndex >= folders.size()) {
        return 0;
    }
    const CPlugFilePackFolderDesc *folder = &folders[folderIndex];
    if (folder->parent != 0xffffffffu &&
        !FolderPathRecursive(folder->parent, out, outSize)) {
        return 0;
    }
    return SceneDescriptorFolderPaths::AppendCString(
            out, outSize, folder->name.c_str());
}

int CPlugFilePack::FileDescPlainPath(
        const CPlugFileFidContainer_SFileDesc *file,
        char *out,
        size_t outSize) const {
    if (file == nullptr || out == nullptr || outSize == 0u) {
        return 0;
    }
    out[0] = '\0';
    return FolderPathRecursive(file->folderIndex, out, outSize) &&
           SceneDescriptorFolderPaths::AppendCString(
                   out, outSize, file->name.c_str());
}

int CPlugFilePack::FileDescPathMatches(
        const CPlugFileFidContainer_SFileDesc *file,
        const char *selectedPath) const {
    char path[512];
    if (file == nullptr || selectedPath == nullptr) {
        return 0;
    }
    return FileDescPlainPath(file, path, sizeof(path)) &&
           pack_strings_equal(path, selectedPath);
}

const CPlugFileFidContainer_SFileDesc *CPlugFilePack::FindFileDescByPath(
        const char *selectedPath) const {
    if (selectedPath == nullptr || selectedPath[0] == '\0') {
        return nullptr;
    }
    for (const CPlugFileFidContainer_SFileDesc &file : files) {
        if (FileDescPathMatches(&file, selectedPath)) {
            return &file;
        }
    }
    return nullptr;
}

int CPlugFilePack::SelectedPathForPlainRef(
        const char *plainPath,
        char *out,
        size_t outSize) const {
    if (plainPath == nullptr || plainPath[0] == '\0' ||
        out == nullptr || outSize == 0u) {
        return 0;
    }
    out[0] = '\0';
    if (FindFileDescByPath(plainPath) != nullptr) {
        return copy_cstr_to_fixed(out, outSize, plainPath);
    }
    char hashedPath[512];
    hashedPath[0] = '\0';
    if (!SceneDescriptorFolderPaths::HashPackSelectedPath(
                plainPath,
                hashedPath,
                sizeof(hashedPath)) ||
        FindFileDescByPath(hashedPath) == nullptr) {
        return 0;
    }
    return copy_cstr_to_fixed(out, outSize, hashedPath);
}

int CPlugFilePack::PayloadOffset(
        const CPlugFileFidContainer_SFileDesc &file,
        size_t *payloadOffsetOut) const {
    if (payloadOffsetOut == nullptr ||
        dataStart > UINT32_MAX - file.offsetRel) {
        return 0;
    }
    const size_t payloadOffset = (size_t)dataStart + file.offsetRel;
    if (payloadOffset > bytes.Size()) {
        return 0;
    }
    *payloadOffsetOut = payloadOffset;
    return 1;
}

int CPlugFilePack::ReadPackedPayload(
        const CPlugFileFidContainer_SFileDesc &file,
        size_t payloadOffset,
        u32 payloadByteCount,
        unsigned char *payloadOut) const {
    if ((payloadOut == nullptr && payloadByteCount != 0u) ||
        payloadOffset > bytes.Size() ||
        payloadByteCount > bytes.Size() - payloadOffset) {
        return 0;
    }
    if (file.IsEncryptedPayload()) {
        if (payloadByteCount > UINT32_MAX - 7u) {
            return 0;
        }
        const size_t encryptedByteCount =
                (static_cast<size_t>(payloadByteCount) + 7u) & ~size_t{7u};
        if (encryptedByteCount > UINT32_MAX - 8u ||
            8u + encryptedByteCount > bytes.Size() - payloadOffset) {
            return 0;
        }
        auto reader = CClassicBufferCrypted::CreateBlowfishReaderForMemory(
                bytes.Data() + payloadOffset,
                static_cast<unsigned long>(8u + encryptedByteCount),
                8ul,
                key);
        return reader != nullptr &&
               reader->Read(payloadOut, payloadByteCount) ==
                       payloadByteCount;
    }
    if (payloadByteCount != 0u) {
        copy_pack_bytes(payloadOut, bytes.Data() + payloadOffset, payloadByteCount);
    }
    return 1;
}

int CPlugFilePack::ExtractPathWithPackedPayload(
        const CPlugFileFidContainer_SFileDesc &file,
        ByteBuffer *out) const {
    if (out == nullptr) {
        return 0;
    }
    const u32 payloadByteCount = file.PackedPayloadByteCount();
    size_t payloadOffset = 0u;
    if (!PayloadOffset(file, &payloadOffset) ||
        payloadByteCount > bytes.Size() - payloadOffset) {
        return 0;
    }
    std::vector<unsigned char> payload(payloadByteCount);
    if (!ReadPackedPayload(file,
                           payloadOffset,
                           payloadByteCount,
                           payloadByteCount != 0u ? payload.data() : nullptr)) {
        return 0;
    }
    out->Clear();
    if (file.IsCompressed()) {
        if (!out->Resize(file.uncompressedSize)) {
            return 0;
        }
        if (!inflate_static_solid_part(payloadByteCount != 0u
                                               ? payload.data()
                                               : nullptr,
                                       payloadByteCount,
                                       out->Data(),
                                       file.uncompressedSize)) {
            out->Clear();
            return 0;
        }
    } else {
        if (!out->Resize(payloadByteCount)) {
            return 0;
        }
        if (payloadByteCount != 0u) {
            copy_pack_bytes(out->Data(), payload.data(), payloadByteCount);
        }
    }
    return 1;
}

int CPlugFilePack::ExtractPathWithStreamFeedback(
        const CPlugFileFidContainer_SFileDesc *file,
        const char *selectedPath,
        ByteBuffer *out) const {
    if (file == nullptr || out == nullptr) {
        return 0;
    }
    StaticSolidArchivePayload payload;
    if (!payload.BuildEncryptedPackFilePayload({
                *file,
                dataStart,
                0u,
                0u,
                "",
                selectedPath != nullptr ? selectedPath : ""}) ||
        static_cast<size_t>(payload.PackPayloadOffset()) > bytes.Size() ||
        payload.RawByteCount() >
                bytes.Size() -
                        static_cast<size_t>(payload.PackPayloadOffset())) {
        return 0;
    }

    CGameCtnReplayStaticSolidDecodedPayload decoded;
    CGameCtnReplayStaticSolidArchiveDecodeStats stats;
    if (!CGameCtnReplayStaticSolidArchivePayloadReader::DecodeWithStreamFeedback(
                key,
                payload,
                StaticSolidArchiveRawBytes::FromBytes(
                        bytes.Data() + payload.PackPayloadOffset(),
                        payload.RawByteCount()),
                nullptr,
                nullptr,
                StaticSolidArchiveId::Invalid(),
                &decoded,
                &stats) ||
        !decoded.IsReady() || decoded.ByteCount() != file->uncompressedSize) {
        return 0;
    }
    return decoded.CopyToByteBuffer(out);
}

int CPlugFilePack::ExtractPath(
        const char *selectedPath,
        ByteBuffer *out) const {
    if (selectedPath == nullptr || out == nullptr) {
        return 0;
    }
    out->Clear();
    const CPlugFileFidContainer_SFileDesc *file =
            FindFileDescByPath(selectedPath);
    if (file == nullptr) {
        return 0;
    }
    if (file->ShouldTryStreamFeedbackExtraction(selectedPath)) {
        if (ExtractPathWithStreamFeedback(file, selectedPath, out)) {
            return 1;
        }
        out->Clear();
    }
    return ExtractPathWithPackedPayload(*file, out);
}
