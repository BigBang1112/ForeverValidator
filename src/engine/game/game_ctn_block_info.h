#pragma once

#include "engine/core/engine_types.h"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "engine/game/block_placement_state.h"
#include "engine/game/block_race_role.h"
#include "engine/resources/catalog_asset_handle.h"
#include "engine/core/cfast_buffer.h"
#include "engine/game/game_ctn_collector.h"
#include "engine/core/gm_types.h"
#include "engine/core/mw_id.h"
#include "engine/scene/scene_mobil.h"
struct CHmsItem;
struct CPlugSolid;
struct CPlugTree;

class CGameCtnBlockInfo;
class CGameCtnBlockInfoClip;

enum class BlockInterfaceKind : u32 {
    None = 0u,
    Label = 1u,
};

class CGameCtnBlockUnitInfo : public CMwNod {
public:
    CGameCtnBlockUnitInfo(void);
    CGameCtnBlockUnitInfo(GmNat3 offset,
                          unsigned long helperMask,
                          unsigned long junctionMask,
                          CGameCtnBlockInfoClip *northJunction,
                          CGameCtnBlockInfoClip *westJunction,
                          CGameCtnBlockInfoClip *southJunction,
                          CGameCtnBlockInfoClip *eastJunction,
                          CGameCtnBlockInfo *blockInfo);
    ~CGameCtnBlockUnitInfo(void) override;

    CMwId GetReplacementId(void);
    CMwId GetTerrainModifierId(void);
    int HasTerrainModifierId(void) const;
    CMwId GetSurface(void);
    CGameCtnBlockInfoClip *GetRotatedJunction(ECardinalDir direction,
                                              ECardinalDir rotation);
    CGameCtnBlockUnitInfo *GetReplacementBlockUnitInfo(int isGround);
    static CMwNod *MwNewCGameCtnBlockUnitInfo(void);

    void InitializeUnitFields(const GmNat3 &offset, u32 helperMask,
                              u32 junctionMask, CGameCtnBlockInfo *blockInfo);
    void InitializeDefaultUnitFields(void);
    const GmNat3 &UnitOffset(void) const;
    u32 UnitOffsetX(void) const;
    u32 UnitOffsetY(void) const;
    u32 UnitOffsetZ(void) const;
    u32 UnitLayer(void) const;
    u32 UnitYForBlockType(EBlockType blockType) const;
    int IsUndergroundUnit(void) const;
    u32 JunctionMask(void) const;
    u32 HelperMask(void) const;
    CGameCtnBlockInfo *OwnerBlockInfo(void) const;
    void MarkTerrainModifierInvalid(void);
    void CopySurfaceIdFrom(const CMwId &id);
    CGameCtnBlockInfoClip *JunctionAt(ECardinalDir direction) const;
    const CMwId &SurfaceId(void) const;
    const CMwId &TerrainModifierId(void) const;
    const char *SurfaceIdName(void) const;
    void SetUnderground(bool underground);
    void SetSurfaceIdName(const char *name);
    void SetTerrainModifierId(const CMwId &id);
    void SetJunction(ECardinalDir direction, CGameCtnBlockInfoClip *junction);

private:
    u32 helperMask = 0u;
    u32 junctionMask = 0u;
    bool underground = false;
    std::array<CMwNodRef<CGameCtnBlockInfoClip>, 4> junctions{};
    GmNat3 offset{};
    CMwId surfaceId;
    CGameCtnBlockInfo *blockInfo = nullptr;
    CMwId replacementId;
    CMwId terrainModifierId;
};

class CGameCtnBlockInfo : public CGameCtnCollector {
    class MobilStorage {
    public:
        using MobilVariant = std::vector<CMwNodRef<CSceneMobil>>;
        using MobilVariants = std::vector<MobilVariant>;

        MobilStorage(void) = default;
        ~MobilStorage(void) = default;
        MobilStorage(const MobilStorage &) = delete;
        MobilStorage &operator=(const MobilStorage &) = delete;

        MobilVariant *Find(int family, unsigned long variantIndex);
        const MobilVariant *Find(int family,
                                 unsigned long variantIndex) const;
        MobilVariant &Require(int family, unsigned long variantIndex);
        bool HasAny(int family) const;
        void Reset(int family, unsigned long variantCount);

