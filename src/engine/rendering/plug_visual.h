#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "engine/core/cfast_buffer.h"
#include "engine/core/engine_types.h"
#include "engine/core/gm_types.h"
#include "engine/core/gx_color.h"
#include "engine/core/gx_tex_coord.h"
#include "engine/core/mw_id.h"
#include "engine/rendering/plug.h"
#include "engine/rendering/plug_shader.h"
#include "engine/rendering/plug_vertex_declaration.h"
template <typename Value>
class CStridedArray;
struct CPlugVertexStream;
struct CPlugTree;
struct SPlugTreeOptimGroup;

struct SPlugVDcls {
    static constexpr std::size_t DeclarationCount =
            static_cast<std::size_t>(EPlugVDcl_TangentV1) + 1u;

    void Clear(void);
    void Require(EPlugVDcl declaration);
    bool Requires(EPlugVDcl declaration) const;
    void Merge(const SPlugVDcls &other);
    bool Empty(void) const;

private:
    std::array<bool, DeclarationCount> declarations_{};
};

struct GxVertex {
    GmVec3 position{};
    GmVec3 normal{};
    float color[4]{};
};

struct CPlugVisual : CPlug {
    struct SSkinData;

    CPlugVisual(void);
    CPlugVisual(const CPlugVisual &source);
    CPlugVisual &operator=(const CPlugVisual &) = delete;
    ~CPlugVisual(void) override;

