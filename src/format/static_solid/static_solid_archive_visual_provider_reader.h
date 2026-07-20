#ifndef TMNF_STATIC_SOLID_ARCHIVE_VISUAL_PROVIDER_READER_H
#define TMNF_STATIC_SOLID_ARCHIVE_VISUAL_PROVIDER_READER_H

#include "format/archive/archive_node_reference.h"
#include "format/static_solid/static_solid_archive_payload_slice.h"
#include "format/static_solid/static_solid_archive_id.h"
struct CGameCtnReplayStaticSolidArchiveByteStream;
struct CGameCtnReplayStaticSolidArchiveDecodeProgress;
struct CGameCtnReplayStaticSolidArchiveFeedback;
struct StaticSolidArchiveVisualState;
struct StaticSolidArchiveLoadSession;
class CGameCtnReplayStaticSolidVisualGeometryDefinition;

struct StaticSolidArchiveVisualReader {
    static int ParseVisualGeometryProvider(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
            StaticSolidArchiveVisualState *state,
            StaticSolidArchiveLoadSession *store,
            StaticSolidArchiveId payload,
            ArchiveNodeReference visualNode);

    static int ParseVisual3dFaceStream(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
            StaticSolidArchiveVisualState *state,
            StaticSolidArchiveLoadSession *store,
            u32 classId);

    static int ParseVisualIndexBuffer(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
            CGameCtnReplayStaticSolidArchiveFeedback *feedback,
            CGameCtnReplayStaticSolidArchiveDecodeProgress *decodeProgress,
            StaticSolidArchiveLoadSession *store,
            StaticSolidArchiveVisualState *state);
};

#endif
