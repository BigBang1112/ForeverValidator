#include "engine/rendering/plug_visual.h"
#include <algorithm>
#include <cmath>
#include <limits>

#include "engine/rendering/plug_vertex_stream.h"
namespace {

constexpr u32 MaxTexCoordSetCount = 8u;
constexpr u32 InvalidTexCoordSetIndex =
        std::numeric_limits<u32>::max();
constexpr float TexCoordScaleEpsilon = 1e-5f;

}  // namespace

u32 CPlugVisual::TexCoordSetCount(void) const {
    return static_cast<u32>(texCoordSets.size());
}

GxTexCoordSet &CPlugVisual::TexCoordSetAt(u32 index) {
    return texCoordSets.at(index);
}

const GxTexCoordSet &CPlugVisual::TexCoordSetAt(u32 index) const {
    return texCoordSets.at(index);
}

u32 CPlugVisual::VertexStreamCount(void) const {
    return static_cast<u32>(vertexStreams.size());
}

CPlugVertexStream *CPlugVisual::VertexStreamAt(u32 index) const {
    return vertexStreams.at(index).Get();
}

bool CPlugVisual::TexCoordSetIndexIsInvalid(uint32_t index) const {
    return index == InvalidTexCoordSetIndex;
}

bool CPlugVisual::TexCoordSetCapacityAvailable(void) const {
    return TexCoordSetCount() < MaxTexCoordSetCount;
}

int CPlugVisual::FinalTexCoordSetIndex(void) const {
    return (int)TexCoordSetCount() - 1;
}

u32 CPlugVisual::TexCoordVertexCount(void) const {
    return GetTotalVertexCount();
}

static unsigned long PlugVisualIntegerSquareRoot(unsigned long value) {
    unsigned long result = 0u;
    unsigned long bit = 1ul <<
            (std::numeric_limits<unsigned long>::digits - 2u);
    while (bit > value) {
        bit >>= 2u;
    }
    while (bit != 0u) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1u) + bit;
        } else {
            result >>= 1u;
        }
        bit >>= 2u;
    }
    return result;
}

void CPlugVisual::GenerateDefaultTexCoord(
        GxTexCoordSet &texCoordSet,
        unsigned long dimension) const {
    (void)dimension;
    const GxTexCoordDimension requestedDimension = texCoordSet.Dimension();
    const unsigned long vertexCount = GetTotalVertexCount();
    GxTexCoordSet generated =
            GxTexCoordSet::Create(requestedDimension, vertexCount);

    if (!texCoordSets.empty() &&
        texCoordSets.front().Dimension() == requestedDimension) {
        const GxTexCoordSet &source = texCoordSets.front();
        const unsigned long copyCount = std::min(vertexCount, source.Count());
        for (unsigned long index = 0u; index < copyCount; ++index) {
            generated.SetCoordinate(index, source.Coordinate4At(index));
        }
        texCoordSet = std::move(generated);
        return;
    }

    if (vertexCount != 0u) {
        const unsigned long rowWidth = std::max(
                PlugVisualIntegerSquareRoot(vertexCount),
                2ul);
        for (unsigned long index = 0u; index < vertexCount; ++index) {
            const float u = static_cast<float>(index) /
                    static_cast<float>(rowWidth - 1u);
            const float v = vertexCount == 1u
                    ? std::numeric_limits<float>::quiet_NaN()
                    : static_cast<float>(index) /
                              static_cast<float>(vertexCount - 1u);
            generated.SetCoordinate(index, {u, v, 1.0f, 1.0f});
        }
    }
    texCoordSet = std::move(generated);
}

const CFastBuffer<GxVertex> *CPlugVisual::VertexStreamGetVertexs(
        int wantPositions,
        int wantNormals,
        int wantColors) const {
    if (vertexStreams.empty()) {
        return nullptr;
    }
    std::vector<GxVertex> vertices =
            CanonicalVertices(wantPositions, wantNormals, wantColors);
    if (vertices.empty() && GetTotalVertexCount() != 0u) {
        return nullptr;
    }
    canonicalVertexView_.Values() = std::move(vertices);
    return &canonicalVertexView_;
}

int CPlugVisual::VStreamOrClassic_GetTexCoordSet(
        GxTexCoordSet &texCoordSet,
        unsigned long index,
        CPlugVertexStream **outVertexStream) const {
    if (outVertexStream != nullptr) {
        *outVertexStream = nullptr;
    }

    if (!vertexStreams.empty()) {
        if (index >= MaxTexCoordSetCount) {
            return 0;
        }
        CPlugVertexStream *stream = VertexStreamGetByDecl(
                static_cast<EPlugVDcl>(EPlugVDcl_TexCoord0 + index));
        GxTexCoordSet streamSet;
        if (stream == nullptr ||
            !stream->GetTexCoordSet(
                    static_cast<EPlugVDcl>(EPlugVDcl_TexCoord0 + index),
                    streamSet)) {
            return 0;
        }
        texCoordSet = std::move(streamSet);
        if (outVertexStream != nullptr) {
            *outVertexStream = stream;
        }
        return 1;
    }

    if (index >= texCoordSets.size()) {
        return 0;
    }
    texCoordSet = texCoordSets[index];
    return 1;
}

