#include "engine/core/strided_array.h"
#include <type_traits>

#include "engine/rendering/plug_vertex_stream.h"
#include "engine/rendering/plug_visual.h"
namespace {

template <typename Value>
constexpr EPlugVDclType AttributeType(void);

template <>
constexpr EPlugVDclType AttributeType<GxBGRAColor>(void) {
    return EPlugVDclType_ColorD3D;
}

template <>
constexpr EPlugVDclType AttributeType<float>(void) {
    return EPlugVDclType_Float1;
}

template <>
constexpr EPlugVDclType AttributeType<GmVec2>(void) {
    return EPlugVDclType_Float2;
}

template <>
constexpr EPlugVDclType AttributeType<GmVec3>(void) {
    return EPlugVDclType_Float3;
}

template <>
constexpr EPlugVDclType AttributeType<GmVec4>(void) {
    return EPlugVDclType_Float4;
}

template <typename Value>
std::vector<Value> *ClassicValues(const CPlugVisual &visual,
                                  EPlugVDcl declaration) {
    auto *visual3D = const_cast<CPlugVisual3D *>(
            dynamic_cast<const CPlugVisual3D *>(&visual));
    if (visual3D == nullptr) return nullptr;

    if constexpr (std::is_same_v<Value, GmVec3>) {
        if (declaration == EPlugVDcl_Position) return &visual3D->positions;
        if (declaration == EPlugVDcl_Normal) return &visual3D->normals;
        if (declaration == EPlugVDcl_TangentU) return &visual3D->tangents;
        if (declaration == EPlugVDcl_TangentV) return &visual3D->binormals;
    }
    if constexpr (std::is_same_v<Value, GmVec4>) {
        if (declaration == EPlugVDcl_Color0) return &visual3D->colors;
    }
    return nullptr;
}

template <typename Value>
std::vector<Value> *SemanticValues(const CPlugVisual &visual,
                                   EPlugVDcl declaration) {
    CPlugVertexStream *stream = visual.VertexStreamGetByDecl(declaration);
    if (stream != nullptr) {
        VertexAttributeValues *values =
                stream->MutableAttributeValues(declaration);
        return values != nullptr && values->Type() == AttributeType<Value>()
                ? values->Values<Value>()
                : nullptr;
    }
    return ClassicValues<Value>(visual, declaration);
}

template <typename Value>
int AssignTypedView(CStridedArray<Value> &outValues,
                    const CPlugVisual &visual,
                    EPlugVDcl declaration) {
    std::vector<Value> *values = SemanticValues<Value>(visual, declaration);
    if (values == nullptr) return 0;
    outValues = CStridedArray<Value>(
            values->empty() ? nullptr : values->data(),
            static_cast<unsigned long>(values->size()));
    return 1;
}

VertexAttributeMutableView SemanticView(const CPlugVisual &visual,
                                        EPlugVDcl declaration,
                                        EPlugVDclType type) {
    CPlugVertexStream *stream = visual.VertexStreamGetByDecl(declaration);
    if (stream != nullptr) {
        VertexAttributeValues *values =
                stream->MutableAttributeValues(declaration);
        return values != nullptr && values->Type() == type
                ? values->MutableView()
                : VertexAttributeMutableView{};
    }

    auto *visual3D = const_cast<CPlugVisual3D *>(
            dynamic_cast<const CPlugVisual3D *>(&visual));
    if (visual3D == nullptr) return {};
    if (declaration == EPlugVDcl_Position &&
        type == EPlugVDclType_Float3) {
        return VertexAttributeMutableView(
                type, visual3D->positions.empty()
                              ? static_cast<GmVec3 *>(nullptr)
                              : visual3D->positions.data(),
                static_cast<unsigned long>(visual3D->positions.size()));
    }
    if (declaration == EPlugVDcl_Normal &&
        type == EPlugVDclType_Float3) {
        return VertexAttributeMutableView(
                type, visual3D->normals.empty()
                              ? static_cast<GmVec3 *>(nullptr)
                              : visual3D->normals.data(),
                static_cast<unsigned long>(visual3D->normals.size()));
    }
    if (declaration == EPlugVDcl_Color0 &&
        type == EPlugVDclType_Float4) {
        return VertexAttributeMutableView(
                type, visual3D->colors.empty()
                              ? static_cast<GmVec4 *>(nullptr)
                              : visual3D->colors.data(),
                static_cast<unsigned long>(visual3D->colors.size()));
    }
    if ((declaration == EPlugVDcl_TangentU ||
         declaration == EPlugVDcl_TangentV) &&
        type == EPlugVDclType_Float3) {
        std::vector<GmVec3> &directions = declaration == EPlugVDcl_TangentU
                ? visual3D->tangents
                : visual3D->binormals;
        return VertexAttributeMutableView(
                type, directions.empty() ? static_cast<GmVec3 *>(nullptr)
                                         : directions.data(),
                static_cast<unsigned long>(directions.size()));
    }
    return {};
}

} // namespace

