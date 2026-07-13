#include "format/pack/block_info_catalog/block_info_catalog_identity_reader.h"
#include <stdint.h>
#include <utility>

#include "format/archive/tmnf_archive_ids.h"
#include "format/archive/tmnf_gbx_body_reader.h"
int BlockInfoCatalogIdentity::IsUsableBlockInfoIdentity() const {
    return !identifier.empty() && !collection.empty();
}

int BlockInfoCatalogIdentityReader::ReadIdentifier(
        TmnfFormat::ClassicArchiveIdentifier parts[3]) {
    for (u32 i = 0u; i < 3u; i++) {
        if (!ReadCMwId(&parts[i])) {
            return 0;
        }
    }
    return 1;
}

int BlockInfoCatalogIdentityReader::
ParseLoadableNodRefAndIdChunk() {
    std::string loadableName;
    u32 hasLoadableRef = 0u;
    if (!reader.ReadString(loadableName) ||
        !reader.ReadU32(hasLoadableRef)) {
        return 0;
    }
    if (hasLoadableRef != 0u && !SkipBytes(4u)) {
        return 0;
    }
    TmnfFormat::ClassicArchiveIdentifier ignored;
    return ReadCMwId(&ignored);
}

int BlockInfoCatalogIdentityReader::ParseSGameCtnIdentifierChunk(
        BlockInfoCatalogIdentity *identityOut) {
    if (identityOut == nullptr) {
        return 0;
    }
    TmnfFormat::ClassicArchiveIdentifier parts[3];
    if (!ReadIdentifier(parts)) {
        return 0;
    }
    BlockInfoCatalogIdentity identity;
    identity.identifier = std::move(parts[0].text);
    identity.collection = std::move(parts[1].text);
    identity.author = std::move(parts[2].text);
    if (!identity.IsUsableBlockInfoIdentity()) {
        return 0;
    }
    *identityOut = identity;
    return 1;
}

int BlockInfoCatalogIdentityReader::Parse(
        const unsigned char *archiveBytes,
        u32 archiveByteCount,
        BlockInfoCatalogIdentity *identityOut) {
    if (identityOut == nullptr) {
        return 0;
    }
    *identityOut = BlockInfoCatalogIdentity{};
    u32 classId = 0u;
    u32 bodyOffset = 0u;
    if (!GbxBodyOffsetReader::TryParse(archiveBytes,
                                       archiveByteCount,
                                       &classId,
                                       &bodyOffset) ||
        bodyOffset >= archiveByteCount) {
        return 0;
    }

    if (!Open(archiveBytes, archiveByteCount) || !reader.Seek(bodyOffset)) {
        return 0;
    }
    while (reader.Remaining() >= 4u) {
        u32 chunkId = 0u;
        if (!reader.ReadU32(chunkId)) {
            return 0;
        }
        switch (chunkId) {
            case TMNF_CGameCtnCollectorChunk_0301A006:
                if (!SkipBytes(4u)) {
                    return 0;
                }
                break;
            case TMNF_CGameCtnCollectorChunk_0301A007:
                if (!SkipBytes(24u)) {
                    return 0;
                }
                break;
            case TMNF_CGameCtnCollectorChunk_LoadableNodRefAndId:
                if (!ParseLoadableNodRefAndIdChunk()) {
                    return 0;
                }
                break;
            case TMNF_CGameCtnCollectorChunk_CMwId0301A00A: {
                TmnfFormat::ClassicArchiveIdentifier ignored;
                if (!ReadCMwId(&ignored)) {
                    return 0;
                }
                break;
            }
            case TMNF_CGameCtnCollectorChunk_SGameCtnIdentifier:
                return ParseSGameCtnIdentifierChunk(identityOut);
            default:
                return 0;
        }
    }
    return 0;
}
