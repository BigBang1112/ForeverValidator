#include "format/static_solid/static_solid_archive_decorator_tree_reader.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_chunk_dispatcher.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_decorator_tree_chunk_ids.h"
#include "format/static_solid/static_solid_archive_node_graph.h"
#include "format/static_solid/static_solid_external_node_paths.h"
namespace {

int ResolveDecoratorTreeVisualSource(
    CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
    const SceneDescriptorFolderPaths *externalFolders,
    StaticSolidArchiveLoadSession *store,
    CGameCtnReplayStaticSolidDecoratorTreeDeclaration *declaration) {
  if (store == nullptr || declaration == nullptr) {
    return 0;
  }
  const ArchiveNodeReference visualNode = declaration->Visual().ArchiveNode();
  if (archiveNodeGraph == nullptr ||
      archiveNodeGraph->FindNode(visualNode) == nullptr) {
    return 1;
  }
  const CGameCtnReplayStaticSolidExternalNodeRef visualNodeRef =
      archiveNodeGraph->ExternalNodeRef(visualNode);
  if (!visualNodeRef.CanResolvePath(0)) {
    return 1;
  }

  CGameCtnReplayStaticSolidExternalNodePathResult path;
  if (!CGameCtnReplayStaticSolidExternalNodePathResolver::
          BuildPlainAndTrySelectedPath(externalFolders, store->ExternalPack(),
                                       visualNodeRef, 0, &path)) {
    return 1;
  }
  return declaration->SetExternalDescriptorPaths(
      path.PlainPath(), path.HasSelectedPath() ? path.SelectedPath() : nullptr);
}

int InstallDecoratorTreeDeclaration(
    CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
    const SceneDescriptorFolderPaths *externalFolders,
    StaticSolidArchiveLoadSession *store,
    const CGameCtnReplayStaticSolidDecoratorTreeDeclaration *declaration) {
  if (store == nullptr || declaration == nullptr) {
    return 1;
  }
  CGameCtnReplayStaticSolidDecoratorTreeDeclaration resolvedDeclaration =
      *declaration;
  if (!ResolveDecoratorTreeVisualSource(archiveNodeGraph, externalFolders,
                                        store, &resolvedDeclaration)) {
    return 0;
  }
  return store->MutableArchiveGraph().AddDecoratorTree(resolvedDeclaration);
}

} // namespace

void CPlugDecoratorTreeArchivePayload::Reset(StaticSolidArchiveId payload,
                                             u32 decoratorNodeIndex,
                                             u32 chunkId) {
  declaration = CGameCtnReplayStaticSolidDecoratorTreeDeclaration{};
  declaration.Reset(
      CGameCtnReplayStaticSolidArchiveNodeIdentity::FromPayloadAndArchiveIndex(
          payload, decoratorNodeIndex),
      chunkId);
}

int CPlugDecoratorTreeArchivePayload::ReadBoolOnlySurfaceFromVisual(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
  u32 ignoredBool = 0u;
  return byteStream != nullptr && byteStream->ReadBool(&ignoredBool) &&
         byteStream->ReadBool(&ignoredBool) &&
         (declaration.SetSolidSurfaceFromVisual(ignoredBool), 1);
}

int CPlugDecoratorTreeArchivePayload::ReadTreeIdAndRefs(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
    CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
  if (byteStream == nullptr || cmwIdState == nullptr || nodeRefs == nullptr) {
    return 0;
  }

  CMwId treeId;
  if (!cmwIdState->ReadId(*byteStream, &treeId)) {
    return 0;
  }
  declaration.SetTreeId(treeId);

  if (CPlugDecoratorTreeChunkHasPreMaterialWord(declaration.ChunkId()) &&
      !byteStream->Skip(4u)) {
    return 0;
  }

  ArchiveNodeReference materialNode = ArchiveNodeReference::Invalid();
  if (!nodeRefs->ReadNodeRef(&materialNode)) {
    return 0;
  }
  declaration.SetMaterialNode(materialNode);
  return 1;
}

int CPlugDecoratorTreeArchivePayload::ReadRootMaterialOnlyTail(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
  if (byteStream == nullptr || nodeRefs == nullptr) {
    return 0;
  }
  u32 ignoredBool = 0u;
  return nodeRefs->ReadNodPtr(nullptr) && byteStream->ReadBool(&ignoredBool) &&
         byteStream->ReadBool(&ignoredBool) &&
         (declaration.SetSolidSurfaceFromVisual(ignoredBool), 1);
}