    virtual CPlugVisual *Optimize(int arg, unsigned long flags) const;
    virtual void SetDefaultShaderProperties(CPlugShader *shader);
    virtual void ResetToEmptyVisual(int value);
    virtual void ComputeBoundingBox(unsigned long flags,
                                    unsigned long subVisual);
    virtual int ComputeTangents(unsigned long texCoordSetIndex,
                                int arg1,
                                int computeTangents,
                                int computeBinormals);
    virtual int SelfWeld(float x, float y, float z, float w, int flags);
    virtual void SetVertexCount(unsigned long vertexCount);
    virtual unsigned long GetTotalVertexCount(void) const;
    virtual void GetTotalVertexs(GxVertex *outVertices) const;
    virtual void GetVertexIndexation(unsigned long &outCount,
                                     unsigned short *&outIndices) const;
    virtual unsigned long GetVertexIndexCount(void) const;
    virtual unsigned long GetBytePerVertex(void) const;
    virtual int GetStridedVertexElem(
            CStridedArray<GxBGRAColor> &outValues,
            EPlugVDcl declaration) const;
    virtual int GetStridedVertexElem(CStridedArray<float> &outValues,
                                     EPlugVDcl declaration) const;
    virtual int GetStridedVertexElem(CStridedArray<GmVec2> &outValues,
                                     EPlugVDcl declaration) const;
    virtual int GetStridedVertexElem(CStridedArray<GmVec3> &outValues,
                                     EPlugVDcl declaration) const;
    virtual int GetStridedVertexElem(CStridedArray<GmVec4> &outValues,
                                     EPlugVDcl declaration) const;
    virtual int GetStridedVertexElem(
            CStridedArray<unsigned char> &outValues,
            EPlugVDcl declaration,
            EPlugVDclType type) const;
    virtual SPlugVDcls VStreamOrClassic_GetPlugVDcls(void) const;
    virtual const CFastBuffer<GxVertex> *GetVertexsAll(
            int wantPositions,
            int wantNormals,
            int wantColors) const;
    virtual std::vector<GxVertex> CanonicalVertices(
            int wantPositions,
            int wantNormals,
            int wantColors) const;
    const CFastBuffer<GxVertex> *VertexStreamGetVertexs(
            int wantPositions,
            int wantNormals,
            int wantColors) const;
    virtual void GenerateDefaultTexCoord(GxTexCoordSet &texCoordSet,
                                         unsigned long dimension) const;
    CPlugVertexStream *VertexStreamGetByDecl(EPlugVDcl decl) const;
    int VStreamOrClassic_GetTexCoordSet(
            GxTexCoordSet &texCoordSet,
            unsigned long index,
            CPlugVertexStream **outVertexStream) const;
    GxTexCoord *VStreamOrClassic_GetTexCoord2s(
            unsigned long index,
            CPlugVertexStream **outVertexStream) const;
    void VertexStreamAdd(CPlugVertexStream *vertexStream);
    int VertexStreamSub(CPlugVertexStream *vertexStream);
    void SetTexCoordSetAt(unsigned long index, GxTexCoordSet &texCoordSet);
    int RemoveTexCoordSetAt(unsigned long index);
    int ReleaseTexCoordSetAt(unsigned long index);
    unsigned long AddTexCoordSet(GxTexCoordSet &texCoordSet);
    unsigned long AddTexCoordSet(float uScale,
                                 float vScale,
                                 unsigned long flags,
                                 float uOffset,
                                 float vOffset);
    void RemoveTexCoordSetAll(void);
    void ReleaseTexCoordSetAll(void);
    virtual CPlugVisual *Duplicate_ShareGeometryAndIndexation(
            unsigned long vertexStreamCount);
    std::unique_ptr<CPlugVisual> CloneVisual(void) const;
    virtual void CopyGeometry(const CPlugVisual *source,
                              const unsigned short *indices);
    int AddDefaultTexCoordSet(void);
    int UpdateVisualFromShaderRequirement(const CPlugShader *&shader,
                                          CPlugShader *replacementShader);
    void SetGeometryStatic(int enabled);
    void SetIndexationStatic(int enabled);
    void EnableVertexColor(int enabled);
    void EnableVertexNormal(int enabled);
    void SetOptimizeInVision(int enabled);
    bool IsGeometryStatic(void) const;
    bool IsIndexationStatic(void) const;
    bool HasVertexColor(void) const;
    bool HasVertexNormal(void) const;
    bool IsOptimizedInVision(void) const;
    bool IsFeatureDirty(void) const;
    bool IsVisualDirty(void) const;
    u32 SkinIndexCount(void) const;
    void SetBoundingBox(const GmBoxAligned &box);
    void SetBoundingMinMax(const GmVec3 &minPoint,
                           const GmVec3 &maxPoint);
    void SetBoundingNull(void);
    const GmBoxAligned &BoundingBox(void) const;
    bool HasRenderableGeometry(void) const;
    bool HasEmptyIndexBuffer(void) const;
    GmVec3 *TangentData(void);
    const GmVec3 *TangentData(void) const;
    GmVec3 *BinormalData(void);
    const GmVec3 *BinormalData(void) const;
    virtual int IsValidForRender(void) const;
    std::uint8_t *GetSkinIndexWeights(void) const;
    void SetSkinIndexCount(unsigned long count);
    u32 TexCoordSetCount(void) const;
    GxTexCoordSet &TexCoordSetAt(u32 index);
    const GxTexCoordSet &TexCoordSetAt(u32 index) const;
    virtual u32 TexCoordVertexCount(void) const;
    u32 VertexStreamCount(void) const;
    CPlugVertexStream *VertexStreamAt(u32 index) const;

protected:
    friend struct CPlugTree;
    friend struct CPlugVertexStream;
    friend struct SPlugTreeOptimGroup;
    CMwId id;
    GmBoxAligned boundingBox{};
    std::unique_ptr<SSkinData> skinData;
    std::vector<CMwNodRef<CPlugVertexStream>> vertexStreams;
    mutable std::vector<GxTexCoordSet> texCoordSets;
    mutable CFastBuffer<GxVertex> canonicalVertexView_;
    mutable std::optional<GxTexCoordSet> streamTexCoordView_;

    struct State {
        u32 skinIndexCount = 0u;
        bool geometryStatic = false;
        bool indexationStatic = true;
        bool vertexNormal = true;
        bool vertexColor = false;
        bool optimizeInVision = false;
        bool featureDirty = true;
        bool visualDirty = true;

        static State ForDuplicate(const State &source);
    };

    State state_{};

    bool HasMatchingVertexFormat(const CPlugVisual &other) const {
        return state_.vertexNormal == other.state_.vertexNormal &&
               state_.vertexColor == other.state_.vertexColor;
    }
    void MarkFeatureDirty(void) {
        state_.featureDirty = true;
    }
    void MarkVisualDirty(void) {
        state_.visualDirty = true;
    }
    void MarkTexCoordSetDirty(void) {
        MarkVisualDirty();
    }
    int ComputeTangentsForRequirements(u32 requiresShadowTexCoords,
                                       u32 requiresLightTexCoords);
    u32 VertexStreamTexCoordCount(void) const;
    bool VertexStreamsHave(EPlugVDcl declaration) const;
    bool RequiresTexCoordInference(void) const;
    u32 AvailableTexCoordCountForShader(void) const;
    int EnsureShaderTexCoordSetCount(const CPlugShader &shader);
    bool HasShadowTexCoords(void) const;
    bool HasLightTexCoords(void) const;
    bool NeedsShadowTexCoords(const CPlugShader &shader) const;
    bool NeedsLightTexCoords(const CPlugShader &shader) const;
    int UpdateRequirementChannels(u32 requiresShadowTexCoords,
                                  u32 requiresLightTexCoords);
    bool TexCoordSetIndexIsInvalid(u32 index) const;
    bool TexCoordSetCapacityAvailable(void) const;
    int FinalTexCoordSetIndex(void) const;
};

