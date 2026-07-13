#include "engine/physics/dynamics/hms_corpus_registry.h"
#include <algorithm>

void CHmsCorpusCategoryStore::Clear(void) {
    for (Category &category : categories_) {
        category.clear();
    }
}

CHmsCorpus *CHmsCorpusCategoryStore::RemoveByLastAt(
        Category &category,
        unsigned long index) {
    const std::size_t position = static_cast<std::size_t>(index);
    CHmsCorpus *removed = category[position];
    if (position + 1u != category.size()) {
        category[position] = category.back();
    }
    category.pop_back();
    return removed;
}

CHmsCorpus *CHmsCorpusCategoryStore::RemoveByFirstAt(
        Category &category,
        unsigned long index) {
    const std::size_t position = static_cast<std::size_t>(index);
    CHmsCorpus *removed = category[position];
    if (position != 0u) {
        std::swap(category[0], category[position]);
    }
    category.erase(category.begin());
    return removed;
}

CHmsCorpusCategoryStore::Membership
CHmsCorpusCategoryStore::ChangeCategory(
        Membership membership,
        EHmsCorpusCat newCategory) {
    while (membership.category < newCategory) {
        CHmsCorpus *moving = RemoveByLastAt(
                categories_[CategoryIndex(membership.category)],
                membership.index);
        membership.category = NextCategory(membership.category);
        Category &destination = categories_[CategoryIndex(membership.category)];
        destination.insert(destination.begin(), moving);
        membership.index = 0u;
    }

    while (membership.category > newCategory) {
        CHmsCorpus *moving = RemoveByFirstAt(
                categories_[CategoryIndex(membership.category)],
                membership.index);
        membership.category = PreviousCategory(membership.category);
        Category &destination = categories_[CategoryIndex(membership.category)];
        membership.index = static_cast<unsigned long>(destination.size());
        destination.push_back(moving);
    }

    return membership;
}

CHmsCorpusCategoryStore::Membership CHmsCorpusCategoryStore::Add(
        CHmsCorpus &corpus,
        EHmsCorpusCat category) {
    Category &background = categories_[CategoryIndex(EHmsCorpusCat_Background)];
    Membership membership{
        EHmsCorpusCat_Background,
        static_cast<unsigned long>(background.size()),
    };
    background.push_back(&corpus);
    if (category != EHmsCorpusCat_Background) {
        membership = ChangeCategory(membership, category);
    }
    return membership;
}

std::optional<CHmsCorpusCategoryStore::Membership>
CHmsCorpusCategoryStore::Find(const CHmsCorpus &corpus) const {
    for (std::size_t categoryIndex = 0u;
         categoryIndex < categories_.size();
         ++categoryIndex) {
        const Category &category = categories_[categoryIndex];
        for (std::size_t index = 0u; index < category.size(); ++index) {
            if (category[index] == &corpus) {
                return Membership{
                    static_cast<EHmsCorpusCat>(categoryIndex),
                    static_cast<unsigned long>(index),
                };
            }
        }
    }
    return std::nullopt;
}

bool CHmsCorpusCategoryStore::Remove(CHmsCorpus &corpus) {
    std::optional<Membership> membership = Find(corpus);
    if (!membership.has_value()) {
        return false;
    }
    if (membership->category != EHmsCorpusCat_Background) {
        *membership = ChangeCategory(*membership, EHmsCorpusCat_Background);
    }
    RemoveByLastAt(categories_[CategoryIndex(EHmsCorpusCat_Background)],
                   membership->index);
    return true;
}

CHmsCorpus *CHmsCorpusCategoryStore::FirstOrNull(void) const {
    for (const Category &category : categories_) {
        if (!category.empty()) {
            return category.back();
        }
    }
    return nullptr;
}
