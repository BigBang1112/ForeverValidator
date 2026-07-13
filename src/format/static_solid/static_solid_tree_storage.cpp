#include "format/static_solid/static_solid_tree_storage.h"
#include "format/static_solid/static_solid_archive_graph.h"
#include <new>
CPlugTree *StaticSolidTreeStorage::
        TreeRecord::Tree() {
    return tree;
}

const CPlugTree *StaticSolidTreeStorage::
        TreeRecord::Tree() const {
    return tree;
}

int StaticSolidTreeStorage::TreeRecord::
        Construct(const CGameCtnReplayStaticSolidArchiveNode &archiveNode) {
    try {
        owner = std::make_unique<CPlugTree>();
    } catch (const std::bad_alloc &) {
        owner.reset();
    }
    tree = owner.get();
    if (tree == nullptr) {
        node = CGameCtnReplayStaticSolidArchiveNodeIdentity::Invalid();
        return 0;
    }
    if (archiveNode.HasTreeId()) {
        tree->SetPlugId(archiveNode.TreeId());
    }
    node = archiveNode.Identity();
    return 1;
}

std::unique_ptr<CPlugTree>
StaticSolidTreeStorage::TreeRecord::
        TakeOwnership() {
    return std::move(owner);
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
StaticSolidTreeStorage::TreeRecord::Node()
        const {
    return node;
}

void StaticSolidTreeStorage::ChildSlots::Clear() {
    capacity = 0u;
    nextChild = 0u;
}

int StaticSolidTreeStorage::ChildSlots::Allocate(
        u32 childCount) {
    Clear();
    capacity = childCount;
    return 1;
}

int StaticSolidTreeStorage::ChildSlots::
        CanConnectChild() const {
    return nextChild < capacity;
}

void StaticSolidTreeStorage::ChildSlots::
        RecordChild() {
    nextChild++;
}

void StaticSolidTreeStorage::Clear() {
    trees.clear();
    childSlots.clear();
}

int StaticSolidTreeStorage::Allocate(
        u32 treeCount) {
    Clear();
    if (treeCount == 0u) {
        return 1;
    }
    try {
        trees.resize(treeCount);
        childSlots.resize(treeCount);
    } catch (const std::bad_alloc &) {
        Clear();
        return 0;
    }
    return 1;
}

int StaticSolidTreeStorage::ConstructTree(
        u32 treeIndex,
        const CGameCtnReplayStaticSolidArchiveNode &node) {
    return treeIndex < trees.size() ? trees[treeIndex].Construct(node) : 0;
}

int StaticSolidTreeStorage::AllocateChildren(
        u32 treeIndex,
        u32 childCount) {
    return treeIndex < childSlots.size()
            ? childSlots[treeIndex].Allocate(childCount)
            : 0;
}

int StaticSolidTreeStorage::ConnectChild(
        u32 parentTreeIndex,
        u32 childTreeIndex) {
    if (parentTreeIndex >= childSlots.size() ||
        childTreeIndex >= trees.size() ||
        !childSlots[parentTreeIndex].CanConnectChild()) {
        return 0;
    }
    CPlugTree *parent = TreeAt(parentTreeIndex);
    CPlugTree *child = TreeAt(childTreeIndex);
    if (parent == nullptr || child == nullptr ||
        child->ParentTree() != nullptr) {
        return 0;
    }
    std::unique_ptr<CPlugTree> childOwner =
            trees[childTreeIndex].TakeOwnership();
    if (childOwner == nullptr) {
        return 0;
    }
    parent->AddChild(childOwner.release());
    childSlots[parentTreeIndex].RecordChild();
    return 1;
}

CPlugTree *StaticSolidTreeStorage::TreeAt(
        u32 index) {
    return index < trees.size() ? trees[index].Tree() : nullptr;
}

CPlugTree *
StaticSolidTreeStorage::TreeForNode(
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree) {
    const u32 treeIndex = TreeIndexForNode(tree);
    return treeIndex != UINT32_MAX ? TreeAt(treeIndex) : nullptr;
}

u32 StaticSolidTreeStorage::TreeIndexForNode(
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree) const {
    for (u32 i = 0u; i < trees.size(); i++) {
        if (trees[i].Node().Matches(tree)) {
            return i;
        }
    }
    return UINT32_MAX;
}

u32 StaticSolidTreeStorage::TreeCount() const {
    return static_cast<u32>(trees.size());
}

int StaticSolidTreeStorage::HasTrees() const {
    return !trees.empty();
}
