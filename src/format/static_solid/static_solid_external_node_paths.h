#ifndef TMNF_STATIC_SOLID_EXTERNAL_NODE_PATHS_H
#define TMNF_STATIC_SOLID_EXTERNAL_NODE_PATHS_H

#include <string>

#include "engine/core/engine_types.h"
struct CPlugFilePack;
struct SceneDescriptorFolderPaths;

class CGameCtnReplayStaticSolidExternalNodeRef {
public:
    void Install(u32 present,
                 u32 external,
                 u32 folderIndex,
                 const std::string &name);
    int IsPresent() const;
    int IsExternal() const;
    int HasUsableName() const;
    int CanResolvePath(int requirePresent) const;
    u32 FolderIndex() const;
    const std::string &Name() const;

private:
    u32 present = 0u;
    u32 external = 0u;
    u32 folderIndex = 0xffffffffu;
    std::string name;
};

class CGameCtnReplayStaticSolidExternalNodePathResult {
public:
    void Clear();
    int InstallPlainPath(const char *plainPath, const char *name);
    int InstallSelectedPath(const char *selectedPath);
    int HasPlainPath() const;
    int HasSelectedPath() const;
    const char *PlainPath() const;
    const char *SelectedPath() const;
    const char *DisplayNameOr(const char *fallback) const;

private:
    std::string plainPath;
    std::string selectedPath;
    std::string name;
};

struct CGameCtnReplayStaticSolidExternalNodePathResolver {
    static int BuildPlainPath(
            const SceneDescriptorFolderPaths *folders,
            const CGameCtnReplayStaticSolidExternalNodeRef &nodeRef,
            int requirePresent,
            CGameCtnReplayStaticSolidExternalNodePathResult *resultOut);
    static int ResolveSelectedPath(
            const SceneDescriptorFolderPaths *folders,
            const CPlugFilePack *pack,
            const CGameCtnReplayStaticSolidExternalNodeRef &nodeRef,
            int requirePresent,
            CGameCtnReplayStaticSolidExternalNodePathResult *resultOut);
    static int BuildPlainAndTrySelectedPath(
            const SceneDescriptorFolderPaths *folders,
            const CPlugFilePack *pack,
            const CGameCtnReplayStaticSolidExternalNodeRef &nodeRef,
            int requirePresent,
            CGameCtnReplayStaticSolidExternalNodePathResult *resultOut);
};

#endif
