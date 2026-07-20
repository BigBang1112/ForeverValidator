#ifndef TMNF_PLUG_FILE_PACK_H
#define TMNF_PLUG_FILE_PACK_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "format/pack/installed/byte_buffer.h"
#include "engine/core/engine_types.h"

class InstalledPackKeyCatalog;

int CommitArchiveFeedbackExtraction(
        ByteBuffer &&decoded,
        int readerError,
        ByteBuffer *out);

struct CPlugFilePackFolderDesc {
    u32 parent = 0xffffffffu;
    std::string name;
};

struct CPlugFileFidContainer_SFileDesc {
    static constexpr std::uint64_t CompressionFlagsMask = UINT64_C(0x3c);
    static constexpr std::uint64_t PublicFileFlag =
            UINT64_C(0x2000000000000);
    static constexpr std::uint64_t ForceNoCryptFlag =
            UINT64_C(0x4000000000000);

    u32 folderIndex = 0u;
    std::string name;
    u32 uncompressedSize = 0u;
    u32 compressedSize = 0u;
    u32 offsetRel = 0u;
    u32 classId = 0u;
    std::uint64_t flags = 0u;

    int IsCompressed() const;
    int IsPublicFile() const;
    int ForceNoCrypt() const;
    int IsEncryptedPayload() const;
    u32 PackedPayloadByteCount() const;
    int IsDefaultVehicleTuningsPath(const char *selectedPath) const;
    int ShouldTryStreamFeedbackExtraction(const char *selectedPath) const;
};

// Installed-pack reader with owned archive bytes and format-local path routing.
struct CPlugFilePack {
    ByteBuffer bytes;
    SNat128 key;
    u32 dataStart = 0u;
    std::vector<CPlugFilePackFolderDesc> folders;
    std::vector<CPlugFileFidContainer_SFileDesc> files;

private:
    int PayloadOffset(
            const CPlugFileFidContainer_SFileDesc &file,
            std::size_t *payloadOffsetOut) const;
    int ReadPackedPayload(
            const CPlugFileFidContainer_SFileDesc &file,
            std::size_t payloadOffset,
            u32 payloadByteCount,
            unsigned char *payloadOut) const;
    int ExtractPathWithPackedPayload(
            const CPlugFileFidContainer_SFileDesc &file,
            ByteBuffer *out) const;
    int ExtractPathWithStreamFeedback(
            const CPlugFileFidContainer_SFileDesc *file,
            const char *selectedPath,
            ByteBuffer *out) const;

public:
    CPlugFilePack();
    virtual ~CPlugFilePack();
    CPlugFilePack(const CPlugFilePack &) = delete;
    CPlugFilePack &operator=(const CPlugFilePack &) = delete;
    int IsCrypted(void) const;
    const std::string &PackName(void) const;
    static int ComputeKey(const char *keyString, SNat128 &keyOut);
    int LoadHeadersFromMemory();
    int OpenFromMemory(
            const void *pakBytes,
            std::size_t pakByteCount,
            const void *packlistBytes,
            std::size_t packlistByteCount);
    int OpenFromMemory(
            const void *pakBytes,
            std::size_t pakByteCount,
            const void *packlistBytes,
            std::size_t packlistByteCount,
            const char *packName);
    int OpenFromMemory(
            const void *pakBytes,
            std::size_t pakByteCount,
            const InstalledPackKeyCatalog &keyCatalog,
            const char *packName);
    void FreeLoadedPack();
    int FolderPathRecursive(
            u32 folderIndex, char *out, std::size_t outSize) const;
    int FileDescPlainPath(
            const CPlugFileFidContainer_SFileDesc *file,
            char *out,
            std::size_t outSize) const;
    int FileDescPathMatches(
            const CPlugFileFidContainer_SFileDesc *file,
            const char *selectedPath) const;
    const CPlugFileFidContainer_SFileDesc *FindFileDescByPath(
            const char *selectedPath) const;
    int SelectedPathForPlainRef(
            const char *plainPath,
            char *out,
            std::size_t outSize) const;
    int ExtractPath(const char *selectedPath, ByteBuffer *out) const;
    int ExtractPathWithStreamFeedbackStrict(
            const char *selectedPath,
            ByteBuffer *out) const;
    int ExtractReferenceTablePrefix(
            const char *selectedPath,
            ByteBuffer *out) const;

private:
    std::string packName_;
};

#endif
