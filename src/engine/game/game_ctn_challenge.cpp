#include "engine/game/game_ctn_challenge.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "engine/core/binary32_math.h"
#include "engine/game/game_app.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/game/game_ctn_block_unit.h"
#include "engine/game/game_ctn_catalog.h"
#include "engine/game/game_ctn_collection.h"
#include "engine/game/game_ctn_collector.h"
#include "engine/game/game_ctn_decoration.h"
#include "engine/game/game_ctn_field_unit.h"
#include "engine/game/game_ctn_pylon_column.h"
#include "engine/game/game_ctn_utils.h"
#include "engine/game/game_ctn_types.h"
#include "engine/physics/geometry/geometry_helpers.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/scene/plug_solid.h"
#include "engine/physics/geometry/plug_surface.h"
#include "engine/rendering/plug_tree.h"
#include "engine/scene/scene_mobil.h"
#include "engine/resources/system_fid_parameter_types.h"
#include "engine/resources/system_pack_desc.h"
SCtnForcedMods::SEnvMod::SEnvMod(void) = default;
SCtnForcedMods::SEnvMod::~SEnvMod(void) = default;
SCtnForcedMods::~SCtnForcedMods(void) = default;

CGameCtnChallenge::CGameCtnChallenge(void) = default;

void CGameCtnChallenge::SetMapSize(const GmNat3 &size) {
    mapWidth = size.x;
    mapHeight = size.y;
    mapDepth = size.z;
}

GmNat3 CGameCtnChallenge::MapSize(void) const {
    return {mapWidth, mapHeight, mapDepth};
}

u32 CGameCtnChallenge::MapHeight(void) const {
    return mapHeight;
}

void CGameCtnChallenge::FastInitChallengeData(
        CGameCtnCollection *newCollection,
        CGameCtnDecoration *newDecoration) {
    collection = newCollection;
    decoration = newDecoration;
    if (decoration != nullptr) {
        mapWidth = decoration->Size().mapWidth;
        mapHeight = decoration->Size().mapHeight;
        mapDepth = decoration->Size().mapDepth;
        defaultZoneHeight = decoration->Size().defaultZoneHeight;
    }
    InitializeFieldUnitGrid();
    InitializeZoneGrid(CMwId{}, defaultZoneHeight, true);
    pylonColumns.Reset(mapWidth, mapDepth);
}

void CGameCtnChallenge::InitChallengeData(const SCtnForcedMods *forcedMods) {
    LoadDecorationAndCollection(forcedMods);
    FastInitChallengeData(collection, decoration);
    for (const std::unique_ptr<CGameCtnBlock> &block : blocks_) {
        if (block != nullptr) {
            UpdateFieldUnits(block.get());
            CreateMobilForBlock(block.get());
        }
    }
    CreateBlocksAndPylons();
}

CGameCtnBlock *CGameCtnChallenge::CreateBlock(
        CGameCtnBlockInfo *blockInfo,
        GmNat3 coord,
        ECardinalDir direction,
        unsigned long variantOrSource,
        int isGround,
        int createMobilFlag,
        CGameCtnBlockSkin *skin) {
    if (blockInfo == nullptr) {
        return nullptr;
    }
    auto block = std::make_unique<CGameCtnBlock>(
            blockInfo,
            coord,
            direction,
            variantOrSource,
            isGround,
            createMobilFlag,
            skin);
    return AddLoadedBlock(std::move(block));
}

CGameCtnBlock *CGameCtnChallenge::AddLoadedBlock(
        std::unique_ptr<CGameCtnBlock> block) {
    if (block == nullptr || block->BlockInfoRef() == nullptr) {
        return nullptr;
    }
    CGameCtnBlock *created = block.get();
    blocks_.push_back(std::move(block));
    if (!fieldUnitColumns.empty()) {
        UpdateFieldUnits(created);
    }
    CreateMobilForBlock(created);
    return created;
}

void CGameCtnChallenge::CreateMobilForBlock(CGameCtnBlock *block) {
    if (block == nullptr) {
        return;
    }
    if (block->GetType() == EBlockType::Clip) {
        CreateMobilForClip(block);
        return;
    }
    block->CreateBlockMobil(0, 0);
    AddBlockToAddList(block);
}

void CGameCtnChallenge::CreateMobilForClip(CGameCtnBlock *block) {
    if (block != nullptr) {
        block->CreateBlockMobil(0, 0);
        AddBlockToAddList(block);
    }
}

CGameCtnBlockInfo &CGameCtnChallenge::OwnBlockInfo(
        std::unique_ptr<CGameCtnBlockInfo> blockInfo) {
    CGameCtnBlockInfo *owned = blockInfo.release();
    ownedBlockInfos_.emplace_back(owned);
    return *owned;
}

const std::vector<std::unique_ptr<CGameCtnBlock>> &
CGameCtnChallenge::Blocks(void) const {
    return blocks_;
}

void CGameCtnChallenge::ClearConstructedBlocks(void) {
    blocksToAdd.clear();
    blocks_.clear();
    ownedBlockInfos_.clear();
}

CGameCtnBlockInfo *CGameCtnChallenge::GetDescFromBlock(CGameCtnBlock *block) {
    return block->BlockInfoRef();
}

uint8_t CGameCtnChallenge::GetNeighbourPylonIndex(uint8_t index) {
    // Each pylon side maps to the matching side on its neighbour.
    static const uint8_t neighbourIndexByPylonIndex[8] = {
        5u, 4u, 7u, 6u, 1u, 0u, 3u, 2u,
    };
    return index < 8u ? neighbourIndexByPylonIndex[index] : 0xffu;
}

CGameCtnPylonColumn * CGameCtnChallenge::GetColumnNorth(GmNat3 coord) {
    return pylonColumns.North(coord);
}


CGameCtnPylonColumn * CGameCtnChallenge::GetColumnSouth(GmNat3 coord) {
    return pylonColumns.South(coord);
}


CGameCtnPylonColumn * CGameCtnChallenge::GetColumnEast(GmNat3 coord) {
    return pylonColumns.East(coord);
}


CGameCtnPylonColumn * CGameCtnChallenge::GetColumnWest(GmNat3 coord) {
    return pylonColumns.West(coord);
}


CGameCtnPylonColumn * CGameCtnChallenge::GetColumn(
        GmNat3 coord,
        ECardinalDir direction,
        int isEvenSide) {
    const bool evenSide = isEvenSide != 0;
    switch (direction) {
    case ECardinalDir::North:
        coord.z += 1u;
        return evenSide ? GetColumnNorth(coord) : GetColumnSouth(coord);
    case ECardinalDir::South:
        return evenSide ? GetColumnNorth(coord) : GetColumnSouth(coord);
    case ECardinalDir::East:
        coord.x += 1u;
        return evenSide ? GetColumnWest(coord) : GetColumnEast(coord);
    case ECardinalDir::West:
        return evenSide ? GetColumnWest(coord) : GetColumnEast(coord);
    default:
        return evenSide ? GetColumnWest(coord) : GetColumnEast(coord);
    }
}


