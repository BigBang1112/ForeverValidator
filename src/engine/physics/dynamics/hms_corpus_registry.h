#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

struct CHmsCorpus;

enum EHmsCorpusCat : std::uint32_t {
    EHmsCorpusCat_Static = 0u,
    EHmsCorpusCat_StaticVision = 1u,
    EHmsCorpusCat_Dynamic = 2u,
    EHmsCorpusCat_Dormant = 3u,
    EHmsCorpusCat_Background = 4u,
};

class CHmsCorpusId {
public:
    constexpr CHmsCorpusId() = default;

    static constexpr CHmsCorpusId FromRegistrationOrder(
            std::uint32_t registrationOrder) {
        return CHmsCorpusId(registrationOrder + 1u);
    }

    static constexpr CHmsCorpusId FromValue(std::uint32_t value) {
        return CHmsCorpusId(value);
    }

    constexpr bool IsAssigned() const { return value_ != 0u; }
    constexpr std::uint32_t Value() const { return value_; }
    constexpr bool operator==(CHmsCorpusId other) const {
        return value_ == other.value_;
    }
    constexpr bool operator!=(CHmsCorpusId other) const {
        return !(*this == other);
    }

private:
    explicit constexpr CHmsCorpusId(std::uint32_t value) : value_(value) {}

    std::uint32_t value_ = 0u;
};

class CHmsCorpusCategoryStore {
public:
    struct Membership {
        EHmsCorpusCat category;
        unsigned long index;
    };

    void Clear(void);
    Membership Add(CHmsCorpus &corpus, EHmsCorpusCat category);
    Membership ChangeCategory(Membership membership,
                              EHmsCorpusCat newCategory);
    std::optional<Membership> Find(const CHmsCorpus &corpus) const;
    bool Remove(CHmsCorpus &corpus);
    CHmsCorpus *FirstOrNull(void) const;

private:
    using Category = std::vector<CHmsCorpus *>;

    static constexpr std::size_t CategoryCount =
            static_cast<std::size_t>(EHmsCorpusCat_Background) + 1u;

    static constexpr std::size_t CategoryIndex(EHmsCorpusCat category) {
        return static_cast<std::size_t>(category);
    }

    static constexpr EHmsCorpusCat NextCategory(EHmsCorpusCat category) {
        return static_cast<EHmsCorpusCat>(
                static_cast<std::uint32_t>(category) + 1u);
    }

    static constexpr EHmsCorpusCat PreviousCategory(EHmsCorpusCat category) {
        return static_cast<EHmsCorpusCat>(
                static_cast<std::uint32_t>(category) - 1u);
    }

    static CHmsCorpus *RemoveByLastAt(Category &category,
                                     unsigned long index);
    static CHmsCorpus *RemoveByFirstAt(Category &category,
                                      unsigned long index);

    std::array<Category, CategoryCount> categories_;
};
