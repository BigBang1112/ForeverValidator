#pragma once

#include "engine/core/engine_types.h"
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "engine/resources/asset_audience.h"
#include "engine/core/cmw_nod.h"
#include "engine/core/cfast_buffer.h"
#include "engine/resources/system_fid_parameters.h"
struct CSystemFid;
struct CSystemFids;
struct CSystemFidsFolder;
struct CSystemPackDesc;

class CGameSkin : public CMwNod {
public:
    CGameSkin(void);
    ~CGameSkin(void) override;
    CGameSkin(const CGameSkin &) = delete;
    CGameSkin &operator=(const CGameSkin &) = delete;
    CGameSkin(CGameSkin &&) = delete;
    CGameSkin &operator=(CGameSkin &&) = delete;

    void AddFidRemap(
            CSystemFid &target,
            std::string prefix,
            AssetAudience audience,
            unsigned long packElementIndex =
                    CSystemFidParameters::DefaultPackElemIndex);
    void AddFidsRemap(
            CSystemFids &target,
            std::string prefix,
            AssetAudience audience,
            unsigned long packElementIndex =
                    CSystemFidParameters::DefaultPackElemIndex);
    int IsFidParamsInstalled(void) const;
    void EndLoading(void) const;
    void StartLoading(CSystemPackDesc *packDesc,
                      const CGameSkin *parentSkin,
                      CSystemPackDesc *parentPackDesc) const;
    void StartLoading(CFastBuffer<CSystemPackDesc *> &packDescs) const;
    void AddRemappingParams(CSystemFidParameters &fidParameters,
                            CSystemFidsFolder *fidsFolder) const;

private:
    class FidDependency {
    public:
        FidDependency(CGameSkin &owner, CSystemFid &target);
        ~FidDependency(void);
        FidDependency(const FidDependency &) = delete;
        FidDependency &operator=(const FidDependency &) = delete;
        FidDependency(FidDependency &&other) noexcept;
        FidDependency &operator=(FidDependency &&) = delete;

        CSystemFid *Get(void) const;
        bool ClearKilled(CMwNod *killedNod);

    private:
        CGameSkin *owner_ = nullptr;
        CSystemFid *target_ = nullptr;
    };

    struct FidRemap {
        FidRemap(void) = delete;
        FidRemap(CGameSkin &owner,
                 CSystemFid &target,
                 std::string prefix,
                 AssetAudience audience,
                 unsigned long packElementIndex);
        CSystemFid *Target(void) const;
        bool ClearKilled(CMwNod *killedNod);
        std::string prefix;
        AssetAudience audience;
        unsigned long packElementIndex;
        FidDependency target;
    };
    struct FidsRemap {
        FidsRemap(void) = delete;
        FidsRemap(CSystemFids &target,
                  std::string prefix,
                  AssetAudience audience,
                  unsigned long packElementIndex);
        CSystemFids *Target(void) const;
        bool ClearKilled(CMwNod *killedNod);
        CSystemFids *target;
        std::string prefix;
        AssetAudience audience;
        unsigned long packElementIndex;
    };
    using Remap = std::variant<FidRemap, FidsRemap>;

    void MwIsKilled(CMwNod *killedNod) override;
    void InternalStartLoading(const CGameSkin *sourceSkin,
                              CSystemPackDesc *packDesc) const;
    void StartLoadingPacks(
            const std::vector<CSystemPackDesc *> &packDescs) const;

    mutable std::vector<CSystemFidsFolder *> remappingRoots;
    mutable CSystemFidParameters activeParameters;
    std::vector<Remap> remaps;
    mutable std::optional<CSystemFidParameters::Scope> fidParameterScope;
};
