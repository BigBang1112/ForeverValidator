#include "format/static_solid/static_solid_archive_root_reader.h"
#include <string>

#include "format/static_solid/static_scene_archive_loader.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_node_graph.h"
void CGameCtnReplayStaticSolidArchiveRootHeader::Clear() {
    *this = CGameCtnReplayStaticSolidArchiveRootHeader{};
}

int CGameCtnReplayStaticSolidArchiveRootReader::ReadGbxRootArchive(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        SceneDescriptorFolderPaths *externalFolders,
        u32 maxPayloadByteCount,
        CGameCtnReplayStaticSolidArchiveRootHeader *headerOut) {
    if (byteStream == nullptr || archiveNodeGraph == nullptr ||
        externalFolders == nullptr || headerOut == nullptr) {
        return 0;
    }
    headerOut->Clear();

    unsigned char magic[3];
    unsigned char versionBytes[2];
    if (!byteStream->Read(magic, sizeof(magic)) ||
        magic[0] != 'G' || magic[1] != 'B' || magic[2] != 'X' ||
        !byteStream->Read(versionBytes, sizeof(versionBytes)) ||
        !byteStream->Skip(4u) ||
        !byteStream->ReadU32(&headerOut->classId) ||
        !byteStream->ReadU32(&headerOut->headerSize) ||
        headerOut->headerSize > maxPayloadByteCount ||
        !byteStream->Skip(headerOut->headerSize)) {
        return 0;
    }
    headerOut->version = (u32)versionBytes[0] |
                         ((u32)versionBytes[1] << 8u);
    return ParseRefSection(byteStream,
                           archiveNodeGraph,
                           externalFolders,
                           headerOut->version,
                           &headerOut->numNodes);
}

int CGameCtnReplayStaticSolidArchiveRootReader::ParseRefSection(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        SceneDescriptorFolderPaths *externalFolders,
        u32 version,
        u32 *numNodesOut) {
    if (byteStream == nullptr || archiveNodeGraph == nullptr ||
        externalFolders == nullptr) {
        return 0;
    }
    u32 numNodes = 0u;
    u32 externalCount = 0u;
    if (!byteStream->ReadU32(&numNodes) ||
        !byteStream->ReadU32(&externalCount) ||
        numNodes > 0x100000u ||
        externalCount > 0x100000u) {
        return 0;
    }
    if (numNodesOut != nullptr) {
        *numNodesOut = numNodes;
    }
    if (!archiveNodeGraph->EnsureNodeCapacity(numNodes + 1u)) {
        return 0;
    }
    if (externalCount == 0u) {
        return 1;
    }
    u32 externalRoot = 0u;
    SceneDescriptorFolderStack stack;
    if (!byteStream->ReadU32(&externalRoot)) {
        return 0;
    }
    (void)externalRoot;
    if (!externalFolders->ParseSubfolders(byteStream, stack, 0u)) {
        return 0;
    }
    for (u32 i = 0; i < externalCount; i++) {
        u32 flags = 0u;
        u32 nodeIndex = 0u;
        std::string name;
        u32 loadable = 0u;
        u32 folderIndex = 0xffffffffu;
        if (!byteStream->ReadU32(&flags)) {
            return 0;
        }
        if ((flags & 4u) != 0u) {
            if (!byteStream->Skip(4u)) {
                return 0;
            }
        } else {
            if (!byteStream->ReadStringOwned(&name) ||
                name.size() >= CGameCtnReplayStaticMobilModelNameCapacity) {
                return 0;
            }
        }
        if (!byteStream->ReadU32(&nodeIndex)) {
            return 0;
        }
        if (nodeIndex > numNodes) {
            return 0;
        }
        if (version >= 5u &&
            !byteStream->ReadU32(&loadable)) {
            return 0;
        }
        if ((flags & 4u) == 0u &&
            !byteStream->ReadU32(&folderIndex)) {
            return 0;
        }
        if (!archiveNodeGraph->EnsureNodeCapacity(nodeIndex)) {
            return 0;
        }
        if (!archiveNodeGraph->MarkExternalNode(
                    ArchiveNodeReference::FromIndex(nodeIndex),
                    loadable,
                    folderIndex,
                    name)) {
            return 0;
        }
    }
    return 1;
}
