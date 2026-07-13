#include "format/pack/block_info_catalog/block_info_catalog_loader.h"
#include <array>
#include <optional>
#include <string_view>
#include <utility>

#include "engine/resources/block_info_catalog.h"
#include "format/pack/block_info_catalog/block_info_asset_store.h"
#include "format/pack/block_info_catalog/block_info_catalog_identity_reader.h"
#include "format/pack/installed/byte_buffer.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/replay/replay_static_descriptor_limits.h"
#include "format/archive/tmnf_archive_ids.h"
#include <new>
namespace {

std::optional<EBlockType> BlockTypeForArchiveClass(u32 classId) {
    switch (classId) {
        case TMNF_CLASS_CGameCtnBlockInfoFlat:
            return EBlockType::Flat;
        case TMNF_CLASS_CGameCtnBlockInfoFrontier:
            return EBlockType::Frontier;
        case TMNF_CLASS_CGameCtnBlockInfoClassic:
            return EBlockType::Classic;
        case TMNF_CLASS_CGameCtnBlockInfoRoad:
            return EBlockType::Road;
        case TMNF_CLASS_CGameCtnBlockInfoClip:
            return EBlockType::Clip;
        case TMNF_CLASS_CGameCtnBlockInfoRectAsym:
            return EBlockType::RectAsym;
        default:
            return std::nullopt;
    }
}

bool IsConstructionBlockInfoPath(std::string_view path) {
    return path.find("ConstructionBlockInfo") != std::string_view::npos;
}

std::optional<u32> CollectionLandZoneHeight(
        const BlockInfoCatalogIdentity &identity,
        EBlockType blockType) {
    if (identity.collection != "Stadium" ||
        blockType > EBlockType::Frontier) {
        return std::nullopt;
    }
    if (identity.identifier == "StadiumDirtHill") {
        return 2u;
    }
    if (identity.identifier == "StadiumDirtBorder" ||
        identity.identifier == "StadiumDirt" ||
        identity.identifier == "StadiumGrass") {
        return 0u;
    }
    return std::nullopt;
}

bool TryAddPackFile(const CPlugFilePack &pack,
                    const CPlugFileFidContainer_SFileDesc &file,
                    BlockInfoAssetStore &assets,
                    BlockInfoCatalog &catalog) {
    const std::optional<EBlockType> blockType =
            BlockTypeForArchiveClass(file.classId);
    if (!blockType) {
        return true;
    }

    std::array<char, CGameCtnReplayStaticDescriptorPathCapacity> path{};
    if (!pack.FileDescPlainPath(&file, path.data(), path.size()) ||
        !IsConstructionBlockInfoPath(path.data())) {
        return true;
    }

    ByteBuffer bytes;
    BlockInfoCatalogIdentity identity;
    BlockInfoCatalogIdentityReader reader;
    if (!pack.ExtractPath(path.data(), &bytes) ||
        bytes.Empty() || bytes.Size() > UINT32_MAX ||
        !reader.Parse(bytes.Data(),
                      static_cast<u32>(bytes.Size()),
                      &identity)) {
        return true;
    }

    const std::optional<u32> landZoneHeight =
            CollectionLandZoneHeight(identity, *blockType);
    BlockInfoCatalogEntry entry;
    try {
        entry.identifier = std::move(identity.identifier);
        entry.collection = std::move(identity.collection);
        entry.blockType = *blockType;
        entry.asset = assets.Register(path.data());
        entry.collectionLandZoneHeight = landZoneHeight;
    } catch (const std::bad_alloc &) {
        return false;
    }
    return entry.asset.IsValid() && catalog.Add(std::move(entry));
}

}  // namespace

bool LoadBlockInfoCatalog(const CPlugFilePack &pack,
                          BlockInfoAssetStore &assets,
                          BlockInfoCatalog &catalog) {
    catalog.Clear();
    for (const CPlugFileFidContainer_SFileDesc &file : pack.files) {
        if (!TryAddPackFile(pack, file, assets, catalog)) {
            catalog.Clear();
            return false;
        }
    }
    return catalog.Size() != 0u;
}
