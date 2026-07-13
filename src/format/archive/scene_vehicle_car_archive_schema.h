#ifndef TMNF_SCENE_VEHICLE_CAR_ARCHIVE_SCHEMA_H
#define TMNF_SCENE_VEHICLE_CAR_ARCHIVE_SCHEMA_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
enum class CSceneVehicleCarArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneVehicleCar,
    WheelIdsOld = 0x0a02b001u,
    MaterialRefsAndNaturalOld = 0x0a02b002u,
    TuningContainer = 0x0a02b003u,
    TuningContainerDiscardOld = 0x0a02b004u,
    TuningContainerAndVectorOld0 = 0x0a02b005u,
    ThreeNodeRefsOld = 0x0a02b006u,
    TuningContainerAndVectorOld1 = 0x0a02b007u,
    MaterialContainer = 0x0a02b008u,
    WheelVisualIdsOld = 0x0a02b009u,
    SpeedParamsOld = 0x0a02b00au,
    TwoMaterialRefsOld = 0x0a02b00bu,
    PhysicalParams = 0x0a02b00cu,
    EmptyOld0 = 0x0a02b00du,
    EmptyOld1 = 0x0a02b00eu,
    EmptyOld2 = 0x0a02b00fu,
    EmptyOld3 = 0x0a02b010u,
    EmptyOld4 = 0x0a02b011u,
    EmptyOld5 = 0x0a02b012u,
    VisualVehicleStruct = 0x0a02b013u,
    VehicleStructRef = 0x0a02b014u,
};

constexpr u32 ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCSceneVehicleCarArchiveChunk(u32 chunkId) {
    return chunkId >= TMNF_CLASS_CSceneVehicleCar &&
           chunkId <= ArchiveChunkIdValue(
                              CSceneVehicleCarArchiveChunkId::VehicleStructRef);
}

constexpr int IsCSceneVehicleCarInfo3Chunk(u32 chunkId) {
    return chunkId ==
           ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::PhysicalParams);
}

constexpr int IsCSceneVehicleCarInfo1Chunk(u32 chunkId) {
    return IsCSceneVehicleCarArchiveChunk(chunkId) &&
           !IsCSceneVehicleCarInfo3Chunk(chunkId);
}

#endif
