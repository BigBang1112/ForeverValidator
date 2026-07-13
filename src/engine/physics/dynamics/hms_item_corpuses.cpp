#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/physics/world/hms_zone.h"
#include <algorithm>

int CHmsItem::IsZombieCorpus(void) const {
    return properties.zombie;
}

void CHmsItem::AddCorpus(CHmsCorpus *corpus) {
    if (corpus == nullptr ||
        std::find(corpuses.begin(), corpuses.end(), corpus) != corpuses.end()) {
        return;
    }
    corpuses.push_back(corpus);
}

void CHmsItem::RemoveCorpus(CHmsCorpus *corpus) {
    auto found = std::find(corpuses.begin(), corpuses.end(), corpus);
    if (found == corpuses.end()) {
        return;
    }
    *found = corpuses.back();
    corpuses.pop_back();
}

void CHmsItem::UpdateCorpusCat() {
    const EHmsCorpusCat newCategory = GetCorpusCat();
    const u32 count = static_cast<u32>(corpuses.size());
    for (u32 corpusIndex = 0; corpusIndex < count; corpusIndex++) {
        CHmsCorpus *corpus = corpuses[corpusIndex];
        if (corpus == nullptr || corpus->OwningZone() == nullptr) {
            continue;
        }
        corpus->OwningZone()->ChangeCorpusCategory(*corpus, newCategory);
    }
}
