#include "format/static_solid/static_solid_tree_assembler.h"
#include <string.h>

#include "format/static_solid/static_solid_archive_graph.h"
#include "format/static_solid/static_solid_archive_node_kind.h"
#include <new>
void StaticSolidTreeAssembler::SolidRootDirectory::Clear() {
    roots.clear();
}

void StaticSolidTreeAssembler::SolidRootDirectory::Install(
        StaticSolidArchiveId payload,
        CPlugTree *root,
        const CGameCtnReplayStaticSolidArchiveSolidPhysicsDefinition *physical) {
    if (!payload.IsValid() || root == nullptr) {
        return;
    }
    for (Root &entry : roots) {
        if (entry.payload.Matches(payload)) {
            entry.tree = root;
            entry.physical = physical;
            return;
        }
    }
    try {
        roots.push_back(Root{payload, root, physical});
    } catch (const std::bad_alloc &) {
    }
}

CPlugTree *
StaticSolidTreeAssembler::SolidRootDirectory::RootForPayload(
        StaticSolidArchiveId payload) const {
    for (const Root &entry : roots) {
        if (entry.payload.Matches(payload)) {
            return entry.tree;
        }
    }
    return nullptr;
}

const CGameCtnReplayStaticSolidArchiveSolidPhysicsDefinition *
StaticSolidTreeAssembler::SolidRootDirectory::
        PhysicalForPayload(
                StaticSolidArchiveId payload) const {
    for (const Root &entry : roots) {
        if (entry.payload.Matches(payload)) {
            return entry.physical;
        }
    }
    return nullptr;
}

int StaticSolidTreeAssembler::SolidRootDirectory::HasRoots()
        const {
    return !roots.empty();
}

void StaticSolidTreeAssembler::Free() {
    trees.Clear();
    solidRoots.Clear();
    treeCount = 0u;
}

int StaticSolidTreeAssembler::BuildTrees(
        const CGameCtnReplayStaticSolidArchiveNodeDirectory &nodes,
        const CGameCtnReplayStaticSolidArchiveTreeGraph &treeGraph) {
    if (nodes.Empty()) {
        return 1;
    }
    nodes.ForEachTree([&](u32, const auto &) {
        treeCount++;
        return 1;
    });
    if (treeCount == 0u) {
        return 1;
    }

    std::vector<u32> childCounts;
    try {
        childCounts.assign(treeCount, 0u);
        if (!trees.Allocate(treeCount)) {
            childCounts.clear();
            treeCount = 0u;
            return 0;
        }
    } catch (const std::bad_alloc &) {
        trees.Clear();
        childCounts.clear();
        treeCount = 0u;
        return 0;
    }

    u32 treeIndex = 0u;
    nodes.ForEachTree([&](u32,
                                  const CGameCtnReplayStaticSolidArchiveNode
                                          &node) {
        if (!trees.ConstructTree(treeIndex, node)) {
            return 0;
        }
        treeIndex++;
        return 1;
    });

    treeGraph.ForEachChildLink([&](
            const CGameCtnReplayStaticSolidArchiveTreeChildLink &link) {
        const u32 parentTree = TreeIndexForNode(link.ParentTree());
        const u32 childTree = TreeIndexForNode(link.ChildTree());
        if (parentTree == UINT32_MAX || childTree == UINT32_MAX) {
            return 1;
        }
        childCounts[parentTree]++;
        return 1;
    });

    for (u32 i = 0; i < treeCount; i++) {
        if (childCounts[i] == 0u) {
            continue;
        }
        if (!trees.AllocateChildren(i, childCounts[i])) {
            return 0;
        }
    }

    treeGraph.ForEachChildLink([&](
            const CGameCtnReplayStaticSolidArchiveTreeChildLink &link) {
        const u32 parentTree = TreeIndexForNode(link.ParentTree());
        const u32 childTree = TreeIndexForNode(link.ChildTree());
        if (parentTree == UINT32_MAX || childTree == UINT32_MAX) {
            return 1;
        }
        if (!trees.ConnectChild(parentTree, childTree)) {
            return 1;
        }
        return 1;
    });

    treeGraph.ForEachTreeState([&](
            const CGameCtnReplayStaticSolidArchiveTreeStateDefinition &state) {
        CPlugTree *tree = TreeForNode(state.Tree());
        if (tree == nullptr) {
            return 1;
        }
        state.ApplyTreeState(tree);
        return 1;
    });

    return 1;
}

CPlugTree *StaticSolidTreeAssembler::TreeAt(u32 index) {
    return index < treeCount ? trees.TreeAt(index) : nullptr;
}

CPlugTree *StaticSolidTreeAssembler::TreeForNode(
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree) {
    const u32 treeIndex = TreeIndexForNode(tree);
    return treeIndex != UINT32_MAX ? TreeAt(treeIndex) : nullptr;
}

CPlugTree *StaticSolidTreeAssembler::CollisionRootForPayload(
        StaticSolidArchiveId payload) {
    return solidRoots.RootForPayload(payload);
}

CPlugTree *StaticSolidTreeAssembler::ModelRoot(
        StaticSolidArchiveId payload,
        std::optional<u32> treeNodeIndex) {
    return treeNodeIndex.has_value()
            ? TreeForNode(CGameCtnReplayStaticSolidArchiveNodeIdentity::
                                  FromPayloadAndArchiveIndex(
                                          payload, *treeNodeIndex))
            : CollisionRootForPayload(payload);
}

