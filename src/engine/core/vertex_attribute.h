#pragma once

#include <array>
#include <cstdint>
#include <variant>
#include <vector>

#include "engine/core/gm_types.h"
#include "engine/core/gx_color.h"
#include "engine/core/gx_tex_coord.h"
#include "engine/rendering/plug_vertex_declaration.h"
struct VertexUByte4 { std::array<std::uint8_t, 4> value{}; };
struct VertexShort2 { std::array<std::int16_t, 2> value{}; };
struct VertexShort4 { std::array<std::int16_t, 4> value{}; };
struct VertexUByte4N { std::array<std::uint8_t, 4> value{}; };
struct VertexShort2N { std::array<std::int16_t, 2> value{}; };
struct VertexShort4N { std::array<std::int16_t, 4> value{}; };
struct VertexUShort2N { std::array<std::uint16_t, 2> value{}; };
struct VertexUShort4N { std::array<std::uint16_t, 4> value{}; };
struct VertexUDec3 { std::uint32_t value = 0u; };
struct VertexDec3N { std::uint32_t value = 0u; };
struct VertexHalf2 { std::array<std::uint16_t, 2> value{}; };
struct VertexHalf4 { std::array<std::uint16_t, 4> value{}; };

using VertexAttributeValue = std::variant<
        float,
        GmVec2,
        GmVec3,
        GmVec4,
        GxBGRAColor,
        VertexUByte4,
        VertexShort2,
        VertexShort4,
        VertexUByte4N,
        VertexShort2N,
        VertexShort4N,
        VertexUShort2N,
        VertexUShort4N,
        VertexUDec3,
        VertexDec3N,
        VertexHalf2,
        VertexHalf4>;

class VertexAttributeMutableView {
public:
    using Storage = std::variant<
            std::monostate,
            float *,
            GmVec2 *,
            GmVec3 *,
            GmVec4 *,
            GxBGRAColor *,
            VertexUByte4 *,
            VertexShort2 *,
            VertexShort4 *,
            VertexUByte4N *,
            VertexShort2N *,
            VertexShort4N *,
            VertexUShort2N *,
            VertexUShort4N *,
            VertexUDec3 *,
            VertexDec3N *,
            VertexHalf2 *,
            VertexHalf4 *>;

    VertexAttributeMutableView(void) = default;

    template <typename Value>
    VertexAttributeMutableView(EPlugVDclType type,
                               Value *values,
                               unsigned long count)
            : type_(type), count_(count), storage_(values), bound_(true) {}

    bool IsBound(void) const;
    EPlugVDclType Type(void) const;
    unsigned long Count(void) const;
    bool GetAt(unsigned long index, VertexAttributeValue &outValue) const;
    bool SetAt(unsigned long index, const VertexAttributeValue &value) const;

private:
    EPlugVDclType type_ = EPlugVDclType_Unused;
    unsigned long count_ = 0u;
    Storage storage_{};
    bool bound_ = false;
};

class VertexAttributeValues {
public:
    using Storage = std::variant<
            std::vector<float>,
            std::vector<GmVec2>,
            std::vector<GmVec3>,
            std::vector<GmVec4>,
            std::vector<GxBGRAColor>,
            std::vector<VertexUByte4>,
            std::vector<VertexShort2>,
            std::vector<VertexShort4>,
            std::vector<VertexUByte4N>,
            std::vector<VertexShort2N>,
            std::vector<VertexShort4N>,
            std::vector<VertexUShort2N>,
            std::vector<VertexUShort4N>,
            std::vector<VertexUDec3>,
            std::vector<VertexDec3N>,
            std::vector<VertexHalf2>,
            std::vector<VertexHalf4>>;

    explicit VertexAttributeValues(EPlugVDclType type = EPlugVDclType_Float1,
                                   unsigned long count = 0u);
    explicit VertexAttributeValues(std::vector<float> values);
    explicit VertexAttributeValues(std::vector<GmVec2> values);
    explicit VertexAttributeValues(std::vector<GmVec3> values);
    explicit VertexAttributeValues(std::vector<GmVec4> values);
    explicit VertexAttributeValues(std::vector<GxBGRAColor> values);
    explicit VertexAttributeValues(GxTexCoordSet values);

    EPlugVDclType Type(void) const;
    unsigned long Count(void) const;
    const Storage &StorageValues(void) const;
    Storage &StorageValues(void);
    VertexAttributeValue ValueAt(unsigned long index) const;
    bool SetValueAt(unsigned long index, const VertexAttributeValue &value);
    VertexAttributeMutableView MutableView(void);

    template <typename Value>
    const std::vector<Value> *Values(void) const {
        return std::get_if<std::vector<Value>>(&values_);
    }

    template <typename Value>
    std::vector<Value> *Values(void) {
        return std::get_if<std::vector<Value>>(&values_);
    }

    bool ToTexCoordSet(GxTexCoordSet &outSet) const;
    void Resize(unsigned long count);
    void InsertDefault(unsigned long index, unsigned long count);
    void Erase(unsigned long index, unsigned long count);
    void MoveRange(unsigned long destinationIndex,
                   unsigned long sourceIndex,
                   unsigned long count);
    void Clear(void);

private:
    EPlugVDclType type_;
    Storage values_;
};
