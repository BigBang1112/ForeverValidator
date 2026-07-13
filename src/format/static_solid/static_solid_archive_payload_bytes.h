#ifndef TMNF_STATIC_SOLID_ARCHIVE_PAYLOAD_BYTES_H
#define TMNF_STATIC_SOLID_ARCHIVE_PAYLOAD_BYTES_H

#include "format/static_solid/static_scene_archive_loader.h"
class CGameCtnReplayStaticSolidRawPayloadDecoder {
public:
    static int Decode(
            const SNat128 &key,
            const StaticSolidArchivePayload &payloadAsset,
            StaticSolidArchiveRawBytes rawPayload,
            CGameCtnReplayStaticSolidDecodedPayload *decodedOut);
};

#endif
