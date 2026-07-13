#ifndef TMNF_DEFAULT_VEHICLE_ARCHIVE_SCHEMA_H
#define TMNF_DEFAULT_VEHICLE_ARCHIVE_SCHEMA_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
constexpr u32 TMNF_CLASS_CSceneVehicleStruct = 0x0a039000u;
constexpr u32 TMNF_CLASS_CSceneVehicleMaterial = 0x0a031000u;
constexpr u32 TMNF_CLASS_CMwRefBuffer = 0x01026000u;

enum class CSceneVehicleStructArchiveChunkId : u32 {
    Root = TMNF_CLASS_CSceneVehicleStruct,
    SimulationWheelBuffer = 0x0a039005u,
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

constexpr u32 ArchiveChunkIdValue(CSceneVehicleMaterialArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CMwRefBufferArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

#endif
