#include "format/static_solid/static_solid_archive_assembler.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_graph.h"
void StaticSolidArchiveAssembler::Clear() {
    trees_.Free();
    surfaces_.Free();
    decorators_.Free();
}

bool StaticSolidArchiveAssembler::Assemble(
        const CGameCtnReplayStaticSolidArchiveGraph &archive,
        StaticSolidArchiveLoadSession &payloads) {
    Clear();
    const auto &nodes = archive.Nodes();
    const auto &trees = archive.TreeGraph();
    if (!trees_.BuildTrees(nodes, trees)) {
        Clear();
        return false;
    }
    if (nodes.Empty()) {
        return true;
    }

    const StaticSolidDecodedPayloads decoded(payloads);
    if (!surfaces_.MaterializeArchiveGraphWithDefaultMaterials(
                nodes, archive.SurfaceGraph(), decoded)) {
        Clear();
        return false;
    }
    trees_.ApplyTreeSurfaceLinks(trees, surfaces_);
    trees_.ApplyTreeSourceLinks(nodes, trees, surfaces_);
    if (!decorators_.MaterializeDecoratorTrees(
                archive.DecoratorTrees(), payloads, trees_, surfaces_)) {
        Clear();
        return false;
    }
    trees_.ApplySolidTreeLinks(trees);
    return true;
}

CPlugTree *StaticSolidArchiveAssembler::CollisionRoot(
        StaticSolidArchiveId archiveId) {
    return trees_.CollisionRootForPayload(archiveId);
}

CPlugTree *StaticSolidArchiveAssembler::ModelRoot(
        StaticSolidArchiveId archiveId,
        std::optional<u32> treeNodeIndex) {
    return trees_.ModelRoot(archiveId, treeNodeIndex);
}

const CGameCtnReplayStaticSolidArchiveSolidPhysicsDefinition *
StaticSolidArchiveAssembler::Physics(StaticSolidArchiveId archiveId) {
    return trees_.PhysicsDefinitionForPayload(archiveId);
}

void StaticSolidArchiveAssembler::ApplyReplacementMaterials(
        CPlugTree *tree,
        StaticSolidArchiveId archiveId) {
    surfaces_.ApplyMaterialRemapToTree(
            tree,
            archiveId,
            StaticSolidMaterialRemap::Replacement);
}

void StaticSolidArchiveAssembler::ApplyDecorationSkinMaterials(
        CPlugTree *tree,
        StaticSolidArchiveId archiveId) {
    surfaces_.ApplyMaterialRemapToTree(
            tree,
            archiveId,
            StaticSolidMaterialRemap::DecorationSkin);
}

void StaticSolidArchiveAssembler::ApplyWarpDecoration(CPlugTree *tree) {
    decorators_.ApplyDecoratorTreeFlagsToWarpRoot(tree);
}
