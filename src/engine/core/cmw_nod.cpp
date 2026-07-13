#include "engine/core/cmw_nod.h"
#include <algorithm>

CMwNod::~CMwNod(void) {
    DependantSendMwIsKilled();
}

void CMwNod::OnNodLoaded(void) {
}

unsigned long CMwNod::MwAddRef(void) {
    refCount++;
    return refCount;
}

unsigned long CMwNod::MwForceRef(unsigned long forcedRefCount) {
    const unsigned long previousRefCount = refCount;
    refCount = forcedRefCount;
    return previousRefCount;
}

unsigned long CMwNod::MwRelease(void) {
    if (refCount == 0u) {
        return 0u;
    }

    const unsigned long newRefCount = --refCount;
    if (newRefCount != 0u) {
        return newRefCount;
    }

    const std::vector<CMwNod *> finalDependants = dependants;
    for (CMwNod *dependant : finalDependants) {
        if (dependant != nullptr) {
            dependant->MwFinalSubDependant(this);
        }
    }

    delete this;
    return 0u;
}

void CMwNod::MwAddDependant(CMwNod *dependant) const {
    if (dependant != nullptr) {
        dependants.push_back(dependant);
    }
}

void CMwNod::MwSubDependant(CMwNod *dependant) const {
    const auto found = std::find(dependants.begin(), dependants.end(), dependant);
    if (found != dependants.end()) {
        dependants.erase(found);
    }
}

void CMwNod::MwSubDependantSafe(CMwNod *dependant) const {
    MwSubDependant(dependant);
}

void CMwNod::MwFinalSubDependant(CMwNod *releasedNod) const {
    if (releasedNod != nullptr) {
        releasedNod->MwSubDependant(const_cast<CMwNod *>(this));
    }
}

void CMwNod::DependantSendMwIsKilled(void) {
    while (!dependants.empty()) {
        std::vector<CMwNod *> currentDependants;
        currentDependants.swap(dependants);
        for (CMwNod *dependant : currentDependants) {
            if (dependant != nullptr) {
                dependant->MwIsKilled(this);
            }
        }
    }
}

void CMwNod::SetIdName(const char *idName) {
    (void)idName;
}

void CMwNod::MwIsKilled(CMwNod *killedNod) {
    (void)killedNod;
}
