#include "engine/game/game_skin.h"
#include "engine/resources/system_engine.h"
#include "engine/resources/system_fid.h"
#include "engine/resources/system_fid_file.h"
#include "engine/resources/system_fid_parameters.h"
#include "engine/resources/system_fids.h"
#include "engine/resources/system_pack_desc.h"
#include "engine/resources/system_pack_manager.h"
#include <algorithm>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

CFastString FastStringFrom(const std::string &source) {
    CFastString result;
    result.SetString(static_cast<u32>(source.size()), source.data());
    return result;
}

template <typename Remap>
bool CanImportRemap(
        const Remap &remap,
        CSystemPackDesc *packDesc,
        const CFastString &prefix) {
    auto *target = remap.Target();
    if constexpr (std::is_same_v<decltype(target), CSystemFids *>) {
        if (target != nullptr) {
            return true;
        }
    }

    if (CSystemEngine::s_PackManager == nullptr) {
        return false;
    }

    int missing = 0;
    CSystemFidFile *packElem = CSystemEngine::s_PackManager->SimpleGetPackElem(
            packDesc,
            prefix,
            remap.audience,
            missing,
            0);
    return packElem != nullptr || missing == 0;
}

template <typename Remap>
void AddRemappingParameter(
        CSystemFidParameters &parameters,
        const Remap &remap,
        const CFastString &prefix,
        CSystemPackDesc *packDesc) {
    auto *target = remap.Target();
    if (target == nullptr) {
        return;
    }

    if constexpr (std::is_same_v<decltype(target), CSystemFids *>) {
        parameters.AddTemporaryFidsRemap(
                target,
                packDesc,
                &prefix,
                remap.packElementIndex,
                1);
    } else {
        parameters.AddTemporaryFidRemap(
                target,
                packDesc,
                &prefix,
                remap.packElementIndex);
    }
}

std::string PrefixUnderFolder(
        const CFastString &folder,
        const std::string &prefix) {
    std::string composed(folder.Data(), folder.Length());
    composed.append(prefix);
    return composed;
}

}  // namespace

CGameSkin::FidDependency::FidDependency(
        CGameSkin &owner,
        CSystemFid &target) {
    target.MwAddDependant(&owner);
    owner_ = &owner;
    target_ = &target;
}

CGameSkin::FidDependency::~FidDependency(void) {
    if (target_ != nullptr) {
        target_->MwSubDependantSafe(owner_);
    }
}

CGameSkin::FidDependency::FidDependency(
        FidDependency &&other) noexcept
    : owner_(other.owner_),
      target_(other.target_) {
    other.owner_ = nullptr;
    other.target_ = nullptr;
}

CSystemFid *CGameSkin::FidDependency::Get(void) const {
    return target_;
}

bool CGameSkin::FidDependency::ClearKilled(CMwNod *killedNod) {
    if (target_ != killedNod) {
        return false;
    }
    owner_ = nullptr;
    target_ = nullptr;
    return true;
}

CGameSkin::FidRemap::FidRemap(
        CGameSkin &owner,
        CSystemFid &targetValue,
        std::string prefixValue,
        AssetAudience audienceValue,
        unsigned long packElementIndexValue)
    : prefix(std::move(prefixValue)),
      audience(std::move(audienceValue)),
      packElementIndex(packElementIndexValue),
      target(owner, targetValue) {
}

CSystemFid *CGameSkin::FidRemap::Target(void) const {
    return target.Get();
}

bool CGameSkin::FidRemap::ClearKilled(CMwNod *killedNod) {
    return target.ClearKilled(killedNod);
}

CGameSkin::FidsRemap::FidsRemap(
        CSystemFids &targetValue,
        std::string prefixValue,
        AssetAudience audienceValue,
        unsigned long packElementIndexValue)
    : target(&targetValue),
      prefix(std::move(prefixValue)),
      audience(std::move(audienceValue)),
      packElementIndex(packElementIndexValue) {
}

CSystemFids *CGameSkin::FidsRemap::Target(void) const {
    return target;
}

bool CGameSkin::FidsRemap::ClearKilled(CMwNod *killedNod) {
    if (target != killedNod) {
        return false;
    }
    target = nullptr;
    return true;
}

CGameSkin::CGameSkin(void) = default;

