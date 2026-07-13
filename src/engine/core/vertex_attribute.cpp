#include "engine/core/vertex_attribute.h"
#include <algorithm>
#include <type_traits>
#include <utility>

namespace {

VertexAttributeValues::Storage MakeStorage(EPlugVDclType type,
                                           unsigned long count) {
    switch (type) {
    case EPlugVDclType_Float1: return std::vector<float>(count);
    case EPlugVDclType_Float2: return std::vector<GmVec2>(count);
    case EPlugVDclType_Float3: return std::vector<GmVec3>(count);
    case EPlugVDclType_Float4: return std::vector<GmVec4>(count);
    case EPlugVDclType_ColorD3D: return std::vector<GxBGRAColor>(count);
    case EPlugVDclType_UByte4: return std::vector<VertexUByte4>(count);
    case EPlugVDclType_Short2: return std::vector<VertexShort2>(count);
    case EPlugVDclType_Short4: return std::vector<VertexShort4>(count);
    case EPlugVDclType_UByte4N: return std::vector<VertexUByte4N>(count);
    case EPlugVDclType_Short2N: return std::vector<VertexShort2N>(count);
    case EPlugVDclType_Short4N: return std::vector<VertexShort4N>(count);
    case EPlugVDclType_UShort2N: return std::vector<VertexUShort2N>(count);
    case EPlugVDclType_UShort4N: return std::vector<VertexUShort4N>(count);
    case EPlugVDclType_UDec3: return std::vector<VertexUDec3>(count);
    case EPlugVDclType_Dec3N: return std::vector<VertexDec3N>(count);
    case EPlugVDclType_Half2: return std::vector<VertexHalf2>(count);
    case EPlugVDclType_Half4: return std::vector<VertexHalf4>(count);
    case EPlugVDclType_Unused: break;
    }
    return std::vector<float>();
}

template <typename Values>
void MoveRows(Values &values,
              unsigned long destinationIndex,
              unsigned long sourceIndex,
              unsigned long count) {
    if (count == 0u || destinationIndex == sourceIndex) {
        return;
    }
    Values copied(values.begin() + sourceIndex,
                values.begin() + sourceIndex + count);
    std::copy(copied.begin(), copied.end(),
              values.begin() + destinationIndex);
}

} // namespace

bool VertexAttributeMutableView::IsBound(void) const { return bound_; }

EPlugVDclType VertexAttributeMutableView::Type(void) const { return type_; }

unsigned long VertexAttributeMutableView::Count(void) const { return count_; }

bool VertexAttributeMutableView::GetAt(
        unsigned long index,
        VertexAttributeValue &outValue) const {
    if (!bound_ || index >= count_) {
        return false;
    }
    return std::visit(
            [index, &outValue](auto values) {
                using Pointer = decltype(values);
                if constexpr (std::is_same_v<Pointer, std::monostate>) {
                    return false;
                } else {
                    if (values == nullptr) return false;
                    outValue = values[index];
                    return true;
                }
            },
            storage_);
}

bool VertexAttributeMutableView::SetAt(
        unsigned long index,
        const VertexAttributeValue &value) const {
    if (!bound_ || index >= count_) {
        return false;
    }
    return std::visit(
            [index, &value](auto values) {
                using Pointer = decltype(values);
                if constexpr (std::is_same_v<Pointer, std::monostate>) {
                    return false;
                } else {
                    using Value = std::remove_pointer_t<Pointer>;
                    const Value *typed = std::get_if<Value>(&value);
                    if (values == nullptr || typed == nullptr) return false;
                    values[index] = *typed;
                    return true;
                }
            },
            storage_);
}

VertexAttributeValues::VertexAttributeValues(EPlugVDclType type,
                                             unsigned long count)
        : type_(PlugVDclTypeIsMaterialized(type) ? type
                                                 : EPlugVDclType_Float1),
          values_(MakeStorage(type_, count)) {}

VertexAttributeValues::VertexAttributeValues(std::vector<float> values)
        : type_(EPlugVDclType_Float1), values_(std::move(values)) {}

VertexAttributeValues::VertexAttributeValues(std::vector<GmVec2> values)
        : type_(EPlugVDclType_Float2), values_(std::move(values)) {}

VertexAttributeValues::VertexAttributeValues(std::vector<GmVec3> values)
        : type_(EPlugVDclType_Float3), values_(std::move(values)) {}

VertexAttributeValues::VertexAttributeValues(std::vector<GmVec4> values)
        : type_(EPlugVDclType_Float4), values_(std::move(values)) {}

VertexAttributeValues::VertexAttributeValues(
        std::vector<GxBGRAColor> values)
        : type_(EPlugVDclType_ColorD3D), values_(std::move(values)) {}

