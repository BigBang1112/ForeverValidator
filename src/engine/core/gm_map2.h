#pragma once

#include <vector>

#include "engine/core/engine_types.h"
#include "engine/core/gm_types.h"
template <typename Value>
class GmMap2 {
public:
    GmMap2(void);
    ~GmMap2(void);

    void Init(const GmVec2 &cellSize,
              const GmVec2 &origin,
              const GmNat2 &dimensions,
              const Value &outsideValue);
    void SetValue(const GmNat2 &point, const Value &value);
    int IsInside(const GmVec2 &point);
    int IsInside(const GmVec2 &point) const;
    const Value &GetValue(const GmVec2 &point);
    const Value &GetValue(const GmVec2 &point) const;
    const Value &OutsideValue(void) const { return outsideValue_; }

private:
    u32 CellCoordinate(float coordinate,
                       float origin,
                       float cellSize) const;
    GmNat2 CellAt(const GmVec2 &point) const {
        return {
                CellCoordinate(point.x, origin_.x, cellSize_.x),
                CellCoordinate(point.y, origin_.y, cellSize_.y),
        };
    }
    bool Contains(const GmNat2 &cell) const {
        return cell.x < dimensions_.x && cell.y < dimensions_.y;
    }
    u32 CellIndex(const GmNat2 &cell) const {
        return cell.x + dimensions_.x * cell.y;
    }
    const Value &ValueAt(const GmNat2 &cell) const {
        return Contains(cell) ? cells_[CellIndex(cell)] : outsideValue_;
    }

    GmVec2 cellSize_{};
    GmVec2 origin_{};
    GmNat2 dimensions_{};
    Value outsideValue_{};
    std::vector<Value> cells_;
};

extern template class GmMap2<unsigned char>;
