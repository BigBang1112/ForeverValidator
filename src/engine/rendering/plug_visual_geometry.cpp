#include "engine/rendering/plug_visual.h"
#include <algorithm>
#include <utility>
#include <vector>

#include "engine/core/binary32_math.h"
#include "engine/rendering/plug_vertex_stream.h"
int CPlugVisual3D::IsValidForRender(void) const {
    if (!positions.empty()) {
        return 1;
    }
    return CPlugVisual::IsValidForRender();
}

void CPlugVisual3D::ResetToEmptyVisual(int value) {
    ReleasePtrVertexs();
    CPlugVisual::ResetToEmptyVisual(value);
}

void CPlugVisual3D::ComputeBoundingBox(unsigned long computeFlags,
                                       unsigned long subVisual) {
    (void)computeFlags;
    (void)subVisual;
    if (positions.empty()) {
        SetBoundingNull();
        return;
    }
    GmVec3 minimum = positions.front();
    GmVec3 maximum = positions.front();
    for (const GmVec3 &position : positions) {
        minimum.x = std::min(minimum.x, position.x);
        minimum.y = std::min(minimum.y, position.y);
        minimum.z = std::min(minimum.z, position.z);
        maximum.x = std::max(maximum.x, position.x);
        maximum.y = std::max(maximum.y, position.y);
        maximum.z = std::max(maximum.z, position.z);
    }
    SetBoundingMinMax(minimum, maximum);
}

void CPlugVisual3D::TransformByGmIso4(const GmIso4 &transform) {
    for (GmVec3 &position : positions) position.Mult(transform);
    const GmMat3 rotation = transform.RotationMatrix();
    for (GmVec3 &normal : normals) normal.Mult(rotation);
    for (GmVec3 &tangent : tangents) tangent.Mult(rotation);
    for (GmVec3 &binormal : binormals) binormal.Mult(rotation);
    ComputeBoundingBox(0u, 0u);
}

void CPlugVisual3D::NegNormals(void) {
    for (GmVec3 &normal : normals) {
        normal.x = -normal.x;
        normal.y = -normal.y;
        normal.z = -normal.z;
    }
}

void CPlugVisual3D::ReleasePtrVertexs(void) {
    positions.clear();
    normals.clear();
    colors.clear();
    tangents.clear();
    binormals.clear();
}


u32 CPlugVisual3D::TangentCount(void) const {
    return static_cast<u32>(tangents.size());
}

u32 CPlugVisual3D::BinormalCount(void) const {
    return static_cast<u32>(binormals.size());
}

u32 CPlugVisual3D::ShaderCount(void) const {
    return static_cast<u32>(shaders.size());
}


unsigned long CPlugVisual3D::GetTotalVertexCount(void) const {
    if (!vertexStreams.empty()) {
        return static_cast<unsigned long>(
                CPlugVisual::CanonicalVertices(1, 1, 1).size());
    }
    return static_cast<u32>(positions.size());
}

unsigned long CPlugVisual3D::GetTexCoordSize(void) const {
    return GetTotalVertexCount();
}

SPlugVDcls CPlugVisual3D::VStreamOrClassic_GetPlugVDcls(void) const {
    SPlugVDcls result = CPlugVisual::VStreamOrClassic_GetPlugVDcls();
    if (!vertexStreams.empty()) return result;
    if (!positions.empty()) result.Require(EPlugVDcl_Position);
    if (!tangents.empty()) result.Require(EPlugVDcl_TangentU);
    if (!binormals.empty()) result.Require(EPlugVDcl_TangentV);
    return result;
}

const CFastBuffer<GxVertex> *CPlugVisual3D::GetVertexsAll(
        int wantPositions,
        int wantNormals,
        int wantColors) const {
    std::vector<GxVertex> vertices =
            CanonicalVertices(wantPositions, wantNormals, wantColors);
    if (vertices.empty() && GetTotalVertexCount() != 0u) {
        return nullptr;
    }
    canonicalVertexView_.Values() = std::move(vertices);
    return &canonicalVertexView_;
}

std::vector<GxVertex> CPlugVisual3D::CanonicalVertices(
        int wantPositions,
        int wantNormals,
        int wantColors) const {
    if (!vertexStreams.empty()) {
        return CPlugVisual::CanonicalVertices(
                wantPositions, wantNormals, wantColors);
    }
    std::vector<GxVertex> vertices(positions.size());
    for (std::size_t index = 0u; index < positions.size(); ++index) {
        GxVertex &vertex = vertices[index];
        vertex.position = positions[index];
        if (index < normals.size()) vertex.normal = normals[index];
        if (index < colors.size()) {
            vertex.color[0] = colors[index].x;
            vertex.color[1] = colors[index].y;
            vertex.color[2] = colors[index].z;
            vertex.color[3] = colors[index].w;
        }
    }
    return vertices;
}


