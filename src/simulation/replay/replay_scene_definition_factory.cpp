#include "simulation/replay/replay_scene_definition_factory.h"
#include <cstdio>
#include <optional>
#include <string>

#include "engine/resources/block_info_catalog.h"
#include "engine/game/game_ctn_block_info.h"
#include "simulation/replay/replay_scene_surface_resolution.h"
#include "simulation/replay/replay_terrain_top_markers.h"
namespace {

const char *BlockTypeName(EBlockType type) {
    switch (type) {
        case EBlockType::Flat: return "flat";
        case EBlockType::Frontier: return "frontier";
        case EBlockType::Classic: return "classic";
        case EBlockType::Road: return "road";
        case EBlockType::Clip: return "clip";
        case EBlockType::Pylon: return "pylon";
        case EBlockType::Slope: return "slope";
        case EBlockType::RectAsym: return "rect-asym";
    }
    return "unknown";
}

std::string ResolveCollectionName(
        const CGameCtnReplayMapInput &mapInput,
        const BlockInfoCatalog &catalog) {
    std::string collection = mapInput.DefaultCollectionName();
    if (collection.empty() || collection.size() >= 64u) {
        collection = catalog.CollectionNameForMapInput(mapInput)
                             .value_or(std::string{});
    }
    if (collection.size() >= 64u) {
        collection.clear();
    }
    return collection;
}

bool PrepareBlockInfo(
        CGameCtnBlockInfo &blockInfo,
        const CGameCtnReplayMapInputBlock *placement,
        const BlockInfoCatalogEntry &catalogEntry,
        const BlockPlacementState &placementState) {
    blockInfo.ConfigurePlacement(catalogEntry.blockType,
                                 placementState.UsesCustomSize(),
                                 placementState.Source());
    if (placement != nullptr) {
        blockInfo.identifier.id.SetLocalName(
                placement->Identifier().Name().c_str());
    } else {
        blockInfo.identifier.id.SetLocalName(catalogEntry.identifier.c_str());
    }
    blockInfo.OnNodLoaded();
    return true;
}

class ReplaySceneDefinitionFactory {
public:
    ReplaySceneDefinitionFactory(const CGameCtnReplayMapInput &mapInput,
                                 CatalogAssetRepository &assets,
                                 ReplaySceneDefinition &scene)
            : mapInput_(mapInput), assets_(assets), scene_(scene) {}

    bool Build();

private:
    bool LoadCollection();
    bool LoadAuthoredBlocks();
    bool BuildSpatialSources();
    bool ResolveClipSources();
    bool ResolveMaterialRemaps();
    bool Fail(const char *reason,
              const CGameCtnReplayMapInputBlock *placement = nullptr,
              const BlockInfoCatalogEntry *catalogEntry = nullptr);

