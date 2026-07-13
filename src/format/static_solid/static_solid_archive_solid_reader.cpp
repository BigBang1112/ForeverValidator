#include "format/static_solid/static_solid_archive_solid_reader.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_chunk_dispatcher.h"
#include "format/static_solid/static_solid_archive_graph_writer.h"
#include "format/static_solid/static_solid_archive_node_graph.h"
#include "format/static_solid/static_solid_archive_solid_chunk_ids.h"
void CPlugSolidPhysicalArchivePayload::Reset(StaticSolidArchiveId payload,
                                             u32 solidNodeIndex, u32 chunkId) {
  definition.Begin(
      CGameCtnReplayStaticSolidArchiveNodeIdentity::FromPayloadAndArchiveIndex(
          payload, solidNodeIndex),
      chunkId);
}

int CPlugSolidPhysicalArchivePayload::ReadMassCenterAndImpulseInertia(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
  if (byteStream == nullptr) {
    return 0;
  }
  float mass = 0.0f;
  float localCenterOfMass[3] = {};
  float impulseInertia[9] = {};
  if (!byteStream->ReadF32(&mass) ||
      !byteStream->ReadF32(&localCenterOfMass[0]) ||
      !byteStream->ReadF32(&localCenterOfMass[1]) ||
      !byteStream->ReadF32(&localCenterOfMass[2])) {
    return 0;
  }
  for (u32 i = 0u; i < 9u; i++) {
    if (!byteStream->ReadF32(&impulseInertia[i])) {
      return 0;
    }
  }
  definition.SetMassCenterAndImpulseInertia(mass, localCenterOfMass,
                                            impulseInertia);
  return 1;
}

int CPlugSolidPhysicalArchivePayload::ReadLegacyPhysicalDefinition(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
  if (byteStream == nullptr) {
    return 0;
  }
  float ignoredLinearSpeed = 0.0f;
  float ignoredAngularSpeed = 0.0f;
  definition.SetLegacyResponseDefaults();
  return byteStream->ReadF32(&ignoredLinearSpeed) &&
         byteStream->ReadF32(&ignoredAngularSpeed) &&
         ReadMassCenterAndImpulseInertia(byteStream);
}

int CPlugSolidPhysicalArchivePayload::ReadPhysicalDefinition(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
  float linearFluidFriction = 0.0f;
  float physicalResponseCoefA = 0.0f;
  float physicalResponseCoefB = 0.0f;
  if (!ReadMassCenterAndImpulseInertia(byteStream) ||
      !byteStream->ReadF32(&linearFluidFriction) ||
      !byteStream->ReadF32(&physicalResponseCoefA) ||
      !byteStream->ReadF32(&physicalResponseCoefB)) {
    return 0;
  }
  definition.SetResponse(linearFluidFriction, physicalResponseCoefA,
                         physicalResponseCoefB);
  return 1;
}

int CPlugSolidPhysicalArchivePayload::Install(
    StaticSolidArchiveLoadSession *store) const {
  return store == nullptr ||
         store->MutableArchiveGraph().TreeGraph().AddSolidPhysicalDefinition(
             definition);
}

void CPlugSolidTreeLinkArchivePayload::Reset(StaticSolidArchiveId newPayload,
                                             u32 newSolidNodeIndex,
                                             u32 newChunkId) {
  payload = newPayload;
  solidNode = ArchiveNodeReference::FromIndex(newSolidNodeIndex);
  chunkId = newChunkId;
  selectedTreeNode = ArchiveNodeReference::Invalid();
}

int CPlugSolidTreeLinkArchivePayload::ReadUseModelFlags(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 *useModel,
    u32 *hasModel) const {
  return byteStream != nullptr && byteStream->ReadBool(useModel) &&
         byteStream->ReadBool(hasModel);
}

int CPlugSolidTreeLinkArchivePayload::ReadOptionalModelNode(
    u32 hasModel,
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) const {
  ArchiveNodeReference modelNode = ArchiveNodeReference::Invalid();
  return hasModel == 0u || nodeRefs->ReadNodeRef(&modelNode);
}

int CPlugSolidTreeLinkArchivePayload::ReadTreeIfSolidDoesNotUseModel(
    u32 useModel, u32 hasModel,
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
  if (useModel != 0u && hasModel != 0u) {
    return 1;
  }
  return nodeRefs->ReadNodeRef(&selectedTreeNode);
}

int CPlugSolidTreeLinkArchivePayload::ReadTreeLinkWithMode(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
  if (byteStream == nullptr || nodeRefs == nullptr) {
    return 0;
  }
  u32 useModel = 0u;
  u32 hasModel = 0u;
  if (!ReadUseModelFlags(byteStream, &useModel, &hasModel) ||
      !ReadOptionalModelNode(hasModel, nodeRefs)) {
    return 0;
  }
  if (useModel != 0u && hasModel != 0u) {
    return 1;
  }

  u32 treeMode = ArchiveNodeReference::InvalidIndex;
  u32 readPrimary = 0u;
  u32 readSecondary = 0u;
  if (!byteStream->ReadU32(&treeMode) || !byteStream->ReadBool(&readPrimary) ||
      !byteStream->ReadBool(&readSecondary)) {
    return 0;
  }
  if (treeMode == 3u) {
    treeMode = 1u;
  }

  if (treeMode == 0u) {
    ArchiveNodeReference treeNode = ArchiveNodeReference::Invalid();
    if (!nodeRefs->ReadNodeRef(&treeNode)) {
      return 0;
    }
    if (!(useModel == 0u && hasModel != 0u)) {
      selectedTreeNode = treeNode;
    }
    return 1;
  }

  if (treeMode == 1u) {
    ArchiveNodeReference primaryTreeNode = ArchiveNodeReference::Invalid();
    ArchiveNodeReference secondaryTreeNode = ArchiveNodeReference::Invalid();
    const int primaryComesFromModel =
        useModel == 0u && hasModel != 0u && readPrimary != 0u;
    if (readPrimary && !nodeRefs->ReadNodeRef(&primaryTreeNode)) {
      return 0;
    }
    if (readPrimary) {
      selectedTreeNode = primaryTreeNode;
    }
    if (primaryComesFromModel) {
      selectedTreeNode = ArchiveNodeReference::Invalid();
    }
    if (readSecondary) {
      if (!nodeRefs->ReadNodeRef(&secondaryTreeNode)) {
        return 0;
      }
      if (!primaryComesFromModel && selectedTreeNode.IsInvalid()) {
        selectedTreeNode = secondaryTreeNode;
      }
    }
    return 1;
  }

  if (treeMode == 2u) {
    return nodeRefs->ReadNodeRef(&selectedTreeNode);
  }

  return 0;
}

