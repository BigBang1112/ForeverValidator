#pragma once

#include "engine/core/engine_types.h"
#include <string>

#include "format/archive/classic_archive_reader.h"
struct BlockInfoCatalogIdentity {
    std::string identifier;
    std::string collection;
    std::string author;

    int IsUsableBlockInfoIdentity() const;
};

class BlockInfoCatalogIdentityReader {
public:
    int Parse(const unsigned char *archiveBytes,
              u32 archiveByteCount,
              BlockInfoCatalogIdentity *identityOut);

private:
    static constexpr u32 CmwIdTableCapacity = 512u;

    TmnfFormat::ClassicArchiveReader reader;

    int Open(const unsigned char *archiveBytes, u32 archiveByteCount) {
        return reader.Open(archiveBytes,
                           archiveByteCount,
                           CmwIdTableCapacity);
    }
    int SkipBytes(u32 count) {
        return reader.Skip(count);
    }
    int ReadCMwId(TmnfFormat::ClassicArchiveIdentifier *out) {
        return out != nullptr && reader.ReadIdentifier(*out);
    }
    int ReadIdentifier(TmnfFormat::ClassicArchiveIdentifier parts[3]);
    int ParseLoadableNodRefAndIdChunk();
    int ParseSGameCtnIdentifierChunk(
            BlockInfoCatalogIdentity *identityOut);
};