CGameCtnPylonColumn * CGameCtnChallenge::GetColumn(
        GmNat3 coord,
        unsigned long pylonIndex) {
    return GetColumn(
            coord,
            static_cast<ECardinalDir>(pylonIndex >> 1u),
            (pylonIndex & 1u) == 0u);
}

namespace {

void RaisePylonVisualProvider(
        CPlugVisual *provider,
        uint32_t variantIndex) {
    if (provider != nullptr) {
        if (auto *visual3D = dynamic_cast<CPlugVisual3D *>(provider)) {
            visual3D->TranslatePylonGeometry(variantIndex, SQUARE_HEIGHT);
        }
    }
}

void RaisePylonMeshSurface(
        CPlugSurface *surface,
        uint32_t variantIndex) {
    if (surface == nullptr) {
        return;
    }
    const float squareHeight = SQUARE_HEIGHT;
    const float delta =
            squareHeight * Binary32::FromUnsignedInteger(variantIndex);
    surface->TranslateMeshVerticesAbove(squareHeight * 0.5f, delta);
}

}  // namespace

CSceneMobil *CGameCtnChallenge::CreateNewPylonMobil(
        CGameCtnBlockInfoPylon *pylonBlockInfo,
        unsigned long variantIndex) {
    /*
     * clone the pylon source mobil, disconnect its solid/tree from the model,
     * duplicate and height-shift visual providers and mesh surfaces, then mark
     * the result as a static pylon mobil and install a cloned solid.
     */
    if (pylonBlockInfo == nullptr ||
        pylonBlockInfo->MiddleSourceMobil() == nullptr) {
        return nullptr;
    }
    CSceneMobil *sourceMobil = pylonBlockInfo->MiddleSourceMobil();
    std::unique_ptr<CSceneMobil> mobil(sourceMobil->CreateModelInstance());
    if (mobil == nullptr) {
        return nullptr;
    }

    CHmsItem *item = mobil->HmsItem();
    if (item == nullptr || item->Solid() == nullptr) {
        return nullptr;
    }

    mobil->DisconnectFromModel();
    item->Solid()->DisconnectFromModel(0);

    CPlugTree *rootTree = mobil->GetTree();
    if (rootTree == nullptr) {
        return nullptr;
    }
    CPlugTree::CIteratorVisual visualIterator(
            rootTree,
            CPlugTree::CIteratorTree::Mode_IncludeRoot);
    while (visualIterator.HasNext()) {
        CPlugTree *visualTree = nullptr;
        CPlugVisual *provider =
                visualIterator.GetNextVisual(&visualTree);
        if (provider == nullptr || visualTree == nullptr) {
            return nullptr;
        }
        std::unique_ptr<CPlugVisual> duplicatedVisual =
                provider->CloneVisual();
        if (duplicatedVisual == nullptr) {
            return nullptr;
        }
        visualTree->SetVisual(
                duplicatedVisual.get(),
                visualTree->Shader(),
                visualTree->Material(),
                0);
        RaisePylonVisualProvider(
                duplicatedVisual.get(),
                variantIndex);
        duplicatedVisual.release();
    }

    CPlugTree::CIteratorSurface surfaceIterator(
            rootTree,
            CPlugTree::CIteratorTree::Mode_IncludeRoot);
    while (surfaceIterator.HasNext()) {
        CPlugTree *surfaceTree = nullptr;
        CPlugSurface *surface =
                surfaceIterator.GetNextSurface(&surfaceTree);
        if (surface == nullptr || surfaceTree == nullptr) {
            return nullptr;
        }
        if (dynamic_cast<const GmSurfMesh *>(surface->Geometry()) ==
            nullptr) {
            continue;
        }
        std::unique_ptr<CPlugSurface> duplicatedSurface =
                surface->CloneMeshSurface();
        if (duplicatedSurface == nullptr) {
            return nullptr;
        }
        surfaceTree->SetSurface(duplicatedSurface.get());
        RaisePylonMeshSurface(
                duplicatedSurface.get(),
                variantIndex);
        duplicatedSurface.release();
    }

    CPlugTree *tree = rootTree;
    tree->SetModelInstance(true);
    (void)tree->UpdateBoundingBox(1);
    tree->HideInvalidTrees();
    tree->SetCollisionEnabled(true);
    item->SetCollisionGroup(CHmsItem::ECollisionGroup_Static);
    item->SetIsVisionStatic(1);
    if (tree->GetChildCount() == 0u) {
        tree->SetShadowCaster(false);
    }

    CPlugSolid *modelSolid = item->Solid()->CreateModelInstance();
    if (modelSolid == nullptr) {
        return nullptr;
    }
    mobil->SetSolid(modelSolid);

    return mobil.release();
}