int CPlugSolidTreeLinkArchivePayload::ReadTreeLink(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
  if (byteStream == nullptr || nodeRefs == nullptr) {
    return 0;
  }
  u32 useModel = 0u;
  u32 hasModel = 0u;
  return ReadUseModelFlags(byteStream, &useModel, &hasModel) &&
         ReadOptionalModelNode(hasModel, nodeRefs) &&
         ReadTreeIfSolidDoesNotUseModel(useModel, hasModel, nodeRefs);
}

int CPlugSolidTreeLinkArchivePayload::ReadTreeLinkWithFidModel(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
    CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
  if (byteStream == nullptr || nodeRefs == nullptr) {
    return 0;
  }
  u32 useModel = 0u;
  u32 hasModel = 0u;
  if (!ReadUseModelFlags(byteStream, &useModel, &hasModel)) {
    return 0;
  }
  if (hasModel != 0u) {
    u32 modelIsFid = 0u;
    if (!byteStream->ReadBool(&modelIsFid)) {
      return 0;
    }
    if (modelIsFid != 0u) {
      u32 modelFidRefIndex = ArchiveNodeReference::InvalidIndex;
      if (!byteStream->ReadU32(&modelFidRefIndex)) {
        return 0;
      }
      if (archiveNodeGraph != nullptr &&
          !archiveNodeGraph->RememberSolidModelFidLink(
              solidNode, ArchiveNodeReference::FromIndex(modelFidRefIndex))) {
        return 0;
      }
    } else if (!ReadOptionalModelNode(hasModel, nodeRefs)) {
      return 0;
    }
  }
  return ReadTreeIfSolidDoesNotUseModel(useModel, hasModel, nodeRefs);
}

int CPlugSolidTreeLinkArchivePayload::Install(
    StaticSolidArchiveLoadSession *store) const {
  CGameCtnReplayStaticSolidArchiveGraphWriter writer(
      store != nullptr ? &store->MutableArchiveGraph() : nullptr, payload);
  return writer.AddSolidRootLink(solidNode, selectedTreeNode, chunkId);
}

int CGameCtnReplayStaticSolidArchiveSolidReader::ParseCPlugSolidChunk(
    const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
    u32 chunkId) {
  if (context.byteStream == nullptr || context.nodeRefs == nullptr) {
    return 0;
  }

  const u32 solidNodeIndex = context.currentArchiveNode.Index();

  switch (chunkId) {
  case TMNF_CLASS_CPlugSolid:
    return context.byteStream->Skip(4u);
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam1):
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam2):
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam3):
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam4):
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam5):
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyEmpty8):
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyEmpty9):
    return 1;
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::PhysicalFactsLegacy): {
    CPlugSolidPhysicalArchivePayload payload;
    payload.Reset(context.payload, solidNodeIndex, chunkId);
    return payload.ReadLegacyPhysicalDefinition(context.byteStream) &&
           payload.Install(context.materialStore);
  }
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyBool):
    return context.byteStream->Skip(4u);
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::TreeLinkWithMode): {
    CPlugSolidTreeLinkArchivePayload payload;
    payload.Reset(context.payload, solidNodeIndex, chunkId);
    return payload.ReadTreeLinkWithMode(context.byteStream, context.nodeRefs) &&
           payload.Install(context.materialStore);
  }
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::BoolFlagsRealNatural):
    return context.byteStream->Skip(32u);
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::ExtendedBoolFlagsAndReals):
    return context.byteStream->Skip(72u);
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::TreeLink): {
    CPlugSolidTreeLinkArchivePayload payload;
    payload.Reset(context.payload, solidNodeIndex, chunkId);
    return payload.ReadTreeLink(context.byteStream, context.nodeRefs) &&
           payload.Install(context.materialStore);
  }
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::PhysicalFacts): {
    CPlugSolidPhysicalArchivePayload payload;
    payload.Reset(context.payload, solidNodeIndex, chunkId);
    return payload.ReadPhysicalDefinition(context.byteStream) &&
           payload.Install(context.materialStore);
  }
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::TwoReals):
    return context.byteStream->Skip(8u);
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::RefBufferDiscard):
    return context.nodeRefs->ReadNodPtr(nullptr);
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::TreeLinkWithFidModel): {
    CPlugSolidTreeLinkArchivePayload payload;
    payload.Reset(context.payload, solidNodeIndex, chunkId);
    return payload.ReadTreeLinkWithFidModel(context.byteStream,
                                            context.archiveNodeGraph,
                                            context.nodeRefs) &&
           payload.Install(context.materialStore);
  }
  case ArchiveChunkIdValue(CPlugSolidArchiveChunkId::PackedUseModelFlags):
    return context.byteStream->Skip(1u);
  default:
    return 0;
  }
}
