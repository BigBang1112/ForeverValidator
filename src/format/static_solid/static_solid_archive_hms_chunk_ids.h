#ifndef TMNF_STATIC_SOLID_ARCHIVE_HMS_CHUNK_IDS_H
#define TMNF_STATIC_SOLID_ARCHIVE_HMS_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
enum class CHmsZoneArchiveChunkId : u32 {
    Root = TMNF_CLASS_CHmsZone,
    EmptyOld = 0x06004001u,
    MaterialsAndFog = 0x06004002u,
    ReflectGround = 0x06004003u,
    NodRef = 0x06004004u,
    MaterialBuffer = 0x06004005u,
    RefBuffer = 0x06004006u,
};

enum class CHmsItemArchiveChunkId : u32 {
    Root = TMNF_CLASS_CHmsItem,
    Solid = 0x06003001u,
    CorpusArray = 0x06003002u,
    PhysicsFlags20 = 0x06003003u,
    PhysicsFlags17 = 0x06003004u,
    PhysicsFlags21 = 0x06003005u,
    PhysicsFlags25A = 0x06003006u,
    PhysicsFlags25B = 0x06003007u,
    Flags12 = 0x06003008u,
    Flags4 = 0x06003009u,
    PhysicsFlagsLegacy = 0x0600300au,
    PhysicsFlagsAndNat16A = 0x0600300bu,
    PhysicsFlagsAndNat16B = 0x0600300cu,
    PhysicsFlagsAndNat16C = 0x0600300du,
    PhysicsFlagsAndNat16D = 0x0600300eu,
    PhysicsFlagsAndNat16E = 0x0600300fu,
    PhysicsFlagsAndNat16F = 0x06003010u,
    PhysicsFlagsAndNat16G = 0x06003011u,
};

enum class CHmsLightArchiveChunkId : u32 {
    Root = TMNF_CLASS_CHmsLight,
    GxLight = 0x0600c001u,
    ProjectorBitmap = 0x0600c002u,
    RefBuffer = 0x0600c003u,
};

enum class CHmsSoundSourceArchiveChunkId : u32 {
    Root = TMNF_CLASS_CHmsSoundSource,
    SoundRef = 0x0600d001u,
    VolumeAndPitch = 0x0600d002u,
    VolumePitchAndPan = 0x0600d003u,
    SoundRefIntegerBool = 0x0600d004u,
    SoundRefRealBool = 0x0600d005u,
    PocEmitter = TMNF_CLASS_CHmsPocEmitter,
};

constexpr u32 ArchiveChunkIdValue(CHmsZoneArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CHmsItemArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CHmsLightArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCHmsZoneArchiveClass(u32 classId) {
    return classId == TMNF_CLASS_CHmsZone ||
           classId == TMNF_CLASS_CHmsZoneDynamic;
}

constexpr int IsCHmsZoneInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CHmsZoneArchiveChunkId::Root) ||
           chunkId == ArchiveChunkIdValue(CHmsZoneArchiveChunkId::EmptyOld) ||
           chunkId == ArchiveChunkIdValue(CHmsZoneArchiveChunkId::NodRef);
}

constexpr int IsCHmsZoneInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CHmsZoneArchiveChunkId::MaterialsAndFog) ||
           chunkId == ArchiveChunkIdValue(CHmsZoneArchiveChunkId::ReflectGround) ||
           chunkId == ArchiveChunkIdValue(CHmsZoneArchiveChunkId::MaterialBuffer) ||
           chunkId == ArchiveChunkIdValue(CHmsZoneArchiveChunkId::RefBuffer);
}

constexpr int IsCHmsItemInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::Root) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::CorpusArray) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags20) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags17) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags21) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags25A) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlags25B) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::Flags12) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::Flags4) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsLegacy) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16A) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16B) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16C) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16D) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16E) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16F);
}

constexpr int IsCHmsItemInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::Solid) ||
           chunkId == ArchiveChunkIdValue(CHmsItemArchiveChunkId::PhysicsFlagsAndNat16G);
}

constexpr int IsCHmsLightInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CHmsLightArchiveChunkId::Root) ||
           chunkId == ArchiveChunkIdValue(CHmsLightArchiveChunkId::GxLight) ||
           chunkId == ArchiveChunkIdValue(CHmsLightArchiveChunkId::ProjectorBitmap);
}

constexpr int IsCHmsLightInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CHmsLightArchiveChunkId::RefBuffer);
}

constexpr int IsCHmsSoundSourceInfo1Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::Root) ||
           chunkId == ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::SoundRef) ||
           chunkId == ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::VolumeAndPitch);
}

constexpr int IsCHmsSoundSourceInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::VolumePitchAndPan) ||
           chunkId == ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::SoundRefIntegerBool) ||
           chunkId == ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::SoundRefRealBool) ||
           chunkId == ArchiveChunkIdValue(CHmsSoundSourceArchiveChunkId::PocEmitter);
}

#endif