VertexAttributeValues::VertexAttributeValues(GxTexCoordSet values)
        : type_(values.Dimension() == GxTexCoordDimension::Two
                        ? EPlugVDclType_Float2
                        : (values.Dimension() == GxTexCoordDimension::Three
                                   ? EPlugVDclType_Float3
                                   : EPlugVDclType_Float4)),
          values_(MakeStorage(type_, values.Count())) {
    for (unsigned long index = 0u; index < values.Count(); ++index) {
        const GxTexCoord4 coordinate = values.Coordinate4At(index);
        if (type_ == EPlugVDclType_Float2) {
            std::get<std::vector<GmVec2>>(values_)[index] =
                    {coordinate.u, coordinate.v};
        } else if (type_ == EPlugVDclType_Float3) {
            std::get<std::vector<GmVec3>>(values_)[index] =
                    {coordinate.u, coordinate.v, coordinate.w};
        } else {
            std::get<std::vector<GmVec4>>(values_)[index] =
                    {coordinate.u, coordinate.v,
                     coordinate.w, coordinate.q};
        }
    }
}

EPlugVDclType VertexAttributeValues::Type(void) const { return type_; }

unsigned long VertexAttributeValues::Count(void) const {
    return std::visit(
            [](const auto &values) {
                return static_cast<unsigned long>(values.size());
            },
            values_);
}

const VertexAttributeValues::Storage &
VertexAttributeValues::StorageValues(void) const { return values_; }

VertexAttributeValues::Storage &VertexAttributeValues::StorageValues(void) {
    return values_;
}

VertexAttributeValue VertexAttributeValues::ValueAt(
        unsigned long index) const {
    return std::visit(
            [index](const auto &values) -> VertexAttributeValue {
                return values.at(index);
            },
            values_);
}

bool VertexAttributeValues::SetValueAt(
        unsigned long index,
        const VertexAttributeValue &value) {
    if (index >= Count()) return false;
    return std::visit(
            [index, &value](auto &values) {
                using Value = typename std::decay_t<decltype(values)>::value_type;
                const Value *typed = std::get_if<Value>(&value);
                if (typed == nullptr) return false;
                values[index] = *typed;
                return true;
            },
            values_);
}

VertexAttributeMutableView VertexAttributeValues::MutableView(void) {
    return std::visit(
            [this](auto &values) {
                using Value = typename std::decay_t<decltype(values)>::value_type;
                return VertexAttributeMutableView(
                        type_, values.empty() ? static_cast<Value *>(nullptr)
                                              : values.data(),
                        static_cast<unsigned long>(values.size()));
            },
            values_);
}

bool VertexAttributeValues::ToTexCoordSet(GxTexCoordSet &outSet) const {
    GxTexCoordDimension dimension;
    if (type_ == EPlugVDclType_Float2) {
        dimension = GxTexCoordDimension::Two;
    } else if (type_ == EPlugVDclType_Float3) {
        dimension = GxTexCoordDimension::Three;
    } else if (type_ == EPlugVDclType_Float4) {
        dimension = GxTexCoordDimension::Four;
    } else {
        return false;
    }
    GxTexCoordSet result = GxTexCoordSet::Create(dimension, Count());
    for (unsigned long index = 0u; index < Count(); ++index) {
        GxTexCoord4 coordinate{};
        if (const auto *values = Values<GmVec2>()) {
            coordinate = {(*values)[index].x, (*values)[index].y, 1.0f, 1.0f};
        } else if (const auto *values = Values<GmVec3>()) {
            coordinate = {(*values)[index].x, (*values)[index].y,
                          (*values)[index].z, 1.0f};
        } else if (const auto *values = Values<GmVec4>()) {
            coordinate = {(*values)[index].x, (*values)[index].y,
                          (*values)[index].z, (*values)[index].w};
        }
        result.SetCoordinate(index, coordinate);
    }
    outSet = std::move(result);
    return true;
}

void VertexAttributeValues::Resize(unsigned long count) {
    std::visit([count](auto &values) { values.resize(count); }, values_);
}

void VertexAttributeValues::InsertDefault(unsigned long index,
                                          unsigned long count) {
    if (index > Count() || count == 0u) return;
    std::visit(
            [index, count](auto &values) {
                using Value = typename std::decay_t<decltype(values)>::value_type;
                values.insert(values.begin() + index, count, Value{});
            },
            values_);
}

void VertexAttributeValues::Erase(unsigned long index,
                                  unsigned long count) {
    if (index > Count() || count > Count() - index || count == 0u) return;
    std::visit(
            [index, count](auto &values) {
                values.erase(values.begin() + index,
                             values.begin() + index + count);
            },
            values_);
}

void VertexAttributeValues::MoveRange(unsigned long destinationIndex,
                                      unsigned long sourceIndex,
                                      unsigned long count) {
    const unsigned long size = Count();
    if (destinationIndex > size || sourceIndex > size ||
        count > size - destinationIndex || count > size - sourceIndex) {
        return;
    }
    std::visit(
            [destinationIndex, sourceIndex, count](auto &values) {
                MoveRows(values, destinationIndex, sourceIndex, count);
            },
            values_);
}

void VertexAttributeValues::Clear(void) { Resize(0u); }