void CPlugVisual3D::GetTotalVertexs(GxVertex *outVertices) const {
    if (outVertices == nullptr) {
        return;
    }
    const std::vector<GxVertex> vertices = CanonicalVertices(1, 1, 1);
    std::copy(vertices.begin(), vertices.end(), outVertices);
}

int CPlugVisual3D::ComputeTangents(unsigned long texCoordSetIndex,
                                  int arg1,
                                  int computeTangents,
                                  int computeBinormals) {
    (void)texCoordSetIndex;
    (void)arg1;
    int changed = 0;
    if (computeTangents != 0) {
        tangents.assign(GetTotalVertexCount(), GmVec3{});
        changed = 1;
    }
    if (computeBinormals != 0) {
        binormals.assign(GetTotalVertexCount(), GmVec3{});
        changed = 1;
    }
    return changed;
}

void CPlugVisual3D::TranslatePylonGeometry(u32 variantIndex,
                                           float squareHeight) {
    const float variant = Binary32::FromUnsignedInteger(variantIndex);
    const float delta = squareHeight * variant;
    for (GmVec3 &position : positions) {
        if (position.y > squareHeight * 0.5f) {
            position.y += delta;
        }
    }
}



CPlugVisualIndexedTriangles::CPlugVisualIndexedTriangles(void) = default;
CPlugVisualIndexedTriangles::CPlugVisualIndexedTriangles(
        const CPlugVisualIndexedTriangles &source)
        : CPlugVisualIndexed(source) {}
CPlugVisualIndexedTriangles::~CPlugVisualIndexedTriangles(void) = default;

CPlugVisual *CPlugVisualIndexedTriangles::Duplicate(void) const {
    return const_cast<CPlugVisualIndexedTriangles *>(this)
            ->Duplicate_ShareGeometryAndIndexation(VertexStreamCount());
}


void CPlugVisualIndexed::GetVertexIndexation(
        unsigned long &outCount,
        unsigned short *&outIndices) const {
    outCount = static_cast<unsigned long>(indices.size());
    outIndices = indices.empty()
            ? nullptr
            : const_cast<uint16_t *>(indices.data());
}


unsigned long CPlugVisualIndexed::GetVertexIndexCount(void) const {
    return static_cast<unsigned long>(indices.size());
}


int CPlugVisualIndexed::IsValidForRender(void) const {
    return CPlugVisual3D::IsValidForRender() != 0 && !indices.empty();
}


unsigned long CPlugVisualIndexedTriangles::GetTriangleIndexsCount(
        unsigned short **outIndices) const {
    if (outIndices != nullptr) {
        *outIndices = indices.empty()
                ? nullptr
                : const_cast<uint16_t *>(indices.data());
    }
    return static_cast<unsigned long>(indices.size());
}


void CPlugVisualIndexedTriangles::GetTriangleIndexs(uint16_t *outIndices) const {
    CopyIndicesTo(outIndices);
}

void CPlugVisualIndexed::CopyIndicesTo(uint16_t *outIndices) const {
    if (outIndices != nullptr) {
        std::copy(indices.begin(), indices.end(), outIndices);
    }
}


void CPlugVisual3D::SetVertices(unsigned long vertexCount,
                                GxVertex *sourceVertices) {
    if (vertexCount == 0u || sourceVertices == nullptr) {
        positions.clear();
        normals.clear();
        colors.clear();
        return;
    }
    positions.resize(vertexCount);
    normals.resize(vertexCount);
    colors.resize(vertexCount);
    for (unsigned long index = 0u; index < vertexCount; ++index) {
        positions[index] = sourceVertices[index].position;
        normals[index] = sourceVertices[index].normal;
        colors[index] = {sourceVertices[index].color[0],
                         sourceVertices[index].color[1],
                         sourceVertices[index].color[2],
                         sourceVertices[index].color[3]};
    }
}

void CPlugVisual3D::SetVertexCount(unsigned long vertexCount) {
    positions.resize(vertexCount);
    if (!normals.empty()) normals.resize(vertexCount);
    if (!colors.empty()) colors.resize(vertexCount);
    if (!tangents.empty()) tangents.resize(vertexCount);
    if (!binormals.empty()) binormals.resize(vertexCount);
    CPlugVisual::SetVertexCount(vertexCount);
}

