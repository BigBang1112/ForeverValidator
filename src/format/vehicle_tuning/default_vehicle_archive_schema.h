#ifndef TMNF_DEFAULT_VEHICLE_ARCHIVE_SCHEMA_H
#define TMNF_DEFAULT_VEHICLE_ARCHIVE_SCHEMA_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
enum class CSceneVehicleStructArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneVehicleStruct,
    SimulationWheelBuffer = 0x0a039005u,
    VisualVehicleCount = 0x0a039006u,
    VisualArms = 0x0a039009u,
    VisualLights = 0x0a03900au,
    VisualWheels = 0x0a03900fu,
    VisualIds = 0x0a039010u,
    VehicleMaterialGroups = 0x0a039012u,
    FeedbackCurves = 0x0a039013u,
    VehicleEmitters = 0x0a039014u,
};

enum class CSceneVehicleMaterialGroupArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneVehicleMaterialGroup,
};

enum class CSceneVehicleEmitterArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneVehicleEmitter,
    Chunk002 = 0x0a010002u,
    Chunk003 = 0x0a010003u,
    Chunk004 = 0x0a010004u,
    Chunk005 = 0x0a010005u,
};

enum class CSceneVehicleMaterialArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneVehicleMaterial,
    FakeContactBitmapAndPeriod = 0x0a031004u,
    BlendableValues = 0x0a031005u,
    NaturalIdAndFakeContact = 0x0a03100eu,
    FeedbackResponse = 0x0a03100fu,
};

enum class CMwRefBufferArchiveChunkId : u32 {
    Root = TMNF_CLASS_CMwRefBuffer,
};

constexpr u32 ArchiveChunkIdValue(CSceneVehicleStructArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(
        CSceneVehicleMaterialGroupArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(
        CSceneVehicleEmitterArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CSceneVehicleMaterialArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CMwRefBufferArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

#endif
