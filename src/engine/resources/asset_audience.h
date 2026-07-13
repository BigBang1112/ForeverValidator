#pragma once

#include <initializer_list>
#include <vector>

#include "engine/resources/asset_type.h"
struct CSystemFid;

class AssetAudience {
public:
    AssetAudience(void) = default;
    explicit AssetAudience(AssetType acceptedType);
    AssetAudience(std::initializer_list<AssetType> acceptedTypes);
    explicit AssetAudience(std::vector<AssetType> acceptedTypes);

    static AssetAudience Any(void);

    bool Accepts(AssetType type) const;
    bool Accepts(CSystemFid &fid) const;
    bool Overlaps(const AssetAudience &other) const;
    bool IsAny(void) const;
    bool IsExactly(AssetType type) const;

private:
    AssetAudience(bool acceptsAny, std::vector<AssetType> acceptedTypes);

    bool acceptsAny_ = false;
    std::vector<AssetType> acceptedTypes_;
};

AssetAudience AnyAssetAudience(void);
AssetAudience ShaderParameterAudience(void);
AssetAudience ShaderAndPassParameterAudience(void);
AssetAudience InterfaceLabelParameterAudience(void);
AssetAudience BitmapParameterAudience(void);
AssetAudience ShaderFlagsParameterAudience(void);
AssetAudience SceneEngineAudience(void);
AssetAudience DecorationAudience(void);
AssetAudience BlockInfoAudience(void);
