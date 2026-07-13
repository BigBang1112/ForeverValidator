#include "format/static_solid/static_solid_geometry_decoder.h"
#include <stdint.h>
#include <utility>

#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_visual_provider_state.h"
namespace {

}  // namespace

StaticSolidDecodedPayloads::
        StaticSolidDecodedPayloads(
                StaticSolidArchiveLoadSession &archive)
        : store(archive) {}

StaticSolidArchiveDecodedBytes
StaticSolidDecodedPayloads::Slice(
        StaticSolidArchiveId payload,
        CGameCtnReplayStaticSolidArchivePayloadSlice slice) const {
    if (!slice.HasRecords()) {
        return StaticSolidArchiveDecodedBytes::Empty();
    }
    if (!payload.IsValid()) {
        return StaticSolidArchiveDecodedBytes::Missing();
    }
    return store.DecodedPayloadBytes(payload,
                                     slice.ByteOffset(),
                                     slice.ByteCount());
}

StaticSolidDecodedVisualGeometry
StaticSolidDecodedVisualGeometry::FromArchiveDefinition(
        const CGameCtnReplayStaticSolidArchiveVisualGeometryDefinition &definition,
        const StaticSolidDecodedPayloads
                &decodedPayloads) {
    StaticSolidDecodedVisualGeometry geometry;
    geometry.BindBox(definition.BoundingBox());

    const CGameCtnReplayStaticSolidArchivePayloadSlice vertexRecords =
            definition.VertexRecords();
    const CGameCtnReplayStaticSolidArchivePayloadSlice indexRecords =
            definition.Indices();
    const StaticSolidArchiveDecodedBytes vertexBytes =
            decodedPayloads.Slice(
            definition.VisualProvider().Payload(),
            vertexRecords);
    const StaticSolidArchiveDecodedBytes indexBytes =
            decodedPayloads.Slice(
            definition.VisualProvider().Payload(),
            indexRecords);
    geometry.BindVertexStream(definition.SerializedFlags(),
                              definition.VertexCount(),
                              definition.VertexCount(),
                              vertexBytes.IsAvailable()
                                      ? vertexBytes.Bytes()
                                      : nullptr,
                              vertexRecords);
    geometry.BindIndexBuffer(definition.IndexCount(),
                             indexBytes.IsAvailable()
                                     ? indexBytes.Bytes()
                                     : nullptr,
                             indexRecords);
    return geometry;
}

void StaticSolidDecodedVisualGeometry::BindBox(
        const GmBoxAligned &newBoundingBox) {
    boundingBox = newBoundingBox;
}

void StaticSolidDecodedVisualGeometry::BindVertexStream(
        u32 newSerializedFlags,
        u32 newVertexCount,
        u32 newTexCoordVertexCount,
        const uint8_t *newVertexRecordBytes,
        CGameCtnReplayStaticSolidArchivePayloadSlice newVertexRecords) {
    serializedFlags = newSerializedFlags;
    vertexCount = newVertexCount;
    texCoordVertexCount = newTexCoordVertexCount;
    vertexRecordBytes = newVertexRecordBytes;
    vertexRecords = newVertexRecords;
}

void StaticSolidDecodedVisualGeometry::BindIndexBuffer(
        u32 newIndexCount,
        const uint8_t *newIndexBytes,
        CGameCtnReplayStaticSolidArchivePayloadSlice newIndices) {
    indexCount = newIndexCount;
    indexBytes = newIndexBytes;
    indices = newIndices;
}

u32 StaticSolidDecodedVisualGeometry::VertexCount() const {
    return vertexCount;
}

u32 StaticSolidDecodedVisualGeometry::TexCoordVertexCount()
        const {
    return texCoordVertexCount;
}

u32 StaticSolidDecodedVisualGeometry::IndexCount() const {
    return indexCount;
}

int StaticSolidDecodedVisualGeometry::CopyVerticesToGx(
        GxVertex *destination) const {
    return CGameCtnReplayStaticSolidVisualVertexFormat::FromSerializedFlags(
            serializedFlags)
            .CopyArchiveVerticesToGx(vertexRecordBytes,
                                     vertexRecords.RecordStride(),
                                     vertexRecords.ByteCount(),
                                     vertexCount,
                                     destination);
}

void StaticSolidDecodedVisualGeometry::CopyIndicesTo(
        uint16_t *destination) const {
    if (destination == nullptr ||
        indexBytes == nullptr ||
        !indices.HasRecords()) {
        return;
    }
    if (indices.ByteCount() <
        static_cast<size_t>(indexCount) * sizeof(uint16_t)) {
        return;
    }
    for (u32 index = 0u; index < indexCount; index++) {
        const uint8_t *source =
                indexBytes + static_cast<size_t>(index) * sizeof(uint16_t);
        destination[index] = static_cast<uint16_t>(
                static_cast<uint16_t>(source[0]) |
                static_cast<uint16_t>(source[1]) << 8u);
    }
}

const GmBoxAligned &
StaticSolidDecodedVisualGeometry::BoundingBox() const {
    return boundingBox;
}

void StaticSolidArchiveVisual::InstallGeometry(
        StaticSolidDecodedVisualGeometry newGeometry) {
    std::vector<GxVertex> ownedVertices(newGeometry.VertexCount());
    std::vector<uint16_t> ownedIndices(newGeometry.IndexCount());
    if (!ownedVertices.empty() &&
        !newGeometry.CopyVerticesToGx(ownedVertices.data())) {
        ownedVertices.clear();
    }
    if (!ownedIndices.empty()) {
        newGeometry.CopyIndicesTo(ownedIndices.data());
    }
    SetOwnedGeometry(std::move(ownedVertices), std::move(ownedIndices));
    archiveTexCoordVertexCount = newGeometry.TexCoordVertexCount();
    SetBoundingBox(newGeometry.BoundingBox());
}

void StaticSolidArchiveVisual::ConfigureGeometry(
        StaticSolidDecodedVisualGeometry newGeometry) {
    InstallGeometry(newGeometry);
}

u32 StaticSolidArchiveVisual::TexCoordVertexCount() const {
    return archiveTexCoordVertexCount;
}

void StaticSolidArchiveVisual::ComputeBoundingBox(
        unsigned long flags,
        unsigned long subVisual) {
    (void)flags;
    (void)subVisual;
}

CPlugVisual *
StaticSolidArchiveVisual::Visual() {
    return this;
}
