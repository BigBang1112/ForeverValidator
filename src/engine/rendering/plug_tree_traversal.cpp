#include "engine/rendering/plug_tree.h"
#include <vector>

CPlugTree::CIteratorTree::CIteratorTree(CPlugTree *tree, EMode modeValue) {
    ResetItTree(tree, modeValue);
}

bool CPlugTree::CIteratorTree::HasNext(void) const {
    return currentTree != nullptr;
}

void CPlugTree::CIteratorTree::AdvanceAfter(CPlugTree *tree) {
    currentTree = nullptr;
    if (tree->GetChildCount() != 0u) {
        childIndexStack.push_back(0u);
        currentTree = tree->GetChild(0u);
        return;
    }

    CPlugTree *parent = tree->parent;
    while (!childIndexStack.empty() && parent != nullptr) {
        unsigned long &childIndex = childIndexStack.back();
        if (childIndex + 1u < parent->GetChildCount()) {
            currentTree = parent->GetChild(++childIndex);
            return;
        }
        childIndexStack.pop_back();
        parent = parent->parent;
    }
}

CPlugTree *CPlugTree::CIteratorTree::GetNextTree(void) {
    CPlugTree *previous = currentTree;
    if (previous == nullptr) {
        return nullptr;
    }

    AdvanceAfter(previous);
    while (mode == Mode_SkipRootedSubtrees &&
           currentTree != nullptr &&
           currentTree->GetIsRooted() != 0) {
        AdvanceAfter(currentTree);
    }

    return previous;
}

void CPlugTree::CIteratorTree::ResetItTree(CPlugTree *tree,
                                           EMode modeValue) {
    mode = modeValue;
    currentTree = tree;
    childIndexStack.clear();
    if (mode == Mode_ExcludeRoot) {
        GetNextTree();
    }
}

CPlugTree::CIteratorVisual::CIteratorVisual(CPlugTree *tree,
                                            EMode modeValue) {
    ResetItVisual(tree, modeValue);
}

CPlugTree::CIteratorSurface::CIteratorSurface(CPlugTree *tree,
                                              EMode modeValue) {
    ResetItSurface(tree, modeValue);
}

CPlugTree::CIteratorShader::CIteratorShader(CPlugTree *tree,
                                            EMode modeValue) {
    ResetItShader(tree, modeValue);
}

CPlugTree::CIteratorMaterial::CIteratorMaterial(CPlugTree *tree,
                                                EMode modeValue) {
    ResetItMaterial(tree, modeValue);
}

void CPlugTree::CIteratorVisual::ResetItVisual(
        CPlugTree *tree,
        EMode modeValue) {
    ResetToResource(tree, modeValue, &CPlugTree::visual);
}

CPlugVisual *CPlugTree::CIteratorVisual::GetNextVisual(
        CPlugTree **outTree) {
    return GetNextResource(outTree, &CPlugTree::visual);
}

void CPlugTree::CIteratorMaterial::ResetItMaterial(
        CPlugTree *tree,
        EMode modeValue) {
    ResetToResource(tree, modeValue, &CPlugTree::material);
}

CPlugMaterial *CPlugTree::CIteratorMaterial::GetNextMaterial(
        CPlugTree **outTree) {
    return GetNextResource(outTree, &CPlugTree::material);
}

void CPlugTree::CIteratorSurface::ResetItSurface(
        CPlugTree *tree,
        EMode modeValue) {
    ResetToResource(tree, modeValue, &CPlugTree::surface);
}

CPlugSurface *CPlugTree::CIteratorSurface::GetNextSurface(
        CPlugTree **outTree) {
    return GetNextResource(outTree, &CPlugTree::surface);
}

void CPlugTree::CIteratorShader::ResetItShader(
        CPlugTree *tree,
        EMode modeValue) {
    ResetToResource(tree, modeValue, &CPlugTree::shader);
}

CPlugShader *CPlugTree::CIteratorShader::GetNextShader(
        CPlugTree **outTree) {
    return GetNextResource(outTree, &CPlugTree::shader);
}

CPlugTree *CPlugTree::GetPlugFromId(const CMwId &id) {
    if (!this->GetIsRooted()) {
        return nullptr;
    }

    if (id == this->id || (id.IsInvalid() && this->parent == nullptr)) {
        return this;
    }

    const u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = this->GetChild(childIndex);
        CPlugTree *found = child->GetPlugFromId(id);
        if (found != nullptr) {
            return found;
        }
    }

    return nullptr;
}

CPlugTree *CPlugTree::GetPlugFromModelId(
        const CMwId &modelId) {
    if (!this->GetIsRooted()) {
        return nullptr;
    }

    if (modelId == this->modelId) {
        return this;
    }

    const u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = this->GetChild(childIndex);
        CPlugTree *found = child->GetPlugFromModelId(modelId);
        if (found != nullptr) {
            return found;
        }
    }

    return nullptr;
}

CPlugTree *CPlugTree::GetChildFromId(const CMwId &id) const {
    const u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = this->GetChild(childIndex);
        if (child->id == id) {
            return child;
        }
    }
    return nullptr;
}

int CPlugTree::FindTree(const CPlugTree *candidate) const {
    while (candidate != nullptr) {
        if (candidate == this) {
            return 1;
        }
        candidate = candidate->parent;
    }
    return 0;
}

unsigned long CPlugTree::GetAllTreeStart(void) {
    return 0ul;
}

namespace {
struct DescendantLookup {
    CPlugTree *tree = nullptr;
    bool hasNext = false;
};

DescendantLookup DescendantAt(CPlugTree *root, unsigned long ordinal) {
    std::vector<CPlugTree *> pending;
    const u32 rootChildCount = root->GetChildCount();
    pending.reserve(rootChildCount);
    for (u32 index = rootChildCount; index != 0u; --index) {
        pending.push_back(root->GetChild(index - 1u));
    }

    unsigned long currentOrdinal = 0u;
    while (!pending.empty()) {
        CPlugTree *tree = pending.back();
        pending.pop_back();
        if (currentOrdinal == ordinal) {
            return {tree,
                    tree->GetChildCount() != 0u || !pending.empty()};
        }
        currentOrdinal++;

        const u32 childCount = tree->GetChildCount();
        for (u32 index = childCount; index != 0u; --index) {
            pending.push_back(tree->GetChild(index - 1u));
        }
    }
    return {};
}
}

unsigned long CPlugTree::GetAllChildStart(void) {
    return GetChildCount() == 0u ? InvalidEngineIndex : 0ul;
}

CPlugTree *CPlugTree::GetAllChildNext(unsigned long &cursor) {
    if (cursor == InvalidEngineIndex) {
        return nullptr;
    }

    const DescendantLookup lookup = DescendantAt(this, cursor);
    if (lookup.tree == nullptr || !lookup.hasNext) {
        cursor = InvalidEngineIndex;
    } else {
        cursor++;
    }
    return lookup.tree;
}

CPlugTree *CPlugTree::GetAllTreeNext(unsigned long &cursor) {
    if (cursor == InvalidEngineIndex) {
        return nullptr;
    }
    if (cursor == 0u) {
        const unsigned long childCursor = GetAllChildStart();
        cursor = childCursor == InvalidEngineIndex
                ? InvalidEngineIndex
                : childCursor + 1u;
        return this;
    }

    unsigned long childCursor = cursor - 1u;
    CPlugTree *tree = GetAllChildNext(childCursor);
    cursor = childCursor == InvalidEngineIndex
            ? InvalidEngineIndex
            : childCursor + 1u;
    return tree;
}