    const CGameCtnReplayMapInput &mapInput_;
    CatalogAssetRepository &assets_;
    ReplaySceneDefinition &scene_;
    const BlockInfoCatalog *catalog_ = nullptr;
    std::string collectionName_;
    TerrainTopMarkers terrainMarkers_;
    CollectionColumnSurfaces columnSurfaces_;
    ReplaySceneUnits units_;
};

bool ReplaySceneDefinitionFactory::Fail(
        const char *reason,
        const CGameCtnReplayMapInputBlock *placement,
        const BlockInfoCatalogEntry *catalogEntry) {
    std::fprintf(stderr,
                 "replay scene definition failed reason=%s placement=%u "
                 "block=%s collection=%s type=%s variant=%u family=%s\n",
                 reason != nullptr ? reason : "unspecified",
                 placement != nullptr ? placement->Id().Ordinal() : UINT32_MAX,
                 placement != nullptr
                         ? placement->Identifier().Name().c_str()
                         : "",
                 collectionName_.c_str(),
                 catalogEntry != nullptr
                         ? BlockTypeName(catalogEntry->blockType)
                         : "unknown",
                 placement != nullptr
                         ? placement->Placement().VariantIndex()
                         : 0u,
                 placement != nullptr &&
                                 placement->Placement()
                                         .UsesGroundMobilFamily()
                         ? "ground"
                         : "air");
    scene_.Clear();
    return false;
}

bool ReplaySceneDefinitionFactory::LoadCollection() {
    collectionName_ = ResolveCollectionName(mapInput_, *catalog_);
    if (collectionName_.empty()) {
        return true;
    }
    std::optional<CatalogCollectionDefinition> collection =
            assets_.Collection(collectionName_);
    if (!collection) {
        return Fail("collection");
    }
    scene_.SetCollection(std::move(*collection));
    return true;
}

bool ReplaySceneDefinitionFactory::LoadAuthoredBlocks() {
    for (const CGameCtnReplayMapInputBlock &placement : mapInput_.Blocks()) {
        bool ambiguous = false;
        const BlockInfoCatalogEntry *catalogEntry =
                catalog_->FindForBlock(placement, &ambiguous);
        if (catalogEntry == nullptr || !catalogEntry->asset.IsValid()) {
            return Fail(ambiguous ? "ambiguous-block" : "unknown-block",
                        &placement,
                        catalogEntry);
        }
        CGameCtnBlockInfo *blockInfo = assets_.BlockInfo(catalogEntry->asset);
        if (blockInfo == nullptr ||
            !PrepareBlockInfo(*blockInfo,
                              &placement,
                              *catalogEntry,
                              placement.Placement())) {
            return Fail("block-asset", &placement, catalogEntry);
        }
        auto definition = ReplaySceneBlockDefinition::Authored(
                placement, *catalogEntry, collectionName_, *blockInfo);
        if (!definition) {
            return Fail("block-definition", &placement, catalogEntry);
        }
        if (definition->IsClip() &&
            !definition->InstallDefaultClipSourceSides(assets_.Mobil(
                    catalogEntry->asset,
                    placement.Placement().UsesGroundMobilFamily(),
                    placement.Placement().VariantIndex()))) {
            return Fail("clip-source", &placement, catalogEntry);
        }
        if (!scene_.Add(std::move(*definition))) {
            return Fail("block-storage", &placement, catalogEntry);
        }
    }
    return true;
}

bool ReplaySceneDefinitionFactory::BuildSpatialSources() {
    if (!terrainMarkers_.BuildFromSceneDefinition(scene_, mapInput_)) {
        return Fail("terrain-markers");
    }
    for (const ReplaySceneBlockDefinition &definition : scene_.Blocks()) {
        if (!definition.IsAuthored()) {
            continue;
        }
        const CGameCtnReplayMapInputBlock *placement =
                mapInput_.FindBlock(*definition.PlacementId());
        if (placement == nullptr ||
            !columnSurfaces_.AddBlockSurfaces(
                    *placement, definition, definition.BlockInfo()) ||
            !units_.AddBlockUnitSources(*placement,
                                        definition,
                                        definition.BlockInfo(),
                                        &terrainMarkers_)) {
            return Fail("spatial-sources", placement);
        }
    }
    return true;
}

bool ReplaySceneDefinitionFactory::ResolveClipSources() {
    ReplaySceneAssetResolver resolver;
    resolver.assets = &assets_;
    return units_.ApplyClipNeighbourSourceSides(
                   mapInput_, resolver, scene_) ||
           Fail("clip-neighbours");
}

bool ReplaySceneDefinitionFactory::ResolveMaterialRemaps() {
    ReplaySceneAssetResolver assetResolver;
    assetResolver.assets = &assets_;
    for (ReplaySceneBlockDefinition &definition : scene_.MutableBlocks()) {
        const CGameCtnReplayMapInputBlock *placement =
                definition.PlacementId()
                        ? mapInput_.FindBlock(*definition.PlacementId())
                        : nullptr;
        ReplaySceneSurfaceResolver resolver;
        resolver.assets = &assets_;
        resolver.collectionName = collectionName_;
        resolver.scene = &scene_;
        resolver.units = &units_;
        resolver.columnSurfaces = &columnSurfaces_;
        resolver.block = placement;
        resolver.definition = &definition;
        resolver.assetResolver = &assetResolver;
        int remapResolved = 1;
        const bool replacement =
                resolver.ReplacementRemapApplies(&remapResolved) != 0;
        definition.SetMaterialRemaps(remapResolved && replacement, false);
    }
    return true;
}

bool ReplaySceneDefinitionFactory::Build() {
    scene_.Clear();
    catalog_ = assets_.Catalog();
    if (catalog_ == nullptr || !scene_.Reserve(mapInput_.BlockCount())) {
        return Fail("catalog");
    }
    return LoadCollection() && LoadAuthoredBlocks() &&
           BuildSpatialSources() && ResolveClipSources() &&
           ResolveMaterialRemaps() &&
           (scene_.BlockCount() == mapInput_.BlockCount() ||
            Fail("block-coverage"));
}

}  // namespace

bool BuildReplaySceneDefinition(
        const CGameCtnReplayMapInput &mapInput,
        CatalogAssetRepository &assets,
        ReplaySceneDefinition &scene) {
    return ReplaySceneDefinitionFactory(mapInput, assets, scene).Build();
}

bool ReplaySceneDefinition::AppendAutomaticBase(
        const CGameCtnReplayMapInput &mapInput,
        CatalogAssetRepository &assets) {
    if (AutomaticBaseBlock() != nullptr ||
        DefaultZoneKind() != CatalogConstructionZoneKind::Flat) {
        return AutomaticBaseBlock() != nullptr;
    }
    const std::optional<std::string> identifier = DefaultBaseIdentifier();
    const BlockInfoCatalog *catalog = assets.Catalog();
    if (!identifier || !Collection() || catalog == nullptr) {
        return false;
    }
    const BlockInfoCatalogEntry *entry =
            catalog->FindForIdentifierAndCollection(*identifier,
                                                    Collection()->name);
    CGameCtnBlockInfo *blockInfo = entry != nullptr
            ? assets.BlockInfo(entry->asset)
            : nullptr;
    const BlockPlacementState placement = BlockPlacementState::AutomaticBase();
    if (entry == nullptr || blockInfo == nullptr ||
        !PrepareBlockInfo(*blockInfo, nullptr, *entry, placement)) {
        return false;
    }
    auto definition = ReplaySceneBlockDefinition::AutomaticBase(
            *entry, *blockInfo);
    if (!definition) {
        return false;
    }
    if (definition->IsClip() &&
        !definition->InstallDefaultClipSourceSides(
                assets.Mobil(entry->asset,
                             true,
                             placement.VariantIndex()))) {
        return false;
    }
    (void)mapInput;
    return Add(std::move(*definition));
}
