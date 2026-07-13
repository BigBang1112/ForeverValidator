#pragma once

#include "engine/rendering/plug_material_custom_parameters.h"
namespace TmnfFormat {

struct MaterialCustomShaderFlagsArchiveRecord {
    u32 shaderConfigurationWord;
    u32 shaderApplyConfigurationWord;
};

struct MaterialCustomShaderFlagsArchiveCodec {
    static CPlugMaterialCustom::SFlags Decode(
            const MaterialCustomShaderFlagsArchiveRecord &record) {
        CPlugMaterialCustom::SFlags flags;
        flags.shaderConfiguration_ =
                std::bitset<31>(record.shaderConfigurationWord);
        flags.shaderApplyConfiguration_ =
                std::bitset<32>(record.shaderApplyConfigurationWord);
        return flags;
    }

    static MaterialCustomShaderFlagsArchiveRecord Encode(
            const CPlugMaterialCustom::SFlags &flags) {
        return {
                static_cast<u32>(flags.shaderConfiguration_.to_ulong()),
                static_cast<u32>(
                        flags.shaderApplyConfiguration_.to_ulong()),
        };
    }
};

}  // namespace TmnfFormat
