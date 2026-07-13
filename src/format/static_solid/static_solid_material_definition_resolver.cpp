#include "format/static_solid/static_solid_material_definition_resolver.h"
#include <utility>

#include "engine/game/material_definition.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_graph_writer.h"
#include "format/static_solid/static_solid_archive_node_graph.h"
#include "format/static_solid/static_solid_external_node_paths.h"
#include "format/archive/archive_class_ids.h"
void StaticSolidMaterialReference::InstallEmbedded(
        CGameCtnReplayStaticSolidArchiveNodeIdentity material) {
    material_ = material;
    source_ = Source::Embedded;
    identifier_.clear();
}

void StaticSolidMaterialReference::InstallExternal(
        CGameCtnReplayStaticSolidArchiveNodeIdentity material,
        std::string identifier) {
    material_ = material;
    source_ = Source::ExternalAsset;
    identifier_ = std::move(identifier);
}

void StaticSolidMaterialReference::InstallUnsupported(
        CGameCtnReplayStaticSolidArchiveNodeIdentity material) {
    material_ = material;
    source_ = Source::Unsupported;
    identifier_.clear();
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
StaticSolidMaterialReference::Material() const {
    return material_;
}

bool StaticSolidMaterialReference::RequiresAssetResolution() const {
    return source_ == Source::ExternalAsset && !identifier_.empty();
}

const std::string &StaticSolidMaterialReference::Identifier() const {
    return identifier_;
}

bool StaticSolidMaterialAssetLinker::ResolveAndAppend(
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        const SceneDescriptorFolderPaths *externalFolders,
        StaticSolidArchiveLoadSession *store,
        StaticSolidArchiveId payload,
        u32 materialNodeIndex) {
    return ResolveAndAppend(
            archiveNodeGraph,
            externalFolders,
            store,
            CGameCtnReplayStaticSolidArchiveNodeIdentity::
                    FromPayloadAndArchiveIndex(payload, materialNodeIndex));
}

bool StaticSolidMaterialAssetLinker::ResolveAndAppend(
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        const SceneDescriptorFolderPaths *externalFolders,
        StaticSolidArchiveLoadSession *store,
        CGameCtnReplayStaticSolidArchiveNodeIdentity material) {
    if (store == nullptr || archiveNodeGraph == nullptr) {
        return true;
    }
    const ArchiveNodeReference materialNode =
            material.ArchiveNode();
    StaticSolidMaterialReference reference;
    if (!archiveNodeGraph->BuildMaterialRef(
                material.Payload(), materialNode, &reference) ||
        !reference.RequiresAssetResolution()) {
        return true;
    }
    MaterialAssetRepository *assets = store->MaterialAssets();
    if (assets == nullptr) {
        return true;
    }
    std::optional<ResolvedMaterialDefinition> resolved;
    CGameCtnReplayStaticSolidExternalNodePathResult path;
    const CGameCtnReplayStaticSolidExternalNodeRef externalRef =
            archiveNodeGraph->ExternalNodeRef(materialNode);
    if (CGameCtnReplayStaticSolidExternalNodePathResolver::BuildPlainPath(
                externalFolders, externalRef, 0, &path)) {
        resolved = assets->ResolveMaterialPath(path.PlainPath());
    }
    if (!resolved) {
        resolved = assets->ResolveMaterial(reference.Identifier());
    }
    if (!resolved) {
        return true;
    }

    CGameCtnReplayStaticSolidArchiveMaterialDefinition definition;
    definition.InstallResolved(reference.Material(), *resolved);
    archiveNodeGraph->SetNodeClassId(materialNode,
                                     TMNF_CLASS_CPlugMaterial);
    CGameCtnReplayStaticSolidArchiveGraphWriter writer(
            &store->MutableArchiveGraph(), material.Payload());
    return writer.AppendNode(materialNode, TMNF_CLASS_CPlugMaterial) &&
           store->MutableArchiveGraph()
                   .SurfaceGraph()
                   .AddMaterialDefinition(definition);
}