void CGameCtnChallenge::UpdateColumnMobils(GmNat3 coord) {
    /*
     * Refresh the eight pylon columns around a coordinate.  Existing column
     * mobils are first scheduled for removal, then the column is either cleared
     * or repopulated from the zone's pylon block-info source.
     */
    CGameCtnZoneFlat *zone =
            dynamic_cast<CGameCtnZoneFlat *>(GetZone(coord));
    CGameCtnBlockInfoPylon *pylonBlockInfo = nullptr;
    if (zone != nullptr) {
        pylonBlockInfo = zone->blockInfoPylon.Get();
    }

    for (unsigned long pylonIndex = 0u; pylonIndex < 8u; ++pylonIndex) {
        CGameCtnPylonColumn *column = GetColumn(coord, pylonIndex);
        CSceneMobil *singleMobil = column->SingleMobil();
        if (singleMobil != nullptr) {
            AddMobilToRemoveList(singleMobil);
        } else {
            CSceneMobil *lowerMobil = column->LowerMobil();
            if (lowerMobil != nullptr) {
                AddMobilToRemoveList(lowerMobil);
            }
            CSceneMobil *middleMobil = column->MiddleMobil();
            if (middleMobil != nullptr) {
                AddMobilToRemoveList(middleMobil);
            }
            CSceneMobil *upperMobil = column->UpperMobil();
            if (upperMobil != nullptr) {
                AddMobilToRemoveList(upperMobil);
            }
        }

        const std::optional<CGameCtnPylonColumn::Span> &span =
                column->PylonSpan();
        if (!span.has_value() || pylonBlockInfo == nullptr) {
            column->DeleteMobils();
            continue;
        }

        const unsigned long firstY = span->firstY;
        const unsigned long variantIndex = span->lastY - firstY;
        if (pylonBlockInfo->MiddleSourceMobil() != nullptr) {
            if (pylonBlockInfo->GetMobil(
                        1,
                        variantIndex,
                        0u) == nullptr) {
                CSceneMobil *newPylonMobil =
                        CreateNewPylonMobil(pylonBlockInfo, variantIndex);
                if (newPylonMobil != nullptr) {
                    pylonBlockInfo->AddMobil(
                            1,
                            variantIndex,
                            newPylonMobil);
                }
            }

            CSceneMobil *middleSource =
                    pylonBlockInfo->GetMobil(
                            1,
                            variantIndex,
                            0u);
            CSceneMobil *firstSource = pylonBlockInfo->FirstSourceMobil();
            CSceneMobil *lastSource = pylonBlockInfo->LastSourceMobil();
            if (middleSource == nullptr ||
                firstSource == nullptr ||
                lastSource == nullptr) {
                continue;
            }

            std::unique_ptr<CSceneMobil> middleMobil(
                    middleSource->CreateModelInstance());
            std::unique_ptr<CSceneMobil> firstMobil(
                    firstSource->CreateModelInstance());
            std::unique_ptr<CSceneMobil> lastMobil(
                    lastSource->CreateModelInstance());
            if (middleMobil == nullptr ||
                firstMobil == nullptr ||
                lastMobil == nullptr) {
                continue;
            }

            column->SetMobils(
                    firstMobil.get(),
                    middleMobil.get(),
                    lastMobil.get());
            CSceneMobil *firstMobilResult = firstMobil.release();
            CSceneMobil *middleMobilResult = middleMobil.release();
            CSceneMobil *lastMobilResult = lastMobil.release();
            AddPylonToAddList(
                    firstMobilResult,
                    middleMobilResult,
                    lastMobilResult,
                    {coord.x, static_cast<u32>(firstY), coord.z},
                    pylonIndex,
                    variantIndex + 1u);
        } else {
            CSceneMobil *sourceMobil =
                    pylonBlockInfo->GetMobil(
                            1,
                            variantIndex,
                            0u);
            if (sourceMobil == nullptr) {
                continue;
            }
            std::unique_ptr<CSceneMobil> mobil(
                    sourceMobil->CreateModelInstance());
            if (mobil == nullptr || mobil->GetTree() == nullptr) {
                continue;
            }
            CPlugTree *tree = mobil->GetTree();
            tree->SetModelInstance(true);
            column->SetMobil(mobil.get());
            CSceneMobil *mobilResult = mobil.release();
            AddPylonToAddList(
                    mobilResult,
                    {coord.x, static_cast<u32>(firstY), coord.z},
                    pylonIndex,
                    variantIndex + 1u);
        }
    }
}


void CGameCtnChallenge::CreateBlocksAndPylons(void) {
    /*
     * CGameCtnChallenge::CreateBlocksAndPylons allocates the two optional
     * collector lists when terrain data exists, rebuilds the pylon-column grid
     * when pylon data exists, then refreshes the pylon block-info ground mobil
     * array from the challenge height minus the decoration offset.
     */
    const uint32_t width = mapWidth;
    const uint32_t depth = mapDepth;

    if (hasTerrainData) {
        if (terrainBlocksCollectorList == nullptr) {
            terrainBlocksCollectorList =
                    std::make_unique<CGameCtnCollectorList>();
        }
        if (terrainPylonsCollectorList == nullptr) {
            terrainPylonsCollectorList =
                    std::make_unique<CGameCtnCollectorList>();
        }
    }

    if (!hasPylonData) {
        return;
    }

    pylonColumns.Reset(width, depth);

    CGameCtnCollection *pylonCollection = collection;
    CGameCtnZoneFlat *pylonZone = pylonCollection != nullptr
            ? pylonCollection->DefaultZone()
            : nullptr;
    CGameCtnBlockInfoPylon *pylonBlockInfo = pylonZone != nullptr
            ? pylonZone->blockInfoPylon.Get()
            : nullptr;
    if (pylonBlockInfo != nullptr &&
        pylonBlockInfo->MiddleSourceMobil() != nullptr) {
        CGameCtnDecoration *decoration = this->decoration;
        CGameCtnDecorationSize *size = &decoration->Size();
        const uint32_t groundMobilCount =
                mapHeight -
                size->defaultZoneHeight;
        pylonBlockInfo->UpdateGroundMobilArray(groundMobilCount);
    }
}


void CGameCtnChallenge::LoadDecorationAndCollection(
        const SCtnForcedMods *forcedMods) {
    if (decoration != nullptr && collection != nullptr) {
        return;
    }

    CGameCtnChapter *chapter = GetChapter();
    if (chapter == nullptr) {
        return;
    }

    CSystemPackDesc *modPackDesc = GetModPackDesc(forcedMods);

    CGameCtnDecoration *loadedDecoration =
            CGameCtnDecoration::CreateAndInitSkinnedDecorationFromIdent(
                    decorationId,
                    modPackDesc);
    decoration = loadedDecoration;

    if (decoration == nullptr && decorationId.id.IsInvalid()) {
        CGameCtnArticle *article =
                chapter->GetDecorationIfUnique();
        if (article != nullptr) {
            loadedDecoration =
                    CGameCtnDecoration::CreateAndInitSkinnedDecorationFromIdent(
                            article->Identifier(),
                            modPackDesc);
            decoration = loadedDecoration;
            if (decoration != nullptr) {
                decorationId = decoration->identifier;
            }
        }
    }

    if (decoration == nullptr) {
        CMwNodRef<CGameCtnCollection> collectionForDefault(
                chapter->GetNod());

        SGameCtnIdentifier defaultIdentifier;
        if (collectionForDefault) {
            collectionForDefault->GetDefaultDecoration(defaultIdentifier);
        }

        loadedDecoration =
                CGameCtnDecoration::CreateAndInitSkinnedDecorationFromIdent(
                        defaultIdentifier,
                        modPackDesc);
        decoration = loadedDecoration;
        if (decoration != nullptr) {
            decorationId = decoration->identifier;
        }
    }

    CSystemFidParameters fidParameters(CSystemFidParameters::Empty);

    CSystemFidParameters::SParam_Id paramId(
            CPlugSolid::s_ParamId_PackBrotherShaders,
            1u,
            AnyAssetAudience(),
            0);
    fidParameters.AddParam(paramId);

    {
        const CSystemFidParameters::Scope parameterScope(fidParameters, 1);
        CGameCtnCollection *chapterCollection = chapter->GetNod();
        collection = chapterCollection;
    }

    if (decoration != nullptr) {
        defaultZoneHeight = decoration->Size().defaultZoneHeight;
    }
}


