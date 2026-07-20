#ifndef TMNF_STATIC_SOLID_ARCHIVE_PAYLOAD_STREAM_H
#define TMNF_STATIC_SOLID_ARCHIVE_PAYLOAD_STREAM_H

#include "engine/core/engine_types.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_id.h"
class CGameCtnReplayStaticSolidArchivePayloadReader {
public:
    static int DecodeWithStreamFeedback(
            const SNat128 &key,
            const StaticSolidArchivePayload &payloadAsset,
            StaticSolidArchiveRawBytes rawPayload,
            StaticSolidArchiveLoadSession *materialStore,
            CGameCtnReplayArchiveStaticModelCollection *archiveModels,
            StaticSolidArchiveId payload,
            CGameCtnReplayStaticSolidDecodedPayload *decodedOut,
            CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut);

    static int DecodeReferenceTablePrefixWithStreamFeedback(
            const SNat128 &key,
            const StaticSolidArchivePayload &payloadAsset,
            StaticSolidArchiveRawBytes rawPayload,
            CGameCtnReplayStaticSolidDecodedPayload *decodedOut);

    static int EnqueueExternalRefDependencies(
            const StaticSolidArchivePayload &payloadAsset,
            const unsigned char *decodedBytes,
            u32 decodedByteCount,
            StaticSolidArchiveLoadSession *store,
            CGameCtnReplayStaticSolidDescriptorDependencyQueue *dependencyQueue);

    static int ParseDecodedIntoStore(
            const StaticSolidArchivePayload &payloadAsset,
            const unsigned char *decodedBytes,
            u32 decodedByteCount,
            StaticSolidArchiveLoadSession *materialStore,
            CGameCtnReplayArchiveStaticModelCollection *archiveModels,
            StaticSolidArchiveId payload,
            CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut);
};

#endif
