#include "format/pack/block_info_catalog/block_info_asset_store.h"

#include <utility>

#include "format/pack/block_info_catalog/blockinfo_descriptor_mobil_parser.h"
#include "format/pack/installed/byte_buffer.h"
#include "engine/game/game_ctn_block_info.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/static_solid/static_solid_archive_reference.h"
#include "format/archive/gm_wire_conversion.h"
#include "format/archive/hms_item_archive_state.h"
#include "format/archive/mw_id_archive_codec.h"
#include <new>
namespace {

GmNat3 NaturalSize(const GmInt3 &size) {
    return {static_cast<u32>(size.x),
            static_cast<u32>(size.y),
            static_cast<u32>(size.z)};
}

std::unique_ptr<CGameCtnBlockUnitInfo> MaterializeUnit(
        const BlockInfoDescriptorUnitDefinition &source,
        CGameCtnBlockInfo &owner,
        BlockInfoAssetStore &assets) {
    auto unit = std::make_unique<CGameCtnBlockUnitInfo>();
    unit->InitializeUnitFields(
            {static_cast<u32>(source.offset.x),
             static_cast<u32>(source.offset.y),
             static_cast<u32>(source.offset.z)},
            0u,
            0u,
            &owner);
    unit->SetUnderground(source.underground);
    CMwId terrainModifierId;
    if (!TmnfFormat::CMwIdArchiveCodec::Materialize(
                source.terrainModifierId,
                terrainModifierId)) {
        return nullptr;
    }
    unit->SetTerrainModifierId(terrainModifierId);
    if (!source.surfaceIdName.empty()) {
        unit->SetSurfaceIdName(source.surfaceIdName.c_str());
    }
    for (u32 side = 0u; side < source.sourceDescriptorPaths.size(); ++side) {
        if (source.sourceDescriptorPaths[side].empty()) {
            continue;
        }
        const BlockInfoAssetHandle sourceAsset =
                assets.Register(source.sourceDescriptorPaths[side]);
        if (!sourceAsset.IsValid()) {
            return nullptr;
        }
        CMwNodRef<CGameCtnBlockInfoClip> clip =
                MakeMwNod<CGameCtnBlockInfoClip>();
        clip->SetSourceAsset(sourceAsset);
        unit->SetJunction(static_cast<ECardinalDir>(side), clip.Get());
    }
    return unit;
}

StaticSolidAssetReference MobilStaticSolid(
        const BlockInfoParsedMobil &model,
        StaticSolidArchiveReferenceCatalog &references) {
    return references.RegisterMobilSource(
            model.modelName,
            model.plainPackPath,
            model.hashedDescriptorPath,
            model.loadable != 0u);
}

void InstallInitialPhysicsState(
        CSceneMobil &mobil,
        const BlockInfoParsedMobil &model) {
    if (model.hasHmsItemState != 0u) {
        const CHmsItem::Properties properties = DecodeHmsItemArchiveState({
                model.hmsItemPhysicsFlags,
                model.hmsItemRenderingFlags,
                model.hmsItemVisibilityFlags});
        mobil.SetInitialItemProperties(properties);
    }
}

CSceneMobil *NewMobil(StaticSolidArchiveReferenceCatalog &references,
                      const BlockInfoParsedMobil *model = nullptr) {
    CSceneMobil *mobil = new CSceneMobil();
    mobil->SetSolid(nullptr);
    if (model != nullptr) {
        mobil->StaticSolidAsset() = MobilStaticSolid(*model, references);
        InstallInitialPhysicsState(*mobil, *model);
    }
    return mobil;
}

CSceneMobil *PrimaryMobilForModel(
        CGameCtnBlockInfo &blockInfo,
        const BlockInfoParsedMobil &model) {
    if (model.selectorGroup > 1u) {
        return nullptr;
    }
    return blockInfo.GetMobil(
            static_cast<int>(model.selectorGroup),
            model.variantIndex,
            model.mobilIndex);
}

void LinkMobil(
        CSceneMobil &parent,
        CSceneMobil &child,
        const BlockInfoParsedMobil &model) {
    if (model.isRaceTriggerMobil != 0u) {
        child.id.SetLocalName("TriggerCheckpoint");
    }
    CSceneObjectLink *link = nullptr;
    parent.AddObject(&child, link);
    if (link != nullptr && model.hasSceneLinkIso4 != 0u) {
        link->SetRelativeLocation(GmWire::DecodeIso4(
                model.sceneLinkIso4));
    }
}

void MaterializeMobilFamilies(
        CGameCtnBlockInfo &blockInfo,
        const BlockInfoDescriptorParseResult &descriptor,
        StaticSolidArchiveReferenceCatalog &references) {
    for (int family = 0; family <= 1; ++family) {
        unsigned long variantCount = 0u;
        for (unsigned long variant = 0u; variant < 64u; ++variant) {
            const u32 count = family == 0
                    ? descriptor.counts.airVariantCount[variant]
                    : descriptor.counts.groundVariantCount[variant];
            if (count != 0u) {
                variantCount = variant + 1u;
            }
        }
        blockInfo.ResetMobilVariants(family, variantCount);
        for (unsigned long variant = 0u; variant < variantCount; ++variant) {
            const u32 count = family == 0
                    ? descriptor.counts.airVariantCount[variant]
                    : descriptor.counts.groundVariantCount[variant];
            for (u32 choice = 0u; choice < count; ++choice) {
                blockInfo.AddMobil(family, variant, NewMobil(references));
            }
        }
    }
}

void MaterializeModel(
        CGameCtnBlockInfo &blockInfo,
        const BlockInfoParsedMobil &model,
        StaticSolidArchiveReferenceCatalog &references) {
    CSceneMobil *mobil = nullptr;
    if (model.isSceneObjectLinkMobil == 0u) {
        mobil = PrimaryMobilForModel(blockInfo, model);
        if (mobil != nullptr && mobil->StaticSolidAsset().IsSpecified()) {
            mobil = nullptr;
        }
    }
    if (mobil == nullptr) {
        mobil = NewMobil(references, &model);
    } else {
        mobil->StaticSolidAsset() = MobilStaticSolid(model, references);
        InstallInitialPhysicsState(*mobil, model);
    }

    blockInfo.AddSourceMobil(mobil);
    if (model.isSceneObjectLinkMobil != 0u) {
        CSceneMobil *parent = PrimaryMobilForModel(blockInfo, model);
        if (parent != nullptr) {
            LinkMobil(*parent, *mobil, model);
        }
    }
    if (model.selectorGroup == 2u &&
        blockInfo.FamilyHelperMobilSource(true) == nullptr) {
        blockInfo.SetFamilyHelperMobilSource(true, mobil);
    } else if (model.selectorGroup == 3u &&
               blockInfo.FamilyHelperMobilSource(false) == nullptr) {
        blockInfo.SetFamilyHelperMobilSource(false, mobil);
    } else if (model.selectorGroup == 4u &&
               blockInfo.CommonHelperMobilSource() == nullptr) {
        blockInfo.SetCommonHelperMobilSource(mobil);
    }
}

CSceneMobil *MaterializeHelperPlaceholder(
        CSceneMobil *existing,
        const std::string &archivePath,
        StaticSolidArchiveReferenceCatalog &references) {
    if (existing != nullptr || archivePath.empty()) {
        return existing;
    }
    CSceneMobil *mobil = NewMobil(references);
    mobil->StaticSolidAsset() = references.RegisterArchivePath(archivePath);
    return mobil;
}

CMwNodRef<CGameCtnBlockInfo> MaterializeBlockInfo(
        const BlockInfoDescriptorParseResult &descriptor,
        const BlockInfoParsedMobilCollection &models,
        BlockInfoAssetStore &assets,
        StaticSolidArchiveReferenceCatalog &references) {
    CMwNodRef<CGameCtnBlockInfo> blockInfo =
            MakeMwNod<CGameCtnBlockInfo>();
    blockInfo->SetMobilFamilySize(true,
                                 NaturalSize(descriptor.SizeWhenGround()));
    blockInfo->SetMobilFamilySize(false,
                                 NaturalSize(descriptor.SizeWhenAir()));
    blockInfo->SetRespawnUsesCurrentTransform(
            descriptor.blockInfoRespawnUsesCurrentTransform != 0u);
    if (descriptor.hasBlockInfoIso4A != 0u) {
        blockInfo->SetSpawnLocationForMobilFamily(
                true, GmWire::DecodeIso4(descriptor.blockInfoIso4A));
    }
    if (descriptor.hasBlockInfoIso4B != 0u) {
        blockInfo->SetSpawnLocationForMobilFamily(
                false, GmWire::DecodeIso4(descriptor.blockInfoIso4B));
    }
    for (const BlockInfoDescriptorUnitDefinition &source :
         descriptor.UnitDefinitionsForFieldUnits(true)) {
        auto unit = MaterializeUnit(source, *blockInfo, assets);
        if (unit == nullptr) {
            return {};
        }
        blockInfo->AddBlockUnitInfo(true, std::move(unit));
    }
    for (const BlockInfoDescriptorUnitDefinition &source :
         descriptor.UnitDefinitionsForFieldUnits(false)) {
        auto unit = MaterializeUnit(source, *blockInfo, assets);
        if (unit == nullptr) {
            return {};
        }
        blockInfo->AddBlockUnitInfo(false, std::move(unit));
    }

    MaterializeMobilFamilies(*blockInfo, descriptor, references);
    for (u32 index = 0u; index < models.Count(); ++index) {
        const BlockInfoParsedMobil *model = models.ModelAt(index);
        if (model != nullptr &&
            model->descriptorPath == descriptor.descriptorPath) {
            MaterializeModel(*blockInfo, *model, references);
        }
    }
    blockInfo->SetFamilyHelperMobilSource(
            true,
            MaterializeHelperPlaceholder(
                    blockInfo->FamilyHelperMobilSource(true),
                    descriptor.helperGroundDescriptorPath,
                    references));
    blockInfo->SetFamilyHelperMobilSource(
            false,
            MaterializeHelperPlaceholder(
                    blockInfo->FamilyHelperMobilSource(false),
                    descriptor.helperAirDescriptorPath,
                    references));
    blockInfo->SetCommonHelperMobilSource(
            MaterializeHelperPlaceholder(
                    blockInfo->CommonHelperMobilSource(),
                    descriptor.helperCommonDescriptorPath,
                    references));
    return blockInfo;
}

}  // namespace

