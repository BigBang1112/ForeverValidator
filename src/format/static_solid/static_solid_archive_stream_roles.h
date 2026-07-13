#ifndef TMNF_STATIC_SOLID_ARCHIVE_STREAM_ROLES_H
#define TMNF_STATIC_SOLID_ARCHIVE_STREAM_ROLES_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_node_reference.h"
class CGameCtnReplayStaticSolidArchiveFeedbackSink {
public:
    virtual ~CGameCtnReplayStaticSolidArchiveFeedbackSink() = default;

    virtual int ApplyFeedbackU32(u32 value, int nested) = 0;
};

class CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser {
public:
    virtual ~CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser() = default;

    virtual int ParseEmbeddedNodeArchive(u32 classId) = 0;
};

class CGameCtnReplayStaticSolidArchiveNodeParser {
public:
    virtual ~CGameCtnReplayStaticSolidArchiveNodeParser() = default;

    virtual int ParseNodeArchiveInternal(
            ArchiveNodeReference nodeRef,
            u32 classId,
            int nested,
            int applyFeedback) = 0;
};

#endif
