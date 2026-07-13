#ifndef TMNF_STATIC_SOLID_ARCHIVE_ANIMATION_MOTION_READER_H
#define TMNF_STATIC_SOLID_ARCHIVE_ANIMATION_MOTION_READER_H


#include "engine/core/engine_types.h"
#include "format/static_solid/static_solid_archive_node_ref_reader.h"
#include "format/static_solid/static_solid_archive_stream_roles.h"
struct CGameCtnReplayStaticSolidArchiveByteStream;
struct CGameCtnReplayStaticSolidArchiveCMwIdState;

struct CAnimationMotionDiscardedRefsPayload {
    static int ReadSingle(
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs);
    static int ReadCountedArray(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs);
};

struct CFuncKeysNaturalArchivePayload {
    explicit CFuncKeysNaturalArchivePayload(
            CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState);

    int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
             u32 chunkId);

private:
    CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState_;
};

struct CFuncTreeSubVisualSequenceArchivePayload {
    CFuncTreeSubVisualSequenceArchivePayload(
            CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
            CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodes);

    int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
             u32 chunkId);

private:
    CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState_;
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs_;
    CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodes_;
};

struct CMotionPlayerArchivePayload {
    CMotionPlayerArchivePayload(
            CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
            CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodes);

    int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
             u32 chunkId);

private:
    int ReadOptionalRef(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream);
    int ReadBaseCommandAndRefArray(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
            int hasTrackRef,
            int hasStateBool);

    CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState_;
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs_;
    CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodes_;
};

struct CMotionDayTimeArchivePayload {
    CMotionDayTimeArchivePayload(
            CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs);

    int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
             u32 chunkId);

private:
    CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState_;
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs_;
};

struct CMotionCmdBaseArchivePayload {
    int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
             u32 chunkId);
};

struct CMotionShaderArchivePayload {
    explicit CMotionShaderArchivePayload(
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs);

    int Read(u32 chunkId);

private:
    CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs_;
};

struct CGameCtnReplayStaticSolidArchiveAnimationMotionReader {
    static int ParseAnimationMotionChunk(
            CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
            CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
            u32 classId,
            u32 chunkId,
            CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
            CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser *embeddedNodes);
};

#endif
