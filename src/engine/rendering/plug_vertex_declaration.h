#pragma once

#include <cstddef>
#include <cstdint>

enum EPlugVDcl : std::uint32_t {
    EPlugVDcl_Position = 0u,
    EPlugVDcl_Position1 = 1u,
    EPlugVDcl_PositionT = 2u,
    EPlugVDcl_BlendWeight = 3u,
    EPlugVDcl_BlendIndices = 4u,
    EPlugVDcl_Normal = 5u,
    EPlugVDcl_Normal1 = 6u,
    EPlugVDcl_PointSize = 7u,
    EPlugVDcl_Color0 = 8u,
    EPlugVDcl_Color1 = 9u,
    EPlugVDcl_TexCoord0 = 10u,
    EPlugVDcl_TexCoord1 = 11u,
    EPlugVDcl_TexCoord2 = 12u,
    EPlugVDcl_TexCoord3 = 13u,
    EPlugVDcl_TexCoord4 = 14u,
    EPlugVDcl_TexCoord5 = 15u,
    EPlugVDcl_TexCoord6 = 16u,
    EPlugVDcl_TexCoord7 = 17u,
    EPlugVDcl_TangentU = 18u,
    EPlugVDcl_TangentU1 = 19u,
    EPlugVDcl_TangentV = 20u,
    EPlugVDcl_TangentV1 = 21u,
};

enum EPlugVDclType : std::uint32_t {
    EPlugVDclType_Float1 = 0u,
    EPlugVDclType_Float2 = 1u,
    EPlugVDclType_Float3 = 2u,
    EPlugVDclType_Float4 = 3u,
    EPlugVDclType_ColorD3D = 4u,
    EPlugVDclType_UByte4 = 5u,
    EPlugVDclType_Short2 = 6u,
    EPlugVDclType_Short4 = 7u,
    EPlugVDclType_UByte4N = 8u,
    EPlugVDclType_Short2N = 9u,
    EPlugVDclType_Short4N = 10u,
    EPlugVDclType_UShort2N = 11u,
    EPlugVDclType_UShort4N = 12u,
    EPlugVDclType_UDec3 = 13u,
    EPlugVDclType_Dec3N = 14u,
    EPlugVDclType_Half2 = 15u,
    EPlugVDclType_Half4 = 16u,
    EPlugVDclType_Unused = 17u,
};

enum EPlugVertexTangent : std::uint32_t {
    EPlugVertexTangent_U = 0u,
    EPlugVertexTangent_V = 1u,
};

enum class EPlugVDclSpace : std::uint32_t {
    Point = 0u,
    Direction = 1u,
    Invariant = 2u,
};

constexpr bool PlugVDclIsValid(EPlugVDcl declaration) {
    return static_cast<std::uint32_t>(declaration) <=
           static_cast<std::uint32_t>(EPlugVDcl_TangentV1);
}

constexpr bool PlugVDclTypeIsMaterialized(EPlugVDclType type) {
    return static_cast<std::uint32_t>(type) <=
           static_cast<std::uint32_t>(EPlugVDclType_Half4);
}

constexpr EPlugVDclType PlugVDclDefaultType(EPlugVDcl declaration) {
    switch (declaration) {
    case EPlugVDcl_Position:
    case EPlugVDcl_Position1:
    case EPlugVDcl_Normal:
    case EPlugVDcl_Normal1:
    case EPlugVDcl_TangentU:
    case EPlugVDcl_TangentU1:
    case EPlugVDcl_TangentV:
    case EPlugVDcl_TangentV1:
        return EPlugVDclType_Float3;
    case EPlugVDcl_PositionT:
        return EPlugVDclType_Float4;
    case EPlugVDcl_BlendWeight:
    case EPlugVDcl_PointSize:
        return EPlugVDclType_Float1;
    case EPlugVDcl_BlendIndices:
        return EPlugVDclType_UByte4;
    case EPlugVDcl_Color0:
    case EPlugVDcl_Color1:
        return EPlugVDclType_ColorD3D;
    case EPlugVDcl_TexCoord0:
    case EPlugVDcl_TexCoord1:
    case EPlugVDcl_TexCoord2:
    case EPlugVDcl_TexCoord3:
    case EPlugVDcl_TexCoord4:
    case EPlugVDcl_TexCoord5:
    case EPlugVDcl_TexCoord6:
    case EPlugVDcl_TexCoord7:
        return EPlugVDclType_Float2;
    }
    return EPlugVDclType_Unused;
}

