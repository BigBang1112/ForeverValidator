#ifndef TMNF_STATIC_SOLID_ARCHIVE_TREE_CHUNK_IDS_H
#define TMNF_STATIC_SOLID_ARCHIVE_TREE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/tmnf_archive_ids.h"
enum class CPlugTreeLightArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugTreeLight,
    EmbeddedLightRefs = 0x09062001u,
    EmbeddedLightExtraRefA = 0x09062002u,
    EmbeddedLightExtraRefB = 0x09062003u,
    LightRef = 0x09062004u,
};

enum class CPlugTreeVisualMipArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugTreeVisualMip,
    LegacyMipFlag = 0x09015001u,
    ChildLinks = 0x09015002u,
};

enum class CPlugTreeArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugTree,
    LegacyNoPayload1 = 0x0904f001u,
    LegacyNoPayload2 = 0x0904f002u,
    LegacyNoPayload3 = 0x0904f003u,
    LegacyNoPayload4 = 0x0904f004u,
    LegacyNoPayload5 = 0x0904f005u,
    ChildBuffer = 0x0904f006u,
    LegacyNoPayload7 = 0x0904f007u,
    LegacyNoPayload8 = 0x0904f008u,
    LegacyNoPayload9 = 0x0904f009u,
    LegacyNoPayloadA = 0x0904f00au,
    LegacyNoPayloadB = 0x0904f00bu,
    LegacyIdListDiscard = 0x0904f00cu,
    TreeIdAndObject = 0x0904f00du,
    SourceSurfaceRefs = 0x0904f00eu,
    LegacyNoPayloadF = 0x0904f00fu,
    StateAndTransformA = 0x0904f010u,
    FuncTreeRef = 0x0904f011u,
    SourceSurfaceGeneratorRefs = 0x0904f012u,
    LegacyStateAndTransformB = 0x0904f013u,
    SourceMaterialSurfaceGeneratorRefs = 0x0904f014u,
    LegacyStateAndTransformC = 0x0904f015u,
    SourceSurfaceGeneratorRefs2 = 0x0904f016u,
    RefBufferDiscard = 0x0904f017u,
    StateAndTransformB = 0x0904f018u,
    StateAndTransformC = 0x0904f019u,
    StateAndTransformD = 0x0904f01au,
};

enum class CPlugTreeGeneratedArchiveChunkId : u32 {
    LocalBoxProviderRef = 0x09050000u,
    IsoBoolParentSolidRefs = 0x09050001u,
    IsoBoolOnly = 0x09050002u,
    SurfaceOnlySource = 0x09050003u,
};

constexpr u32 ArchiveChunkIdValue(CPlugTreeLightArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugTreeVisualMipArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugTreeArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugTreeGeneratedArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

inline u32 CPlugTreeCanonicalArchiveChunkId(u32 chunkId) {
    return TMNF_CPlugTreeCanonicalChunkIdFromArchiveWord(chunkId);
}

constexpr int IsCPlugTreeLightBaseArchiveChunk(u32 chunkId) {
    return chunkId >= TMNF_CLASS_CPlugTreeLight &&
           chunkId <= ArchiveChunkIdValue(CPlugTreeLightArchiveChunkId::EmbeddedLightExtraRefB);
}

constexpr int IsCPlugTreeVisualMipBaseArchiveChunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CPlugTreeVisualMip ||
           chunkId == ArchiveChunkIdValue(CPlugTreeVisualMipArchiveChunkId::LegacyMipFlag);
}

inline int IsCPlugTreeOptionalPayloadChunk(u32 chunkId) {
    chunkId = CPlugTreeCanonicalArchiveChunkId(chunkId);
    return (chunkId >= TMNF_CLASS_CPlugTree &&
            chunkId <= ArchiveChunkIdValue(CPlugTreeArchiveChunkId::LegacyStateAndTransformC)) ||
           (chunkId >= ArchiveChunkIdValue(CPlugTreeArchiveChunkId::RefBufferDiscard) &&
            chunkId <= ArchiveChunkIdValue(CPlugTreeArchiveChunkId::StateAndTransformC));
}

inline int IsCPlugTreeRequiredPayloadChunk(u32 chunkId) {
    chunkId = CPlugTreeCanonicalArchiveChunkId(chunkId);
    return chunkId == ArchiveChunkIdValue(CPlugTreeArchiveChunkId::LegacyNoPayload7) ||
           chunkId == ArchiveChunkIdValue(CPlugTreeArchiveChunkId::TreeIdAndObject) ||
           chunkId == ArchiveChunkIdValue(CPlugTreeArchiveChunkId::FuncTreeRef) ||
           chunkId == ArchiveChunkIdValue(CPlugTreeArchiveChunkId::SourceSurfaceGeneratorRefs2) ||
           chunkId == ArchiveChunkIdValue(CPlugTreeArchiveChunkId::StateAndTransformD) ||
           chunkId == ArchiveChunkIdValue(CPlugTreeGeneratedArchiveChunkId::LocalBoxProviderRef) ||
           chunkId == ArchiveChunkIdValue(CPlugTreeGeneratedArchiveChunkId::IsoBoolParentSolidRefs) ||
           chunkId == ArchiveChunkIdValue(CPlugTreeGeneratedArchiveChunkId::IsoBoolOnly) ||
           chunkId == ArchiveChunkIdValue(CPlugTreeGeneratedArchiveChunkId::SurfaceOnlySource);
}

inline int IsCPlugTreeIgnoredStandaloneChunk(u32 chunkId) {
    chunkId = CPlugTreeCanonicalArchiveChunkId(chunkId);
    return (chunkId >= TMNF_CLASS_CPlugTree &&
            chunkId <= ArchiveChunkIdValue(CPlugTreeArchiveChunkId::StateAndTransformD));
}

#endif