int CPlugDecoratorTreeArchivePayload::ReadLegacyVisibilityTail(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
  if (byteStream == nullptr) {
    return 0;
  }
  u32 ignoredBool = 0u;
  return byteStream->ReadBool(&ignoredBool) &&
         byteStream->ReadBool(&ignoredBool) &&
         (declaration.SetSolidSurfaceFromVisual(ignoredBool), 1);
}

int CPlugDecoratorTreeArchivePayload::ReadConditionalTail(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
  if (byteStream == nullptr) {
    return 0;
  }

  u32 showCondition = 0u;
  u32 visibleCondition = 0u;
  u32 applyVisibleToChildren = 0u;
  u32 casterCondition = 0u;
  u32 applyCasterToChildren = 0u;
  u32 solidSurfaceFromVisual = 0u;
  if (!byteStream->ReadU32(&showCondition) ||
      !byteStream->ReadU32(&visibleCondition) ||
      !byteStream->ReadBool(&applyVisibleToChildren) ||
      !byteStream->ReadU32(&casterCondition) ||
      !byteStream->ReadBool(&applyCasterToChildren) ||
      !byteStream->ReadBool(&solidSurfaceFromVisual)) {
    return 0;
  }
  declaration.SetVisibilityConditions(
      showCondition, visibleCondition, applyVisibleToChildren, casterCondition,
      applyCasterToChildren, solidSurfaceFromVisual);
  if (CPlugDecoratorTreeChunkHasIdentityGate(declaration.ChunkId())) {
    u32 nearlyEqualIdentityCondition = 0u;
    if (!byteStream->ReadBool(&nearlyEqualIdentityCondition)) {
      return 0;
    }
    declaration.SetNearlyEqualIdentityCondition(nearlyEqualIdentityCondition);
  }
  if (CPlugDecoratorTreeChunkHasCollisionGate(declaration.ChunkId())) {
    u32 collisionCondition = 0u;
    if (!byteStream->ReadU32(&collisionCondition)) {
      return 0;
    }
    declaration.SetCollisionCondition(collisionCondition);
  }
  return 1;
}

int CPlugDecoratorTreeArchivePayload::ReadFromArchive(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
    CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
  if (byteStream == nullptr || cmwIdState == nullptr || nodeRefs == nullptr) {
    return 0;
  }
  const u32 chunkId = declaration.ChunkId();
  if (!IsCPlugDecoratorTreeArchiveChunk(chunkId)) {
    return 0;
  }

  if (chunkId ==
      ArchiveChunkIdValue(
          CPlugDecoratorTreeArchiveChunkId::BoolOnlySurfaceFromVisual)) {
    return ReadBoolOnlySurfaceFromVisual(byteStream);
  }

  if (!ReadTreeIdAndRefs(byteStream, cmwIdState, nodeRefs)) {
    return 0;
  }

  if (chunkId == TMNF_CLASS_CPlugDecoratorTree) {
    return ReadRootMaterialOnlyTail(byteStream, nodeRefs);
  }

  ArchiveNodeReference visualNode = ArchiveNodeReference::Invalid();
  if (!nodeRefs->ReadNodeRef(&visualNode)) {
    return 0;
  }
  declaration.SetVisualNode(visualNode);

  if (CPlugDecoratorTreeChunkUsesLegacyVisibilityTail(chunkId)) {
    return ReadLegacyVisibilityTail(byteStream);
  }

  return ReadConditionalTail(byteStream);
}

int CPlugDecoratorTreeArchivePayload::Install(
    CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
    const SceneDescriptorFolderPaths *externalFolders,
    StaticSolidArchiveLoadSession *store) const {
  return InstallDecoratorTreeDeclaration(archiveNodeGraph, externalFolders,
                                         store, &declaration);
}

int CGameCtnReplayStaticSolidArchiveDecoratorTreeReader::
    ParseCPlugDecoratorTreeChunk(
        const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
        u32 chunkId) {
  CPlugDecoratorTreeArchivePayload decoratorPayload;
  decoratorPayload.Reset(context.payload, context.currentArchiveNode.Index(),
                         chunkId);
  return decoratorPayload.ReadFromArchive(
             context.byteStream, context.cmwIdState, context.nodeRefs) &&
         decoratorPayload.Install(context.archiveNodeGraph,
                                  context.externalFolders,
                                  context.materialStore);
}