BlockInfoAssetStore::BlockInfoAssetStore(
        CPlugFilePack &sourcePack,
        StaticSolidArchiveReferenceCatalog &sourceReferences)
        : pack(sourcePack), solidReferences(sourceReferences) {}

BlockInfoAssetHandle BlockInfoAssetStore::Register(
        std::string_view archivePath) {
    if (archivePath.empty()) {
        return {};
    }
    for (u32 index = 0u; index < assets.size(); ++index) {
        if (assets[index].archivePath == archivePath) {
            return HandleForStorageIndex(index);
        }
    }
    if (assets.size() >= UINT32_MAX) {
        return {};
    }
    try {
        Asset asset;
        asset.archivePath.assign(archivePath.data(), archivePath.size());
        assets.push_back(std::move(asset));
        return HandleForStorageIndex(
                static_cast<u32>(assets.size() - 1u));
    } catch (const std::bad_alloc &) {
        return {};
    }
}

CGameCtnBlockInfo *BlockInfoAssetStore::Load(
        BlockInfoAssetHandle asset,
        bool includeArchiveModels) {
    lastLoadFailureReason = "none";
    if (!asset.IsValid() || StorageIndex(asset) >= assets.size()) {
        lastLoadFailureReason = "unknown-asset";
        return nullptr;
    }
    const u32 index = StorageIndex(asset);
    CMwNodRef<CGameCtnBlockInfo> &loaded = includeArchiveModels
            ? assets[index].archiveBlockInfo
            : assets[index].blockInfo;
    if (loaded.Get() != nullptr) {
        return loaded.Get();
    }

    const std::string archivePath = assets[index].archivePath;
    ByteBuffer bytes;
    if (!pack.ExtractPath(archivePath.c_str(), &bytes) ||
        bytes.Empty() || bytes.Size() > UINT32_MAX) {
        lastLoadFailureReason = "descriptor-bytes";
        return nullptr;
    }
    const BlockInfoDescriptorParseRequest parseRequest{
        bytes.Data(),
        static_cast<u32>(bytes.Size()),
        archivePath.c_str(),
        &pack,
        includeArchiveModels,
    };
    auto parsed = BlockInfoDescriptorMobilParser::ParseDescriptorAndModels(
            parseRequest);
    if (!parsed.has_value()) {
        lastLoadFailureReason = "descriptor-parse";
        return nullptr;
    }
    if (!includeArchiveModels) {
        BlockInfoDescriptorMobilParser::TryParseHelperPaths(
                parseRequest, parsed->descriptor);
    }
    CMwNodRef<CGameCtnBlockInfo> blockInfo =
            MaterializeBlockInfo(parsed->descriptor,
                                 parsed->models,
                                 *this,
                                 solidReferences);
    if (blockInfo.Get() == nullptr) {
        lastLoadFailureReason = "descriptor-assets";
        return nullptr;
    }
    loaded = std::move(blockInfo);
    return loaded.Get();
}

CSceneMobil *BlockInfoAssetStore::Mobil(
        BlockInfoAssetHandle asset,
        bool ground,
        u32 variant) {
    CGameCtnBlockInfo *blockInfo = Load(asset);
    return blockInfo != nullptr
            ? blockInfo->GetMobil(ground ? 1 : 0, variant, 0u)
            : nullptr;
}

std::optional<std::string> BlockInfoAssetStore::FirstGroundSurface(
        BlockInfoAssetHandle asset) {
    CGameCtnBlockInfo *blockInfo = Load(asset);
    if (blockInfo == nullptr || !blockInfo->HasBlockUnitInfos(true)) {
        return std::nullopt;
    }
    const char *name = blockInfo->FirstBlockUnitInfo(1)->SurfaceIdName();
    return name != nullptr && name[0] != '\0'
            ? std::optional<std::string>(name)
            : std::nullopt;
}

const char *BlockInfoAssetStore::LastLoadFailureReason() const {
    return lastLoadFailureReason;
}
