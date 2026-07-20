#include "format/materials/skin_material_remap_catalog.h"

#include <cstring>
#include <new>
#include <utility>
#include <vector>

#include "format/archive/archive_binary.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/archive_node_reference.h"
#include "format/pack/block_info_catalog/blockinfo_archive_stream.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include "format/pack/block_info_catalog/replay_collection_archive.h"
#include "format/pack/block_info_catalog/replay_collection_archive_chunk_ids.h"
#include "format/pack/installed/byte_buffer.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/replay/replay_static_descriptor_limits.h"

namespace {

enum class SkinArchiveChunk : u32 {
    Metadata = 0x03031003u,
    MaterialRemaps = 0x03031004u,
};

constexpr u32 DecorationTerrainModifierChunk = 0x0303c000u;
constexpr u32 BayCryptedCollectionSurfaceReplacements = 0x5601d808u;
constexpr u32 FastBufferArchiveVersion = 10u;
constexpr u32 MaxTerrainModifierCount = 128u;

enum class MaterialSelectorKind {
    ExactAsset,
    Folder,
};

struct MaterialRemapRule {
    MaterialSelectorKind selector = MaterialSelectorKind::ExactAsset;
    std::string targetPath;
    std::string replacementFolder;
    std::string replacementPrefix;
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
    return sourcePath == rule.targetPath ||
           (rule.selector == MaterialSelectorKind::Folder &&
            IsUnderFolder(sourcePath, rule.targetPath));
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

bool ParseCollectionTerrainModifierRefs(
        const CatalogCollectionArchive &collection,
        std::vector<ArchiveNodeReference> &refsOut) {
    const unsigned char *bytes = collection.Bytes();
    const u32 byteCount = collection.ByteCount();
    const u32 bodyOffset =
            collection.ExternalRefs().BodyOffsetForFormatParser();
    if (bytes == nullptr || bodyOffset > byteCount || byteCount < 4u) {
        return false;
    }

    u32 chunkOffset = UINT32_MAX;
    for (u32 offset = bodyOffset; offset <= byteCount - 4u; ++offset) {
        const u32 chunk = TmnfFormat::ArchiveBinary::ReadU32LE(
                bytes + offset);
        if (chunk == ArchiveChunkIdValue(
                             CGameCtnCollectionArchiveChunkId::
                                     SurfaceReplacementPairs) ||
            chunk == BayCryptedCollectionSurfaceReplacements) {
            chunkOffset = offset;
            break;
        }
    }
    if (chunkOffset == UINT32_MAX) {
        return true;
    }

    BlockInfoSizeParseStream stream{};
    stream.bytes = bytes;
    stream.byteCount = byteCount;
    stream.offset = chunkOffset + 4u;

    u32 replacementPairCount = 0u;
    if (!stream.ReadU32(&replacementPairCount) ||
        replacementPairCount > 0x100000u) {
        return false;
    }
    for (u32 index = 0u; index < replacementPairCount; ++index) {
        if (!stream.ReadCMwIdText(nullptr, 0u) ||
            !stream.ReadCMwIdText(nullptr, 0u)) {
            return false;
        }
    }

    // CFastBuffer::ArchiveFastBuffer stores its version, then its count,
    // after the replacement-pair array. The following node refs are the
    // CGameCtnDecorationTerrainModifier entries in that buffer.
    u32 archiveVersion = 0u;
    u32 modifierCount = 0u;
    if (!stream.ReadU32(&archiveVersion) ||
        archiveVersion != FastBufferArchiveVersion ||
        !stream.ReadU32(&modifierCount) ||
        modifierCount > MaxTerrainModifierCount) {
        return false;
    }
    try {
        refsOut.reserve(modifierCount);
        for (u32 index = 0u; index < modifierCount; ++index) {
            ArchiveNodeReference reference = ArchiveNodeReference::Invalid();
            if (!stream.ReadArchiveNodeRef(&reference) ||
                !reference.IsValid()) {
                return false;
            }
            refsOut.push_back(reference);
        }
    } catch (const std::bad_alloc &) {
        return false;
    }
    return true;
}

bool ExtractPackPath(const CPlugFilePack &pack,
                     const std::string &plainPath,
                     ByteBuffer &bytes) {
    char selected[CGameCtnReplayStaticSelectedPathCapacity]{};
    return (pack.SelectedPathForPlainRef(
                    plainPath.c_str(), selected, sizeof(selected)) &&
            pack.ExtractPathWithStreamFeedbackStrict(selected, &bytes)) ||
           pack.ExtractPathWithStreamFeedbackStrict(
                   plainPath.c_str(), &bytes);
}

std::string PackQualifiedPath(const CPlugFilePack &pack,
                              const std::string &path) {
    const std::string prefix = pack.PackName() + "\\";
    if (path.compare(0u, prefix.size(), prefix) == 0) {
        return path;
    }
    return prefix + path;
}

bool ParseSkinMaterialRemaps(
        const CPlugFilePack &pack,
        const unsigned char *bytes,
        u32 byteCount,
        const std::string &replacementFolder,
        std::vector<MaterialRemapRule> &rules) {
    BlockInfoDescriptorExternalRefs refs;
    if (bytes == nullptr || !refs.ParseGbx(bytes, byteCount)) {
        return false;
    }
    refs.AttachInstalledPack(&pack);

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
        u32 audienceClassId = 0u;
        u32 hasTarget = 0u;
        u32 packElementIndex = 0u;
        ArchiveNodeReference target = ArchiveNodeReference::Invalid();
        char replacementPrefix[96]{};
        if (!stream.ReadU32(&audienceClassId) ||
            !stream.ReadStringFixed(replacementPrefix,
                                    sizeof(replacementPrefix)) ||
            !stream.ReadU32(&hasTarget) ||
            !stream.ReadArchiveNodeRef(&target) ||
            !stream.ReadU32(&packElementIndex)) {
            return false;
        }
        if (packElementIndex != 0u) {
            return false;
        }
        if (hasTarget == 0u || audienceClassId != TMNF_CLASS_CPlugMaterial) {
            continue;
        }
        bool loadable = false;
        std::optional<std::string> targetPath =
                ExternalPath(refs, target, loadable);
        if (!targetPath || targetPath->empty() ||
            replacementPrefix[0] == '\0') {
            continue;
        }
        try {
            rules.push_back({loadable ? MaterialSelectorKind::ExactAsset
                                      : MaterialSelectorKind::Folder,
                             std::move(*targetPath),
                             replacementFolder,
                             replacementPrefix});
        } catch (const std::bad_alloc &) {
            return false;
        }
    }
    return true;
}

bool LoadTerrainModifierRules(
        const CPlugFilePack &pack,
        const CatalogCollectionArchive &collection,
        ArchiveNodeReference modifier,
        std::vector<MaterialRemapRule> &rules) {
    char descriptorPath[CGameCtnReplayStaticSolidDescriptorPathCapacity]{};
    if (!collection.ExternalRefs().PlainPathForReference(
                modifier, descriptorPath, sizeof(descriptorPath)) ||
        descriptorPath[0] == '\0') {
        return false;
    }

    ByteBuffer descriptorBytes;
    if (!ExtractPackPath(pack, descriptorPath, descriptorBytes) ||
        descriptorBytes.Empty() || descriptorBytes.Size() > UINT32_MAX) {
        return false;
    }
    BlockInfoDescriptorExternalRefs descriptorRefs;
    if (!descriptorRefs.ParseGbx(
                descriptorBytes.Data(),
                static_cast<u32>(descriptorBytes.Size())) ||
        descriptorRefs.ClassId() !=
                TMNF_CLASS_CGameCtnDecorationTerrainModifier ||
        !descriptorRefs.AttachInstalledPackRelativeTo(&pack,
                                                      descriptorPath)) {
        return false;
    }

    BlockInfoSizeParseStream stream{};
    stream.bytes = descriptorBytes.Data();
    stream.byteCount = static_cast<u32>(descriptorBytes.Size());
    stream.offset = descriptorRefs.BodyOffsetForFormatParser();
    u32 chunk = 0u;
    ArchiveNodeReference skin = ArchiveNodeReference::Invalid();
    char replacementFolder[CGameCtnReplayStaticSolidDescriptorPathCapacity]{};
    if (!stream.ReadU32(&chunk) ||
        chunk != DecorationTerrainModifierChunk ||
        !stream.ReadArchiveNodeRef(&skin) || !skin.IsValid() ||
        !stream.ReadStringFixed(replacementFolder,
                                sizeof(replacementFolder)) ||
        replacementFolder[0] == '\0') {
        return false;
    }

    char relativeSkinPath[CGameCtnReplayStaticSolidDescriptorPathCapacity]{};
    if (!descriptorRefs.PlainPathForReference(
                skin, relativeSkinPath, sizeof(relativeSkinPath)) ||
        relativeSkinPath[0] == '\0') {
        return false;
    }
    std::string skinPath;
    try {
        skinPath = PackQualifiedPath(pack, relativeSkinPath);
    } catch (const std::bad_alloc &) {
        return false;
    }
    ByteBuffer skinBytes;
    return ExtractPackPath(pack, skinPath, skinBytes) &&
           !skinBytes.Empty() && skinBytes.Size() <= UINT32_MAX &&
           ParseSkinMaterialRemaps(
                   pack,
                   skinBytes.Data(),
                   static_cast<u32>(skinBytes.Size()),
                   replacementFolder,
                   rules);
}

std::optional<std::string> ReplacementMaterialPath(
        const MaterialRemapRule &rule) {
    if (rule.replacementFolder.empty() ||
        rule.replacementPrefix.empty()) {
        return std::nullopt;
    }
    try {
        std::string path = rule.replacementFolder;
        if (path.back() != '\\') {
            path.push_back('\\');
        }
        path += rule.replacementPrefix;
        if (path.find(".Material.Gbx") == std::string::npos) {
            path += ".Material.Gbx";
        }
        return path;
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
}

}  // namespace

struct SkinMaterialRemapCatalog::Impl {
    std::vector<MaterialRemapRule> rules;
};

SkinMaterialRemapCatalog::SkinMaterialRemapCatalog()
        : impl_(std::make_unique<Impl>()) {}

SkinMaterialRemapCatalog::~SkinMaterialRemapCatalog() = default;

bool SkinMaterialRemapCatalog::Load(const CPlugFilePack &pack) {
    CatalogCollectionArchive collection;
    if (!collection.Load(&pack, pack.PackName().c_str())) {
        impl_->rules.clear();
        return true;
    }
    std::vector<ArchiveNodeReference> modifiers;
    if (!ParseCollectionTerrainModifierRefs(collection, modifiers)) {
        return false;
    }
    std::vector<MaterialRemapRule> rules;
    for (ArchiveNodeReference modifier : modifiers) {
        if (!LoadTerrainModifierRules(pack, collection, modifier, rules)) {
            return false;
        }
    }
    impl_->rules = std::move(rules);
    return true;
}

std::optional<DecorationTerrainModifierRemapPath>
SkinMaterialRemapCatalog::FindDecorationTerrainModifier(
        const std::string &sourceMaterialPath) const {
    for (const MaterialRemapRule &rule : impl_->rules) {
        if (!Matches(rule, sourceMaterialPath)) {
            continue;
        }
        std::optional<std::string> target = ReplacementMaterialPath(rule);
        if (!target) {
            return std::nullopt;
        }
        return DecorationTerrainModifierRemapPath{std::move(*target)};
    }
    return std::nullopt;
}