void CGameCtnChallenge::UpdatePylons(GmNat3 coord) {
    /*
     * remove stale pylon spans around a changed field-unit coordinate, scan
     * upward through non-real-zone air layers for new spans allowed by block
     * unit side masks, then refresh the mobil columns if any span changed.
     */
    if (!hasPylonData) {
        return;
    }

    int changed = 0;

    for (unsigned long pylonIndex = 0u; pylonIndex < 8u; ++pylonIndex) {
        CGameCtnPylonColumn *column = GetColumn(coord, pylonIndex);
        if (column->HasPylon()) {
            column->RemovePylon(coord.y);
            changed = 1;
        }
    }

    CGameCtnFieldUnit *groundFieldUnit =
            GetFieldUnit({coord.x, 0u, coord.z});
    if (groundFieldUnit != nullptr &&
        groundFieldUnit->BlockUnitRef() != nullptr) {
        const CMwId &realZoneId =
                GetRealZoneId({coord.x, 0u, coord.z});
        CGameCtnZone *realZone =
                collection->GetZone(realZoneId);

        if (dynamic_cast<CGameCtnZoneFrontier *>(realZone) == nullptr) {
            const uint32_t firstSpanY =
                    (uint32_t)GetZoneHeight(coord) +
                    1u;
            uint32_t scanY = mapHeight - 1u;

            while (scanY > firstSpanY) {
                CGameCtnBlockUnit *centerBlockUnit =
                        GetBlockUnitFromPlayField(
                                {coord.x, scanY, coord.z});

                for (uint32_t pylonIndex = 0u; pylonIndex < 8u; pylonIndex++) {
                    const uint32_t neighbourDirection =
                            pylonIndex < 2u ? 0u
                            : pylonIndex < 4u
                                    ? 1u
                                    : pylonIndex < 6u ? 2u : 3u;
                    const GmNat3 neighbourCoord =
                            CGameCtnUtils::GetNeighbourCoord(
                                    {coord.x, scanY, coord.z},
                                    static_cast<ECardinalDir>(neighbourDirection));
                    const uint32_t neighbourX = neighbourCoord.x;
                    const uint32_t neighbourZ = neighbourCoord.z;

                    CGameCtnBlockUnit *neighbourBlockUnit = nullptr;
                    int shouldConsiderSpan = 0;
                    if (!IsEditableCoord(neighbourCoord)) {
                        shouldConsiderSpan = 1;
                    } else {
                        neighbourBlockUnit =
                                GetBlockUnitFromPlayField(neighbourCoord);
                        CGameCtnZone *neighbourZone =
                                GetZone(neighbourCoord);
                        shouldConsiderSpan =
                                dynamic_cast<CGameCtnZoneFrontier *>(
                                        neighbourZone) == nullptr;
                    }

                    if (!shouldConsiderSpan) {
                        continue;
                    }

                    const uint8_t neighbourPylonIndex =
                            GetNeighbourPylonIndex((uint8_t)pylonIndex);
                    const int centerNeedsPylon =
                            centerBlockUnit != nullptr &&
                            ((centerBlockUnit->RotatedJunctionMask() >> pylonIndex) &
                             1u) != 0u;
                    const int neighbourNeedsPylon =
                            neighbourBlockUnit != nullptr &&
                            ((neighbourBlockUnit->RotatedJunctionMask() >> neighbourPylonIndex) &
                             1u) != 0u;
                    if (!centerNeedsPylon && !neighbourNeedsPylon) {
                        continue;
                    }

                    CGameCtnPylonColumn *column = GetColumn(
                            {coord.x, scanY, coord.z},
                            pylonIndex);
                    if (column->HasPylon()) {
                        continue;
                    }

                    int clearSpan = 1;
                    for (uint32_t testY = firstSpanY; testY < scanY;
                         testY++) {
                        CGameCtnBlockUnit *centerBelow =
                                GetBlockUnitFromPlayField(
                                        {coord.x, testY, coord.z});
                        CGameCtnBlockUnit *neighbourBelow =
                                GetBlockUnitFromPlayField(
                                        {neighbourX, testY, neighbourZ});
                        const int centerAllowsPylon =
                                centerBelow == nullptr ||
                                ((centerBelow->RotatedHelperMask() >> pylonIndex) &
                                 1u) != 0u;
                        const int neighbourAllowsPylon =
                                neighbourBelow == nullptr ||
                                ((neighbourBelow->RotatedHelperMask() >> neighbourPylonIndex) &
                                 1u) != 0u;
                        if (!centerAllowsPylon || !neighbourAllowsPylon) {
                            clearSpan = 0;
                        }
                    }

                    if (clearSpan) {
                        column->AddPylon(firstSpanY, scanY - 1u);
                        changed = 1;
                    }
                }

                scanY--;
            }
        }
    }

    if (changed) {
        UpdateColumnMobils(coord);
    }
}

CGameCtnBlockUnit *CGameCtnChallenge::CreatePlacedBlockUnit(
        CGameCtnBlock *block,
        CGameCtnBlockUnitInfo *unitInfo,
        CGameCtnFieldUnit *fieldUnit,
        GmNat3 localOffset,
        uint32_t junctionMask,
        uint32_t helperMask) {
    return new CGameCtnBlockUnit(
            block,
            unitInfo,
            fieldUnit,
            localOffset,
            junctionMask,
            helperMask);
}

CGameCtnBlockUnit *CGameCtnChallenge::AttachPlacedBlockUnit(
        CGameCtnBlock *block,
        CGameCtnBlockUnitInfo *unitInfo,
        CGameCtnFieldUnit *fieldUnit,
        GmNat3 localOffset,
        uint32_t junctionMask,
        uint32_t helperMask) {
    CGameCtnBlockUnit *unit = CreatePlacedBlockUnit(
            block,
            unitInfo,
            fieldUnit,
            localOffset,
            junctionMask,
            helperMask);
    block->AddBlockUnit(unit);
    return unit;
}

CGameCtnZone *CGameCtnChallenge::ZoneForId(const CMwId &id) const {
    return collection->GetZone(id);
}

CGameCtnZone *CGameCtnChallenge::RealZoneForBlock(CGameCtnBlock *block) const {
    return collection->GetZoneFromLandBlockInfo(block->BlockInfoRef());
}

int CGameCtnChallenge::IsFrontierZoneAt(GmNat3 coord) {
    return dynamic_cast<CGameCtnZoneFrontier *>(GetRealZone(coord)) != nullptr;
}

int CGameCtnChallenge::IsGroundZoneAt(GmNat3 coord) {
    return dynamic_cast<CGameCtnZoneFlat *>(GetRealZone(coord)) != nullptr;
}

void CGameCtnChallenge::SetFieldTerrain(CGameCtnFieldUnit *fieldUnit,
                                          ETerrain terrain) {
    if (fieldUnit != nullptr) {
        fieldUnit->SetTerrain(terrain);
    }
}

CGameCtnFieldUnit *CGameCtnChallenge::EnsureFieldUnit(
        GmNat3 coord,
        ETerrain terrain) {
    CGameCtnFieldUnit *fieldUnit = GetFieldUnit(coord);
    if (fieldUnit != nullptr) {
        SetFieldTerrain(fieldUnit, terrain);
        return fieldUnit;
    }
    return CreateFieldUnit(coord, terrain);
}

void CGameCtnChallenge::RemoveGroundBlockAt(uint32_t x, uint32_t z) {
    AddBlockToRemoveList(GetBlockFromPlayField({x, 0u, z}));
}

