#include "format/static_solid/static_solid_archive_surface_geom_reader.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_chunk_dispatcher.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_decode_progress.h"
#include "format/static_solid/static_solid_archive_feedback.h"
CGameCtnReplayStaticSolidSurfaceGeometryDefinition
CGameCtnReplayStaticSolidSurfaceGeometryDefinition::FromArchive(
    StaticSolidArchiveId payload, ArchiveNodeReference geomNode,
    u32 newSurfaceType, uint16_t newMaterialId,
    const GmBoxAligned &newBoundingBox,
    CGameCtnReplayStaticSolidArchiveMeshPayload newMeshPayload) {
  CGameCtnReplayStaticSolidSurfaceGeometryDefinition definition;
  definition.geom =
      CGameCtnReplayStaticSolidArchiveNodeIdentity::FromPayloadAndArchiveIndex(
          payload, geomNode.Index());
  definition.surfaceType = newSurfaceType;
  definition.materialId = newMaterialId;
  definition.meshPayload = newMeshPayload;
  definition.boundingBox = newBoundingBox;
  return definition;
}

CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition
CGameCtnReplayStaticSolidSurfaceGeometryDefinition::ArchiveDefinition() const {
  CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition definition;
  definition.Install(geom, surfaceType, materialId, boundingBox, meshPayload);
  return definition;
}

int CGameCtnReplayStaticSolidSurfaceGeometryDefinition::Commit(
    StaticSolidArchiveLoadSession *store) const {
  return store == nullptr ||
         store->MutableArchiveGraph()
             .SurfaceGraph()
             .AddSurfaceGeometryDefinition(ArchiveDefinition());
}

int CGameCtnReplayStaticSolidArchiveSurfaceGeomReader::ParseGmSurfMeshArchive(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
    CGameCtnReplayStaticSolidArchiveDecodeProgress *decodeProgress,
    GmSurfMeshArchiveRecords *recordsOut) {
  if (byteStream == nullptr || recordsOut == nullptr) {
    return 0;
  }

  u32 mode = 0u;
  u32 count = 0u;
  if (!byteStream->ReadU32(&mode) || mode != 3u ||
      !byteStream->ReadU32(&count)) {
    return 0;
  }
  recordsOut->vertexCount = count;
  recordsOut->vertexByteOffset = byteStream->Offset();
  if (!byteStream->SkipCounted(
          count,
          CGameCtnReplayStaticSolidArchiveMeshPayload::VertexRecordBytes) ||
      !byteStream->ReadU32(&count)) {
    return 0;
  }

  recordsOut->triangleCount = count;
  recordsOut->triangleByteOffset = byteStream->Offset();
  if (!byteStream->SkipCounted(
          count,
          CGameCtnReplayStaticSolidArchiveMeshPayload::TriangleRecordBytes) ||
      !byteStream->ReadU32(&mode) || mode != 3u ||
      !byteStream->ReadU32(&count)) {
    return 0;
  }

  recordsOut->octreeCellCount = count;
  recordsOut->octreeCellByteOffset = byteStream->Offset();
  if (!byteStream->SkipCounted(
          count,
          CGameCtnReplayStaticSolidArchiveMeshPayload::OctreeCellRecordBytes)) {
    return 0;
  }
  if (decodeProgress != nullptr) {
    decodeProgress->RecordArchiveMesh();
  }
  return 1;
}

int CGameCtnReplayStaticSolidArchiveSurfaceGeomReader::ParseSurfaceGeometry(
    const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context) {
  if (context.byteStream == nullptr || context.cmwIdState == nullptr ||
      context.feedback == nullptr) {
    return 0;
  }
  if (!context.cmwIdState->ReadSkip(*context.byteStream)) {
    return 0;
  }

  GmBoxAligned boundingBox;
  if (!context.byteStream->ReadF32(&boundingBox.center.x) ||
      !context.byteStream->ReadF32(&boundingBox.center.y) ||
      !context.byteStream->ReadF32(&boundingBox.center.z) ||
      !context.byteStream->ReadF32(&boundingBox.halfExtents.x) ||
      !context.byteStream->ReadF32(&boundingBox.halfExtents.y) ||
      !context.byteStream->ReadF32(&boundingBox.halfExtents.z)) {
    return 0;
  }
  const float minX = boundingBox.center.x - boundingBox.halfExtents.x;
  if (!context.byteStream->ApplyFeedbackFloat(*context.feedback, minX, 1)) {
    return 0;
  }

  u32 surfType = 0u;
  if (!context.byteStream->ReadU32(&surfType)) {
    return 0;
  }
  GmSurfMeshArchiveRecords meshRecords{};
  switch (surfType) {
  case 0u:
    if (!context.byteStream->Skip(4u)) {
      return 0;
    }
    break;
  case 1u:
    if (!context.byteStream->Skip(12u)) {
      return 0;
    }
    break;
  case 6u:
    if (!context.byteStream->Skip(24u)) {
      return 0;
    }
    break;
  case 7u:
    if (!ParseGmSurfMeshArchive(context.byteStream, context.decodeProgress,
                                &meshRecords)) {
      return 0;
    }
    break;
  default:
    return 0;
  }

  u32 materialId = 0u;
  if (context.decodeProgress != nullptr) {
    context.decodeProgress->RecordArchiveSurfaceGeom();
  }
  if (!context.byteStream->ReadU16(&materialId)) {
    return 0;
  }
  if (context.materialStore == nullptr) {
    return 1;
  }

  const CGameCtnReplayStaticSolidArchiveMeshPayload meshPayload =
      surfType == static_cast<u32>(GmSurf::EGmSurfType::Mesh)
          ? CGameCtnReplayStaticSolidArchiveMeshPayload::FromArchiveRecords(
                meshRecords.vertexByteOffset, meshRecords.vertexCount,
                meshRecords.triangleByteOffset, meshRecords.triangleCount,
                meshRecords.octreeCellByteOffset, meshRecords.octreeCellCount)
          : CGameCtnReplayStaticSolidArchiveMeshPayload::Empty();
  const CGameCtnReplayStaticSolidSurfaceGeometryDefinition geometry =
      CGameCtnReplayStaticSolidSurfaceGeometryDefinition::FromArchive(
          context.payload, context.currentArchiveNode, surfType,
          (uint16_t)materialId, boundingBox, meshPayload);
  return geometry.Commit(context.materialStore);
}
