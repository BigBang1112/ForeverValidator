#include "engine/core/gx_tex_coord.h"
#include <algorithm>
#include <type_traits>

namespace {

GxTexCoordSet::Values EmptyValues(GxTexCoordDimension dimension) {
    if (dimension == GxTexCoordDimension::Three) {
        return std::vector<GxTexCoord3>{};
    }
    if (dimension == GxTexCoordDimension::Four) {
        return std::vector<GxTexCoord4>{};
    }
    return std::vector<GxTexCoord>{};
}

} // namespace

GxTexCoordSet::GxTexCoordSet(void)
        : dimension_(GxTexCoordDimension::Two),
          values_(std::vector<GxTexCoord>{}) {}

GxTexCoordSet GxTexCoordSet::Create(
        GxTexCoordDimension dimension,
        unsigned long count) {
    GxTexCoordSet result;
    result.dimension_ = dimension;
    result.values_ = EmptyValues(dimension);
    result.Resize(count);
    return result;
}

void GxTexCoordSet::Alloc(unsigned long count) {
    values_ = EmptyValues(dimension_);
    Resize(count);
}

GxTexCoordDimension GxTexCoordSet::Dimension(void) const {
    return dimension_;
}

unsigned long GxTexCoordSet::Count(void) const {
    return std::visit(
            [](const auto &values) {
                return static_cast<unsigned long>(values.size());
            },
            values_);
}

bool GxTexCoordSet::Empty(void) const { return Count() == 0u; }

GxTexCoord *GxTexCoordSet::Data2(void) {
    auto *values = std::get_if<std::vector<GxTexCoord>>(&values_);
    return values != nullptr && !values->empty() ? values->data() : nullptr;
}

const GxTexCoord *GxTexCoordSet::Data2(void) const {
    const auto *values = std::get_if<std::vector<GxTexCoord>>(&values_);
    return values != nullptr && !values->empty() ? values->data() : nullptr;
}

void GxTexCoordSet::Reset(void) { values_ = EmptyValues(dimension_); }

GxTexCoord4 GxTexCoordSet::Coordinate4At(unsigned long index) const {
    if (const auto *values = std::get_if<std::vector<GxTexCoord>>(&values_)) {
        const GxTexCoord &value = values->at(index);
        return {value.u, value.v, 1.0f, 1.0f};
    }
    if (const auto *values = std::get_if<std::vector<GxTexCoord3>>(&values_)) {
        const GxTexCoord3 &value = values->at(index);
        return {value.u, value.v, value.w, 1.0f};
    }
    return std::get<std::vector<GxTexCoord4>>(values_).at(index);
}

void GxTexCoordSet::SetCoordinate(
        unsigned long index,
        const GxTexCoord4 &coordinate) {
    if (auto *values = std::get_if<std::vector<GxTexCoord>>(&values_)) {
        values->at(index) = {coordinate.u, coordinate.v};
    } else if (auto *values =
                       std::get_if<std::vector<GxTexCoord3>>(&values_)) {
        values->at(index) = {coordinate.u, coordinate.v, coordinate.w};
    } else {
        std::get<std::vector<GxTexCoord4>>(values_).at(index) = coordinate;
    }
}

void GxTexCoordSet::Resize(unsigned long count) {
    std::visit([count](auto &values) { values.resize(count); }, values_);
}

void GxTexCoordSet::InsertDefault(unsigned long index,
                                  unsigned long count) {
    if (index > Count() || count == 0u) return;
    std::visit(
            [index, count](auto &values) {
                using Value = typename std::decay_t<decltype(values)>::value_type;
                values.insert(values.begin() + index, count, Value{});
            },
            values_);
}

void GxTexCoordSet::Erase(unsigned long index, unsigned long count) {
    if (index > Count() || count > Count() - index || count == 0u) return;
    std::visit(
            [index, count](auto &values) {
                values.erase(values.begin() + index,
                             values.begin() + index + count);
            },
            values_);
}

void GxTexCoordSet::MoveRange(unsigned long destinationIndex,
                              unsigned long sourceIndex,
                              unsigned long count) {
    const unsigned long size = Count();
    if (destinationIndex > size || sourceIndex > size ||
        count > size - destinationIndex || count > size - sourceIndex ||
        count == 0u || destinationIndex == sourceIndex) {
        return;
    }
    std::visit(
            [destinationIndex, sourceIndex, count](auto &values) {
                using Rows = std::decay_t<decltype(values)>;
                Rows copied(values.begin() + sourceIndex,
                            values.begin() + sourceIndex + count);
                std::copy(copied.begin(), copied.end(),
                          values.begin() + destinationIndex);
            },
            values_);
}

void GxTexCoordSet::Apply2DScaleOffset(float uScale,
                                       float vScale,
                                       float uOffset,
                                       float vOffset) {
    auto *values = std::get_if<std::vector<GxTexCoord>>(&values_);
    if (values == nullptr) return;
    for (GxTexCoord &value : *values) {
        value.u = value.u * uScale + uOffset;
        value.v = value.v * vScale + vOffset;
    }
}
