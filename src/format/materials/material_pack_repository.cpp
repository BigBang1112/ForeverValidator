#include "format/materials/material_pack_repository.h"
#include <string>
#include <utility>
#include <vector>

#include "format/pack/installed/byte_buffer.h"
#include "format/materials/material_archive_decoder.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include "format/materials/skin_material_remap_catalog.h"
#include <new>
namespace {

struct StoredMaterialAsset {
    std::string path;
    MaterialSurfaceDefinition surface;
};

bool ExtractMaterialBytes(const CPlugFilePack &pack,
                          const std::string &path,
                          ByteBuffer &bytes) {
    if (pack.ExtractPath(path.c_str(), &bytes)) {
        return true;
    }
    const std::size_t slash = path.find_last_of('\\');
    if (slash == std::string::npos) {
        return false;
    }
    const std::string base = path.substr(0u, slash + 1u);
    char hashedPath[512]{};
    return SceneDescriptorFolderPaths::HashFileNameDescriptorPathWithBase(
                   path.c_str(),
                   base.c_str(),
                   hashedPath,
                   sizeof(hashedPath)) &&
           pack.ExtractPath(hashedPath, &bytes);
}

}  // namespace

struct MaterialPackRepository::Impl {
    explicit Impl(CPlugFilePack &sourcePack) : pack(sourcePack) {}

    CPlugFilePack &pack;
    std::vector<StoredMaterialAsset> assets;
    SkinMaterialRemapCatalog remaps;
    bool remapsLoaded = false;

    std::optional<MaterialAssetDefinition> Load(const std::string &path);
};

std::optional<MaterialAssetDefinition> MaterialPackRepository::Impl::Load(
        const std::string &path) {
    for (u32 index = 0u; index < assets.size(); ++index) {
        if (assets[index].path == path) {
            return MaterialAssetDefinition{
                    MaterialAssetHandle::FromRepositoryIndex(index),
                    assets[index].surface};
        }
    }
    if (assets.size() >= UINT32_MAX) {
        return std::nullopt;
    }
    ByteBuffer bytes;
    if (!ExtractMaterialBytes(pack, path, bytes) ||
        bytes.Empty() || bytes.Size() > UINT32_MAX) {
        return std::nullopt;
    }
    std::optional<MaterialSurfaceDefinition> surface = DecodeMaterialArchive(
            bytes.Data(), static_cast<u32>(bytes.Size()));
    if (!surface) {
        return std::nullopt;
    }
    try {
        assets.push_back({path, *surface});
        return MaterialAssetDefinition{
                MaterialAssetHandle::FromRepositoryIndex(
                        static_cast<u32>(assets.size() - 1u)),
                *surface};
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
}

MaterialPackRepository::MaterialPackRepository(CPlugFilePack &pack)
        : impl_(std::make_unique<Impl>(pack)) {}

MaterialPackRepository::~MaterialPackRepository() = default;

std::optional<ResolvedMaterialDefinition> MaterialPackRepository::Resolve(
        std::string_view identifier) {
    if (identifier.empty()) {
        return std::nullopt;
    }
    std::string sourcePath;
    try {
        sourcePath = SceneDescriptorFolderPaths::StadiumMediaMaterialPrefix();
        sourcePath.append(identifier.data(), identifier.size());
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
    return ResolvePath(sourcePath);
}

std::optional<ResolvedMaterialDefinition> MaterialPackRepository::ResolvePath(
        std::string_view plainPath) {
    if (plainPath.empty()) {
        return std::nullopt;
    }
    std::string sourcePath;
    try {
        sourcePath.assign(plainPath.data(), plainPath.size());
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
    if (sourcePath.find(".Material.Gbx") == std::string::npos) {
        return std::nullopt;
    }
    std::optional<MaterialAssetDefinition> source = impl_->Load(sourcePath);
    if (!source) {
        return std::nullopt;
    }

    if (!impl_->remapsLoaded) {
        if (!impl_->remaps.Load(impl_->pack)) {
            return std::nullopt;
        }
        impl_->remapsLoaded = true;
    }

    ResolvedMaterialDefinition resolved;
    resolved.material = *source;
    const std::optional<TerrainModifierDirtRemapPaths> remap =
            impl_->remaps.FindTerrainModifierDirt(sourcePath);
    if (!remap) {
        return resolved;
    }
    std::optional<MaterialAssetDefinition> target =
            impl_->Load(remap->preferred);
    if (!target && !remap->fallback.empty()) {
        target = impl_->Load(remap->fallback);
    }
    resolved.remaps.terrainModifierDirt = std::move(target);
    return resolved;
}
