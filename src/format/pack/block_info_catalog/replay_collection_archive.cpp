#include "format/pack/block_info_catalog/replay_collection_archive.h"
#include <stdio.h>

#include "format/pack/installed/plug_file_pack.h"
int CatalogCollectionArchive::Load(
        const CPlugFilePack *installedPack,
        const char *collectionName) {
    if (installedPack == nullptr ||
        collectionName == nullptr ||
        collectionName[0] == '\0') {
        return 0;
    }
    if (snprintf(collectionPath,
                 sizeof(collectionPath),
                 "Collections\\%s.TMCollection.Gbx",
                 collectionName) >= (int)sizeof(collectionPath)) {
        return 0;
    }
    if (!installedPack->ExtractPath(collectionPath, &bytes) ||
        bytes.Empty() || bytes.Size() > UINT32_MAX) {
        return 0;
    }
    return refs.ParseGbx(bytes.Data(), static_cast<u32>(bytes.Size()));
}

const unsigned char *CatalogCollectionArchive::Bytes() const {
    return bytes.Data();
}

u32 CatalogCollectionArchive::ByteCount() const {
    return static_cast<u32>(bytes.Size());
}

const BlockInfoDescriptorExternalRefs &
CatalogCollectionArchive::ExternalRefs() const {
    return refs;
}
