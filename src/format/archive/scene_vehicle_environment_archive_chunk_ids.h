#ifndef TMNF_SCENE_VEHICLE_ENVIRONMENT_ARCHIVE_CHUNK_IDS_H
#define TMNF_SCENE_VEHICLE_ENVIRONMENT_ARCHIVE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
enum class CSceneVehicleEnvironmentArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneVehicleEnvironment,
    MaterialRef = 0x0a033001u,
    MaterialRefArray = 0x0a033002u,
};

constexpr u32 ArchiveChunkIdValue(CSceneVehicleEnvironmentArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCSceneVehicleEnvironmentInfo3Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CSceneVehicleEnvironment ||
           chunkId == ArchiveChunkIdValue(
                              CSceneVehicleEnvironmentArchiveChunkId::MaterialRefArray);
}

constexpr int IsCSceneVehicleEnvironmentInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CSceneVehicleEnvironmentArchiveChunkId::MaterialRef);
}

#endif