void CPlugVisual3D::SetOwnedVertices(std::vector<GxVertex> newVertices) {
    SetVertices(static_cast<unsigned long>(newVertices.size()),
                newVertices.empty() ? nullptr : newVertices.data());
}

void CPlugVisual3D::CopyGeometry(
        const CPlugVisual *source,
        const unsigned short *remap) {
    if (source == nullptr) return;
    const unsigned long count = source->GetTotalVertexCount();
    std::vector<GxVertex> sourceVertices(count);
    source->GetTotalVertexs(sourceVertices.data());
    std::vector<GxVertex> copied(count);
    for (unsigned long sourceIndex = 0u; sourceIndex < count; ++sourceIndex) {
        const unsigned long destinationIndex = remap == nullptr
                ? sourceIndex
                : remap[sourceIndex];
        if (destinationIndex != 0xffffu && destinationIndex < count) {
            copied[destinationIndex] = sourceVertices[sourceIndex];
        }
    }
    SetOwnedVertices(std::move(copied));

    tangents.clear();
    binormals.clear();
    if (const auto *source3D = dynamic_cast<const CPlugVisual3D *>(source)) {
        const auto copyDirections = [count, remap](
                const std::vector<GmVec3> &input,
                std::vector<GmVec3> &output) {
            if (input.size() != count) return;
            output.assign(count, GmVec3{});
            for (unsigned long sourceIndex = 0u;
                 sourceIndex < count;
                 ++sourceIndex) {
                const unsigned long destinationIndex = remap == nullptr
                        ? sourceIndex
                        : remap[sourceIndex];
                if (destinationIndex != 0xffffu && destinationIndex < count) {
                    output[destinationIndex] = input[sourceIndex];
                }
            }
        };
        copyDirections(source3D->tangents, tangents);
        copyDirections(source3D->binormals, binormals);
    }
    CPlugVisual::CopyGeometry(source, remap);
}


void CPlugVisualIndexed::SetOwnedGeometry(
        std::vector<GxVertex> newVertices,
        std::vector<uint16_t> newIndices) {
    SetOwnedVertices(std::move(newVertices));
    indices = std::move(newIndices);
}

CPlugIndexBuffer::CPlugIndexBuffer(void) = default;

CPlugIndexBuffer::~CPlugIndexBuffer(void) = default;




CPlugVisual3D::CPlugVisual3D(void) = default;
CPlugVisual3D::CPlugVisual3D(const CPlugVisual3D &source)
        : CPlugVisual(source),
          positions(source.positions),
          normals(source.normals),
          colors(source.colors),
          tangents(source.tangents),
          binormals(source.binormals),
          shaders(source.shaders) {}
CPlugVisual3D::~CPlugVisual3D(void) = default;



CPlugVisualIndexed::CPlugVisualIndexed(void) = default;
CPlugVisualIndexed::CPlugVisualIndexed(const CPlugVisualIndexed &source)
        : CPlugVisual3D(source), indices(source.indices) {}
CPlugVisualIndexed::~CPlugVisualIndexed(void) = default;

CPlugVisual *CPlugVisualIndexed::Duplicate_ShareGeometryAndIndexation(
        unsigned long vertexStreamCount) {
    return CPlugVisual::Duplicate_ShareGeometryAndIndexation(
            vertexStreamCount);
}

void CPlugVisualIndexed::ReleasePtrVertexs(void) {
    CPlugVisual3D::ReleasePtrVertexs();
    indices.clear();
}

void CPlugVisualIndexed::ResetToEmptyVisual(int value) {
    ReleasePtrVertexs();
    CPlugVisual::ResetToEmptyVisual(value);
}

void CPlugVisualIndexed::SetVerticesAndIndices(
        unsigned long vertexCount,
        GxVertex *sourceVertices,
        unsigned long indexCount,
        unsigned short *sourceIndices) {
    SetVertices(vertexCount, sourceVertices);
    if (indexCount == 0u || sourceIndices == nullptr) {
        indices.clear();
    } else {
        indices.assign(sourceIndices, sourceIndices + indexCount);
    }
}

void CPlugVisual::SetVertexCount(unsigned long vertexCount) {
    for (CMwNodRef<CPlugVertexStream> &stream : vertexStreams) {
        if (stream) stream->SetVertexCount(vertexCount);
    }
    for (GxTexCoordSet &set : texCoordSets) set.Resize(vertexCount);
    MarkVisualDirty();
}

unsigned long CPlugVisual::GetTotalVertexCount(void) const {
    return static_cast<unsigned long>(CanonicalVertices(1, 1, 1).size());
}

