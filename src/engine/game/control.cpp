#include "engine/game/control.h"
#include <algorithm>
#include <limits>
#include <utility>

#include "engine/core/mw_cmd.h"
CSceneToy::CSceneToy(void) = default;

CSceneToy::~CSceneToy(void) = default;

CControlBase::CControlBase(void) = default;

CControlBase::~CControlBase(void) = default;

int CControlBase::CreateStack(CMwNod *nod, const char *stack) {
    const std::string expression = stack != nullptr ? stack : "";
    stackExpression_ = expression;
    boundNod = nod;
    return nod != nullptr && !stackExpression_.empty();
}

int CControlBase::CreateStack_MwStack(CMwNod *nod, CMwStack *stack) {
    (void)stack;
    stackExpression_.clear();
    boundNod = nod;
    return 0;
}

void CControlBase::RecursiveConnect(CControlBase *control,
                                    CMwNod *nod,
                                    int connect) {
    if (control == nullptr) {
        return;
    }

    if (control->stackBindingEnabled) {
        if (connect != 0 && control->boundNod == nullptr) {
            (void)control->CreateStack(
                    nod,
                    control->stackExpression_.c_str());
        } else if (connect == 0 && control->boundNod == nod) {
            (void)control->CreateStack(
                    nullptr,
                    control->stackExpression_.c_str());
        }
    }

    CControlContainer *container =
            dynamic_cast<CControlContainer *>(control);
    if (container == nullptr) {
        return;
    }
    for (const CMwNodRef<CControlBase> &child : container->childControls) {
        RecursiveConnect(child.Get(), nod, connect);
    }
}

CControlContainer::CControlContainer(void) {
    stackBindingEnabled = false;
}

CControlContainer::~CControlContainer(void) {
    ReleaseChilds();
}

unsigned long CControlContainer::AddChild(
        CControlBase *child,
        const GmVec3 &location) {
    (void)location;
    const unsigned long notFound = InvalidEngineIndex;
    if (child == nullptr || child == this ||
        child->parentControl != nullptr) {
        return notFound;
    }
    for (CControlBase *ancestor = this;
         ancestor != nullptr;
         ancestor = ancestor->parentControl) {
        if (ancestor == child) {
            return notFound;
        }
    }
    const auto duplicate = std::find_if(
            childControls.begin(),
            childControls.end(),
            [child](const CMwNodRef<CControlBase> &candidate) {
                return candidate.Get() == child;
            });
    if (duplicate != childControls.end()) {
        return notFound;
    }

    childControls.emplace_back(child);
    child->parentControl = this;
    return static_cast<unsigned long>(childControls.size() - 1u);
}

unsigned long CControlContainer::RemoveChild(CControlBase *child) {
    return InternalRemoveChild(child, 0);
}

unsigned long CControlContainer::InternalRemoveChild(
        CControlBase *child,
        int removeFromScene) {
    const unsigned long notFound = InvalidEngineIndex;
    if (child == nullptr) {
        return notFound;
    }
    const auto found = std::find_if(
            childControls.begin(),
            childControls.end(),
            [child](const CMwNodRef<CControlBase> &candidate) {
                return candidate.Get() == child;
            });
    if (found == childControls.end()) {
        return notFound;
    }

    const unsigned long index = static_cast<unsigned long>(
            std::distance(childControls.begin(), found));
    CMwNodRef<CControlBase> temporaryOwner(child);
    CMwNodRef<CControlBase> containerOwner(std::move(*found));
    childControls.erase(found);
    child->parentControl = nullptr;
    containerOwner.MwSetNod(nullptr);
    if (removeFromScene == 0 && child->scene != nullptr) {
        child->RemoveFromScene();
    }
    temporaryOwner.MwSetNod(nullptr);
    return index;
}

void CControlContainer::ReleaseChilds(void) {
    std::vector<CMwNodRef<CControlBase>> snapshot(childControls);
    for (CMwNodRef<CControlBase> &child : snapshot) {
        CControlBase *childNod = child.Get();
        if (childNod != nullptr) {
            (void)InternalRemoveChild(childNod, 0);
        }
        child.MwSetNod(nullptr);
    }
}

void CControlContainer::RemoveAllChilds(void) {
    ReleaseChilds();
}

void CControlContainer::ChangeChildIndex(
        unsigned long oldIndex,
        unsigned long newIndex) {
    const size_t originalCount = childControls.size();
    if (oldIndex == newIndex ||
        oldIndex >= originalCount ||
        newIndex >= originalCount) {
        return;
    }

    CMwNodRef<CControlBase> child =
            std::move(childControls[oldIndex]);
    childControls.erase(childControls.begin() + oldIndex);
    if (newIndex >= childControls.size()) {
        childControls.emplace_back(std::move(child));
    } else {
        childControls.insert(
                childControls.begin() + newIndex,
                std::move(child));
    }
}