GmNat3 CGameCtnChallenge::AddBlockCoordOffset(const CGameCtnBlock &block,
                                               const GmNat3 &offset) const {
    const GmNat3 &base = block.BaseCoord();
    return {base.x + offset.x, base.y + offset.y, base.z + offset.z};
}

GmNat3 CGameCtnChallenge::RotatedBlockUnitOffset(
        CGameCtnBlockInfo &blockInfo,
        uint32_t index,
        ECardinalDir direction,
        int isGround) {
    return blockInfo.GetRotatedOffset(index, direction, isGround);
}

GmNat3 CGameCtnChallenge::AbsoluteBlockUnitCoord(
        const CGameCtnBlock &block,
        const GmNat3 &offset) const {
    return AddBlockCoordOffset(block, offset);
}

int CGameCtnChallenge::IsEditableBlockUnitCoord(const CGameCtnBlock &block,
                                                 const GmNat3 &offset) {
    const GmNat3 coord = AbsoluteBlockUnitCoord(block, offset);
    return IsEditableCoord(coord);
}

void CGameCtnChallenge::UpdatePylonsForBlockUnit(CGameCtnBlockUnit *unit) {
    UpdatePylons(unit->GetCoord());
}

uint32_t CGameCtnChallenge::AdjustRoadJunctionMask(const CGameCtnBlock &block,
                                                    uint32_t junctionMask) {
    if (!block.IsType(EBlockType::Road) || junctionMask == 0u) {
        return junctionMask;
    }

    const CMwId &realZoneId = GetRealZoneId(block.BaseCoord());
    CMwId copiedZoneId(realZoneId);
    CGameCtnZone *zone = ZoneForId(copiedZoneId);
    if (dynamic_cast<CGameCtnZoneFrontier *>(zone) != nullptr) {
        return 0u;
    }

    CGameCtnZoneFlat *zoneFlat = collection->GetZoneFlat(copiedZoneId);
    if (zoneFlat == nullptr) {
        return junctionMask;
    }
    if (!zoneFlat->blockInfoPylon) {
        return 0u;
    }

    switch (block.VariantIndex()) {
    case 0u:
    case 5u:
        return 255u;
    case 1u:
        return 3u;
    case 2u:
        return 15u;
    case 3u:
        return 51u;
    case 4u:
        return 63u;
    default:
        return junctionMask;
    }
}



int CGameCtnChallenge::UpdateFieldUnits(CGameCtnBlock *block) {
    CGameCtnBlock *ctnBlock = block;
    CGameCtnBlockInfo *desc = GetDescFromBlock(ctnBlock);
    if (desc == nullptr) {
        return 0;
    }

    const int shouldCreateBlockUnits = !ctnBlock->HasBlockUnits();
    int isGround = 1;
    if (desc->Type() >= EBlockType::Classic) {
        isGround = IsBlockOnGround(ctnBlock->BlockInfoRef(),
                                   ctnBlock->BaseCoord(),
                                   ctnBlock->Direction());
    }

    uint32_t unitCount = desc->GetNbBlockUnitInfos(isGround);
    if (unitCount == 0u) {
        isGround = IsBlockOnGround(ctnBlock->BlockInfoRef(),
                                   ctnBlock->BaseCoord(),
                                   ctnBlock->Direction());
    }

    for (uint32_t index = 0u; index < unitCount; index++) {
        const GmNat3 offset = RotatedBlockUnitOffset(
                *desc,
                index,
                ctnBlock->Direction(),
                isGround);
        if (IsEditableBlockUnitCoord(*ctnBlock, offset) == 0) {
            return 0;
        }
    }

    if (desc->Type() <= EBlockType::Frontier) {
        const uint32_t blockX = ctnBlock->BaseCoord().x;
        const int32_t blockY = (int32_t)ctnBlock->BaseCoord().y;
        const uint32_t blockZ = ctnBlock->BaseCoord().z;
        SetZoneHeight(ctnBlock->BaseCoord(), (unsigned long)blockY);

        if (blockY >= 0) {
            for (uint32_t y = 0u; y <= (uint32_t)blockY; y++) {
                EnsureFieldUnit(
                        {blockX, y, blockZ},
                        ETerrain::Underground);
            }
        }

        const uint32_t topY = (uint32_t)blockY + 1u;
        EnsureFieldUnit({blockX, topY, blockZ}, ETerrain::Ground);

        CGameCtnBlockUnitInfo *unitInfo = desc->FirstBlockUnitInfo(isGround);
        const GmNat3 offset = RotatedBlockUnitOffset(*desc, 0u, ctnBlock->Direction(), isGround);
        const GmNat3 coord = AbsoluteBlockUnitCoord(*ctnBlock, offset);
        CGameCtnBlockUnit *newUnit = AttachPlacedBlockUnit(
                ctnBlock,
                unitInfo,
                GetFieldUnit(coord),
                offset,
                0u,
                0u);

        CGameCtnZone *zone = collection->GetZoneFromLandBlockInfo(desc);
        const int32_t zoneHeightBase = (int32_t)zone->height;
        const int32_t zoneStackHeight = (int32_t)zone->depth;
        if (zoneStackHeight != 0) {
            int32_t firstY = blockY - zoneStackHeight - zoneHeightBase;
            if (firstY < 0) {
                firstY = 0;
            }
            int32_t lastY = blockY - zoneHeightBase;
            if (lastY < 0) {
                lastY = 0;
            }
            for (int32_t y = firstY + 1; y <= lastY; y++) {
                CGameCtnBlockUnit *extraUnit = AttachPlacedBlockUnit(
                        ctnBlock,
                        unitInfo,
                        GetFieldUnit({blockX, (uint32_t)y, blockZ}),
                        {0u, (uint32_t)y, 0u},
                        0u,
                        0u);
                newUnit = extraUnit;
            }
        }

        UpdatePylonsForBlockUnit(newUnit);
        return 1;
    }

    for (uint32_t index = 0u; index < unitCount; index++) {
        CGameCtnBlockUnitInfo *unitInfo = desc->GetBlockUnitInfo(index, isGround);
        const GmNat3 offset = RotatedBlockUnitOffset(
                *desc,
                index,
                ctnBlock->Direction(),
                isGround);
        const GmNat3 coord = AbsoluteBlockUnitCoord(*ctnBlock, offset);
        CGameCtnFieldUnit *fieldUnit = GetFieldUnit(coord);
        if (fieldUnit == nullptr) {
            for (uint32_t y = 0u; y <= coord.y; y++) {
                const GmNat3 fillCoord{coord.x, y, coord.z};
                if (GetFieldUnit(fillCoord) == nullptr) {
                    CreateFieldUnit(fillCoord, ETerrain::Air);
                }
            }
            fieldUnit = GetFieldUnit(coord);
        }

        const uint32_t junctionMask = AdjustRoadJunctionMask(
                *ctnBlock,
                unitInfo->JunctionMask());
        CGameCtnBlockUnit *blockUnit = nullptr;
        if (shouldCreateBlockUnits) {
            blockUnit = AttachPlacedBlockUnit(
                    ctnBlock,
                    unitInfo,
                    fieldUnit,
                    offset,
                    junctionMask,
                    unitInfo->HelperMask());
        } else {
            blockUnit = ctnBlock->BlockUnitAt(index);
        }

        UpdatePylonsForBlockUnit(blockUnit);
        if (fieldUnit->Terrain() == ETerrain::Ground) {
            RemoveGroundBlockAt(coord.x, coord.z);
        }
    }
    return 1;
}

