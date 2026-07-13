#pragma once

#include <cstdint>

class ArchiveNodeReference {
public:
    using IndexType = std::uint32_t;

    static constexpr IndexType InvalidIndex = 0xffffffffu;
    static constexpr IndexType DeferredIndex = 0xfffffffeu;

    constexpr ArchiveNodeReference() = default;

    static constexpr ArchiveNodeReference Invalid() {
        return ArchiveNodeReference(InvalidIndex);
    }

    static constexpr ArchiveNodeReference FromIndex(IndexType index) {
        return ArchiveNodeReference(index);
    }

    constexpr bool IsValid() const {
        return index_ != InvalidIndex && index_ != DeferredIndex;
    }

    constexpr bool IsInvalid() const {
        return index_ == InvalidIndex;
    }

    constexpr bool IsDeferred() const {
        return index_ == DeferredIndex;
    }

    constexpr bool Matches(ArchiveNodeReference other) const {
        return index_ == other.index_;
    }

    constexpr IndexType Index() const {
        return index_;
    }

private:
    explicit constexpr ArchiveNodeReference(IndexType index)
        : index_(index) {}

    IndexType index_ = InvalidIndex;
};
