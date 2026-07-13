#ifndef TMNF_STATIC_SOLID_ARCHIVE_DECODE_PROGRESS_H
#define TMNF_STATIC_SOLID_ARCHIVE_DECODE_PROGRESS_H

#include "engine/core/engine_types.h"
#include "format/static_solid/static_scene_archive_loader.h"
struct CGameCtnReplayStaticSolidArchiveFeedback;

struct CGameCtnReplayStaticSolidArchiveDecodeFailure {
    u32 classId = 0u;
    u32 chunkId = 0u;
    u32 offset = 0u;
    u32 compressedRead = 0u;
};

struct CGameCtnReplayStaticSolidArchiveDecodeProgress {
    void MarkBodyOffsetParsed();
    void MarkFailure(
            u32 classId,
            u32 chunkId,
            u32 offset,
            u32 compressedRead);

    void RecordArchiveNode(u32 classId);
    void RecordArchiveTreeChildLinks(u32 count);
    void RecordArchiveMesh();
    void RecordArchiveSurfaceGeom();

    const CGameCtnReplayStaticSolidArchiveDecodeFailure &FirstFailure() const;
    void CaptureStats(
            const CGameCtnReplayStaticSolidArchiveFeedback &feedback,
            CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut) const;

private:
    CGameCtnReplayStaticSolidArchiveDecodeFailure firstFailure;
    u32 bodyOffsetParsed = 0u;
    u32 archiveNodeCount = 0u;
    u32 archiveTreeCount = 0u;
    u32 archiveSurfaceCount = 0u;
    u32 archiveSurfaceGeomCount = 0u;
    u32 archiveMeshCount = 0u;
    u32 archiveTreeChildLinkCount = 0u;
};

#endif