int CGameCtnChallenge::IsEditableCoord(GmNat3 coord) {
    return ContainsCoord(coord);
}



CGameCtnFieldUnit *CGameCtnChallenge::GetFieldUnit(GmNat3 coord) {
    if (!IsEditableCoord(coord)) {
        return nullptr;
    }
    FieldUnitColumn &column = fieldUnitColumns[GridIndex(coord.x, coord.z)];
    return coord.y < column.size() ? column[coord.y].get() : nullptr;
}



void CGameCtnChallenge::RemoveFieldUnit(GmNat3 coord) {
    FieldUnitColumn &column = fieldUnitColumns[GridIndex(coord.x, coord.z)];
    column.erase(column.begin() + coord.y);
}


int CGameCtnChallenge::ContainsCoord(GmNat3 coord) const {
    return coord.x < mapWidth &&
           coord.y < mapHeight &&
           coord.z < mapDepth;
}

uint32_t CGameCtnChallenge::GridIndex(uint32_t x, uint32_t z) const {
    return z + x * mapDepth;
}

uint32_t CGameCtnChallenge::ColumnModifierFirstScanY(
        GmNat3 coord) {
    return (uint32_t)GetZoneHeight(coord) + 2u;
}

CGameCtnBlockUnitInfo *CGameCtnChallenge::BlockUnitInfoAtCoord(
        GmNat3 coord) {
    return GetBlockUnitInfoFromPlayField(coord);
}

CGameCtnBlockUnitInfo *CGameCtnChallenge::FindColumnTerrainModifierUnit(
        GmNat3 coord) {
    for (uint32_t y = ColumnModifierFirstScanY(coord); y < mapHeight; ++y) {
        CGameCtnBlockUnitInfo *unitInfo =
                BlockUnitInfoAtCoord({coord.x, y, coord.z});
        if (unitInfo != nullptr && unitInfo->HasTerrainModifierId()) {
            return unitInfo;
        }
    }
    return nullptr;
}

int SZoneGenealogy::operator==(const SZoneGenealogy &other) const {
    if (ancestors.size() != other.ancestors.size()) {
        return 0;
    }
    for (size_t index = 0u; index < ancestors.size(); ++index) {
        const SZoneGenealogyEntry &entry = ancestors[index];
        const SZoneGenealogyEntry &otherEntry = other.ancestors[index];
        if (entry.zoneId != otherEntry.zoneId ||
            entry.zoneHeight != otherEntry.zoneHeight) {
            return 0;
        }
    }
    return 1;
}

void CGameCtnChallenge::InitializeZoneGrid(
        const CMwId &zoneId,
        unsigned long height,
        bool genealogyEnabled) {
    std::vector<ChallengeZoneCell> initializedCells(
            static_cast<size_t>(mapWidth) * static_cast<size_t>(mapDepth));
    const uint8_t storedHeight = static_cast<uint8_t>(height);
    for (uint32_t x = 0u; x < mapWidth; ++x) {
        for (uint32_t z = 0u; z < mapDepth; ++z) {
            ChallengeZoneCell &cell = initializedCells[GridIndex(x, z)];
            cell.zoneId = zoneId;
            cell.height = storedHeight;
            if (genealogyEnabled) {
                cell.genealogy.emplace();
            }
        }
    }
    zoneCells.swap(initializedCells);
}

void CGameCtnChallenge::ResetZoneGrid(void) {
    zoneCells.clear();
}

void CGameCtnChallenge::InitializeFieldUnitGrid(void) {
    ResetFieldUnitGrid();
    fieldUnitColumns.resize(
            static_cast<size_t>(mapWidth) * static_cast<size_t>(mapDepth));
}

void CGameCtnChallenge::ResetFieldUnitGrid(void) {
    fieldUnitColumns.clear();
}

CGameCtnChallenge::~CGameCtnChallenge(void) = default;

SPylonMobil::SPylonMobil(
        CSceneMobil &mobil,
        GmNat3 coord,
        unsigned long pylonIndex,
        unsigned long pylonHeight)
    : coord(coord),
      pylonIndex(pylonIndex),
      pylonHeight(pylonHeight),
      mobils(SSingleMobil(mobil)) {
}

SPylonMobil::SPylonMobil(
        CSceneMobil &lower,
        CSceneMobil &middle,
        CSceneMobil &upper,
        GmNat3 coord,
        unsigned long pylonIndex,
        unsigned long pylonHeight)
    : coord(coord),
      pylonIndex(pylonIndex),
      pylonHeight(pylonHeight),
      mobils(STripleMobils(lower, middle, upper)) {
}

bool SPylonMobil::IsSingleMobil(void) const {
    return std::holds_alternative<SSingleMobil>(mobils);
}

CSceneMobil &SPylonMobil::SingleMobil(void) const {
    return std::get<SSingleMobil>(mobils).Mobil();
}

const SPylonMobil::STripleMobils &SPylonMobil::TripleMobils(void) const {
    return std::get<STripleMobils>(mobils);
}


ETerrain CGameCtnChallenge::GetTerrainFromPlayField(GmNat3 coord) {
    if (!IsEditableCoord(coord)) {
        return ETerrain::Outside;
    }

    CGameCtnFieldUnit *fieldUnit = GetFieldUnit(coord);
    return fieldUnit != nullptr ? fieldUnit->Terrain() : ETerrain::Air;
}


CGameCtnBlockUnit *CGameCtnChallenge::GetBlockUnitFromPlayField(
        GmNat3 coord) {
    CGameCtnFieldUnit *fieldUnit = GetFieldUnit(coord);
    return fieldUnit != nullptr ? fieldUnit->BlockUnitRef() : nullptr;
}



CGameCtnBlockUnitInfo *CGameCtnChallenge::GetBlockUnitInfoFromPlayField(
        GmNat3 coord) {
    CGameCtnBlockUnit *blockUnit = GetBlockUnitFromPlayField(coord);
    return blockUnit != nullptr ? blockUnit->InfoRef() : nullptr;
}




CGameCtnBlock *CGameCtnChallenge::GetBlockFromPlayField(GmNat3 coord) {
    if (!IsEditableCoord(coord)) {
        return nullptr;
    }
    CGameCtnFieldUnit *fieldUnit = GetFieldUnit(coord);
    return fieldUnit != nullptr ? fieldUnit->BlockRef() : nullptr;
}



