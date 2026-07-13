#pragma once

#include <algorithm>
#include <vector>

struct CHmsCorpus;

class CollisionPairSchedule {
public:
    void Include(CHmsCorpus &source, CHmsCorpus &target) {
        if (Find(source, target) == nullptr) {
            pairs_.push_back({&source, &target, false});
        }
    }

    void RemoveSource(const CHmsCorpus &source) {
        pairs_.erase(
                std::remove_if(
                        pairs_.begin(),
                        pairs_.end(),
                        [&source](const Pair &pair) {
                            return pair.source == &source;
                        }),
                pairs_.end());
    }

    void RemoveTarget(const CHmsCorpus &target) {
        pairs_.erase(
                std::remove_if(
                        pairs_.begin(),
                        pairs_.end(),
                        [&target](const Pair &pair) {
                            return pair.target == &target;
                        }),
                pairs_.end());
    }

    void SetEnabled(const CHmsCorpus &source,
                    const CHmsCorpus &target,
                    bool enabled) {
        if (Pair *pair = Find(source, target)) {
            pair->enabled = enabled;
        }
    }

    bool IsEnabled(const CHmsCorpus &source,
                   const CHmsCorpus &target) const {
        const Pair *pair = Find(source, target);
        return pair != nullptr && pair->enabled;
    }

    void Clear(void) { pairs_.clear(); }

private:
    struct Pair {
        CHmsCorpus *source;
        CHmsCorpus *target;
        bool enabled;
    };

    Pair *Find(const CHmsCorpus &source, const CHmsCorpus &target) {
        const auto found = std::find_if(
                pairs_.begin(),
                pairs_.end(),
                [&source, &target](const Pair &pair) {
                    return pair.source == &source && pair.target == &target;
                });
        return found != pairs_.end() ? &*found : nullptr;
    }

    const Pair *Find(const CHmsCorpus &source,
                     const CHmsCorpus &target) const {
        const auto found = std::find_if(
                pairs_.begin(),
                pairs_.end(),
                [&source, &target](const Pair &pair) {
                    return pair.source == &source && pair.target == &target;
                });
        return found != pairs_.end() ? &*found : nullptr;
    }

    std::vector<Pair> pairs_;
};