unsigned long CPlugVisual::GetBytePerVertex(void) const {
    unsigned long result = 0u;
    for (const CMwNodRef<CPlugVertexStream> &stream : vertexStreams) {
        if (stream) result += stream->GetStrideTotal();
    }
    for (const GxTexCoordSet &set : texCoordSets) {
        result += set.Dimension() == GxTexCoordDimension::Two
                ? PlugVDclTypeByteCount(EPlugVDclType_Float2)
                : (set.Dimension() == GxTexCoordDimension::Three
                           ? PlugVDclTypeByteCount(EPlugVDclType_Float3)
                           : PlugVDclTypeByteCount(EPlugVDclType_Float4));
    }
    return result;
}

unsigned long CPlugVisual3D::GetBytePerVertex(void) const {
    if (!vertexStreams.empty()) return CPlugVisual::GetBytePerVertex();
    unsigned long result = CPlugVisual::GetBytePerVertex();
    if (!positions.empty()) result += PlugVDclTypeByteCount(EPlugVDclType_Float3);
    if (!normals.empty()) result += PlugVDclTypeByteCount(EPlugVDclType_Float3);
    if (!colors.empty()) result += PlugVDclTypeByteCount(EPlugVDclType_Float4);
    if (!tangents.empty()) result += PlugVDclTypeByteCount(EPlugVDclType_Float3);
    if (!binormals.empty()) result += PlugVDclTypeByteCount(EPlugVDclType_Float3);
    return result;
}

int CPlugVisual::GetStridedVertexElem(
        CStridedArray<GxBGRAColor> &outValues,
        EPlugVDcl declaration) const {
    return AssignTypedView(outValues, *this, declaration);
}

int CPlugVisual::GetStridedVertexElem(
        CStridedArray<float> &outValues,
        EPlugVDcl declaration) const {
    return AssignTypedView(outValues, *this, declaration);
}

int CPlugVisual::GetStridedVertexElem(
        CStridedArray<GmVec2> &outValues,
        EPlugVDcl declaration) const {
    return AssignTypedView(outValues, *this, declaration);
}

int CPlugVisual::GetStridedVertexElem(
        CStridedArray<GmVec3> &outValues,
        EPlugVDcl declaration) const {
    return AssignTypedView(outValues, *this, declaration);
}

int CPlugVisual::GetStridedVertexElem(
        CStridedArray<GmVec4> &outValues,
        EPlugVDcl declaration) const {
    return AssignTypedView(outValues, *this, declaration);
}

int CPlugVisual::GetStridedVertexElem(
        CStridedArray<unsigned char> &outValues,
        EPlugVDcl declaration,
        EPlugVDclType type) const {
    VertexAttributeMutableView values = SemanticView(*this, declaration, type);
    if (!values.IsBound()) return 0;
    outValues = CStridedArray<unsigned char>(values);
    return 1;
}
