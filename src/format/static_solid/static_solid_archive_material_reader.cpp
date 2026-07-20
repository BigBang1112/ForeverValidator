#include "format/static_solid/static_solid_archive_material_reader.h"
#include "format/materials/material_archive_schema.h"
#include "format/materials/material_surface_definition_format_access.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_graph_writer.h"
#include "format/static_solid/static_solid_archive_primitive_reader.h"

namespace {

constexpr u32 CPlugMaterialCustomClassId = 0x0903a000u;
constexpr u32 CPlugMaterialCustomChunkIntArray = 0x0903a004u;
constexpr u32 CPlugMaterialCustomChunkBitmaps = 0x0903a006u;
constexpr u32 CPlugMaterialCustomChunkGpuFx = 0x0903a00au;
constexpr u32 CPlugMaterialCustomChunkBitmapSkip = 0x0903a00cu;
constexpr u32 CPlugMaterialCustomChunkFlags = 0x0903a00du;
constexpr u32 CPlugMaterialCustomChunkFloats = 0x0903a00fu;
constexpr u32 CPlugMaterialCustomChunkLegacySkip = 0x0903a011u;
constexpr u32 SkipBlockMarker = 0x534b4950u;
constexpr u32 MaxArchiveArrayCount = 0x100000u;

int SkipU32Array(CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    u32 count = 0u;
    return byteStream != nullptr && byteStream->ReadU32(&count) &&
           count <= MaxArchiveArrayCount &&
           byteStream->SkipCounted(count, 4u);
}

int SkipGpuFxArray(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState) {
    u32 count = 0u;
    if (byteStream == nullptr || cmwIdState == nullptr ||
        !byteStream->ReadU32(&count) || count > MaxArchiveArrayCount) {
        return 0;
    }
    for (u32 index = 0u; index < count; ++index) {
        u32 componentCount = 0u;
        u32 registerCount = 0u;
        u32 enabled = 0u;
        if (!cmwIdState->ReadSkip(*byteStream) ||
            !byteStream->ReadU32(&componentCount) ||
            !byteStream->ReadU32(&registerCount) ||
            !byteStream->ReadBool(&enabled) || enabled > 1u ||
            componentCount > MaxArchiveArrayCount ||
            registerCount > MaxArchiveArrayCount ||
            (componentCount != 0u &&
             registerCount > UINT32_MAX / componentCount) ||
            !byteStream->SkipCounted(componentCount * registerCount, 4u)) {
            return 0;
        }
    }
    return 1;
}

int ParseCPlugMaterialCustomChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || cmwIdState == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    switch (chunkId) {
    case CPlugMaterialCustomChunkIntArray:
        return SkipU32Array(byteStream);
    case CPlugMaterialCustomChunkBitmaps: {
        u32 count = 0u;
        if (!byteStream->ReadU32(&count) || count > MaxArchiveArrayCount) {
            return 0;
        }
        for (u32 index = 0u; index < count; ++index) {
            if (!cmwIdState->ReadSkip(*byteStream) ||
                !byteStream->Skip(4u) || !nodeRefs->ReadNodPtr(nullptr)) {
                return 0;
            }
        }
        return 1;
    }
    case CPlugMaterialCustomChunkGpuFx:
        return SkipGpuFxArray(byteStream, cmwIdState) &&
               SkipGpuFxArray(byteStream, cmwIdState);
    case CPlugMaterialCustomChunkBitmapSkip: {
        u32 count = 0u;
        if (!byteStream->ReadU32(&count) || count > MaxArchiveArrayCount) {
            return 0;
        }
        for (u32 index = 0u; index < count; ++index) {
            u32 enabled = 0u;
            if (!cmwIdState->ReadSkip(*byteStream) ||
                !byteStream->ReadBool(&enabled) || enabled > 1u) {
                return 0;
            }
        }
        return 1;
    }
    case CPlugMaterialCustomChunkFlags: {
        u32 flags = 0u;
        return byteStream->ReadU32(&flags) && byteStream->Skip(12u) &&
               ((flags & 1u) == 0u || byteStream->Skip(4u));
    }
    case CPlugMaterialCustomChunkFloats: {
        u32 marker = 0u;
        u32 byteCount = 0u;
        return byteStream->ReadU32(&marker) && marker == SkipBlockMarker &&
               byteStream->ReadU32(&byteCount) &&
               byteStream->Skip(byteCount);
    }
    case CPlugMaterialCustomChunkLegacySkip: {
        u32 marker = 0u;
        u32 byteCount = 0u;
        return byteStream->ReadU32(&marker) && marker == SkipBlockMarker &&
               byteStream->ReadU32(&byteCount) && byteCount == 4u &&
               byteStream->Skip(byteCount);
    }
    default:
        return 0;
    }
}

}  // namespace

int CPlugMaterialDiscardedRefArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    return nodeRefs != nullptr && nodeRefs->ReadNodPtr(nullptr);
}

CPlugMaterialDeviceMatsArchivePayload::CPlugMaterialDeviceMatsArchivePayload(
        u32 chunkId)
        : chunkId_(chunkId),
          materialModelNode_(
                  ArchiveNodeReference::Invalid()) {}

int CPlugMaterialDeviceMatsArchivePayload::ReadDeviceMats(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || cmwIdState == nullptr ||
        nodeRefs == nullptr) {
        return 0;
    }
    constexpr u32 kMaxDeviceMatCount = 0x10000000u;
    u32 count = 0u;
    if (!byteStream->ReadU32(&count) ||
        count > kMaxDeviceMatCount) {
        return 0;
    }
    for (u32 i = 0u; i < count; i++) {
        u32 activeShaderIsFid = 0u;
        if (!byteStream->Skip(4u) ||
            !byteStream->ReadBool(&activeShaderIsFid) ||
            activeShaderIsFid > 1u ||
            (activeShaderIsFid != 0u
                     ? !nodeRefs->ReadFidRef(nullptr)
                     : !CPlugMaterialDiscardedRefArchivePayload::Read(
                               nodeRefs))) {
            return 0;
        }
        if (MaterialDeviceSetHasShaderRefs(chunkId_)) {
            if (!nodeRefs->ReadFidRef(nullptr) ||
                !nodeRefs->ReadFidRef(nullptr)) {
                return 0;
            }
        }
    }
    if (MaterialDeviceSetHasFormats(chunkId_)) {
        return CGameCtnReplayStaticSolidArchivePrimitiveReader::
                SkipD3dFormatArray(byteStream);
    }
    return 1;
}

int CPlugMaterialDeviceMatsArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (nodeRefs == nullptr ||
        !nodeRefs->ReadNodeRef(&materialModelNode_)) {
        return 0;
    }
    if (!materialModelNode_.IsInvalid()) {
        return 1;
    }
    return ReadDeviceMats(byteStream, cmwIdState, nodeRefs);
}

CPlugMaterialSurfaceFlagsArchivePayload::
        CPlugMaterialSurfaceFlagsArchivePayload(u32 chunkId)
        : chunkId_(chunkId) {}

int CPlugMaterialSurfaceFlagsArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    u32 archiveFlags = 0u;
    if (byteStream == nullptr || !byteStream->ReadU32(&archiveFlags)) {
        return 0;
    }
    surface_ = MaterialSurfaceDefinitionFormatAccess::FromArchiveFlags(
            NormalizeMaterialSurfaceFlags(archiveFlags, chunkId_));
    return 1;
}

int CPlugMaterialSurfaceFlagsArchivePayload::AppendDefinition(
        StaticSolidArchiveLoadSession *store,
        StaticSolidArchiveId payload,
        u32 materialNodeIndex) const {
    CGameCtnReplayStaticSolidArchiveGraphWriter writer(
            store != nullptr ? &store->MutableArchiveGraph() : nullptr,
            payload);
    return writer.AddMaterialDefinition(ArchiveNodeReference::FromIndex(
                                              materialNodeIndex),
                                      surface_);
}

int CGameCtnReplayStaticSolidArchiveMaterialReader::ParseCPlugMaterialChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 classId,
        StaticSolidArchiveLoadSession *store,
        StaticSolidArchiveId payload,
        u32 materialNodeIndex,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    if (classId == CPlugMaterialCustomClassId) {
        return ParseCPlugMaterialCustomChunk(
                byteStream, cmwIdState, chunkId, nodeRefs);
    }
    if (IsMaterialNodeReferenceChunk(chunkId)) {
        return CPlugMaterialDiscardedRefArchivePayload::Read(nodeRefs);
    }
    if (IsMaterialDeviceSetChunk(chunkId)) {
        CPlugMaterialDeviceMatsArchivePayload deviceMats(chunkId);
        return deviceMats.Read(byteStream, cmwIdState, nodeRefs);
    }
    if (IsMaterialSurfaceFlagsChunk(chunkId)) {
        CPlugMaterialSurfaceFlagsArchivePayload surfaceFlags(chunkId);
        if (!surfaceFlags.Read(byteStream)) {
            return 0;
        }
        return surfaceFlags.AppendDefinition(store,
                                        payload,
                                        materialNodeIndex);
    }
    if (chunkId == MaterialArchiveChunkValue(
                           MaterialArchiveChunk::FourByteLegacyPayload)) {
        return byteStream->Skip(4u);
    }
    return 1;
}
