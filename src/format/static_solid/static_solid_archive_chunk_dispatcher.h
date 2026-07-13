#ifndef TMNF_STATIC_SOLID_ARCHIVE_CHUNK_DISPATCHER_H
#define TMNF_STATIC_SOLID_ARCHIVE_CHUNK_DISPATCHER_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_node_reference.h"
#include "format/static_solid/static_solid_archive_id.h"
struct CGameCtnReplayStaticSolidArchiveByteStream;
struct CGameCtnReplayStaticSolidArchiveCMwIdState;
struct CGameCtnReplayStaticSolidArchiveDecodeProgress;
struct CGameCtnReplayStaticSolidArchiveFeedback;
struct CGameCtnReplayStaticSolidArchiveNodeGraph;
struct StaticSolidArchiveVisualState;
struct CGameCtnReplayArchiveStaticModelCollection;
struct StaticSolidArchiveLoadSession;
struct SceneDescriptorFolderPaths;

class CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser;
class CGameCtnReplayStaticSolidArchiveFeedbackSink;
class CGameCtnReplayStaticSolidArchiveNodeParser;
class CGameCtnReplayStaticSolidArchiveNodeRefReader;

struct CGameCtnReplayStaticSolidArchiveChunkDispatchContext {
    CGameCtnReplayStaticSolidArchiveByteStream *byteStream;
    CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState;
    CGameCtnReplayStaticSolidArchiveFeedback *feedback;
    CGameCtnReplayStaticSolidArchiveDecodeProgress *decodeProgress;
    CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph;
    StaticSolidArchiveVisualState *visualProviderState;
    SceneDescriptorFolderPaths *externalFolders;
    StaticSolidArchiveLoadSession *materialStore;
    CGameCtnReplayArchiveStaticModelCollection *archiveModels;
    StaticSolidArchiveId payload;
    ArchiveNodeReference currentArchiveNode;
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs;
    CGameCtnReplayStaticSolidArchiveFeedbackSink *feedbackSink;
    CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodeParser;
    CGameCtnReplayStaticSolidArchiveNodeParser *nodeParser;
};

class CGameCtnReplayStaticSolidArchiveChunkDispatcher {
public:
    static int ParseChunk(
            const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
            u32 classId,
            u32 chunkId);
};

#endif