void CPlugVisual::GetTotalVertexs(GxVertex *outVertices) const {
    if (outVertices == nullptr) {
        return;
    }
    const std::vector<GxVertex> vertices = CanonicalVertices(1, 1, 1);
    std::copy(vertices.begin(), vertices.end(), outVertices);
}

void CPlugVisual::GetVertexIndexation(unsigned long &outCount,
                                      unsigned short *&outIndices) const {
    outCount = 0u;
    outIndices = nullptr;
}

unsigned long CPlugVisual::GetVertexIndexCount(void) const {
    return 0u;
}

SPlugVDcls CPlugVisual::VStreamOrClassic_GetPlugVDcls(void) const {
    SPlugVDcls result{};
    if (!vertexStreams.empty()) {
        for (const CMwNodRef<CPlugVertexStream> &stream : vertexStreams) {
            if (!stream) continue;
            for (const CPlugVertexStream::SDataDecl &declaration :
                 stream->DataDeclarations()) {
                result.Require(declaration.Declaration());
            }
        }
        return result;
    }
    if (HasVertexNormal()) result.Require(EPlugVDcl_Normal);
    if (HasVertexColor()) result.Require(EPlugVDcl_Color0);
    for (unsigned long index = 0u; index < texCoordSets.size(); ++index) {
        result.Require(static_cast<EPlugVDcl>(EPlugVDcl_TexCoord0 + index));
    }
    return result;
}

const CFastBuffer<GxVertex> *CPlugVisual::GetVertexsAll(
        int wantPositions,
        int wantNormals,
        int wantColors) const {
    return VertexStreamGetVertexs(wantPositions, wantNormals, wantColors);
}

std::vector<GxVertex> CPlugVisual::CanonicalVertices(
        int wantPositions,
        int wantNormals,
        int wantColors) const {
    for (const CMwNodRef<CPlugVertexStream> &streamRef : vertexStreams) {
        CPlugVertexStream *stream = streamRef.Get();
        if (stream == nullptr) {
            continue;
        }
        std::vector<GxVertex> vertices = stream->CanonicalVertices(
                wantPositions, wantNormals, wantColors);
        if (!vertices.empty() || stream->VertexCount() == 0u) {
            return vertices;
        }
    }
    return {};
}

bool CPlugVisual::HasRenderableGeometry(void) const {
    return GetTotalVertexCount() != 0u;
}

bool CPlugVisual::HasEmptyIndexBuffer(void) const {
    const auto *indexed = dynamic_cast<const CPlugVisualIndexed *>(this);
    return indexed != nullptr && indexed->GetVertexIndexCount() == 0u;
}

GmVec3 *CPlugVisual::TangentData(void) {
    auto *visual3D = dynamic_cast<CPlugVisual3D *>(this);
    return visual3D != nullptr && !visual3D->tangents.empty()
            ? visual3D->tangents.data()
            : nullptr;
}

const GmVec3 *CPlugVisual::TangentData(void) const {
    const auto *visual3D = dynamic_cast<const CPlugVisual3D *>(this);
    return visual3D != nullptr && !visual3D->tangents.empty()
            ? visual3D->tangents.data()
            : nullptr;
}

GmVec3 *CPlugVisual::BinormalData(void) {
    auto *visual3D = dynamic_cast<CPlugVisual3D *>(this);
    return visual3D != nullptr && !visual3D->binormals.empty()
            ? visual3D->binormals.data()
            : nullptr;
}

const GmVec3 *CPlugVisual::BinormalData(void) const {
    const auto *visual3D = dynamic_cast<const CPlugVisual3D *>(this);
    return visual3D != nullptr && !visual3D->binormals.empty()
            ? visual3D->binormals.data()
            : nullptr;
}

const GmVec3 *CPlugVisual3D::VStreamOrClassic_GetTangents(
        EPlugVertexTangent tangent,
        CPlugVertexStream **outVertexStream) const {
    if (outVertexStream != nullptr) {
        *outVertexStream = nullptr;
    }
    if (!vertexStreams.empty()) {
        const EPlugVDcl declaration = tangent != EPlugVertexTangent_U
                ? EPlugVDcl_TangentV
                : EPlugVDcl_TangentU;
        CPlugVertexStream *stream = VertexStreamGetByDecl(declaration);
        const GmVec3 *values = stream != nullptr
                ? stream->GmVec3AttributeData(declaration)
                : nullptr;
        if (stream != nullptr && outVertexStream != nullptr) {
            *outVertexStream = stream;
        }
        return values;
    }
    return tangent != EPlugVertexTangent_U
            ? BinormalData()
            : TangentData();
}