GxTexCoord *CPlugVisual::VStreamOrClassic_GetTexCoord2s(
        unsigned long index,
        CPlugVertexStream **outVertexStream) const {
    if (vertexStreams.empty()) {
        if (outVertexStream != nullptr) *outVertexStream = nullptr;
        return index < texCoordSets.size() &&
                       texCoordSets[index].Dimension() ==
                               GxTexCoordDimension::Two
                ? texCoordSets[index].Data2()
                : nullptr;
    }
    streamTexCoordView_.emplace();
    GxTexCoordSet &texCoords = *streamTexCoordView_;
    if (VStreamOrClassic_GetTexCoordSet(
                texCoords, index, outVertexStream) == 0 ||
        texCoords.Dimension() != GxTexCoordDimension::Two) {
        return nullptr;
    }
    return texCoords.Data2();
}

void CPlugVisual::VertexStreamAdd(CPlugVertexStream *vertexStream) {
    if (vertexStream == nullptr) {
        return;
    }
    const auto insertion = vertexStream->DirtyVision()
            ? vertexStreams.end()
            : std::find_if(
                    vertexStreams.begin(),
                    vertexStreams.end(),
                    [](const CMwNodRef<CPlugVertexStream> &candidate) {
                        return candidate && candidate->DirtyVision();
                    });
    vertexStreams.insert(insertion, CMwNodRef<CPlugVertexStream>(vertexStream));
    MarkVisualDirty();
}

int CPlugVisual::VertexStreamSub(CPlugVertexStream *vertexStream) {
    const auto found = std::find_if(
            vertexStreams.begin(),
            vertexStreams.end(),
            [vertexStream](const CMwNodRef<CPlugVertexStream> &candidate) {
                return candidate.Get() == vertexStream;
            });
    if (found == vertexStreams.end()) {
        return 0;
    }
    vertexStreams.erase(found);
    MarkVisualDirty();
    return 1;
}

void CPlugVisual::SetTexCoordSetAt(
        unsigned long index,
        GxTexCoordSet &texCoordSet) {
    if (index >= texCoordSets.size()) {
        return;
    }
    texCoordSets[index] = std::move(texCoordSet);
    MarkTexCoordSetDirty();
}


int CPlugVisual::RemoveTexCoordSetAt(unsigned long index) {
    if (index == InvalidEngineIndex || index >= texCoordSets.size()) {
        return 0;
    }

    texCoordSets.erase(texCoordSets.begin() + index);
    MarkTexCoordSetDirty();
    return 1;
}



int CPlugVisual::ReleaseTexCoordSetAt(unsigned long index) {
    if (index == InvalidEngineIndex || index >= texCoordSets.size()) {
        return 0;
    }

    texCoordSets.erase(texCoordSets.begin() + index);
    MarkTexCoordSetDirty();
    return 1;
}



void CPlugVisual::RemoveTexCoordSetAll(void) {
    texCoordSets.clear();
    MarkTexCoordSetDirty();
}


void CPlugVisual::ReleaseTexCoordSetAll(void) {
    texCoordSets.clear();
    MarkTexCoordSetDirty();
}



unsigned long CPlugVisual::AddTexCoordSet(GxTexCoordSet &texCoordSet) {
    constexpr unsigned long invalidIndex = InvalidEngineIndex;
    if (texCoordSet.Empty()) {
        return invalidIndex;
    }

    if (!TexCoordSetCapacityAvailable()) {
        return invalidIndex;
    }

    texCoordSets.push_back(std::move(texCoordSet));
    MarkTexCoordSetDirty();
    return static_cast<unsigned long>(FinalTexCoordSetIndex());
}



unsigned long CPlugVisual::AddTexCoordSet(
        float uScale,
        float vScale,
        unsigned long flags,
        float uOffset,
        float vOffset) {
    (void)flags;

    if (!TexCoordSetCapacityAvailable()) {
        return InvalidEngineIndex;
    }

    GxTexCoordSet localSet;
    GenerateDefaultTexCoord(localSet, 0u);
    texCoordSets.push_back(std::move(localSet));
    GxTexCoordSet &lastSet = texCoordSets.back();

    if (fabsf(uScale - 1.0f) >= TexCoordScaleEpsilon ||
        fabsf(vScale - 1.0f) >= TexCoordScaleEpsilon) {
        if (lastSet.Dimension() == GxTexCoordDimension::Two) {
            lastSet.Apply2DScaleOffset(
                    uScale,
                    vScale,
                    uOffset,
                    vOffset);
        }
    }

    MarkTexCoordSetDirty();
    return static_cast<unsigned long>(FinalTexCoordSetIndex());
}

CPlugVertexStream *CPlugVisual::VertexStreamGetByDecl(EPlugVDcl decl) const {
    for (const CMwNodRef<CPlugVertexStream> &stream : vertexStreams) {
        if (stream && stream->HasDeclaration(decl)) {
            return stream.Get();
        }
    }
    return nullptr;
}
