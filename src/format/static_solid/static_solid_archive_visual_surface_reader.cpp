#include "format/static_solid/static_solid_archive_visual_surface_reader.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_chunk_dispatcher.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_surface_geom_reader.h"
#include "format/static_solid/static_solid_archive_surface_reader.h"
#include "format/static_solid/static_solid_archive_visual_provider_reader.h"
#include "format/static_solid/static_solid_archive_visual_surface_chunk_ids.h"
namespace {

struct CGameCtnReplayStaticSolidSkippedNodeRefs {
  static int
  ReadSingle(CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs);
  static int
  ReadFastBuffer(CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
                 CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs);
};

struct GxLightArchiveParser {
  int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 classId,
           u32 chunkId);
};

class CGameCtnReplayStaticSolidVisualSurfaceParser {
public:
  explicit CGameCtnReplayStaticSolidVisualSurfaceParser(
      const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context);

  int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 classId,
           u32 chunkId);

private:
  int ReadCountedByteRecords(
      CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 recordSize);
  int ReadVisualSurfaceBase(
      CGameCtnReplayStaticSolidArchiveByteStream *byteStream);

  const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context_;
};

class CGameCtnReplayStaticSolidSurfaceGeomParser {
public:
  explicit CGameCtnReplayStaticSolidSurfaceGeomParser(
      const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context);

  int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 chunkId);

private:
  const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context_;
};

struct CGameCtnReplayStaticSolidIndexBufferParser {
  int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 chunkId);
};

int CGameCtnReplayStaticSolidSkippedNodeRefs::ReadSingle(
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
  return nodeRefs != nullptr && nodeRefs->ReadNodPtr(nullptr);
}

int CGameCtnReplayStaticSolidSkippedNodeRefs::ReadFastBuffer(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
  if (byteStream == nullptr || nodeRefs == nullptr) {
    return 0;
  }
  constexpr u32 kMaxArchivedRefCount = 0x100000u;
  u32 reservedWord = 0u;
  u32 count = 0u;
  if (!byteStream->ReadU32(&reservedWord) || !byteStream->ReadU32(&count) ||
      count > kMaxArchivedRefCount) {
    return 0;
  }
  for (u32 i = 0u; i < count; i++) {
    if (!ReadSingle(nodeRefs)) {
      return 0;
    }
  }
  return 1;
}

int GxLightArchiveParser::Read(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 classId,
    u32 chunkId) {
  if (byteStream == nullptr) {
    return 0;
  }
  switch (chunkId) {
  case TMNF_CLASS_GxLight:
  case ArchiveChunkIdValue(GxLightArchiveChunkId::LegacyBaseState1):
  case ArchiveChunkIdValue(GxLightArchiveChunkId::LegacyBaseState2):
  case ArchiveChunkIdValue(GxLightArchiveChunkId::LegacyBaseState3):
  case ArchiveChunkIdValue(GxLightArchiveChunkId::LegacyBaseState4):
  case ArchiveChunkIdValue(GxLightArchiveChunkId::LegacyBaseState5):
  case ArchiveChunkIdValue(GxLightArchiveChunkId::LegacyBaseState6):
  case ArchiveChunkIdValue(GxLightArchiveChunkId::LegacyBaseState7):
  case ArchiveChunkIdValue(GxLightArchiveChunkId::LegacyBaseState8):
    return 1;
  case ArchiveChunkIdValue(GxLightArchiveChunkId::FullLightState):
    /*
     * Full light state is 52 bytes: position color, light parameters,
     * six binary32 values, and shadow color.
     */
    return byteStream->Skip(52u);
  case TMNF_CLASS_GxLightAmbient:
    return classId == TMNF_CLASS_GxLightAmbient && byteStream->Skip(8u);
  case TMNF_CLASS_GxLightDirectional:
    return classId == TMNF_CLASS_GxLightDirectional && byteStream->Skip(12u);
  case ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId::EnabledVector):
    return classId == TMNF_CLASS_GxLightDirectional && byteStream->Skip(16u);
  case ArchiveChunkIdValue(
      GxLightDirectionalArchiveChunkId::EnabledVectorAndAngles):
    return classId == TMNF_CLASS_GxLightDirectional && byteStream->Skip(24u);
  case ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId::DirectionVector):
    return classId == TMNF_CLASS_GxLightDirectional && byteStream->Skip(12u);
  case ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId::DirectionColor):
    return classId == TMNF_CLASS_GxLightDirectional && byteStream->Skip(16u);
  case ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId::AngularLimits):
    return classId == TMNF_CLASS_GxLightDirectional && byteStream->Skip(8u);
  default:
    return 0;
  }
}

