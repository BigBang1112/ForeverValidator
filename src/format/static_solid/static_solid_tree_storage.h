#pragma once

#include "engine/rendering/plug_tree.h"
#include "engine/core/engine_types.h"
#include <stdint.h>
#include <deque>
#include <memory>
#include <vector>

#include "format/static_solid/static_solid_archive_identity.h"
class CGameCtnReplayStaticSolidArchiveNode;

class StaticSolidTreeStorage {
public:
    ~StaticSolidTreeStorage() = default;
    void Clear();
    int Allocate(u32 treeCount);
    int ConstructTree(u32 treeIndex,
                      const CGameCtnReplayStaticSolidArchiveNode &node);
    int AllocateChildren(u32 treeIndex, u32 childCount);
    int ConnectChild(u32 parentTreeIndex, u32 childTreeIndex);

    CPlugTree *TreeAt(u32 index);
    CPlugTree *TreeForNode(
            CGameCtnReplayStaticSolidArchiveNodeIdentity tree);
    u32 TreeIndexForNode(
            CGameCtnReplayStaticSolidArchiveNodeIdentity tree) const;
    u32 TreeCount() const;
    int HasTrees() const;

private:
    class TreeRecord {
    public:
        CPlugTree *Tree();
        const CPlugTree *Tree() const;
        int Construct(const CGameCtnReplayStaticSolidArchiveNode &node);
        CGameCtnReplayStaticSolidArchiveNodeIdentity Node() const;
        std::unique_ptr<CPlugTree> TakeOwnership();

    private:
        std::unique_ptr<CPlugTree> owner;
        CPlugTree *tree = nullptr;
        CGameCtnReplayStaticSolidArchiveNodeIdentity node =
                CGameCtnReplayStaticSolidArchiveNodeIdentity::Invalid();
    };

    class ChildSlots {
    public:
        void Clear();
        int Allocate(u32 childCount);
        int CanConnectChild() const;
        void RecordChild();

    private:
        u32 capacity = 0u;
        u32 nextChild = 0u;
    };

    std::deque<TreeRecord> trees;
    std::vector<ChildSlots> childSlots;
};
