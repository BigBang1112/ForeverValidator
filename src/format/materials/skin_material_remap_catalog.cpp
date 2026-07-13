#include "format/materials/skin_material_remap_catalog.h"
#include <cstring>
#include <utility>
#include <vector>

#include "format/pack/block_info_catalog/blockinfo_archive_stream.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include "format/pack/installed/byte_buffer.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/archive/archive_node_reference.h"
#include "format/replay/replay_static_descriptor_limits.h"
#include "format/archive/archive_class_ids.h"
#include <new>
namespace {

enum class SkinArchiveChunk : u32 {
    Metadata = 0x03031003u,
    MaterialRemaps = 0x03031004u,
};

enum class MaterialSelectorKind {
    ExactAsset,
    Folder,
};

struct MaterialRemapRule {
    MaterialSelectorKind selector = MaterialSelectorKind::ExactAsset;
    std::string sourcePath;
    std::string replacementStem;
};

bool IsUnderFolder(const std::string &path, const std::string &folder) {
    if (folder.empty() || path.size() < folder.size() ||
        path.compare(0u, folder.size(), folder) != 0) {
        return false;
    }
    return folder.back() == '\\' || path.size() == folder.size() ||
           path[folder.size()] == '\\';
}

bool Matches(const MaterialRemapRule &rule, const std::string &sourcePath) {
    return sourcePath == rule.sourcePath ||
           (rule.selector == MaterialSelectorKind::Folder &&
            IsUnderFolder(sourcePath, rule.sourcePath));
}

std::optional<std::string> ExternalPath(
        const BlockInfoDescriptorExternalRefs &refs,
        ArchiveNodeReference node,
        bool &loadable) {
    loadable = false;
    const BlockInfoDescriptorExternalRef *reference =
            refs.FindReference(node);
    if (reference == nullptr || !reference->HasName()) {
        return std::nullopt;
    }
    char path[CGameCtnReplayStaticSolidDescriptorPathCapacity]{};
    if (!refs.PlainPathForReference(reference, path, sizeof(path))) {
        return std::nullopt;
    }
    loadable = reference->IsLoadable() != 0;
    return std::string(path);
}

bool ParseSkinMaterialRemaps(
        const unsigned char *bytes,
        u32 byteCount,
        std::vector<MaterialRemapRule> &rules) {
    BlockInfoDescriptorExternalRefs refs;
    if (bytes == nullptr || !refs.ParseGbx(bytes, byteCount)) {
        return false;
    }
    BlockInfoSizeParseStream stream{};
    stream.bytes = bytes;
    stream.byteCount = byteCount;
    stream.offset = refs.BodyOffsetForFormatParser();

    u32 chunk = 0u;
    if (!stream.ReadU32(&chunk) ||
        chunk != static_cast<u32>(SkinArchiveChunk::Metadata) ||
        !stream.SkipString() || !stream.SkipString() ||
        !stream.ReadU32(&chunk) ||
        chunk != static_cast<u32>(SkinArchiveChunk::MaterialRemaps)) {
        return false;
    }
    u32 version = 0u;
    u32 ruleCount = 0u;
    if (!stream.ReadU8(&version) || version != 4u ||
        !stream.SkipString() || !stream.SkipString() ||
        !stream.SkipString() || !stream.ReadU8(&ruleCount) ||
        ruleCount > 128u) {
        return false;
    }

    for (u32 index = 0u; index < ruleCount; ++index) {
        u32 remappedClass = 0u;
        u32 hasSource = 0u;
        u32 trailingFlag = 0u;
        ArchiveNodeReference source =
                ArchiveNodeReference::Invalid();
        char replacementStem[96]{};
        if (!stream.ReadU32(&remappedClass) ||
            !stream.ReadStringFixed(replacementStem,
                                    sizeof(replacementStem)) ||
            !stream.ReadU32(&hasSource) ||
            !stream.ReadArchiveNodeRef(&source) ||
            !stream.ReadU32(&trailingFlag)) {
            return false;
        }
        (void)trailingFlag;
        if (hasSource == 0u || remappedClass != TMNF_CLASS_CPlugMaterial) {
            continue;
        }
        bool loadable = false;
        std::optional<std::string> sourcePath =
                ExternalPath(refs, source, loadable);
        if (!sourcePath || sourcePath->empty()) {
            continue;
        }
        try {
            rules.push_back({loadable ? MaterialSelectorKind::ExactAsset
                                      : MaterialSelectorKind::Folder,
                             std::move(*sourcePath),
                             replacementStem});
        } catch (const std::bad_alloc &) {
            return false;
        }
    }
    return true;
}

std::string JoinPath(const char *folder, const std::string &name) {
    std::string path = folder;
    path += name;
    return path;
}

std::optional<std::string> FallbackReplacementPath(
        const std::string &sourcePath) {
    const std::size_t leaf = sourcePath.find_last_of('\\');
    const std::size_t begin = leaf == std::string::npos ? 0u : leaf + 1u;
    const std::size_t extension = sourcePath.find(".Material.Gbx", begin);
    if (extension == std::string::npos || extension == begin) {
        return std::nullopt;
    }
    return JoinPath("Stadium\\Media\\MaterialTerrainModifierDirt\\",
                    sourcePath.substr(begin, extension - begin) +
                            ".Material.Gbx");
}

}  // namespace

struct SkinMaterialRemapCatalog::Impl {
    std::vector<MaterialRemapRule> rules;
};

SkinMaterialRemapCatalog::SkinMaterialRemapCatalog()
        : impl_(std::make_unique<Impl>()) {}

SkinMaterialRemapCatalog::~SkinMaterialRemapCatalog() = default;

bool SkinMaterialRemapCatalog::Load(const CPlugFilePack &pack) {
    std::vector<MaterialRemapRule> rules;
    for (const CPlugFileFidContainer_SFileDesc &file : pack.files) {
        if (file.classId != TMNF_CLASS_CGameSkin) {
            continue;
        }
        char folder[512]{};
        if (!pack.FolderPathRecursive(file.folderIndex,
                                      folder,
                                      sizeof(folder))) {
            continue;
        }
        std::string path;
        try {
            path = folder;
            path += file.name;
        } catch (const std::bad_alloc &) {
            return false;
        }
        ByteBuffer bytes;
        if (!pack.ExtractPath(path.c_str(), &bytes) ||
            bytes.Empty() || bytes.Size() > UINT32_MAX) {
            continue;
        }
        if (!ParseSkinMaterialRemaps(bytes.Data(),
                                     static_cast<u32>(bytes.Size()),
                                     rules)) {
            continue;
        }
    }
    impl_->rules = std::move(rules);
    return true;
}

std::optional<TerrainModifierDirtRemapPaths>
SkinMaterialRemapCatalog::FindTerrainModifierDirt(
        const std::string &sourceMaterialPath) const {
    for (const MaterialRemapRule &rule : impl_->rules) {
        if (!Matches(rule, sourceMaterialPath)) {
            continue;
        }
        std::optional<std::string> fallback =
                FallbackReplacementPath(sourceMaterialPath);
        TerrainModifierDirtRemapPaths paths;
        try {
            paths.preferred = JoinPath(
                    "Stadium\\Media\\MaterialTerrainModifierDirt\\",
                    rule.replacementStem + ".Material.Gbx");
            paths.fallback = fallback.value_or(std::string{});
            return paths;
        } catch (const std::bad_alloc &) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}