CGameCtnReplayStaticSolidVisualSurfaceParser::
    CGameCtnReplayStaticSolidVisualSurfaceParser(
        const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context)
    : context_(context) {}

int CGameCtnReplayStaticSolidVisualSurfaceParser::ReadCountedByteRecords(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 recordSize) {
  if (byteStream == nullptr) {
    return 0;
  }
  constexpr u32 kMaxRecordCount = 0x100000u;
  u32 count = 0u;
  if (!byteStream->ReadU32(&count) || count > kMaxRecordCount) {
    return 0;
  }
  return byteStream->SkipCounted(count, recordSize);
}

int CGameCtnReplayStaticSolidVisualSurfaceParser::ReadVisualSurfaceBase(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
  return context_.cmwIdState != nullptr &&
         context_.cmwIdState->ReadSkip(*byteStream) &&
         CGameCtnReplayStaticSolidSkippedNodeRefs::ReadFastBuffer(
             byteStream, context_.nodeRefs);
}

int CGameCtnReplayStaticSolidVisualSurfaceParser::Read(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 classId,
    u32 chunkId) {
  if (byteStream == nullptr || context_.cmwIdState == nullptr ||
      context_.nodeRefs == nullptr) {
    return 0;
  }
  switch (chunkId) {
  case TMNF_CLASS_CPlugVisual:
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmpty2):
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::GeometryHeader):
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmpty7):
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmpty8):
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmptyA):
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmptyC):
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmptyD):
  case TMNF_CLASS_CPlugVisual3D:
  case ArchiveChunkIdValue(CPlugVisual3DArchiveChunkId::VertexStream):
  case ArchiveChunkIdValue(
      CPlugVisual3DArchiveChunkId::VertexStreamAndTangents):
    return 1;
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::Identifier):
    return context_.cmwIdState->ReadSkip(*byteStream);
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::FuncVisualRef):
    return CGameCtnReplayStaticSolidSkippedNodeRefs::ReadSingle(
        context_.nodeRefs);
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::Vec3Array):
    return ReadCountedByteRecords(byteStream, 12u);
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyWord6):
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyWord9):
    return byteStream->Skip(4u);
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::Record32Array):
    return ReadCountedByteRecords(byteStream, 32u);
  case ArchiveChunkIdValue(CPlugVisualArchiveChunkId::VisualGeometryProvider):
    return StaticSolidArchiveVisualReader::ParseVisualGeometryProvider(
        byteStream, context_.visualProviderState, context_.materialStore,
        context_.payload, context_.currentArchiveNode);
  case TMNF_CLASS_CPlugSurface:
    return ReadVisualSurfaceBase(byteStream);
  case ArchiveChunkIdValue(CPlugVisual3DArchiveChunkId::MaterialRef):
    return CGameCtnReplayStaticSolidSkippedNodeRefs::ReadSingle(
        context_.nodeRefs);
  case ArchiveChunkIdValue(CPlugVisual3DArchiveChunkId::FaceStream):
    return StaticSolidArchiveVisualReader::ParseVisual3dFaceStream(
        byteStream, context_.visualProviderState, context_.materialStore,
        classId);
  case ArchiveChunkIdValue(CPlugVisualIndexedArchiveChunkId::IndexBuffer):
    return StaticSolidArchiveVisualReader::ParseVisualIndexBuffer(
        byteStream, context_.feedback, context_.decodeProgress,
        context_.materialStore, context_.visualProviderState);
  case ArchiveChunkIdValue(CPlugVisualSpriteArchiveChunkId::SpriteParameters):
    return byteStream->Skip(24u);
  case ArchiveChunkIdValue(CPlugVisualSpriteArchiveChunkId::AtlasGrid):
    return byteStream->Skip(4u);
  default:
    return 0;
  }
}

CGameCtnReplayStaticSolidSurfaceGeomParser::
    CGameCtnReplayStaticSolidSurfaceGeomParser(
        const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context)
    : context_(context) {}