    private:
        static void ClearFamily(MobilVariants &variants) {
            variants.clear();
        }
        MobilVariants &Family(int family);
        const MobilVariants &Family(int family) const;
        MobilVariants ground_;
        MobilVariants air_;
    };

public:
    static constexpr int AirMobilFamily = 0;
    static constexpr int GroundMobilFamily = 1;
    static constexpr u32 DefaultMobilCollisionGroup = 4u;
    static constexpr u32 DefaultMobilShadowCasterGroupMask = 2u;
    using MobilVariant = std::vector<CMwNodRef<CSceneMobil>>;

    CGameCtnBlockInfo(void);
    ~CGameCtnBlockInfo(void) override;
    void UpdateSize(void);
    void OnNodLoaded(void) override;
    int IsLandscape(void) const;
    unsigned long GetHeight(int isGround);
    unsigned long GetNbBlockUnitInfos(int isGround);
    CGameCtnBlockUnitInfo *GetBlockUnitInfo(unsigned long blockUnitIndex,
                                            int isGround);
    CFastBuffer<CSceneMobil *> *GetMobilBuffer(
            int family,
            unsigned long variantIndex);
    void SetAllBlockUnitSurfaces(void);
    void UpdateBlockHelperMobil(CSceneMobil *mobil, int family, int finalArg);
    void SetMobil(unsigned long mobilIndex, int family,
                  unsigned long variantIndex, CSceneMobil *mobil);
    void AddMobil(int family, unsigned long variantIndex, CSceneMobil *mobil);
    unsigned long GetVariantIndex(int family, unsigned long variantIndex);
    CSceneMobil *GetMobil(int family, unsigned long variantIndex,
                          unsigned long mobilIndex);
    void RemoveMobil(int family, unsigned long variantIndex,
                     unsigned long mobilIndex);
    CSceneMobil *BuildBlockMobil(int family, unsigned long variantIndex,
                                 unsigned long mobilIndex, int finalArg);
    CSceneMobil *BuildBlockHelperMobil(int family, int finalArg);
    void RemoveGroundBlocks(void);
    void RemoveAirBlocks(void);
    int IsPartUnderground(int isGround);
    int IsAllUnderground(int isGround);
    int IsTerrainModifier(int isGround);
    GmNat3 GetRotatedOffset(unsigned long blockUnitIndex,
                            ECardinalDir direction, int isGround);
    int CanPlaceOnGround(void);
    int CanPlaceInAir(void);
    void AddBlock(GmNat3 offset, int helperMask,
                  unsigned long junctionMask, int isGround);
    void ResetMobilVariants(int family, unsigned long variantCount);

    void ConfigureDefaultMobilItem(CHmsItem &item) const;
    void ConfigureDefaultMobil(CSceneMobil &mobil) const;
    const std::vector<std::unique_ptr<CGameCtnBlockUnitInfo>> &BlockUnitInfos(int isGround) const;
    unsigned long BlockUnitInfoCount(int isGround) const;
    CGameCtnBlockUnitInfo *BlockUnitInfoAt(unsigned long index, int isGround) const;
    CGameCtnBlockUnitInfo *FirstBlockUnitInfo(int isGround) const;
    CGameCtnBlockUnitInfo *TerrainModifierUnitAt(int isGround, unsigned long index) const;
    CGameCtnBlockUnitInfo *ReplacementCandidateAt(int isGround, unsigned long index) const;
    unsigned long UnitMaxLayer(int isGround) const;
    int UnitFamilyHasAnyUnderground(int isGround) const;
    int UnitFamilyAllUnderground(int isGround) const;
    const GmNat3 *UnitFamilySize(int isGround) const;
    GmNat3 RotatedUnitOffset(const CGameCtnBlockUnitInfo &unit,
                             ECardinalDir direction, int isGround) const;
    void AssignSurfaceToBlockUnits(int isGround);

