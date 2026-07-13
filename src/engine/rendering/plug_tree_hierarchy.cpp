#include "engine/rendering/plug_tree.h"
#include <algorithm>

#include "engine/scene/plug_solid.h"
void CPlugTree::DeconnectAsChild(CPlugTree *child) {
    if (child == nullptr) {
        return;
    }
    const int childHadOwningSolid = child->owningSolid != nullptr;
    child->parent = nullptr;
    if (childHadOwningSolid && owningSolid != nullptr) {
        this->owningSolid->InternalDisconnectSubTree(child);
    }
}

void CPlugTree::ConnectAsChild(CPlugTree *child,
                                          int notifyRoot) {
    if (child == nullptr) {
        return;
    }
    child->parent = this;
    if (notifyRoot) {
        CPlugSolid *owningSolid = OwningSolid();
        if (owningSolid != nullptr) {
            owningSolid->InternalConnectSubTree(child);
        }
    }
}

unsigned long CPlugTree::GetChildCount(void) const {
    return static_cast<unsigned long>(children.size());
}

CPlugTree *CPlugTree::GetChild(unsigned long index) const {
    return index < children.size() ? children[index].get() : nullptr;
}

CPlugTree *CPlugTree::GetChildToRenderMip(void) const {
    return const_cast<CPlugTree *>(this);
}

CPlugVisual *CPlugTree::Visual(void) const {
    return visual.Get();
}

void CPlugTree::PropagateVisibleToSubtree(bool visible) {
    state_.visible = visible;
    const u32 childCount = GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = GetChild(childIndex);
        if (child != nullptr) {
            child->PropagateVisibleToSubtree(visible);
        }
    }
}

void CPlugTree::PropagateShadowCasterToSubtree(bool castsShadows) {
    state_.castsShadows = castsShadows;
    const u32 childCount = GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = GetChild(childIndex);
        if (child != nullptr) {
            child->PropagateShadowCasterToSubtree(castsShadows);
        }
    }
}

void CPlugTree::ApplyDecorationFlags(bool visible,
                                     bool castsShadows,
                                     bool collision,
                                     bool propagateVisibleToChildren,
                                     bool propagateCasterToChildren) {
    state_.visible = visible;
    state_.castsShadows = castsShadows;
    if (propagateVisibleToChildren) {
        PropagateVisibleToSubtree(visible);
    }
    if (propagateCasterToChildren) {
        PropagateShadowCasterToSubtree(castsShadows);
    }
    SetCollisionEnabled(collision);
}

void CPlugTree::SetUseLocation(int useLocation) {
    state_.usesLocation = useLocation != 0;
    state_.locationDirty = true;
}

void CPlugTree::SetLocation(const GmIso4 &location) {
    this->localIso = location;
    state_.locationDirty = true;
}

void CPlugTree::SetTranslation(const GmVec3 &translation) {
    this->localIso.translation.x = translation.x;
    this->localIso.translation.y = translation.y;
    this->localIso.translation.z = translation.z;
    state_.locationDirty = true;
}

int CPlugTree::GetIsRooted(void) const {
    if (parent != nullptr && !state_.rooted) {
        return 0;
    }

    return 1;
}

void CPlugTree::SetIsRooted(int isRooted) {
    state_.rooted = isRooted != 0;
    if (isRooted != 0 && this->owningSolid != nullptr) {
        this->owningSolid->MakeTreeIdsUnique();
    }
}

void CPlugTree::SetIsVisible(int isVisible) {
    state_.visible = isVisible != 0;
}

void CPlugTree::SetChild(CPlugTree *child,
                         unsigned long index) {
    if (child == nullptr || index >= children.size()) {
        return;
    }
    CPlugTree *oldChild = children[index].get();
    if (oldChild == child || child->parent != nullptr) {
        return;
    }

    std::unique_ptr<CPlugTree> oldChildOwner = std::move(children[index]);
    children[index].reset(child);
    if (this->owningSolid != nullptr) {
        child->Generate(0);
    }
    this->ConnectAsChild(child, 1);
    this->DeconnectAsChild(oldChild);
}

CPlugTree *CPlugTree::DetachChild(unsigned long index) {
    if (index >= children.size()) {
        return nullptr;
    }
    std::unique_ptr<CPlugTree> detached = std::move(children[index]);
    CPlugTree *child = detached.get();
    if (index + 1u != children.size()) {
        children[index] = std::move(children.back());
    }
    children.pop_back();
    this->DeconnectAsChild(child);
    return detached.release();
}

