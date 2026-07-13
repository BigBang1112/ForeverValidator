#ifndef TMNF_SCENE_OBJECT_ARCHIVE_CHUNK_IDS_H
#define TMNF_SCENE_OBJECT_ARCHIVE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
enum class CSceneObjectArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneObject,
    Id = 0x0a005001u,
    BoolFlag = 0x0a005002u,
    MotionRef = 0x0a005003u,
    Natural = 0x0a005004u,
};

enum class CSceneMobilArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneMobil,
    EmptyOld0 = 0x0a011001u,
    EmptyOld1 = 0x0a011002u,
    ChildObjects = 0x0a011003u,
    BoolMarker = 0x0a011004u,
    HmsItemData = 0x0a011005u,
    MessageHandlerRef = 0x0a011006u,
};

constexpr u32 ArchiveChunkIdValue(CSceneObjectArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CSceneMobilArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCSceneObjectInfo1Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CMwNod ||
           chunkId == TMNF_CLASS_CSceneObject ||
           chunkId == ArchiveChunkIdValue(CSceneObjectArchiveChunkId::BoolFlag) ||
           chunkId == ArchiveChunkIdValue(CSceneObjectArchiveChunkId::Natural);
}

constexpr int IsCSceneObjectInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CSceneObjectArchiveChunkId::Id) ||
           chunkId == ArchiveChunkIdValue(CSceneObjectArchiveChunkId::MotionRef);
}

constexpr int IsCSceneMobilInfo1Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CSceneMobil ||
           chunkId == ArchiveChunkIdValue(CSceneMobilArchiveChunkId::EmptyOld0) ||
           chunkId == ArchiveChunkIdValue(CSceneMobilArchiveChunkId::EmptyOld1) ||
           chunkId == ArchiveChunkIdValue(CSceneMobilArchiveChunkId::BoolMarker);
}

constexpr int IsCSceneMobilInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CSceneMobilArchiveChunkId::ChildObjects) ||
           chunkId == ArchiveChunkIdValue(CSceneMobilArchiveChunkId::HmsItemData) ||
           chunkId == ArchiveChunkIdValue(CSceneMobilArchiveChunkId::MessageHandlerRef);
}

constexpr int IsCSceneObjectDescriptorMobilTailChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CSceneObjectArchiveChunkId::Id) ||
           chunkId == ArchiveChunkIdValue(CSceneObjectArchiveChunkId::MotionRef) ||
           IsCSceneMobilInfo3Chunk(chunkId);
}

constexpr u32 CSceneObjectPackedChunkPrefix = 0x0a000000u;
constexpr u32 CSceneMobilPackedChunkPrefix = 0x0a010000u;
constexpr u32 CSceneObjectPackedChunkFirstSelector = 0x5000u;
constexpr u32 CSceneObjectPackedChunkLastSelector = 0x5004u;
constexpr u32 CSceneMobilPackedChunkFirstSelector = 0x1000u;
constexpr u32 CSceneMobilPackedChunkLastSelector = 0x1006u;

constexpr int IsPackedCSceneObjectChunkSelector(u32 lowWord) {
    return lowWord >= CSceneObjectPackedChunkFirstSelector &&
           lowWord <= CSceneObjectPackedChunkLastSelector;
}

constexpr int IsPackedCSceneMobilChunkSelector(u32 lowWord) {
    return lowWord >= CSceneMobilPackedChunkFirstSelector &&
           lowWord <= CSceneMobilPackedChunkLastSelector;
}

constexpr u32 NormalizePackedCSceneObjectOrMobilArchiveChunkId(u32 rawChunkId) {
    const u32 lowWord = rawChunkId & 0xffffu;
    if ((rawChunkId & 0xffff0000u) != CSceneObjectPackedChunkPrefix &&
        IsPackedCSceneObjectChunkSelector(lowWord)) {
        return CSceneObjectPackedChunkPrefix | lowWord;
    }
    if ((rawChunkId & 0xffff0000u) != CSceneMobilPackedChunkPrefix &&
        IsPackedCSceneMobilChunkSelector(lowWord)) {
        return CSceneMobilPackedChunkPrefix | lowWord;
    }
    return rawChunkId;
}

#endif