int CGameCtnReplayStaticSolidSurfaceGeomParser::Read(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 chunkId) {
  if (byteStream == nullptr || context_.cmwIdState == nullptr) {
    return 0;
  }
  switch (chunkId) {
  case TMNF_CLASS_CPlugSurfaceGeom:
    return 1;
  case TMNF_CLASS_CPlugSurface:
    return context_.cmwIdState->ReadSkip(*byteStream);
  case ArchiveChunkIdValue(CPlugSurfaceArchiveChunkId::LegacyWord):
    return byteStream->Skip(4u);
  case ArchiveChunkIdValue(CPlugSurfaceGeomArchiveChunkId::IdentifierAndBox):
    return context_.cmwIdState->ReadSkip(*byteStream) && byteStream->Skip(24u);
  case ArchiveChunkIdValue(CPlugSurfaceGeomArchiveChunkId::SurfaceGeometry):
    return CGameCtnReplayStaticSolidArchiveSurfaceGeomReader::
        ParseSurfaceGeometry(context_);
  default:
    return 0;
  }
}

int CGameCtnReplayStaticSolidIndexBufferParser::Read(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 chunkId) {
  if (byteStream == nullptr || chunkId != TMNF_CLASS_CPlugIndexBuffer) {
    return 0;
  }
  u32 indexCount = 0u;
  return byteStream->Skip(4u) && byteStream->ReadU32(&indexCount) &&
         byteStream->SkipCounted(indexCount, 2u);
}

} // namespace

int CPlugVisualGridArchivePayload::Chunk(
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream, u32 chunkId) {
  return byteStream != nullptr && IsCPlugVisualGridInfo3Chunk(chunkId) &&
         byteStream->ReadU32(&nbPointX) && byteStream->ReadU32(&nbPointZ) &&
         byteStream->ReadF32(&rangeX) && byteStream->ReadF32(&rangeZ);
}

int CGameCtnReplayStaticSolidArchiveVisualSurfaceReader::
    ParseVisualSurfaceChunk(
        const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
        u32 classId, u32 chunkId) {
  if (classId == TMNF_CLASS_GxLightAmbient ||
      classId == TMNF_CLASS_GxLightDirectional) {
    GxLightArchiveParser lightParser;
    return lightParser.Read(context.byteStream, classId, chunkId);
  }
  if (classId == TMNF_CLASS_CPlugVisualGrid &&
      IsCPlugVisualGridInfo3Chunk(chunkId)) {
    CPlugVisualGridArchivePayload grid;
    return grid.Chunk(context.byteStream, chunkId);
  }
  if (classId == TMNF_CLASS_CPlugVisualIndexedTriangles ||
      classId == TMNF_CLASS_CPlugVisualSprite ||
      classId == TMNF_CLASS_CPlugVisualGrid) {
    CGameCtnReplayStaticSolidVisualSurfaceParser visualParser(context);
    return visualParser.Read(context.byteStream, classId, chunkId);
  }
  if (classId == TMNF_CLASS_CPlugSurface &&
      chunkId == TMNF_CLASS_CPlugSurface) {
    return CGameCtnReplayStaticSolidArchiveSurfaceReader::
        ParseSurfaceDefinition(context.byteStream, context.archiveNodeGraph,
                               context.externalFolders,
                               context.materialStore, context.payload,
                               context.currentArchiveNode.Index(),
                               context.nodeRefs);
  }
  if (classId == TMNF_CLASS_CPlugSurfaceGeom) {
    CGameCtnReplayStaticSolidSurfaceGeomParser surfaceGeomParser(context);
    return surfaceGeomParser.Read(context.byteStream, chunkId);
  }
  if (classId == TMNF_CLASS_CPlugIndexBuffer) {
    CGameCtnReplayStaticSolidIndexBufferParser indexBufferParser;
    return indexBufferParser.Read(context.byteStream, chunkId);
  }
  if (classId == TMNF_CLASS_CPlugDecoratorSolid &&
      chunkId == TMNF_CLASS_CPlugDecoratorSolid) {
    return CGameCtnReplayStaticSolidSkippedNodeRefs::ReadFastBuffer(
        context.byteStream, context.nodeRefs);
  }
  return 0;
}
