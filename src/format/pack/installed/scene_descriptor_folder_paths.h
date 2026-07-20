#ifndef TMNF_SCENE_DESCRIPTOR_FOLDER_PATHS_H
#define TMNF_SCENE_DESCRIPTOR_FOLDER_PATHS_H

#include "engine/core/engine_types.h"
#include <array>
#include <stddef.h>
#include <vector>

#include "format/replay/replay_static_descriptor_limits.h"
struct SceneDescriptorFolderPath {
    char path[CGameCtnReplayStaticSelectedPathCapacity]{};
};

struct SceneDescriptorFolderStack {
    static constexpr u32 MaxDepth = 32u;
    static constexpr size_t SegmentCapacity = 64u;

    std::array<std::array<char, SegmentCapacity>, MaxDepth> segments{};

    char *MutableSegment(u32 depth);
    void ClearBelow(u32 depth);
    int BuildPathToDepth(u32 depth, char *out, size_t outSize) const;
};

struct SceneDescriptorFolderPaths {
    std::vector<SceneDescriptorFolderPath> paths;

    static const char *ConstructionBlockInfoPrefix();
    static const char *StadiumMobilPrefix();
    static const char *RacesMobilPrefix();
    static const char *StadiumMediaSolidPrefix();
    static const char *StadiumMediaMaterialPrefix();
    static int StartsWith(const char *text, const char *prefix);
    static int IsBlockInfoDescriptorPath(const char *plainPath);
    static int IsZoneDescriptorPath(const char *plainPath);
    static int IsConstructionBlockInfoPath(const char *plainPath);
    static int IsStadiumMobilPath(const char *plainPath);
    static int IsRacesMobilPath(const char *plainPath);
    static int IsMobilDescriptorPath(const char *plainPath);
    static int IsMediaSolidPath(const char *plainPath);
    static int IsMediaMaterialPath(const char *plainPath);
    static int IsMediaShaderPath(const char *plainPath);
    static int IsMediaTexturePath(const char *plainPath);
    static int IsStadiumMediaSolidPath(const char *plainPath);
    static int IsStadiumMediaMaterialPath(const char *plainPath);
    static int AppendCString(char *dst, size_t dstSize, const char *src);
    static int HashFileNameDescriptorPathWithBase(
            const char *plainPath,
            const char *basePath,
            char *out,
            size_t outSize);
    static int HashStadiumMediaSolidPath(const char *plainPath,
                                         char *out,
                                         size_t outSize);
    static int HashStadiumMediaMaterialPath(const char *plainPath,
                                            char *out,
                                            size_t outSize);
    static int HashMediaSolidPath(const char *plainPath,
                                  char *out,
                                  size_t outSize);
    static int HashMediaMaterialPath(const char *plainPath,
                                     char *out,
                                     size_t outSize);
    static int HashMediaShaderPath(const char *plainPath,
                                   char *out,
                                   size_t outSize);
    static int HashMediaTexturePath(const char *plainPath,
                                    char *out,
                                    size_t outSize);
    static int HashMobilDescriptorPath(const char *plainPath,
                                       char *out,
                                       size_t outSize);
    static int HashPackSelectedPath(const char *plainPath,
                                    char *out,
                                    size_t outSize);
    static int FindMobilDescriptorHashBase(const char *plainPath,
                                           const char **baseOut);
    static int FindPackSelectedPathHashBase(const char *plainPath,
                                            const char **baseOut);
    int FindFolderForRef(u32 folderIndex, const char **folderOut) const;
    int AppendFolderPath(const SceneDescriptorFolderStack &stack, u32 depth);
    template <typename Stream>
    int ParseSubfolders(Stream *stream,
                        SceneDescriptorFolderStack &stack,
                        u32 depth) {
        u32 count = 0u;
        char *segment = stack.MutableSegment(depth);
        if (segment == nullptr ||
            !stream->ReadU32(&count) ||
            count > 0x10000u) {
            return 0;
        }
        for (u32 i = 0; i < count; i++) {
            if (!stream->ReadStringFixed(
                        segment,
                        SceneDescriptorFolderStack::SegmentCapacity) ||
                !AppendFolderPath(stack, depth)) {
                return 0;
            }
            stack.ClearBelow(depth);
            if (!ParseSubfolders(stream, stack, depth + 1u)) {
                return 0;
            }
        }
        return 1;
    }
    static int BuildPackRefFullPath(const char *folder,
                                    const char *name,
                                    char *out,
                                    size_t outSize);
    static int BuildPackRefFullPath(const char *folder,
                                    const char *name,
                                    const char *packRoot,
                                    char *out,
                                    size_t outSize);
};

#endif
