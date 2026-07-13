#include "format/static_solid/static_solid_archive_material_reader.h"
#include "format/materials/material_archive_schema.h"
#include "format/materials/material_surface_definition_format_access.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_graph_writer.h"
#include "format/static_solid/static_solid_archive_primitive_reader.h"
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
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    constexpr u32 kMaxDeviceMatCount = 0x10000000u;
    u32 count = 0u;
    if (!byteStream->ReadU32(&count) ||
        count > kMaxDeviceMatCount) {
        return 0;
    }
    for (u32 i = 0u; i < count; i++) {
        u32 hasShader = 0u;
        if (!byteStream->Skip(4u) ||
            !byteStream->ReadBool(&hasShader)) {
            return 0;
        }
        if (hasShader != 0u) {
            if (!CGameCtnReplayStaticSolidArchivePrimitiveReader::
                        SkipInlineArchivePayload(byteStream)) {
                return 0;
            }
        } else if (!CPlugMaterialDiscardedRefArchivePayload::Read(nodeRefs)) {
            return 0;
        }
        if (MaterialDeviceSetHasShaderRefs(chunkId_)) {
            if (!CPlugMaterialDiscardedRefArchivePayload::Read(nodeRefs) ||
                !CPlugMaterialDiscardedRefArchivePayload::Read(nodeRefs)) {
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
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (nodeRefs == nullptr ||
        !nodeRefs->ReadNodeRef(&materialModelNode_)) {
        return 0;
    }
    if (!materialModelNode_.IsInvalid()) {
        return 1;
    }
    return ReadDeviceMats(byteStream, nodeRefs);
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
        StaticSolidArchiveLoadSession *store,
        StaticSolidArchiveId payload,
        u32 materialNodeIndex,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    if (IsMaterialNodeReferenceChunk(chunkId)) {
        return CPlugMaterialDiscardedRefArchivePayload::Read(nodeRefs);
    }
    if (IsMaterialDeviceSetChunk(chunkId)) {
        CPlugMaterialDeviceMatsArchivePayload deviceMats(chunkId);
        return deviceMats.Read(byteStream, nodeRefs);
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
