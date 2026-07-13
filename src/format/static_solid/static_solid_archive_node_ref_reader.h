#ifndef TMNF_STATIC_SOLID_ARCHIVE_NODE_REF_READER_H
#define TMNF_STATIC_SOLID_ARCHIVE_NODE_REF_READER_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_node_reference.h"
class CGameCtnReplayStaticSolidArchiveNodeRefReader {
public:
    virtual ~CGameCtnReplayStaticSolidArchiveNodeRefReader() = default;

    virtual int ReadNodeRef(
            ArchiveNodeReference *nodeRefOut) = 0;

    int ReadNodPtr(u32 *nodeIndexOut) {
        ArchiveNodeReference nodeRef =
                ArchiveNodeReference::Invalid();
        if (!ReadNodeRef(nodeIndexOut != nullptr ? &nodeRef : nullptr)) {
            return 0;
        }
        if (nodeIndexOut != nullptr) {
            *nodeIndexOut = nodeRef.Index();
        }
        return 1;
    }
};

#endif