const CMwId &CGameCtnChallenge::GetZoneId(GmNat3 coord) {
    return zoneCells[GridIndex(coord.x, coord.z)].zoneId;
}




unsigned char CGameCtnChallenge::GetZoneHeight(GmNat3 coord) {
    return zoneCells[GridIndex(coord.x, coord.z)].height;
}



void CGameCtnChallenge::SetZoneHeight(
        GmNat3 coord,
        unsigned long height) {
    zoneCells[GridIndex(coord.x, coord.z)].height =
            static_cast<uint8_t>(height);
}



CGameCtnZone *CGameCtnChallenge::GetZone(GmNat3 coord) {
    return collection->GetZone(GetZoneId(coord));
}




SZoneGenealogy *CGameCtnChallenge::GetZoneGenealogy(GmNat3 coord) {
    std::optional<SZoneGenealogy> &genealogy =
            zoneCells[GridIndex(coord.x, coord.z)].genealogy;
    return genealogy ? &*genealogy : nullptr;
}



CGameCtnZone *CGameCtnChallenge::GetRealZone(GmNat3 coord) {
    return RealZoneForBlock(GetBlockFromPlayField(coord));
}




const CMwId &CGameCtnChallenge::GetRealZoneId(GmNat3 coord) {
    return GetRealZone(coord)->zoneId;
}




CGameCtnFieldUnit *CGameCtnChallenge::CreateFieldUnit(
        GmNat3 coord,
        ETerrain terrain) {
    std::unique_ptr<CGameCtnFieldUnit> ownedFieldUnit =
            std::make_unique<CGameCtnFieldUnit>(terrain);
    CGameCtnFieldUnit *fieldUnit = ownedFieldUnit.get();
    fieldUnitColumns[GridIndex(coord.x, coord.z)].push_back(
            std::move(ownedFieldUnit));
    return fieldUnit;
}



void CGameCtnChallenge::SetZoneId(
        GmNat3 coord,
        const CMwId &id) {
    zoneCells[GridIndex(coord.x, coord.z)].zoneId = id;
}




void CGameCtnChallenge::AddBlockToAddList(
        CGameCtnBlock *block) {
    if (block != nullptr) {
        blocksToAdd.emplace_back(*block);
    }
}


CGameCtnBlock *CGameCtnChallenge::GetBlockFromAddList(void) {
    return blocksToAdd.empty() ? nullptr : &blocksToAdd.front().get();
}


void CGameCtnChallenge::RemoveBlockFromAddList(void) {
    if (!blocksToAdd.empty()) {
        blocksToAdd.pop_front();
    }
}


SPylonMobil * CGameCtnChallenge::GetPylonFromAddList(void) {
    return pylonsToAdd.empty() ? nullptr : &pylonsToAdd.front();
}



void CGameCtnChallenge::RemovePylonFromAddList(void) {
    if (!pylonsToAdd.empty()) {
        pylonsToAdd.pop_front();
    }
}



CSceneMobil * CGameCtnChallenge::GetMobilFromRemoveList(void) {
    return mobilsToRemove.empty() ? nullptr : mobilsToRemove.front().Get();
}



void CGameCtnChallenge::RemoveMobilFromRemoveList(void) {
    if (!mobilsToRemove.empty()) {
        CMwNodRef<CSceneMobil> removed(
                std::move(mobilsToRemove.front()));
        mobilsToRemove.pop_front();
    }
}



CGameCtnBlockInfoClip *CGameCtnChallenge::GetRequiredBlockUnitJunction(
        GmNat3 coord,
        ECardinalDir direction) {
    CGameCtnBlock *block = GetBlockFromPlayField(coord);
    if (block == nullptr ||
        block->IsType(EBlockType::Road) ||
        block->IsType(EBlockType::Clip)) {
        return nullptr;
    }
    CGameCtnBlockUnit *unit = GetBlockUnitFromPlayField(coord);
    return unit != nullptr ? unit->JunctionAt(direction) : nullptr;
}




CGameCtnBlockInfoClip *CGameCtnChallenge::GetRequiredNeighbourBlockUnitJunction(
        GmNat3 coord,
        ECardinalDir direction) {
    const GmNat3 neighbour = CGameCtnUtils::GetNeighbourCoord(
            coord,
            direction);
    return GetRequiredBlockUnitJunction(
            neighbour,
            CGameCtnUtils::GetOpposedDir(direction));
}

CGameCtnBlockInfoClip *CGameCtnChallenge::GetNeighbourBlockUnitJunction(
        GmNat3 coord,
        ECardinalDir direction) {
    const GmNat3 neighbour = CGameCtnUtils::GetNeighbourCoord(coord, direction);
    if (!IsEditableCoord(neighbour)) {
        return nullptr;
    }

    CGameCtnBlockUnit *unit = GetBlockUnitFromPlayField(neighbour);
    return unit != nullptr
            ? unit->JunctionAt(CGameCtnUtils::GetOpposedDir(direction))
            : nullptr;
}


int CGameCtnChallenge::CanJoinRoadBlock(
        CGameCtnBlock *block,
        ECardinalDir direction) {
    if (!block->HasBlockUnits()) {
        return 1;
    }

    for (const std::unique_ptr<CGameCtnBlockUnit> &ownedBlockUnit :
         block->BlockUnits()) {
        CGameCtnBlockUnit *blockUnit = ownedBlockUnit.get();
        CGameCtnBlockInfoClip *required = blockUnit->JunctionAt(direction);
        if (required == nullptr) {
            continue;
        }

        const GmNat3 coord = blockUnit->GetCoord();
        const GmNat3 neighbour = CGameCtnUtils::GetNeighbourCoord(
                coord,
                direction);
        CGameCtnBlockUnit *neighbourBlockUnit =
                GetBlockUnitFromPlayField(neighbour);
        if (neighbourBlockUnit == nullptr) {
            return 0;
        }

        const ECardinalDir opposed = CGameCtnUtils::GetOpposedDir(direction);
        CGameCtnBlockInfoClip *neighbourRequired =
                neighbourBlockUnit->JunctionAt(opposed);
        if (neighbourRequired != required) {
            return 0;
        }
    }
    return 1;
}




int CGameCtnChallenge::IsFullUnderground(GmNat3 coord) {
    if (GetTerrainFromPlayField(coord) != ETerrain::Underground) {
        return 0;
    }
    CGameCtnZone *zone = GetRealZone(coord);
    CGameCtnZoneFrontier *frontier =
            dynamic_cast<CGameCtnZoneFrontier *>(zone);
    return frontier == nullptr ||
           coord.y <= GetZoneHeight(coord) - zone->height;
}



