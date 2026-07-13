#pragma once

#include "engine/core/vertex_attribute.h"
// An ordinary contiguous typed view with no byte-level stride or hidden owner.
template <typename Value>
class CStridedArray {
public:
    CStridedArray(void) = default;

    CStridedArray(Value *values, unsigned long count)
            : values_(values), count_(count), bound_(true) {}

    bool IsBound(void) const { return bound_; }
    unsigned long Count(void) const { return count_; }
    Value *Data(void) const { return values_; }

    bool GetAt(unsigned long index, Value &outValue) const {
        if (!bound_ || index >= count_ || values_ == nullptr) return false;
        outValue = values_[index];
        return true;
    }

    bool SetAt(unsigned long index, const Value &value) const {
        if (!bound_ || index >= count_ || values_ == nullptr) return false;
        values_[index] = value;
        return true;
    }

private:
    Value *values_ = nullptr;
    unsigned long count_ = 0u;
    bool bound_ = false;
};

template <>
class CStridedArray<unsigned char> {
public:
    CStridedArray(void) = default;
    explicit CStridedArray(VertexAttributeMutableView values)
            : values_(values) {}

    bool IsBound(void) const { return values_.IsBound(); }
    EPlugVDclType DeclaredType(void) const { return values_.Type(); }
    unsigned long Count(void) const { return values_.Count(); }

    bool GetAt(unsigned long index, VertexAttributeValue &outValue) const {
        return values_.GetAt(index, outValue);
    }

    bool SetAt(unsigned long index,
               const VertexAttributeValue &value) const {
        return values_.SetAt(index, value);
    }

private:
    VertexAttributeMutableView values_;
};
