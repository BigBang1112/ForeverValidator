#ifndef TMNF_STATIC_SOLID_ARCHIVE_IDENTITY_H
#define TMNF_STATIC_SOLID_ARCHIVE_IDENTITY_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_node_reference.h"
#include "format/static_solid/static_solid_archive_id.h"
class CGameCtnReplayStaticSolidArchiveNodeIdentity {
public:
    CGameCtnReplayStaticSolidArchiveNodeIdentity()
            : payload(StaticSolidArchiveId::Invalid()),
              archiveNode(ArchiveNodeReference::
                                  Invalid()) {
    }

    static CGameCtnReplayStaticSolidArchiveNodeIdentity Invalid() {
        return CGameCtnReplayStaticSolidArchiveNodeIdentity();
    }

    static CGameCtnReplayStaticSolidArchiveNodeIdentity FromPayloadAndArchiveIndex(
            StaticSolidArchiveId payload,
            u32 archiveIndex) {
        return CGameCtnReplayStaticSolidArchiveNodeIdentity(payload,
                ArchiveNodeReference::FromIndex(archiveIndex));
    }

    static CGameCtnReplayStaticSolidArchiveNodeIdentity FromPayloadAndLocalNode(
            StaticSolidArchiveId payload,
            ArchiveNodeReference node) {
        return CGameCtnReplayStaticSolidArchiveNodeIdentity(payload, node);
    }

    static CGameCtnReplayStaticSolidArchiveNodeIdentity FromIndexAndArchiveIndex(
            u32 archiveId,
            u32 archiveIndex) {
        return FromPayloadAndArchiveIndex(
                StaticSolidArchiveId::
                        FromIndex(archiveId),
                archiveIndex);
    }

    int IsValid() const {
        return payload.IsValid() && archiveNode.IsValid();
    }

    StaticSolidArchiveId Payload() const {
        return payload;
    }

    ArchiveNodeReference ArchiveNode() const {
        return archiveNode;
    }

    u32 ArchiveIndex() const {
        return archiveNode.Index();
    }

    u32 ArchiveId() const {
        return payload.Index();
    }

    int MatchesArchiveIdAndNodeIndex(u32 archiveId,
                                     u32 queryArchiveIndex) const {
        return payload.MatchesIndex(archiveId) &&
               ArchiveIndex() == queryArchiveIndex;
    }

    int MatchesPayload(StaticSolidArchiveId queryPayload)
            const {
        return payload.IsValid() && queryPayload.Matches(payload);
    }

    int MatchesIndex(u32 archiveId) const {
        return payload.MatchesIndex(archiveId);
    }

    int Matches(CGameCtnReplayStaticSolidArchiveNodeIdentity other) const {
        return other.MatchesArchiveIdAndNodeIndex(ArchiveId(), ArchiveIndex());
    }

private:
    CGameCtnReplayStaticSolidArchiveNodeIdentity(
            StaticSolidArchiveId payload,
            ArchiveNodeReference archiveNode)
            : payload(payload),
              archiveNode(archiveNode) {
    }

    StaticSolidArchiveId payload;
    ArchiveNodeReference archiveNode;
};

#endif