const CGameCtnReplayStaticSolidArchiveSolidPhysicsDefinition *
StaticSolidTreeAssembler::PhysicsDefinitionForPayload(
        StaticSolidArchiveId payload) {
    return solidRoots.PhysicalForPayload(payload);
}

const CGameCtnReplayStaticSolidArchiveSolidPhysicsDefinition *
StaticSolidTreeAssembler::PhysicsDefinitionForSolid(
        const CGameCtnReplayStaticSolidArchiveTreeGraph &treeGraph,
        CGameCtnReplayStaticSolidArchiveNodeIdentity solid) const {
    if (!solid.IsValid()) {
        return nullptr;
    }
    const CGameCtnReplayStaticSolidArchiveSolidPhysicsDefinition *physical =
            nullptr;
    treeGraph.ForEachSolidPhysicalDefinition([&](
            const CGameCtnReplayStaticSolidArchiveSolidPhysicsDefinition
                    &candidate) {
        if (candidate.MatchesSolid(solid)) {
            physical = &candidate;
        }
        return 1;
    });
    return physical;
}

int StaticSolidTreeAssembler::HasNodeMap() const {
    return trees.HasTrees();
}

int StaticSolidTreeAssembler::HasSolidRoots() const {
    return solidRoots.HasRoots();
}

u32 StaticSolidTreeAssembler::TreeCount() const {
    return treeCount;
}

void StaticSolidTreeAssembler::AttachSurface(
        CPlugTree *tree,
        CPlugSurface *surface) {
    if (tree != nullptr) {
        tree->SetSurface(surface);
    }
}

void StaticSolidTreeAssembler::LinkTreeToSurface(
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree,
        CGameCtnReplayStaticSolidArchiveNodeIdentity surface,
        StaticSolidSurfaceAssembler &surfaces) {
    CPlugTree *treeObject = TreeForNode(tree);
    CPlugSurface *surfaceObject = surfaces.SurfaceForNode(surface);
    if (treeObject == nullptr || surfaceObject == nullptr) {
        return;
    }

    AttachSurface(treeObject, surfaceObject);
}

void StaticSolidTreeAssembler::ApplyTreeSourceLink(
        const CGameCtnReplayStaticSolidArchiveTreeSourceLink &link,
        const CGameCtnReplayStaticSolidArchiveNodeDirectory &nodes,
        StaticSolidSurfaceAssembler &surfaces) {
    if (link.HasVisual()) {
        CPlugTree *tree = TreeForNode(link.Tree());
        StaticSolidArchiveVisual *provider =
                surfaces.VisualProviderForNode(link.Visual());
        if (tree != nullptr && provider != nullptr) {
            AttachVisual(tree, provider->Visual());
        }
    }

    if (link.HasMaterialOrShader()) {
        CPlugTree *tree = TreeForNode(link.Tree());
        if (tree != nullptr) {
            CPlugMaterial *material = link.HasMaterialNode()
                    ? surfaces.MaterialForNode(link.Material())
                    : nullptr;
            if (material == nullptr && link.HasShaderNode()) {
                material = surfaces.MaterialForNode(link.Shader());
            }
            AttachMaterial(tree, material);
        }
    }

    (void)nodes;
}

void StaticSolidTreeAssembler::ApplySolidTreeLink(
        const CGameCtnReplayStaticSolidArchiveTreeGraph &treeGraph,
        const CGameCtnReplayStaticSolidArchiveSolidTreeLink &link) {
    CPlugTree *tree = TreeForNode(link.Tree());
    if (tree == nullptr) {
        return;
    }
    solidRoots.Install(link.Solid().Payload(),
                       tree,
                       PhysicsDefinitionForSolid(treeGraph, link.Solid()));
    tree->UpdateBoundingBox(0u);
}

void StaticSolidTreeAssembler::ApplyTreeSurfaceLinks(
        const CGameCtnReplayStaticSolidArchiveTreeGraph &treeGraph,
        StaticSolidSurfaceAssembler &surfaces) {
    treeGraph.ForEachTreeSurfaceLink([&](
            const CGameCtnReplayStaticSolidArchiveTreeSurfaceLink &link) {
        LinkTreeToSurface(link.Tree(), link.Surface(), surfaces);
        return 1;
    });
}

void StaticSolidTreeAssembler::ApplyTreeSourceLinks(
        const CGameCtnReplayStaticSolidArchiveNodeDirectory &nodes,
        const CGameCtnReplayStaticSolidArchiveTreeGraph &treeGraph,
        StaticSolidSurfaceAssembler &surfaces) {
    treeGraph.ForEachTreeSourceLink([&](
            const CGameCtnReplayStaticSolidArchiveTreeSourceLink &link) {
        ApplyTreeSourceLink(link, nodes, surfaces);
        return 1;
    });
}

void StaticSolidTreeAssembler::ApplySolidTreeLinks(
        const CGameCtnReplayStaticSolidArchiveTreeGraph &treeGraph) {
    treeGraph.ForEachSolidRootLink([&](
        const CGameCtnReplayStaticSolidArchiveSolidTreeLink &link) {
        ApplySolidTreeLink(treeGraph, link);
        return 1;
    });
}

void StaticSolidTreeAssembler::AttachVisual(
        CPlugTree *tree,
        CPlugVisual *provider) {
    if (tree != nullptr) {
        tree->SetVisual(provider, nullptr, nullptr, 0);
    }
}

void StaticSolidTreeAssembler::AttachMaterial(
        CPlugTree *tree,
        CPlugMaterial *material) {
    if (tree != nullptr && material != nullptr) {
        tree->SetMaterial(material);
    }
}
