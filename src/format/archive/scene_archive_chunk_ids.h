#ifndef TMNF_SCENE_ARCHIVE_CHUNK_IDS_H
#define TMNF_SCENE_ARCHIVE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
enum class CSceneArchiveChunkId : u32 {
    Root = TMNF_CLASS_CScene,
    EmptyOld0 = 0x0a001001u,
    OldSerializeMobils0 = 0x0a001002u,
    RefBuffer = 0x0a001003u,
    FormatArray = 0x0a001004u,
    MotionManagers = 0x0a001005u,
    OldSerializeMobils1 = 0x0a001006u,
};

enum class CScene3dArchiveChunkId : u32 {
    Root = TMNF_CLASS_CScene3d,
    SectorBuffer = 0x0a003001u,
    EmptyOld0 = 0x0a003002u,
    EmptyOld1 = 0x0a003003u,
    SectorLocBuffer = 0x0a003004u,
    PathBuffer = 0x0a003005u,
    EmptyOld2 = 0x0a003006u,
    DefaultSectorIso = 0x0a003007u,
    FieldBuffer = 0x0a003008u,
    ObjectLocBuffer = 0x0a003009u,
    EmptyOld3 = 0x0a00300au,
    OldMood = 0x0a00300bu,
    NaturalField = 0x0a00300cu,
    EmptyOld4 = 0x0a00300du,
    TrafficPairLocs = 0x0a00300eu,
    TrafficGraphPaths = 0x0a00300fu,
    Fog = 0x0a003010u,
    NodRef = 0x0a003011u,
    MoodBuffer = 0x0a003012u,
    NodRefBuffer = 0x0a003013u,
    FrustumIso = 0x0a003014u,
    SingleNodRef = 0x0a003015u,
    ObjectBuffersV1 = 0x0a003016u,
    ObjectBuffersV2 = 0x0a003017u,
    ObjectBuffersV3 = 0x0a003018u,
};

constexpr u32 ArchiveChunkIdValue(CSceneArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CScene3dArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCSceneInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CSceneArchiveChunkId::Root) ||
           chunkId == ArchiveChunkIdValue(CSceneArchiveChunkId::EmptyOld0) ||
           chunkId == ArchiveChunkIdValue(CSceneArchiveChunkId::OldSerializeMobils0) ||
           chunkId == ArchiveChunkIdValue(CSceneArchiveChunkId::OldSerializeMobils1);
}

constexpr int IsCSceneInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CSceneArchiveChunkId::RefBuffer) ||
           chunkId == ArchiveChunkIdValue(CSceneArchiveChunkId::FormatArray) ||
           chunkId == ArchiveChunkIdValue(CSceneArchiveChunkId::MotionManagers);
}

constexpr int IsCScene3dInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::Root) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::SectorBuffer) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld0) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld1) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::SectorLocBuffer) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::PathBuffer) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld2) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::DefaultSectorIso) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::FieldBuffer) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectLocBuffer) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld3) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::OldMood) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld4) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::TrafficPairLocs) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::TrafficGraphPaths) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::NodRef) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::MoodBuffer) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::NodRefBuffer) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::SingleNodRef) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV1) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV2);
}

constexpr int IsCScene3dInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::NaturalField) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::Fog) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::FrustumIso) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV3);
}

constexpr int IsCScene3dObjectBufferChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV1) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV2) ||
           chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV3);
}

constexpr u32 CScene3dObjectBufferArchiveMode(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV1)
            ? 1u
            : (chunkId == ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV2)
                       ? 2u
                       : 3u);
}

#endif
