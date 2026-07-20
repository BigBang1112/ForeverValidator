#include "format/static_solid/static_scene_archive_models.h"
#include <utility>

#include "engine/resources/block_info_catalog.h"
#include "engine/game/game_ctn_block_info.h"
#include "format/pack/block_info_catalog/installed_pack_asset_repository.h"
#include "format/pack/block_info_catalog/installed_pack_asset_repository_internal.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include "format/static_solid/static_solid_archive_reference.h"
#include "format/static_solid/static_solid_descriptor_dependency_queue.h"
#include "format/archive/gm_wire_conversion.h"
#include <new>
const GmIso4 &CGameCtnReplayArchiveStaticModel::WorldIso() const {
    return worldIso_;
}

const std::optional<CHmsItem::Properties> &
CGameCtnReplayArchiveStaticModel::ItemProperties() const {
    return itemProperties_;
}

void CGameCtnReplayArchiveStaticModel::BindPrototype(
        StaticSolidPrototype prototype) {
    prototype_ = std::move(prototype);
}

const StaticSolidPrototype &
CGameCtnReplayArchiveStaticModel::Prototype() const {
    return prototype_;
}

bool StaticSceneArchiveModelSource::IsWarp() const {
    return sceneObjectId == "Warp";
}

int StaticSceneArchiveModelRecord::ConfigureFromBlockInfoMobil(
        const char *blockName,
        CGameCtnBlockInfo *blockInfo,
        CSceneMobil *mobil,
        const StaticSolidArchiveReferenceCatalog &references) {
    if (blockInfo == nullptr || mobil == nullptr ||
        !mobil->StaticSolidAsset().IsSpecified()) {
        return 0;
    }
    source.selectedDescriptorPath =
            references.SelectedDescriptorPath(
                    mobil->StaticSolidAsset());
    model.worldIso_.SetIdentity();
    model.itemProperties_ = mobil->InitialItemProperties();

    const std::string &modelName =
            references.ModelName(mobil->StaticSolidAsset());
    if (blockName != nullptr &&
        std::string(blockName) == "StadiumCircuitBase") {
        const std::optional<GmIso4> &groundSpawn =
                blockInfo->SpawnLocationForMobilFamily(true);
        if (modelName == "BaseGround.Solid.Gbx" &&
            groundSpawn.has_value()) {
            model.worldIso_.translation.y = groundSpawn->translation.y;
        }
    }
    return !source.selectedDescriptorPath.empty();
}

int StaticSceneArchiveModelRecord::ConfigureFromSceneObject(
        const float sceneIso[12],
        u32 treeNodeIndex,
        const char *sceneObjectId,
        const char *selectedDescriptorPath,
        const CHmsItem::Properties *itemProperties) {
    if (sceneIso == nullptr || selectedDescriptorPath == nullptr ||
        selectedDescriptorPath[0] == '\0') {
        return 0;
    }
    model.worldIso_ = GmWire::DecodeIso4(sceneIso);
    source.selectedDescriptorPath = selectedDescriptorPath;
    source.sceneObjectId = sceneObjectId != nullptr ? sceneObjectId : "";
    if (itemProperties != nullptr) {
        model.itemProperties_ = *itemProperties;
    } else {
        model.itemProperties_.reset();
    }
    if (treeNodeIndex != UINT32_MAX) {
        source.treeNodeIndex = treeNodeIndex;
    }
    return 1;
}

void CGameCtnReplayArchiveStaticModelCollection::Free() {
    records.clear();
}

u32 CGameCtnReplayArchiveStaticModelCollection::Count() const {
    return records.size() <= UINT32_MAX
            ? static_cast<u32>(records.size())
            : UINT32_MAX;
}

int CGameCtnReplayArchiveStaticModelCollection::ResizePrefix(u32 count) {
    if (count > records.size()) {
        return 0;
    }
    records.resize(count);
    return 1;
}

int CGameCtnReplayArchiveStaticModelCollection::
ContributeStaticSolidDescriptorDependencies(
        CGameCtnReplayStaticSolidDescriptorDependencyQueue &queue) const {
    for (const StaticSceneArchiveModelRecord &record : records) {
        if (!queue.RequireDescriptor(
                    record.source.selectedDescriptorPath.c_str())) {
            return 0;
        }
    }
    return 1;
}

int CGameCtnReplayArchiveStaticModelCollection::Append(
        const StaticSceneArchiveModelRecord &record) {
    try {
        records.push_back(record);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

int CGameCtnReplayArchiveStaticModelCollection::AppendDecodedSceneObject(
        const StaticSceneArchiveModelRecord &record) {
    return Append(record);
}

int CGameCtnReplayArchiveStaticModelCollection::AppendIfDescriptorMissing(
        const StaticSceneArchiveModelRecord &record) {
    return !record.source.selectedDescriptorPath.empty() &&
           (HasDescriptor(record.source.selectedDescriptorPath.c_str()) ||
            Append(record));
}

bool CGameCtnReplayArchiveStaticModelCollection::HasDescriptor(
        const char *selectedDescriptorPath) const {
    if (selectedDescriptorPath == nullptr ||
        selectedDescriptorPath[0] == '\0') {
        return false;
    }
    for (const StaticSceneArchiveModelRecord &record : records) {
        if (record.source.selectedDescriptorPath == selectedDescriptorPath) {
            return true;
        }
    }
    return false;
}

bool LoadCatalogArchiveStaticModels(
        InstalledPackAssetRepository &assets,
        CGameCtnReplayArchiveStaticModelCollection &archiveModels,
        CatalogArchiveStaticModelUsage usage) {
    static constexpr const char *BlockName = "StadiumCircuitBase";
    static constexpr const char *CollectionName = "Stadium";

    archiveModels.Free();
    const CPlugFilePack *pack =
            InstalledPackAssetRepositoryAccess::Pack(assets);
    if (pack == nullptr) {
        return false;
    }
    if (pack->PackName() != CollectionName) {
        return true;
    }
    const BlockInfoCatalog *catalog = assets.Catalog();
    const BlockInfoCatalogEntry *entry = catalog != nullptr
            ? catalog->FindForIdentifierAndCollection(BlockName,
                                                      CollectionName)
            : nullptr;
    CGameCtnBlockInfo *blockInfo = entry != nullptr
            ? assets.ArchiveBlockInfo(entry->asset)
            : nullptr;
    if (blockInfo == nullptr) {
        return false;
    }
    const StaticSolidArchiveReferenceCatalog &references =
            InstalledPackAssetRepositoryAccess::StaticSolidReferences(assets);
    for (const CMwNodRef<CSceneMobil> &mobil : blockInfo->SourceMobils()) {
        if (mobil.Get() == nullptr ||
            !references.IsLoadable(
                    mobil->StaticSolidAsset()) ||
            !SceneDescriptorFolderPaths::IsMediaSolidPath(
                    references.PlainPackPath(
                            mobil->StaticSolidAsset()).c_str())) {
            continue;
        }
        StaticSceneArchiveModelRecord record;
        record.dependencyOnly =
                usage == CatalogArchiveStaticModelUsage::DependencyOnly;
        if (!record.ConfigureFromBlockInfoMobil(
                    BlockName, blockInfo, mobil.Get(), references) ||
            !archiveModels.AppendIfDescriptorMissing(record)) {
            archiveModels.Free();
            return false;
        }
    }
    return true;
}