struct CPlugIndexBuffer : CPlug {
    CPlugIndexBuffer(void);
    ~CPlugIndexBuffer(void) override;
};

struct CPlugVisual3D : CPlugVisual {
    std::vector<GmVec3> positions;
    std::vector<GmVec3> normals;
    std::vector<GmVec4> colors;
    std::vector<GmVec3> tangents;
    std::vector<GmVec3> binormals;
    std::vector<CMwNodRef<CMwNod>> shaders;

    CPlugVisual3D(void);
    CPlugVisual3D(const CPlugVisual3D &source);
    ~CPlugVisual3D(void) override;
    int IsValidForRender(void) const override;
    void ResetToEmptyVisual(int value) override;
    void ComputeBoundingBox(unsigned long flags,
                            unsigned long subVisual) override;
    virtual void TransformByGmIso4(const GmIso4 &transform);
    void NegNormals(void);
    virtual void ReleasePtrVertexs(void);
    virtual void SetVertices(unsigned long vertexCount, GxVertex *vertices);
    void SetVertexCount(unsigned long vertexCount) override;
    void SetOwnedVertices(std::vector<GxVertex> newVertices);
    u32 TangentCount(void) const;
    u32 BinormalCount(void) const;
    u32 ShaderCount(void) const;
    unsigned long GetTotalVertexCount(void) const override;
    virtual unsigned long GetTexCoordSize(void) const;
    unsigned long GetBytePerVertex(void) const override;
    void GetTotalVertexs(GxVertex *outVertices) const override;
    SPlugVDcls VStreamOrClassic_GetPlugVDcls(void) const override;
    const CFastBuffer<GxVertex> *GetVertexsAll(
            int wantPositions,
            int wantNormals,
            int wantColors) const override;
    std::vector<GxVertex> CanonicalVertices(
            int wantPositions,
            int wantNormals,
            int wantColors) const override;
    int ComputeTangents(unsigned long texCoordSetIndex,
                        int arg1,
                        int computeTangents,
                        int computeBinormals) override;
    void CopyGeometry(const CPlugVisual *source,
                      const unsigned short *indices) override;
    const GmVec3 *VStreamOrClassic_GetTangents(
            EPlugVertexTangent tangent,
            CPlugVertexStream **outVertexStream) const;
    void TranslatePylonGeometry(u32 variantIndex, float squareHeight);

};

struct CPlugVisualIndexed : CPlugVisual3D {
    std::vector<std::uint16_t> indices;

    CPlugVisualIndexed(void);
    CPlugVisualIndexed(const CPlugVisualIndexed &source);
    ~CPlugVisualIndexed(void) override;
    CPlugVisual *Duplicate_ShareGeometryAndIndexation(
            unsigned long vertexStreamCount) override;
    void ReleasePtrVertexs(void) override;
    void ResetToEmptyVisual(int value) override;
    void SetVerticesAndIndices(unsigned long vertexCount,
                               GxVertex *vertices,
                               unsigned long indexCount,
                               unsigned short *indices);
    void CopyIndicesTo(std::uint16_t *outIndices) const;
    void GetVertexIndexation(unsigned long &outCount,
                             unsigned short *&outIndices) const override;
    unsigned long GetVertexIndexCount(void) const override;
    int IsValidForRender(void) const override;
    void SetOwnedGeometry(std::vector<GxVertex> newVertices,
                          std::vector<std::uint16_t> newIndices);
};

struct CPlugVisualIndexedTriangles : CPlugVisualIndexed {
    CPlugVisualIndexedTriangles(void);
    CPlugVisualIndexedTriangles(
            const CPlugVisualIndexedTriangles &source);
    ~CPlugVisualIndexedTriangles(void) override;
    virtual CPlugVisual *Duplicate(void) const;
    unsigned long GetTriangleIndexsCount(
            unsigned short **outIndices) const;
    void GetTriangleIndexs(std::uint16_t *outIndices) const;
};
