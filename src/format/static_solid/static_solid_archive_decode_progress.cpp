#include "format/static_solid/static_solid_archive_decode_progress.h"
#include "format/static_solid/static_solid_archive_feedback.h"
#include "format/archive/archive_class_ids.h"
void CGameCtnReplayStaticSolidArchiveDecodeProgress::MarkBodyOffsetParsed() {
    bodyOffsetParsed = 1u;
}

void CGameCtnReplayStaticSolidArchiveDecodeProgress::MarkFailure(
        u32 classId,
        u32 chunkId,
        u32 offset,
        u32 compressedRead) {
    if (firstFailure.classId == 0u) {
        firstFailure.classId = classId;
        firstFailure.chunkId = chunkId;
        firstFailure.offset = offset;
        firstFailure.compressedRead = compressedRead;
    }
}

void CGameCtnReplayStaticSolidArchiveDecodeProgress::RecordArchiveNode(
        u32 classId) {
    archiveNodeCount++;
    if (classId == TMNF_CLASS_CPlugTree ||
        classId == TMNF_CLASS_CPlugTreeVisualMip ||
        classId == TMNF_CLASS_CPlugTreeLight) {
        archiveTreeCount++;
    } else if (classId == TMNF_CLASS_CPlugSurface) {
        archiveSurfaceCount++;
    }
}

void CGameCtnReplayStaticSolidArchiveDecodeProgress::
RecordArchiveTreeChildLinks(u32 count) {
    archiveTreeChildLinkCount += count;
}

void CGameCtnReplayStaticSolidArchiveDecodeProgress::RecordArchiveMesh() {
    archiveMeshCount++;
}

void CGameCtnReplayStaticSolidArchiveDecodeProgress::RecordArchiveSurfaceGeom() {
    archiveSurfaceGeomCount++;
}

const CGameCtnReplayStaticSolidArchiveDecodeFailure &
CGameCtnReplayStaticSolidArchiveDecodeProgress::FirstFailure() const {
    return firstFailure;
}

void CGameCtnReplayStaticSolidArchiveDecodeProgress::CaptureStats(
        const CGameCtnReplayStaticSolidArchiveFeedback &feedback,
        CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut) const {
    if (statsOut == nullptr) {
        return;
    }
    statsOut->bodyOffsetParsed = bodyOffsetParsed;
    statsOut->rootFeedbackApplied = feedback.RootAppliedFlag();
    statsOut->nestedFeedbackApplied = feedback.NestedAppliedCount();
    statsOut->archiveNodeCount = archiveNodeCount;
    statsOut->archiveTreeCount = archiveTreeCount;
    statsOut->archiveSurfaceCount = archiveSurfaceCount;
    statsOut->archiveSurfaceGeomCount = archiveSurfaceGeomCount;
    statsOut->archiveMeshCount = archiveMeshCount;
    statsOut->archiveTreeChildLinkCount = archiveTreeChildLinkCount;
}
