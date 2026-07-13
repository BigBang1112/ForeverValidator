#pragma once

#include "engine/rendering/plug_material_custom_parameters.h"
namespace TmnfFormat {

struct MaterialCustomGpuParameterArchiveShape {
    u32 componentCount;
    u32 registerCount;
};

struct MaterialCustomGpuParameterArchiveCodec {
    static bool DecodeShape(
            const MaterialCustomGpuParameterArchiveShape &shape,
            CPlugMaterialCustom::SGpuFx &parameter) {
        using GpuParameter = CPlugMaterialCustom::SGpuFx;
        if (shape.componentCount < GpuParameter::MinComponentCount ||
            shape.componentCount > GpuParameter::MaxComponentCount ||
            shape.registerCount > GpuParameter::MaxRegisterCount) {
            return false;
        }
        parameter.SetRegisterCountAndAlloc(shape.registerCount);
        parameter.SetComponentCount(shape.componentCount);
        return true;
    }

    static MaterialCustomGpuParameterArchiveShape EncodeShape(
            const CPlugMaterialCustom::SGpuFx &parameter) {
        return {
                parameter.ComponentCount(),
                static_cast<u32>(parameter.RegisterCount()),
        };
    }
};

}  // namespace TmnfFormat
