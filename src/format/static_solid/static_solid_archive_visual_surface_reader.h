#ifndef TMNF_STATIC_SOLID_ARCHIVE_VISUAL_SURFACE_READER_H
#define TMNF_STATIC_SOLID_ARCHIVE_VISUAL_SURFACE_READER_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_node_reference.h"
#include "format/static_solid/static_solid_archive_id.h"
#include "format/static_solid/static_solid_archive_node_ref_reader.h"
struct CGameCtnReplayStaticSolidArchiveByteStream;
struct CGameCtnReplayStaticSolidArchiveCMwIdState;
struct CGameCtnReplayStaticSolidArchiveDecodeProgress;
struct CGameCtnReplayStaticSolidArchiveFeedback;
struct CGameCtnReplayStaticSolidArchiveNodeGraph;
struct StaticSolidArchiveVisualState;
struct StaticSolidArchiveLoadSession;
struct CGameCtnReplayStaticSolidArchiveChunkDispatchContext;

struct CGameCtnReplayStaticSolidArchiveVisualSurfaceReader {
  static int ParseVisualSurfaceChunk(
      const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
      u32 classId, u32 chunkId);
};

#endif