int CGameCtnChallenge::IsOccluding(const GmNat3 &coord) {
    CGameCtnBlockUnit *blockUnit = GetBlockUnitFromPlayField(coord);
    if (blockUnit != nullptr) {
        CGameCtnBlock *block = blockUnit->BlockRef();
        if (!block->IsType(EBlockType::RectAsym)) {
            return 0;
        }
        return block->HasNoMainMobil();
    }

    if (coord.y >= GetZoneHeight(coord)) {
        return 0;
    }
    return IsGroundZoneAt(coord);
}



int CGameCtnChallenge::IsTunnelOccludedCardinal(
        GmNat3 coord,
        ECardinalDir direction) {
    CGameCtnBlockUnit *blockUnit = GetBlockUnitFromPlayField(coord);
    if (blockUnit == nullptr) {
        return 0;
    }

    const GmNat3 neighbour = CGameCtnUtils::GetNeighbourCoord(
            coord,
            direction);
    CGameCtnBlockInfoClip *junctionBlockInfo = blockUnit->JunctionAt(direction);
    if (junctionBlockInfo != nullptr) {
        if (GetBlockFromPlayField(neighbour) == nullptr) {
            return junctionBlockInfo->IsAllUnderground(1);
        }
        return 0;
    }

    CGameCtnBlock *block = GetBlockFromPlayField(coord);
    if (coord.y > 1u) {
        GmNat3 below = coord;
        below.y--;
        if (GetBlockFromPlayField(below) == block &&
            !IsTunnelOccludedCardinal(below, direction)) {
            return 0;
        }
    }

    CGameCtnBlock *neighbourBlock = GetBlockFromPlayField(neighbour);
    if (neighbourBlock == block) {
        return 0;
    }
    return neighbourBlock == nullptr;
}




int CGameCtnChallenge::IsTunnelOccludedAltitude(
        GmNat3 coord,
        int upward) {
    if (GetBlockUnitFromPlayField(coord) == nullptr) {
        return 0;
    }

    GmNat3 neighbour = coord;
    if (upward != 0) {
        neighbour.y++;
    } else {
        if (coord.y == 1u) {
            return 1;
        }
        neighbour.y--;
    }

    CGameCtnBlock *neighbourBlock = GetBlockFromPlayField(neighbour);
    return GetBlockFromPlayField(coord) != neighbourBlock;
}



CGameCtnBlock *CGameCtnChallenge::GetGroundBlock(GmNat3 coord) {
    /*
     * First look for a block one cell above the zone height. If absent, retry
     * at the original coordinate.
     */
    const uint32_t groundProbeY =
            (uint32_t)GetZoneHeight(coord) + 1u;
    CGameCtnBlock *block =
            GetBlockFromPlayField({coord.x, groundProbeY, coord.z});
    if (block == nullptr) {
        return GetBlockFromPlayField(coord);
    }
    return block;
}


void CGameCtnChallenge::AddPylonToAddList(
        CSceneMobil *mobil,
        GmNat3 coord,
        unsigned long pylonIndex,
        unsigned long pylonHeight) {
    pylonsToAdd.emplace_back(
            *mobil,
            coord,
            pylonIndex,
            pylonHeight);
}



void CGameCtnChallenge::AddPylonToAddList(
        CSceneMobil *lower,
        CSceneMobil *middle,
        CSceneMobil *upper,
        GmNat3 coord,
        unsigned long pylonIndex,
        unsigned long pylonHeight) {
    pylonsToAdd.emplace_back(
            *lower,
            *middle,
            *upper,
            coord,
            pylonIndex,
            pylonHeight);
}



void CGameCtnChallenge::AddMobilToRemoveList(
        CSceneMobil *mobil) {
    if (mobil == nullptr) {
        return;
    }

    const auto existing = std::find_if(
            mobilsToRemove.begin(),
            mobilsToRemove.end(),
            [mobil](const CMwNodRef<CSceneMobil> &queued) {
                return queued.Get() == mobil;
            });
    if (existing == mobilsToRemove.end()) {
        mobilsToRemove.emplace_back(mobil);
    }
}


void CGameCtnChallenge::AddBlockToRemoveList(
        CGameCtnBlock *block) {
    if (block == nullptr) {
        return;
    }
    const auto pending = std::find_if(
            blocksToAdd.begin(),
            blocksToAdd.end(),
            [block](const std::reference_wrapper<CGameCtnBlock> &queued) {
                return &queued.get() == block;
            });
    if (pending != blocksToAdd.end()) {
        blocksToAdd.erase(pending);
        return;
    }
    AddMobilToRemoveList(block->MainMobil());
    AddMobilToRemoveList(block->HelperMobil());
    for (const CMwNodRef<CSceneMobil> &subMobil : block->SubMobils()) {
        AddMobilToRemoveList(subMobil.Get());
    }
}



int CGameCtnChallenge::IsBlockOnGround(
        CGameCtnBlockInfo *blockInfo,
        GmNat3 coord,
        ECardinalDir direction) {
    if (blockInfo == nullptr) {
        return 0;
    }

    const uint32_t unitCount = blockInfo->GetNbBlockUnitInfos(1);
    if (unitCount == 0u) {
        return 0;
    }

    uint32_t selectedLayerY = 0u;
    if (blockInfo->IsPartUnderground(1) != 0) {
        int haveSelectedLayer = 0;
        for (uint32_t index = 0; index < unitCount; ++index) {
            CGameCtnBlockUnitInfo *unit = blockInfo->GetBlockUnitInfo(index, 1);
            if (!unit->IsUndergroundUnit()) {
                const uint32_t unitLayerY = unit->UnitLayer();
                if (!haveSelectedLayer || unitLayerY < selectedLayerY) {
                    selectedLayerY = unitLayerY;
                    haveSelectedLayer = 1;
                }
            }
        }
    }

    for (uint32_t index = 0; index < unitCount; ++index) {
        CGameCtnBlockUnitInfo *unit = blockInfo->GetBlockUnitInfo(index, 1);
        if (unit->UnitLayer() == selectedLayerY && !unit->IsUndergroundUnit()) {
            const GmNat3 rotatedOffset = blockInfo->GetRotatedOffset(
                    index,
                    direction,
                    1);
            const uint32_t queryX = coord.x + rotatedOffset.x;
            const uint32_t queryY = coord.y + rotatedOffset.y;
            const uint32_t queryZ = coord.z + rotatedOffset.z;
            if (GetTerrainFromPlayField({queryX, queryY, queryZ}) !=
                ETerrain::Ground) {
                return 0;
            }
        }
    }
    return 1;
}



CMwId CGameCtnChallenge::GetColumnModifierId(GmNat3 coord) {
    CGameCtnBlockUnitInfo *unitInfo = FindColumnTerrainModifierUnit(coord);
    if (unitInfo != nullptr) {
        return unitInfo->GetTerrainModifierId();
    }
    return CMwId{};
}





unsigned long CGameCtnChallenge::GetReplacementIndex(
        CMwId firstId,
        CMwId secondId) {
    return collection->SurfaceReplacementIndex(firstId, secondId);
}
