#pragma once

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/tmnf_archive_ids.h"
enum class MaterialArchiveChunk : u32 {
    Root = TMNF_CLASS_CPlugMaterial,
    MaterialFxRef = 0x09079001u,
    SurfaceFlagsReplace = 0x09079002u,
    DeviceSetsLegacyA = 0x09079003u,
    DeviceSetsLegacyB = 0x09079004u,
    OldBitmapArray = 0x09079005u,
    OldGpuFxArrays = 0x09079006u,
    CustomMaterialRef = 0x09079007u,
    DeviceSets = 0x09079008u,
    DeviceSetsWithShaderRefs = 0x09079009u,
    SurfaceFlagsOr = 0x0907900au,
    DiscardedCustomMaterialRef = 0x0907900bu,
    DeviceSetsWithFormats = 0x0907900cu,
    DeviceSetsWithShaderRefsAndFormats = 0x0907900du,
    SurfaceFlags = 0x0907900eu,
    FourByteLegacyPayload = 0x0907900fu,
};

constexpr u32 MaterialArchiveDefaultSurfaceFlags = 0x800e0000u;
constexpr u32 MaterialArchiveStoredSurfaceFlagsMask = 0x7e1fffffu;
constexpr u32 MaterialArchiveSurfaceFlagsHighBit = 0x80000000u;
constexpr u32 MaterialArchiveSkipBlockMarker = 0x534b4950u;

constexpr u32 MaterialArchiveChunkValue(MaterialArchiveChunk chunk) {
    return static_cast<u32>(chunk);
}

constexpr bool IsMaterialArchiveChunk(u32 chunk) {
    return chunk >= TMNF_CLASS_CPlugMaterial &&
           chunk <= MaterialArchiveChunkValue(
                   MaterialArchiveChunk::FourByteLegacyPayload);
}

constexpr bool IsMaterialNodeReferenceChunk(u32 chunk) {
    return chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::MaterialFxRef) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::CustomMaterialRef) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DiscardedCustomMaterialRef);
}

constexpr bool IsMaterialDeviceSetChunk(u32 chunk) {
    return chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSets) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSetsWithShaderRefs) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSetsWithFormats) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::
                                    DeviceSetsWithShaderRefsAndFormats);
}

constexpr bool MaterialDeviceSetHasShaderRefs(u32 chunk) {
    return chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSetsWithShaderRefs) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSetsWithFormats) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::
                                    DeviceSetsWithShaderRefsAndFormats);
}

constexpr bool MaterialDeviceSetHasFormats(u32 chunk) {
    return chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSetsWithFormats) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::
                                    DeviceSetsWithShaderRefsAndFormats);
}

constexpr bool IsMaterialSurfaceFlagsChunk(u32 chunk) {
    return chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::SurfaceFlagsReplace) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::SurfaceFlagsOr) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::SurfaceFlags);
}

constexpr u32 NormalizeMaterialSurfaceFlags(u32 flags, u32 chunk) {
    if (chunk == MaterialArchiveChunkValue(
                         MaterialArchiveChunk::SurfaceFlagsReplace)) {
        return (flags & MaterialArchiveStoredSurfaceFlagsMask) |
               MaterialArchiveSurfaceFlagsHighBit;
    }
    if (chunk == MaterialArchiveChunkValue(
                         MaterialArchiveChunk::SurfaceFlagsOr)) {
        return flags | MaterialArchiveSurfaceFlagsHighBit;
    }
    return flags;
}

constexpr bool IsRequiredMaterialPayloadChunk(u32 chunk) {
    return IsMaterialNodeReferenceChunk(chunk) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::
                                    DeviceSetsWithShaderRefsAndFormats) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::SurfaceFlags) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::FourByteLegacyPayload);
}

constexpr bool IsOptionalMaterialPayloadChunk(u32 chunk) {
    return chunk == TMNF_CLASS_CPlugMaterial ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::SurfaceFlagsReplace) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSetsLegacyA) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSetsLegacyB) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::OldBitmapArray) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::OldGpuFxArrays) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSets) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSetsWithShaderRefs) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::SurfaceFlagsOr) ||
           chunk == MaterialArchiveChunkValue(
                            MaterialArchiveChunk::DeviceSetsWithFormats);
}