void CPlugTree::DeleteChild(unsigned long index) {
    if (index >= children.size()) {
        return;
    }
    std::unique_ptr<CPlugTree> removed = std::move(children[index]);
    CPlugTree *child = removed.get();
    if (index + 1u != children.size()) {
        children[index] = std::move(children.back());
    }
    children.pop_back();
    DeconnectAsChild(child);
}

void CPlugTree::DeleteChildPtr(CPlugTree *child) {
    const unsigned long index = GetChildIndex(child);
    if (index != InvalidEngineIndex) {
        DeleteChild(index);
    }
}

CPlugTree *CPlugTree::DetachChildPtr(CPlugTree *child) {
    for (u32 index = 0u; index < children.size(); index++) {
        if (children[index].get() == child) {
            return DetachChild(index);
        }
    }
    return nullptr;
}

void CPlugTree::DeleteAllChilds(void) {
    u32 childCount = this->GetChildCount();
    while (childCount != 0u) {
        this->DeleteChild(0u);
        childCount--;
    }
}

unsigned long CPlugTree::GetRecursiveTreeCount(int requireVisible,
                                               int requireRooted) const {
    if ((requireRooted != 0 && !this->GetIsRooted()) ||
        (requireVisible != 0 && !state_.visible)) {
        return 0;
    }

    unsigned long count = 1ul;
    const u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = this->GetChild(childIndex);
        count += child->GetRecursiveTreeCount(requireVisible, requireRooted);
    }
    return count;
}

unsigned long CPlugTree::GetRecursiveVertexCount(
        int requireVisible,
        int lastVisualMipOnly) const {
    if (requireVisible != 0 && !state_.visible) {
        return 0ul;
    }

    unsigned long count =
            visual != nullptr ? visual->GetTotalVertexCount() : 0ul;
    const u32 childCount = static_cast<u32>(GetChildCount());
    for (u32 childIndex = 0u; childIndex < childCount; childIndex++) {
        CPlugTree *child = GetChild(childIndex);
        if (requireVisible == 0 ||
            child->IsVisible()) {
            count += child->GetRecursiveVertexCount(
                    requireVisible,
                    lastVisualMipOnly);
        }
    }
    return count;
}

CPlugTree *CPlugTree::GetLastChild(void) const {
    return children.empty() ? nullptr : children.back().get();
}

unsigned long CPlugTree::GetVolatileChildCount(void) const {
    unsigned long volatileCount = 0ul;
    const u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = this->GetChild(childIndex);
        if (child->IsVolatileChild()) {
            volatileCount++;
        }
    }
    return volatileCount;
}

unsigned long CPlugTree::GetRootedChildCount(void) const {
    unsigned long rootedCount = 0ul;
    const u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = this->GetChild(childIndex);
        if (child != nullptr && child->GetIsRooted()) {
            rootedCount++;
        }
    }
    return rootedCount;
}

void CPlugTree::AddChild(CPlugTree *child) {
    if (child == nullptr || child == this || child->parent != nullptr) {
        return;
    }
    children.emplace_back(std::unique_ptr<CPlugTree>(child));
    if (OwningSolid() != nullptr) {
        child->Generate(0);
    }
    this->ConnectAsChild(child, 1);
}

void CPlugTree::AddOwnedChild(std::unique_ptr<CPlugTree> child) {
    if (child == nullptr || child.get() == this || child->parent != nullptr) {
        return;
    }
    CPlugTree *attached = child.get();
    children.emplace_back(std::move(child));
    if (OwningSolid() != nullptr) {
        attached->Generate(0);
    }
    ConnectAsChild(attached, 1);
}

unsigned long CPlugTree::GetChildIndex(CPlugTree *child) {
    const u32 count = static_cast<u32>(children.size());
    for (u32 childIndex = 0; childIndex < count; childIndex++) {
        if (children[childIndex].get() == child) {
            return childIndex;
        }
    }
    return InvalidEngineIndex;
}

void CPlugTree::DeleteAllVolatileChilds(void) {
    u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = this->GetChild(childIndex);
        if (child->IsVolatileChild()) {
            this->DeleteChild(childIndex);
            childIndex--;
            childCount--;
        }
    }
}
