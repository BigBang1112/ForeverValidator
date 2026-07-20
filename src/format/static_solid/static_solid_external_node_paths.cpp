#include "format/static_solid/static_solid_external_node_paths.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include <new>
namespace {

int assign_cstr(std::string *dst, const char *src) {
    if (dst == nullptr || src == nullptr) {
        return 0;
    }
    try {
        *dst = src;
    } catch (const std::bad_alloc &) {
        return 0;
    }
    return 1;
}

}  // namespace

void CGameCtnReplayStaticSolidExternalNodeRef::Install(
        u32 newPresent,
        u32 newExternal,
        u32 newFolderIndex,
        const std::string &newName) {
    present = newPresent;
    external = newExternal;
    folderIndex = newFolderIndex;
    name = newName;
}

int CGameCtnReplayStaticSolidExternalNodeRef::IsPresent() const {
    return present != 0u;
}

int CGameCtnReplayStaticSolidExternalNodeRef::IsExternal() const {
    return external != 0u;
}

int CGameCtnReplayStaticSolidExternalNodeRef::HasUsableName() const {
    return !name.empty() && name[0] != '\0';
}

int CGameCtnReplayStaticSolidExternalNodeRef::CanResolvePath(
        int requirePresent) const {
    return (!requirePresent || IsPresent()) &&
           IsExternal() &&
           HasUsableName();
}

u32 CGameCtnReplayStaticSolidExternalNodeRef::FolderIndex() const {
    return folderIndex;
}

const std::string &CGameCtnReplayStaticSolidExternalNodeRef::Name() const {
    return name;
}

void CGameCtnReplayStaticSolidExternalNodePathResult::Clear() {
    plainPath.clear();
    selectedPath.clear();
    name.clear();
}

int CGameCtnReplayStaticSolidExternalNodePathResult::InstallPlainPath(
        const char *newPlainPath,
        const char *newName) {
    if (newPlainPath == nullptr || newName == nullptr) {
        return 0;
    }
    return assign_cstr(&plainPath, newPlainPath) &&
           assign_cstr(&name, newName);
}

int CGameCtnReplayStaticSolidExternalNodePathResult::InstallSelectedPath(
        const char *newSelectedPath) {
    if (newSelectedPath == nullptr || newSelectedPath[0] == '\0') {
        return 0;
    }
    return assign_cstr(&selectedPath, newSelectedPath);
}

int CGameCtnReplayStaticSolidExternalNodePathResult::HasPlainPath() const {
    return !plainPath.empty();
}

int CGameCtnReplayStaticSolidExternalNodePathResult::HasSelectedPath() const {
    return !selectedPath.empty();
}

const char *CGameCtnReplayStaticSolidExternalNodePathResult::PlainPath() const {
    return plainPath.c_str();
}

const char *CGameCtnReplayStaticSolidExternalNodePathResult::SelectedPath()
        const {
    return selectedPath.c_str();
}

const char *CGameCtnReplayStaticSolidExternalNodePathResult::DisplayNameOr(
        const char *fallback) const {
    if (!name.empty()) {
        return name.c_str();
    }
    return fallback;
}

int CGameCtnReplayStaticSolidExternalNodePathResolver::BuildPlainPath(
        const SceneDescriptorFolderPaths *folders,
        const CGameCtnReplayStaticSolidExternalNodeRef &nodeRef,
        int requirePresent,
        CGameCtnReplayStaticSolidExternalNodePathResult *resultOut) {
    return BuildPlainPath(
            folders, nullptr, nodeRef, requirePresent, resultOut);
}

int CGameCtnReplayStaticSolidExternalNodePathResolver::BuildPlainPath(
        const SceneDescriptorFolderPaths *folders,
        const CPlugFilePack *pack,
        const CGameCtnReplayStaticSolidExternalNodeRef &nodeRef,
        int requirePresent,
        CGameCtnReplayStaticSolidExternalNodePathResult *resultOut) {
    if (resultOut == nullptr) {
        return 0;
    }
    resultOut->Clear();
    if (folders == nullptr ||
        !nodeRef.CanResolvePath(requirePresent)) {
        return 0;
    }
    const char *folder = nullptr;
    char plainPath[CGameCtnReplayStaticSolidDescriptorPathCapacity];
    plainPath[0] = '\0';
    if (!folders->FindFolderForRef(nodeRef.FolderIndex(), &folder) ||
        !SceneDescriptorFolderPaths::BuildPackRefFullPath(
                folder,
                nodeRef.Name().c_str(),
                pack != nullptr ? pack->PackName().c_str() : "Stadium",
                plainPath,
                sizeof(plainPath))) {
        return 0;
    }
    return resultOut->InstallPlainPath(plainPath, nodeRef.Name().c_str());
}

int CGameCtnReplayStaticSolidExternalNodePathResolver::ResolveSelectedPath(
        const SceneDescriptorFolderPaths *folders,
        const CPlugFilePack *pack,
        const CGameCtnReplayStaticSolidExternalNodeRef &nodeRef,
        int requirePresent,
        CGameCtnReplayStaticSolidExternalNodePathResult *resultOut) {
    if (resultOut == nullptr) {
        return 0;
    }
    if (!BuildPlainPath(folders,
                        pack,
                        nodeRef,
                        requirePresent,
                        resultOut) ||
        pack == nullptr) {
        return 0;
    }
    char selectedPath[CGameCtnReplayStaticSolidDescriptorPathCapacity];
    selectedPath[0] = '\0';
    if (!pack->SelectedPathForPlainRef(resultOut->PlainPath(),
                                       selectedPath,
                                       sizeof(selectedPath))) {
        return 0;
    }
    return resultOut->InstallSelectedPath(selectedPath);
}

int CGameCtnReplayStaticSolidExternalNodePathResolver::
BuildPlainAndTrySelectedPath(
        const SceneDescriptorFolderPaths *folders,
        const CPlugFilePack *pack,
        const CGameCtnReplayStaticSolidExternalNodeRef &nodeRef,
        int requirePresent,
        CGameCtnReplayStaticSolidExternalNodePathResult *resultOut) {
    if (resultOut == nullptr) {
        return 0;
    }
    if (!BuildPlainPath(folders,
                        pack,
                        nodeRef,
                        requirePresent,
                        resultOut)) {
        return 0;
    }
    char selectedPath[CGameCtnReplayStaticSolidDescriptorPathCapacity];
    selectedPath[0] = '\0';
    if (pack != nullptr &&
        pack->SelectedPathForPlainRef(resultOut->PlainPath(),
                                      selectedPath,
                                      sizeof(selectedPath))) {
        if (!resultOut->InstallSelectedPath(selectedPath)) {
            return 0;
        }
    }
    return 1;
}
