#include "engine/core/gm_map2.h"
#include "engine/core/binary32_math.h"
template <typename Value>
GmMap2<Value>::GmMap2(void) = default;

template <typename Value>
GmMap2<Value>::~GmMap2(void) = default;

template <typename Value>
u32 GmMap2<Value>::CellCoordinate(float coordinate,
                                  float origin,
                                  float cellSize) const {
    const float cellIndex = (coordinate - origin) / cellSize;
    return Binary32::TruncateToUint32Modulo(cellIndex);
}

template <typename Value>
void GmMap2<Value>::SetValue(const GmNat2 &point, const Value &value) {
    if (Contains(point)) {
        cells_[CellIndex(point)] = value;
    }
}

template <typename Value>
int GmMap2<Value>::IsInside(const GmVec2 &point) {
    return static_cast<const GmMap2<Value> &>(*this).IsInside(point);
}

template <typename Value>
int GmMap2<Value>::IsInside(const GmVec2 &point) const {
    return Contains(CellAt(point));
}

template <typename Value>
const Value &GmMap2<Value>::GetValue(const GmVec2 &point) {
    return static_cast<const GmMap2<Value> &>(*this).GetValue(point);
}

template <typename Value>
const Value &GmMap2<Value>::GetValue(const GmVec2 &point) const {
    return ValueAt(CellAt(point));
}

template <typename Value>
void GmMap2<Value>::Init(const GmVec2 &cellSize,
                         const GmVec2 &origin,
                         const GmNat2 &dimensions,
                         const Value &outsideValue) {
    cellSize_ = cellSize;
    origin_ = origin;
    dimensions_ = dimensions;
    outsideValue_ = outsideValue;
    cells_.assign(static_cast<std::size_t>(dimensions.x) * dimensions.y,
                  outsideValue);
}

template class GmMap2<unsigned char>;