constexpr EPlugVDclSpace PlugVDclSpace(EPlugVDcl declaration) {
    switch (declaration) {
    case EPlugVDcl_Position:
    case EPlugVDcl_Position1:
        return EPlugVDclSpace::Point;
    case EPlugVDcl_Normal:
    case EPlugVDcl_Normal1:
    case EPlugVDcl_TangentU:
    case EPlugVDcl_TangentU1:
    case EPlugVDcl_TangentV:
    case EPlugVDcl_TangentV1:
        return EPlugVDclSpace::Direction;
    case EPlugVDcl_PositionT:
    case EPlugVDcl_BlendWeight:
    case EPlugVDcl_BlendIndices:
    case EPlugVDcl_PointSize:
    case EPlugVDcl_Color0:
    case EPlugVDcl_Color1:
    case EPlugVDcl_TexCoord0:
    case EPlugVDcl_TexCoord1:
    case EPlugVDcl_TexCoord2:
    case EPlugVDcl_TexCoord3:
    case EPlugVDcl_TexCoord4:
    case EPlugVDcl_TexCoord5:
    case EPlugVDcl_TexCoord6:
    case EPlugVDcl_TexCoord7:
        return EPlugVDclSpace::Invariant;
    }
    return EPlugVDclSpace::Invariant;
}

constexpr std::size_t PlugVDclTypeByteCount(EPlugVDclType type) {
    switch (type) {
    case EPlugVDclType_Float1:
    case EPlugVDclType_ColorD3D:
    case EPlugVDclType_UByte4:
    case EPlugVDclType_Short2:
    case EPlugVDclType_UByte4N:
    case EPlugVDclType_Short2N:
    case EPlugVDclType_UShort2N:
    case EPlugVDclType_UDec3:
    case EPlugVDclType_Dec3N:
    case EPlugVDclType_Half2:
        return 4u;
    case EPlugVDclType_Float2:
    case EPlugVDclType_Short4:
    case EPlugVDclType_Short4N:
    case EPlugVDclType_UShort4N:
    case EPlugVDclType_Half4:
        return 8u;
    case EPlugVDclType_Float3:
        return 12u;
    case EPlugVDclType_Float4:
        return 16u;
    case EPlugVDclType_Unused:
        return 0u;
    }
    return 0u;
}

constexpr std::size_t PlugClassicVertexByteCount =
        PlugVDclTypeByteCount(EPlugVDclType_Float3) +
        PlugVDclTypeByteCount(EPlugVDclType_Float3) +
        PlugVDclTypeByteCount(EPlugVDclType_Float4);

constexpr std::size_t PlugVDclTypeComponentCount(EPlugVDclType type) {
    switch (type) {
    case EPlugVDclType_Float1:
        return 1u;
    case EPlugVDclType_Float2:
    case EPlugVDclType_Short2:
    case EPlugVDclType_Short2N:
    case EPlugVDclType_UShort2N:
    case EPlugVDclType_Half2:
        return 2u;
    case EPlugVDclType_Float3:
    case EPlugVDclType_UDec3:
    case EPlugVDclType_Dec3N:
        return 3u;
    case EPlugVDclType_Float4:
    case EPlugVDclType_ColorD3D:
    case EPlugVDclType_UByte4:
    case EPlugVDclType_Short4:
    case EPlugVDclType_UByte4N:
    case EPlugVDclType_Short4N:
    case EPlugVDclType_UShort4N:
    case EPlugVDclType_Half4:
        return 4u;
    case EPlugVDclType_Unused:
        return 0u;
    }
    return 0u;
}

constexpr bool PlugVDclIsTexCoord(EPlugVDcl declaration) {
    return declaration >= EPlugVDcl_TexCoord0 &&
           declaration <= EPlugVDcl_TexCoord7;
}
