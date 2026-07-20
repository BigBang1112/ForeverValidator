#include "format/static_solid/static_solid_archive_shader_reader.h"

#include "format/archive/archive_class_ids.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_node_ref_reader.h"
#include "format/static_solid/static_solid_archive_shader_chunk_ids.h"

namespace {

constexpr u32 ShaderArchiveReferenceCountLimit = 0x10000000u;

int ReadNodeReferenceArray(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    u32 count = 0u;
    if (!byteStream->ReadU32(&count) ||
        count > ShaderArchiveReferenceCountLimit) {
        return 0;
    }
    for (u32 index = 0u; index < count; ++index) {
        if (!nodeRefs->ReadNodPtr(nullptr)) {
            return 0;
        }
    }
    return 1;
}

int ReadLegacyCustomShader(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr ||
        !nodeRefs->ReadNodPtr(nullptr) ||
        !ReadNodeReferenceArray(byteStream, nodeRefs) ||
        !nodeRefs->ReadNodPtr(nullptr)) {
        return 0;
    }

    u32 customShaderCount = 0u;
    if (!byteStream->ReadU32(&customShaderCount) ||
        customShaderCount > ShaderArchiveReferenceCountLimit) {
        return 0;
    }
    for (u32 index = 0u; index < customShaderCount; ++index) {
        if (!nodeRefs->ReadNodPtr(nullptr)) {
            return 0;
        }
    }
    return 1;
}

int ReadGpuLoadFxArray(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState) {
    u32 count = 0u;
    if (byteStream == nullptr || cmwIdState == nullptr ||
        !byteStream->ReadU32(&count) ||
        count > ShaderArchiveReferenceCountLimit) {
        return 0;
    }
    for (u32 index = 0u; index < count; ++index) {
        if (!cmwIdState->ReadSkip(*byteStream) ||
            !byteStream->Skip(16u)) {
            return 0;
        }
    }
    return 1;
}

int ReadShaderPassGpuPipeline(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    u32 enabled = 0u;
    if (byteStream == nullptr || cmwIdState == nullptr ||
        nodeRefs == nullptr || !byteStream->ReadBool(&enabled) ||
        enabled > 1u || !nodeRefs->ReadNodPtr(nullptr)) {
        return 0;
    }
    if (enabled == 0u) {
        return 1;
    }
    u32 vec4Count = 0u;
    return ReadGpuLoadFxArray(byteStream, cmwIdState) &&
           byteStream->ReadU32(&vec4Count) &&
           vec4Count <= ShaderArchiveReferenceCountLimit &&
           byteStream->SkipCounted(vec4Count, 16u);
}

int ReadShaderPassPipelines(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    u32 idCount = 0u;
    if (byteStream == nullptr || cmwIdState == nullptr ||
        nodeRefs == nullptr || !byteStream->ReadU32(&idCount) ||
        idCount > ShaderArchiveReferenceCountLimit) {
        return 0;
    }
    for (u32 index = 0u; index < idCount; ++index) {
        if (!cmwIdState->ReadSkip(*byteStream)) {
            return 0;
        }
    }
    return ReadShaderPassGpuPipeline(byteStream, cmwIdState, nodeRefs) &&
           ReadShaderPassGpuPipeline(byteStream, cmwIdState, nodeRefs);
}

int ReadBitmapSampler(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    return byteStream != nullptr && cmwIdState != nullptr &&
           nodeRefs != nullptr && cmwIdState->ReadSkip(*byteStream) &&
           nodeRefs->ReadNodPtr(nullptr) && byteStream->Skip(8u);
}

int ReadBitmapAddressTransform(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    unsigned char hasTransform = 0u;
    return byteStream != nullptr && nodeRefs != nullptr &&
           byteStream->Skip(4u) && nodeRefs->ReadNodPtr(nullptr) &&
           byteStream->Read(&hasTransform, 1u) && hasTransform <= 1u &&
           (hasTransform == 0u || byteStream->Skip(24u));
}

}  // namespace

int CGameCtnReplayStaticSolidArchiveShaderReader::ParseCPlugShaderChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 classId,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || cmwIdState == nullptr ||
        nodeRefs == nullptr) {
        return 0;
    }
    if (classId == TMNF_CLASS_CPlugShaderPass) {
        switch (chunkId) {
        case ArchiveChunkIdValue(
                CPlugShaderPassArchiveChunkId::BitmapSamplers):
            return ReadNodeReferenceArray(byteStream, nodeRefs);
        case ArchiveChunkIdValue(
                CPlugShaderPassArchiveChunkId::RenderState):
            return byteStream->Skip(4u);
        case ArchiveChunkIdValue(
                CPlugShaderPassArchiveChunkId::Pipelines):
            return ReadShaderPassPipelines(
                    byteStream, cmwIdState, nodeRefs);
        default:
            return 0;
        }
    }
    if (classId == TMNF_CLASS_CPlugBitmapApply) {
        switch (chunkId) {
        case ArchiveChunkIdValue(CPlugBitmapSamplerArchiveChunkId::Bitmap):
            return ReadBitmapSampler(byteStream, cmwIdState, nodeRefs);
        case ArchiveChunkIdValue(
                CPlugBitmapAddressArchiveChunkId::Transform):
            return ReadBitmapAddressTransform(byteStream, nodeRefs);
        case ArchiveChunkIdValue(
                CPlugBitmapAddressArchiveChunkId::BumpEnvScale):
        case ArchiveChunkIdValue(CPlugBitmapApplyArchiveChunkId::ApplyFlags):
            return byteStream->Skip(4u);
        default:
            return 0;
        }
    }
    if (classId != TMNF_CLASS_CPlugShaderApply) {
        return 0;
    }
    switch (chunkId) {
    case ArchiveChunkIdValue(CPlugShaderArchiveChunkId::LegacyCustomShader):
        return ReadLegacyCustomShader(byteStream, nodeRefs);
    case ArchiveChunkIdValue(
            CPlugShaderArchiveChunkId::RequirementsWithNat16):
        return byteStream->Skip(8u) &&
               byteStream->Skip(4u) &&
               nodeRefs->ReadNodPtr(nullptr) &&
               byteStream->Skip(2u);
    case ArchiveChunkIdValue(CPlugShaderGenericArchiveChunkId::Material):
        return byteStream->Skip(0x58u);
    case ArchiveChunkIdValue(
            CPlugShaderApplyArchiveChunkId::TextureApplyRefs):
        return ReadNodeReferenceArray(byteStream, nodeRefs);
    case ArchiveChunkIdValue(CPlugShaderApplyArchiveChunkId::ApplyField):
        return byteStream->Skip(4u);
    case ArchiveChunkIdValue(CPlugShaderApplyArchiveChunkId::ApplyFields):
        return byteStream->Skip(8u);
    default:
        return 0;
    }
}
