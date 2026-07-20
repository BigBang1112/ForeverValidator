#ifndef TMNF_STATIC_SOLID_ARCHIVE_VISUAL_SURFACE_CHUNK_IDS_H
#define TMNF_STATIC_SOLID_ARCHIVE_VISUAL_SURFACE_CHUNK_IDS_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/tmnf_archive_ids.h"
enum class GxLightArchiveChunkId : u32 {
    Root = TMNF_CLASS_GxLight,
    LegacyBaseState1 = 0x04001001u,
    LegacyBaseState2 = 0x04001002u,
    LegacyBaseState3 = 0x04001003u,
    LegacyBaseState4 = 0x04001004u,
    LegacyBaseState5 = 0x04001005u,
    LegacyBaseState6 = 0x04001006u,
    LegacyBaseState7 = 0x04001007u,
    LegacyBaseState8 = 0x04001008u,
    FullLightState = 0x04001009u,
};

enum class GxLightDirectionalArchiveChunkId : u32 {
    Root = TMNF_CLASS_GxLightDirectional,
    EnabledVector = 0x04007001u,
    EnabledVectorAndAngles = 0x04007002u,
    DirectionVector = 0x04007003u,
    DirectionColor = 0x04007004u,
    AngularLimits = 0x04007005u,
};

enum class CPlugVisualArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugVisual,
    Identifier = 0x09006001u,
    LegacyEmpty2 = 0x09006002u,
    GeometryHeader = 0x09006003u,
    FuncVisualRef = 0x09006004u,
    Vec3Array = 0x09006005u,
    LegacyWord6 = 0x09006006u,
    LegacyEmpty7 = 0x09006007u,
    LegacyEmpty8 = 0x09006008u,
    LegacyWord9 = 0x09006009u,
    LegacyEmptyA = 0x0900600au,
    Record32Array = 0x0900600bu,
    LegacyEmptyC = 0x0900600cu,
    LegacyEmptyD = 0x0900600du,
    VisualGeometryProvider = 0x0900600eu,
};

enum class CPlugVisual3DArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugVisual3D,
    VertexStream = 0x0902c001u,
    MaterialRef = 0x0902c002u,
    VertexStreamAndTangents = 0x0902c003u,
    FaceStream = 0x0902c004u,
};

enum class CPlugVisualSpriteArchiveChunkId : u32 {
    SpriteParameters = 0x09010005u,
    AtlasGrid = 0x09010006u,
};

enum class CPlugVisualGridArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugVisualGrid,
};

enum class CPlugVisualIndexedArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugVisualIndexed,
    IndexBuffer = 0x0906a001u,
};

enum class CPlugSurfaceArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugSurface,
    LegacyWord = 0x0900c001u,
};

enum class CPlugSurfaceGeomArchiveChunkId : u32 {
    Root = TMNF_CLASS_CPlugSurfaceGeom,
    SphereRadius = 0x0900f001u,
    EllipsoidExtents = 0x0900f002u,
    IdentifierAndBox = 0x0900f003u,
    SurfaceGeometry = 0x0900f004u,
};

constexpr u32 ArchiveChunkIdValue(GxLightArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugVisualArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugVisual3DArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugVisualSpriteArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugVisualGridArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugVisualIndexedArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugSurfaceArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugSurfaceGeomArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsGxLightBaseArchiveChunk(u32 chunkId) {
    return chunkId >= TMNF_CLASS_GxLight &&
           chunkId <= ArchiveChunkIdValue(GxLightArchiveChunkId::LegacyBaseState8);
}

constexpr int IsGxLightInfo1Chunk(u32 chunkId) {
    return IsGxLightBaseArchiveChunk(chunkId);
}

constexpr int IsGxLightInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(GxLightArchiveChunkId::FullLightState);
}

constexpr int IsGxLightAmbientInfo3Chunk(u32 chunkId) {
    return IsGxLightInfo3Chunk(chunkId) || chunkId == TMNF_CLASS_GxLightAmbient;
}

constexpr int IsGxLightDirectionalInfo1Chunk(u32 chunkId) {
    return IsGxLightInfo1Chunk(chunkId) ||
           chunkId == TMNF_CLASS_GxLightDirectional ||
           chunkId == ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId::EnabledVector);
}

constexpr int IsGxLightDirectionalInfo3Chunk(u32 chunkId) {
    return IsGxLightInfo3Chunk(chunkId) ||
           chunkId == ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId::EnabledVectorAndAngles) ||
           chunkId == ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId::DirectionVector) ||
           chunkId == ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId::DirectionColor) ||
           chunkId == ArchiveChunkIdValue(GxLightDirectionalArchiveChunkId::AngularLimits);
}

constexpr int IsCPlugVisualInfo1Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CPlugVisual ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmpty2) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::GeometryHeader) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyWord6) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmpty7) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmpty8) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmptyA) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmptyC) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyEmptyD);
}

constexpr int IsCPlugVisualInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::Identifier) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::FuncVisualRef) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::Vec3Array) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::LegacyWord9) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::Record32Array) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualArchiveChunkId::VisualGeometryProvider);
}

constexpr int IsCPlugVisual3DInfo1Chunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CPlugVisual3D ||
           chunkId == ArchiveChunkIdValue(CPlugVisual3DArchiveChunkId::VertexStream) ||
           chunkId == ArchiveChunkIdValue(CPlugVisual3DArchiveChunkId::VertexStreamAndTangents);
}

constexpr int IsCPlugVisual3DInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CPlugVisual3DArchiveChunkId::MaterialRef) ||
           chunkId == ArchiveChunkIdValue(CPlugVisual3DArchiveChunkId::FaceStream);
}

constexpr int IsCPlugVisualSpriteInfo3Chunk(u32 chunkId) {
    return chunkId ==
                   ArchiveChunkIdValue(CPlugVisualSpriteArchiveChunkId::SpriteParameters) ||
           chunkId ==
                   ArchiveChunkIdValue(CPlugVisualSpriteArchiveChunkId::AtlasGrid);
}

constexpr int IsCPlugVisualGridInfo3Chunk(u32 chunkId) {
    return chunkId ==
           ArchiveChunkIdValue(CPlugVisualGridArchiveChunkId::Root);
}

constexpr int IsCPlugVisualIndexedTrianglesInfo1Chunk(u32 chunkId) {
    return IsCPlugVisualInfo1Chunk(chunkId) ||
           IsCPlugVisual3DInfo1Chunk(chunkId);
}

constexpr int IsCPlugVisualIndexedTrianglesInfo3Chunk(u32 chunkId) {
    return IsCPlugVisualInfo3Chunk(chunkId) ||
           chunkId == TMNF_CLASS_CPlugSurface ||
           IsCPlugVisual3DInfo3Chunk(chunkId) ||
           chunkId == ArchiveChunkIdValue(CPlugVisualIndexedArchiveChunkId::IndexBuffer);
}

constexpr int IsCPlugSurfaceGeomBaseArchiveChunk(u32 chunkId) {
    return chunkId >= TMNF_CLASS_CPlugSurfaceGeom &&
           chunkId <= ArchiveChunkIdValue(CPlugSurfaceGeomArchiveChunkId::IdentifierAndBox);
}

constexpr int IsCPlugSurfaceGeomInfo1Chunk(u32 chunkId) {
    return IsCPlugSurfaceGeomBaseArchiveChunk(chunkId);
}

constexpr int IsCPlugSurfaceGeomInfo3Chunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(CPlugSurfaceGeomArchiveChunkId::SurfaceGeometry);
}

#endif
