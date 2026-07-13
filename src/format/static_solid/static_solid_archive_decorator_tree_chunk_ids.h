#ifndef TMNF_STATIC_SOLID_ARCHIVE_DECORATOR_TREE_CHUNK_IDS_H
#define TMNF_STATIC_SOLID_ARCHIVE_DECORATOR_TREE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/tmnf_archive_ids.h"
enum class CPlugDecoratorTreeArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugDecoratorTree,
    BoolOnlySurfaceFromVisual = 0x090a2001u,
    LegacyMaterialVisualSurfaceFlagA = 0x090a2002u,
    LegacyMaterialVisualSurfaceFlagB = 0x090a2003u,
    LegacyPreMaterialWordSurfaceFlagA = 0x090a2004u,
    LegacyPreMaterialWordSurfaceFlagB = 0x090a2005u,
    ConditionalRow = 0x090a2006u,
    ConditionalRowFull = 0x090a2007u,
    ConditionalRowWithIdentityGate = 0x090a2008u,
    ConditionalRowWithCollisionGate = 0x090a2009u,
};

constexpr u32 ArchiveChunkIdValue(CPlugDecoratorTreeArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCPlugDecoratorTreeArchiveChunk(u32 chunkId) {
    return chunkId >= TMNF_CLASS_CPlugDecoratorTree &&
           chunkId <= ArchiveChunkIdValue(
                   CPlugDecoratorTreeArchiveChunkId::ConditionalRowWithCollisionGate);
}

constexpr int CPlugDecoratorTreeChunkHasPreMaterialWord(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CPlugDecoratorTreeArchiveChunkId::
                                      LegacyPreMaterialWordSurfaceFlagA) ||
           chunkId == ArchiveChunkIdValue(
                              CPlugDecoratorTreeArchiveChunkId::
                                      LegacyPreMaterialWordSurfaceFlagB);
}

constexpr int CPlugDecoratorTreeChunkUsesLegacyVisibilityTail(u32 chunkId) {
    return chunkId <= ArchiveChunkIdValue(
                              CPlugDecoratorTreeArchiveChunkId::
                                      LegacyPreMaterialWordSurfaceFlagB);
}

constexpr int CPlugDecoratorTreeChunkHasIdentityGate(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                   CPlugDecoratorTreeArchiveChunkId::ConditionalRowWithIdentityGate) ||
           chunkId == ArchiveChunkIdValue(
                   CPlugDecoratorTreeArchiveChunkId::ConditionalRowWithCollisionGate);
}

constexpr int CPlugDecoratorTreeChunkHasCollisionGate(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                   CPlugDecoratorTreeArchiveChunkId::ConditionalRowWithCollisionGate);
}

#endif
