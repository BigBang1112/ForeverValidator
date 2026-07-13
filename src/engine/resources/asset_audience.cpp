#include "engine/resources/asset_audience.h"
#include <algorithm>
#include <utility>

#include "engine/game/game_ctn_block_info.h"
#include "engine/game/game_ctn_decoration.h"
#include "engine/rendering/plug_shader.h"
#include "engine/resources/system_fid.h"
namespace {

bool IsBlockInfo(AssetType type) {
    return type == AssetType::GameCtnBlockInfo ||
           type == AssetType::GameCtnBlockInfoFlat ||
           type == AssetType::GameCtnBlockInfoFrontier ||
           type == AssetType::GameCtnBlockInfoClassic ||
           type == AssetType::GameCtnBlockInfoRoad ||
           type == AssetType::GameCtnBlockInfoClip ||
           type == AssetType::GameCtnBlockInfoSlope ||
           type == AssetType::GameCtnBlockInfoPylon ||
           type == AssetType::GameCtnBlockInfoRectAsym;
}

bool TypeMatches(AssetType candidate, AssetType accepted) {
    if (candidate == accepted) {
        return true;
    }
    if (accepted == AssetType::PlugShader) {
        return candidate == AssetType::PlugShader ||
               candidate == AssetType::PlugShaderGeneric ||
               candidate == AssetType::PlugShaderApply ||
               candidate == AssetType::PlugShaderSprite;
    }
    if (accepted == AssetType::GameCtnDecoration) {
        return candidate == AssetType::GameCtnDecoration ||
               candidate == AssetType::GameCtnDecorationTerrainModifier;
    }
    if (accepted == AssetType::GameCtnBlockInfo) {
        return IsBlockInfo(candidate);
    }
    if (accepted == AssetType::GameCtnCollector) {
        return candidate == AssetType::GameCtnCollector ||
               candidate == AssetType::GameCtnDecoration ||
               candidate == AssetType::GameCtnDecorationTerrainModifier ||
               IsBlockInfo(candidate);
    }
    if (accepted == AssetType::Plug) {
        return candidate >= AssetType::PlugAudio &&
               candidate <= AssetType::PlugDecoratorSolid;
    }
    if (accepted == AssetType::SceneObject) {
        return (candidate >= AssetType::SceneObject &&
                candidate <= AssetType::SceneMobil) ||
               candidate == AssetType::SceneVehicle ||
               candidate == AssetType::SceneVehicleCar;
    }
    if (accepted == AssetType::SceneMobil) {
        return candidate == AssetType::SceneMobil ||
               candidate == AssetType::SceneVehicle ||
               candidate == AssetType::SceneVehicleCar;
    }
    if (accepted == AssetType::SceneVehicle) {
        return candidate == AssetType::SceneVehicle ||
               candidate == AssetType::SceneVehicleCar;
    }
    if (accepted == AssetType::MwNod) {
        return candidate != AssetType::Unknown;
    }
    return false;
}

bool LoadedNodeMatches(const CMwNod *node, AssetType accepted) {
    if (node == nullptr) {
        return false;
    }
    if (accepted == AssetType::PlugShader) {
        return dynamic_cast<const CPlugShader *>(node) != nullptr;
    }
    if (accepted == AssetType::PlugShaderPass) {
        return dynamic_cast<const CPlugShaderPass *>(node) != nullptr;
    }
    if (accepted == AssetType::GameCtnDecoration) {
        return dynamic_cast<const CGameCtnDecoration *>(node) != nullptr;
    }
    if (accepted == AssetType::GameCtnBlockInfo) {
        return dynamic_cast<const CGameCtnBlockInfo *>(node) != nullptr;
    }
    return false;
}

}  // namespace

AssetAudience::AssetAudience(AssetType acceptedType)
        : acceptedTypes_{acceptedType} {}

AssetAudience::AssetAudience(
        std::initializer_list<AssetType> acceptedTypes)
        : acceptedTypes_(acceptedTypes) {}

AssetAudience::AssetAudience(std::vector<AssetType> acceptedTypes)
        : acceptedTypes_(std::move(acceptedTypes)) {}

AssetAudience::AssetAudience(
        bool acceptsAny,
        std::vector<AssetType> acceptedTypes)
        : acceptsAny_(acceptsAny),
          acceptedTypes_(std::move(acceptedTypes)) {}

AssetAudience AssetAudience::Any(void) {
    return AssetAudience(true, {});
}

bool AssetAudience::Accepts(AssetType type) const {
    return acceptsAny_ || std::any_of(
            acceptedTypes_.begin(),
            acceptedTypes_.end(),
            [type](AssetType accepted) {
                return TypeMatches(type, accepted);
            });
}

bool AssetAudience::Accepts(CSystemFid &fid) const {
    if (acceptsAny_) {
        return true;
    }
    const CMwNod *node = fid.LoadedNode();
    if (std::any_of(
                acceptedTypes_.begin(),
                acceptedTypes_.end(),
                [node](AssetType accepted) {
                    return LoadedNodeMatches(node, accepted);
                })) {
        return true;
    }
    return Accepts(fid.GetAssetType());
}

bool AssetAudience::Overlaps(const AssetAudience &other) const {
    if (acceptsAny_) {
        return other.acceptsAny_ || !other.acceptedTypes_.empty();
    }
    if (other.acceptsAny_) {
        return !acceptedTypes_.empty();
    }
    for (AssetType left : acceptedTypes_) {
        for (AssetType right : other.acceptedTypes_) {
            if (TypeMatches(left, right) || TypeMatches(right, left)) {
                return true;
            }
        }
    }
    return false;
}

bool AssetAudience::IsAny(void) const {
    return acceptsAny_;
}

bool AssetAudience::IsExactly(AssetType type) const {
    return !acceptsAny_ && acceptedTypes_.size() == 1u &&
           acceptedTypes_.front() == type;
}

AssetAudience AnyAssetAudience(void) {
    return AssetAudience::Any();
}

AssetAudience ShaderParameterAudience(void) {
    return AssetAudience(AssetType::PlugShader);
}

AssetAudience ShaderAndPassParameterAudience(void) {
    return AssetAudience({
        AssetType::PlugShader,
        AssetType::PlugShaderPass,
    });
}

AssetAudience InterfaceLabelParameterAudience(void) {
    return AssetAudience({
        AssetType::PlugShader,
        AssetType::PlugShaderPass,
        AssetType::MwNod,
        AssetType::Scene3d,
    });
}

AssetAudience BitmapParameterAudience(void) {
    return AssetAudience({
        AssetType::PlugShader,
        AssetType::PlugBitmapSampler,
    });
}

AssetAudience ShaderFlagsParameterAudience(void) {
    return AssetAudience({
        AssetType::PlugShader,
        AssetType::PlugShaderPass,
        AssetType::PlugFileGPU,
        AssetType::PlugFileVHlsl,
        AssetType::PlugFileGPUV,
        AssetType::PlugFileGPUP,
    });
}

AssetAudience SceneEngineAudience(void) {
    return AssetAudience(AssetType::SceneEngine);
}

AssetAudience DecorationAudience(void) {
    return AssetAudience(AssetType::GameCtnDecoration);
}

AssetAudience BlockInfoAudience(void) {
    return AssetAudience(AssetType::GameCtnBlockInfo);
}