    void ConfigurePlacement(EBlockType type, bool usesCustomSize,
                            BlockPlacementSource source);
    EBlockType Type(void) const;
    bool UsesCustomSize(void) const;
    BlockPlacementSource DefaultPlacementSource(void) const;
    BlockInterfaceKind InterfaceKind(void) const;
    const GmNat3 &SizeForMobilFamily(bool ground) const;
    const std::optional<GmIso4> &SpawnLocationForMobilFamily(bool ground) const;
    void SetMobilFamilySize(bool ground, const GmNat3 &size);
    void SetSpawnLocationForMobilFamily(bool ground,
                                       std::optional<GmIso4> location);
    void AddBlockUnitInfo(bool ground,
                          std::unique_ptr<CGameCtnBlockUnitInfo> unit);
    bool HasBlockUnitInfos(bool ground) const;
    CSceneMobil *FamilyHelperMobilSource(bool ground) const;
    CSceneMobil *CommonHelperMobilSource(void) const;
    void SetFamilyHelperMobilSource(bool ground, CSceneMobil *mobil);
    void SetCommonHelperMobilSource(CSceneMobil *mobil);
    void AddSourceMobil(CSceneMobil *mobil);
    const std::vector<CMwNodRef<CSceneMobil>> &SourceMobils(void) const;
    BlockRaceRole RaceRole(void) const;
    bool RespawnUsesCurrentTransform(void) const;
    void SetRaceRole(BlockRaceRole role);
    void SetRespawnUsesCurrentTransform(bool enabled);

protected:
    void SetDefaultFlagsOnMobil(CSceneMobil *mobil);
    CSceneMobil *CreateMobilFromSolid(CPlugSolid *solid);

private:
    std::vector<std::unique_ptr<CGameCtnBlockUnitInfo>> &
    MutableBlockUnitInfos(int isGround) {
        return isGround != 0 ? groundBlockUnitInfos_ : airBlockUnitInfos_;
    }
    BlockInterfaceKind interfaceKind_ = BlockInterfaceKind::None;
    GmNat3 groundSize_{1u, 1u, 1u};
    GmNat3 airSize_{1u, 1u, 1u};
    GmNat3 customSize_{0u, 1u, 0u};
    EBlockType blockType_ = EBlockType::Classic;
    bool usesCustomSize_ = false;
    BlockPlacementSource defaultPlacementSource_ =
            BlockPlacementSource::Primary;
    std::vector<std::unique_ptr<CGameCtnBlockUnitInfo>> groundBlockUnitInfos_;
    std::vector<std::unique_ptr<CGameCtnBlockUnitInfo>> airBlockUnitInfos_;
    CMwNodRef<CSceneMobil> groundHelperMobilSource_;
    CMwNodRef<CSceneMobil> airHelperMobilSource_;
    CMwNodRef<CSceneMobil> commonHelperMobilSource_;
    std::vector<CMwNodRef<CSceneMobil>> sourceMobils_;
    std::optional<GmIso4> groundMobilLocation_;
    std::optional<GmIso4> airMobilLocation_;
    BlockRaceRole raceRole_ = BlockRaceRole::None;
    bool respawnUsesCurrentTransform_ = false;
    CMwId blockUnitSurfaceId_;
    MobilStorage mobilVariants_;
    std::array<std::vector<CFastBuffer<CSceneMobil *>>, 2>
            mobilBufferViews_;
    static void SetHelperTreeFlags(CPlugTree *tree, int finalArg);
    static void InstallHelperTree(CSceneMobil *targetMobil,
                                  CSceneMobil *sourceMobil,
                                  const CMwId *helperTreeId,
                                  int finalArg);
};

class CGameCtnBlockInfoPylon : public CGameCtnBlockInfo {
public:
    void UpdateGroundMobilArray(unsigned long count);
    void OnNodLoaded(void) override;
    CSceneMobil *FirstSourceMobil(void) const;
    CSceneMobil *MiddleSourceMobil(void) const;
    CSceneMobil *LastSourceMobil(void) const;
    void SetSourceMobils(CSceneMobil *first,
                         CSceneMobil *middle,
                         CSceneMobil *last);

private:
    CMwNodRef<CSceneMobil> firstSourceMobil_;
    CMwNodRef<CSceneMobil> middleSourceMobil_;
    CMwNodRef<CSceneMobil> lastSourceMobil_;
};

class CGameCtnBlockInfoClip : public CGameCtnBlockInfo {
public:
    int IsClipCompatible(CGameCtnBlockInfoClip *other);
    BlockInfoAssetHandle SourceAsset(void) const;
    void SetSourceAsset(BlockInfoAssetHandle asset);
    void SetCompatibleClipId(const CMwId &id);

private:
    BlockInfoAssetHandle sourceAsset_;
    CMwId compatibleClipId_;
};
