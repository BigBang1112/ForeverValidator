#ifndef TMNF_BLOCKINFO_DESCRIPTOR_EXTERNAL_REFS_H
#define TMNF_BLOCKINFO_DESCRIPTOR_EXTERNAL_REFS_H

#include "engine/core/engine_types.h"
#include <stddef.h>
#include <vector>

#include "format/archive/archive_node_reference.h"
#include "format/replay/replay_static_descriptor_limits.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
class CGameCtnReplayCollectionZoneSourceInfo;
struct CPlugFilePack;

class BlockInfoDescriptorExternalRef {
public:
    void Configure(ArchiveNodeReference sourceNode,
                   u32 sourceLoadable,
                   u32 sourceFolderIndex,
                   const char *sourceName);

    ArchiveNodeReference ArchiveNode() const;
    u32 IsLoadable() const;
    const char *FileName() const;
    int HasName() const;

private:
    friend class BlockInfoDescriptorExternalRefs;

    u32 FolderIndexForFormatLookup() const {
        return folderIndex;
    }

    ArchiveNodeReference archiveNode =
            ArchiveNodeReference::Invalid();
    u32 loadable = 0u;
    u32 folderIndex = 0u;
    char name[CGameCtnReplayStaticMobilModelNameCapacity]{};
};

class BlockInfoDescriptorExternalRefs {
public:
    int ParseGbx(const unsigned char *bytes, u32 byteCount);

    void AttachInstalledPack(const CPlugFilePack *pack);
    const CPlugFilePack *InstalledPack() const;
    u32 BodyOffsetForFormatParser() const;
    u32 NodeCountForFormatBounds() const;
    const std::vector<BlockInfoDescriptorExternalRef> &References() const;

    const BlockInfoDescriptorExternalRef *FindReference(
            ArchiveNodeReference sourceNode) const;
    int PlainPathForReference(ArchiveNodeReference sourceNode,
                              char *out,
                              size_t outSize) const;
    int PlainPathForReference(const BlockInfoDescriptorExternalRef *ref,
                              char *out,
                              size_t outSize) const;
    int OptionalPlainPathForReference(const BlockInfoDescriptorExternalRef *ref,
                                      char *out,
                                      size_t outSize) const;
    int ConstructionZoneSourceInfo(
            ArchiveNodeReference sourceNode,
            CGameCtnReplayCollectionZoneSourceInfo *sourceOut) const;
    int ResolveBlockInfoSourceRef(ArchiveNodeReference sourceNode,
                                  char *out,
                                  size_t outSize,
                                  u32 *resolvedExternalOut) const;

private:
    SceneDescriptorFolderPaths folders{};
    const CPlugFilePack *installedPack = nullptr;
    std::vector<BlockInfoDescriptorExternalRef> references;
    u32 nodeCount = 0u;
    u32 bodyOffset = 0u;
    u32 classId = 0u;
    u32 version = 0u;
};

#endif
