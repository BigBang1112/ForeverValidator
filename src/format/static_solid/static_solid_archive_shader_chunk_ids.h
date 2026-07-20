#ifndef TMNF_STATIC_SOLID_ARCHIVE_SHADER_CHUNK_IDS_H
#define TMNF_STATIC_SOLID_ARCHIVE_SHADER_CHUNK_IDS_H

#include "engine/core/engine_types.h"

enum class CPlugShaderArchiveChunkId : u32 {
    LegacyCustomShader = 0x0900200eu,
    RequirementsWithNat16 = 0x09002016u,
};

enum class CPlugShaderGenericArchiveChunkId : u32 {
    Material = 0x09004003u,
};

enum class CPlugShaderApplyArchiveChunkId : u32 {
    TextureApplyRefs = 0x09026002u,
    ApplyField = 0x09026004u,
    ApplyFields = 0x09026008u,
};

enum class CPlugShaderPassArchiveChunkId : u32 {
    BitmapSamplers = 0x09067006u,
    RenderState = 0x09067007u,
    Pipelines = 0x0906700au,
};

enum class CPlugBitmapSamplerArchiveChunkId : u32 {
    Bitmap = 0x0907e008u,
};

enum class CPlugBitmapAddressArchiveChunkId : u32 {
    Transform = 0x09047007u,
    BumpEnvScale = 0x09047009u,
};

enum class CPlugBitmapApplyArchiveChunkId : u32 {
    ApplyFlags = 0x09012004u,
};

constexpr u32 ArchiveChunkIdValue(CPlugShaderArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugShaderGenericArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugShaderApplyArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugShaderPassArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugBitmapSamplerArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugBitmapAddressArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr u32 ArchiveChunkIdValue(CPlugBitmapApplyArchiveChunkId chunkId) {
    return static_cast<u32>(chunkId);
}

constexpr int IsCPlugShaderRequiredPayloadChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CPlugShaderArchiveChunkId::LegacyCustomShader) ||
           chunkId == ArchiveChunkIdValue(
                              CPlugShaderArchiveChunkId::RequirementsWithNat16);
}

constexpr int IsCPlugShaderGenericRequiredPayloadChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CPlugShaderGenericArchiveChunkId::Material);
}

constexpr int IsCPlugShaderApplyRequiredPayloadChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CPlugShaderApplyArchiveChunkId::TextureApplyRefs) ||
           chunkId == ArchiveChunkIdValue(
                              CPlugShaderApplyArchiveChunkId::ApplyField) ||
           chunkId == ArchiveChunkIdValue(
                              CPlugShaderApplyArchiveChunkId::ApplyFields);
}

constexpr int IsCPlugShaderPassRequiredPayloadChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CPlugShaderPassArchiveChunkId::BitmapSamplers) ||
           chunkId == ArchiveChunkIdValue(
                              CPlugShaderPassArchiveChunkId::RenderState) ||
           chunkId == ArchiveChunkIdValue(
                              CPlugShaderPassArchiveChunkId::Pipelines);
}

constexpr int IsCPlugBitmapApplyRequiredPayloadChunk(u32 chunkId) {
    return chunkId == ArchiveChunkIdValue(
                              CPlugBitmapSamplerArchiveChunkId::Bitmap) ||
           chunkId == ArchiveChunkIdValue(
                              CPlugBitmapAddressArchiveChunkId::Transform) ||
           chunkId == ArchiveChunkIdValue(
                              CPlugBitmapAddressArchiveChunkId::BumpEnvScale) ||
           chunkId == ArchiveChunkIdValue(
                              CPlugBitmapApplyArchiveChunkId::ApplyFlags);
}

#endif