CGameSkin::~CGameSkin(void) = default;

void CGameSkin::AddFidRemap(
        CSystemFid &target,
        std::string prefix,
        AssetAudience audience,
        unsigned long packElementIndex) {
    // Each direct Fid remap owns its weak-dependency registration, including
    // repeated remaps that target the same Fid.
    remaps.emplace_back(
            std::in_place_type<FidRemap>,
            *this,
            target,
            std::move(prefix),
            std::move(audience),
            packElementIndex);
}

void CGameSkin::AddFidsRemap(
        CSystemFids &target,
        std::string prefix,
        AssetAudience audience,
        unsigned long packElementIndex) {
    remaps.emplace_back(
            std::in_place_type<FidsRemap>,
            target,
            std::move(prefix),
            std::move(audience),
            packElementIndex);
}

void CGameSkin::MwIsKilled(CMwNod *killedNod) {
    for (Remap &remap : remaps) {
        const bool cleared = std::visit(
                [killedNod](auto &typedRemap) {
                    return typedRemap.ClearKilled(killedNod);
                },
                remap);
        if (cleared) {
            return;
        }
    }
    CMwNod::MwIsKilled(killedNod);
}

int CGameSkin::IsFidParamsInstalled(void) const {
    return fidParameterScope.has_value();
}

void CGameSkin::EndLoading(void) const {
    fidParameterScope.reset();
}

void CGameSkin::InternalStartLoading(
        const CGameSkin *sourceSkin,
        CSystemPackDesc *packDesc) const {
    for (const Remap &remap : sourceSkin->remaps) {
        std::visit(
                [this, packDesc](const auto &typedRemap) {
                    CFastString prefix = FastStringFrom(typedRemap.prefix);
                    if (!CanImportRemap(typedRemap, packDesc, prefix)) {
                        return;
                    }
                    AddRemappingParameter(
                            activeParameters,
                            typedRemap,
                            prefix,
                            packDesc);
                },
                remap);
    }
}

void CGameSkin::StartLoading(
        CSystemPackDesc *packDesc,
        const CGameSkin *parentSkin,
        CSystemPackDesc *parentPackDesc) const {
    fidParameterScope.reset();
    activeParameters = CSystemFidParameters::Empty;
    if (parentSkin != nullptr && parentPackDesc != nullptr) {
        InternalStartLoading(parentSkin, parentPackDesc);
    }
    if (packDesc != nullptr) {
        InternalStartLoading(this, packDesc);
    }
    fidParameterScope.emplace(activeParameters, 1);
}

void CGameSkin::StartLoading(
        CFastBuffer<CSystemPackDesc *> &packDescs) const {
    StartLoadingPacks(packDescs.Values());
}

void CGameSkin::StartLoadingPacks(
        const std::vector<CSystemPackDesc *> &packDescs) const {
    fidParameterScope.reset();
    activeParameters = CSystemFidParameters::Empty;
    // Remap priority is descriptor-major; null descriptors are forwarded.
    for (CSystemPackDesc *packDesc : packDescs) {
        InternalStartLoading(this, packDesc);
    }
    fidParameterScope.emplace(activeParameters, 1);
}

void CGameSkin::AddRemappingParams(
        CSystemFidParameters &fidParameters,
        CSystemFidsFolder *fidsFolder) const {
    const auto root = std::find(
            remappingRoots.begin(),
            remappingRoots.end(),
            fidsFolder);
    if (root == remappingRoots.end()) {
        fidsFolder->Browse(1);
        remappingRoots.push_back(fidsFolder);
    }

    CFastStringInt folderWide;
    fidsFolder->GetFullName(folderWide, 1);
    CFastString folderAscii;
    folderWide.GetAscii(folderAscii);

    for (const Remap &remap : remaps) {
        std::visit(
                [&fidParameters, &folderAscii](const auto &typedRemap) {
                    if (typedRemap.prefix.empty()) {
                        return;
                    }
                    const std::string fullPrefix = PrefixUnderFolder(
                            folderAscii,
                            typedRemap.prefix);
                    const CFastString prefix = FastStringFrom(fullPrefix);
                    AddRemappingParameter(
                            fidParameters,
                            typedRemap,
                            prefix,
                            nullptr);
                },
                remap);
    }
}
