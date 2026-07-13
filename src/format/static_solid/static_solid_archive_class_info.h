#ifndef TMNF_STATIC_SOLID_ARCHIVE_CLASS_INFO_H
#define TMNF_STATIC_SOLID_ARCHIVE_CLASS_INFO_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
struct CGameCtnReplayStaticSolidArchiveClassInfo {
    static int IsUnknownClassId(u32 classId);
    static int IsZipFileClass(u32 classId);
    static int IsBlockInfoClass(u32 classId);
    static u32 FeedbackClassId(u32 classId);
    static int IsKnownNodeClass(u32 classId);
    static int WordIsSceneObjectLinkChunk(u32 word);
    static u32 ChunkInfo(u32 classId, u32 chunkId);
};

#endif
