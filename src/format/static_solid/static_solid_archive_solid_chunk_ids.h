#ifndef TMNF_STATIC_SOLID_ARCHIVE_SOLID_CHUNK_IDS_H
#define TMNF_STATIC_SOLID_ARCHIVE_SOLID_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/tmnf_archive_ids.h"
enum class CPlugSolidArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugSolid,
    LegacyVirtualParam1 = 0x09005001u,
    LegacyVirtualParam2 = 0x09005002u,
    LegacyVirtualParam3 = 0x09005003u,
    LegacyVirtualParam4 = 0x09005004u,
    LegacyVirtualParam5 = 0x09005005u,
    PhysicalFactsLegacy = 0x09005006u,
    LegacyBool = 0x09005007u,
    LegacyEmpty8 = 0x09005008u,
    LegacyEmpty9 = 0x09005009u,
    TreeLinkWithMode = 0x0900500au,
    BoolFlagsRealNatural = 0x0900500bu,
    ExtendedBoolFlagsAndReals = 0x0900500cu,
    TreeLink = 0x0900500du,
    PhysicalFacts = 0x0900500eu,
    TwoReals = 0x0900500fu,
    RefBufferDiscard = 0x09005010u,
    TreeLinkWithFidModel = 0x09005011u,
    PackedUseModelFlags = 0x09005012u,
};

constexpr u32 ArchiveChunkIdValue(CPlugSolidArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

inline u32 CPlugSolidCanonicalArchiveChunkId(u32 chunkId) {
    return TMNF_CPlugSolidCanonicalChunkIdFromArchiveWord(chunkId);
}

inline int IsCPlugSolidRequiredPayloadChunk(u32 chunkId) {
    chunkId = CPlugSolidCanonicalArchiveChunkId(chunkId);
    return chunkId == TMNF_CLASS_CPlugSolid ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::PhysicalFacts) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::RefBufferDiscard) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::TreeLinkWithFidModel) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::PackedUseModelFlags);
}

inline int IsCPlugSolidOptionalPayloadChunk(u32 chunkId) {
    chunkId = CPlugSolidCanonicalArchiveChunkId(chunkId);
    return chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam1) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam2) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam3) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam4) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyVirtualParam5) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::PhysicalFactsLegacy) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyBool) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyEmpty8) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::LegacyEmpty9) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::TreeLinkWithMode) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::BoolFlagsRealNatural) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::ExtendedBoolFlagsAndReals) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::TreeLink) ||
           chunkId == ArchiveChunkIdValue(CPlugSolidArchiveChunkId::TwoReals);
}

#endif
