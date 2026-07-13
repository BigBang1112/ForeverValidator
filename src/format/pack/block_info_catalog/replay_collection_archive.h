#ifndef TMNF_REPLAY_COLLECTION_ARCHIVE_H
#define TMNF_REPLAY_COLLECTION_ARCHIVE_H

#include "engine/core/engine_types.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include "format/pack/installed/byte_buffer.h"
struct CPlugFilePack;

class CatalogCollectionArchive {
public:
    int Load(const CPlugFilePack *installedPack, const char *collectionName);
    const unsigned char *Bytes() const;
    u32 ByteCount() const;
    const BlockInfoDescriptorExternalRefs &ExternalRefs() const;

private:
    ByteBuffer bytes;
    BlockInfoDescriptorExternalRefs refs;
    char collectionPath[256]{};
};

#endif
