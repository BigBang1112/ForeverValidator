#pragma once

#include <algorithm>
#include <cstdint>
#include <variant>
#include <vector>

#include "engine/core/cfast_buffer.h"
#include "engine/rendering/plug.h"
#include "engine/rendering/plug_vertex_declaration.h"
#include "engine/core/vertex_attribute.h"
struct CPlugVisual;
struct GxVertex;

struct CPlugVertexStream : CPlug {
    struct SDataDecl {
        SDataDecl(EPlugVDcl declaration,
                  EPlugVDclType type,
                  EPlugVDclSpace space,
                  bool skipVision,
                  VertexAttributeValues values);

        EPlugVDcl Declaration(void) const;
        EPlugVDclType Type(void) const;
        EPlugVDclSpace Space(void) const;
        bool SkipVision(void) const;
        unsigned long Count(void) const;
        const VertexAttributeValues &Values(void) const;
        VertexAttributeValues &Values(void);
        VertexAttributeValue ValueAt(unsigned long index) const;

        void Release(void);
        void MoveWholeStride(unsigned long destinationIndex,
                             unsigned long sourceIndex,
                             unsigned long count);
        void AllocateMoreAt(unsigned long prefixCount,
                            unsigned long insertEndIndex,
                            unsigned long tailCount);
        static int SortByDecl(const SDataDecl *left, const SDataDecl *right);
        void SetMult(unsigned long destinationIndex,
                     const SDataDecl &source,
                     unsigned long count,
                     const GmIso4 &transform,
                     GmBoxAligned *boundingBox);

    private:
        friend struct CPlugVertexStream;

        EPlugVDcl declaration_;
        EPlugVDclType type_;
        EPlugVDclSpace space_;
        bool skipVision_;
        VertexAttributeValues values_;
    };

    struct StandaloneDeclSpec {
        EPlugVDcl declaration;
        EPlugVDclType type;
    };

    struct SStreamOrDeclType {
        using Source = std::variant<CMwNodRef<CPlugVertexStream>,
                                    StandaloneDeclSpec>;

        explicit SStreamOrDeclType(CPlugVertexStream *stream);
        SStreamOrDeclType(EPlugVDcl declaration, EPlugVDclType type);

        Source source;
    };

    CPlugVertexStream(void);
    ~CPlugVertexStream(void) override;
    CPlugVertexStream(const CPlugVertexStream &) = delete;
    CPlugVertexStream &operator=(const CPlugVertexStream &) = delete;

    static CMwNod *MwNewCPlugVertexStream(void);

    unsigned long VertexCount(void) const;
    bool IsStatic(void) const;
    bool DirtyVision(void) const;
    bool IsDirty(void) const;
    bool HasDeclaration(EPlugVDcl declaration) const;
    unsigned long TexCoordDeclarationCount(void) const;
    const std::vector<SDataDecl> &DataDeclarations(void) const;
    bool CopyAttributeValues(EPlugVDcl declaration,
                             VertexAttributeValues &outValues) const;
    bool AttributeValueAt(EPlugVDcl declaration,
                          unsigned long index,
                          VertexAttributeValue &outValue) const;
    bool SetAttributeValueAt(EPlugVDcl declaration,
                             unsigned long index,
                             const VertexAttributeValue &value);
    const VertexAttributeValues *AttributeValues(
            EPlugVDcl declaration) const;
    VertexAttributeValues *MutableAttributeValues(EPlugVDcl declaration);
    const GmVec3 *GmVec3AttributeData(EPlugVDcl declaration) const;
    bool GetTexCoordSet(EPlugVDcl declaration, GxTexCoordSet &outSet) const;

    void SetDirty(int dirty);
    const SDataDecl *DataFindByDecl(EPlugVDcl declaration,
                                    unsigned long *outIndex) const;
    const CFastBuffer<GxVertex> *GetGxVertexs(int wantPositions,
                                             int wantNormals,
                                             int wantColors) const;
    std::vector<GxVertex> CanonicalVertices(int wantPositions,
                                           int wantNormals,
                                           int wantColors) const;
    void SetIsStatic(int isStatic);
    void SetDirtyVision(int dirtyVision);
    void SetVertexCount(unsigned long count);
    void MoveAt(unsigned long destinationIndex,
                unsigned long sourceIndex,
                unsigned long count);
    void RemoveAt(unsigned long index, unsigned long count);
    void AllocateMoreAt(unsigned long insertIndex, unsigned long insertCount);
    void DataSetSkipVision(EPlugVDcl declaration, int skipVision);
    void DataChgDecl(EPlugVDcl newDeclaration, EPlugVDcl oldDeclaration);
    void SetAttributeValues(EPlugVDcl declaration,
                            VertexAttributeValues values,
                            bool skipVision = false);
    void SetCanonicalGxVertices(std::vector<GxVertex> vertices,
                                bool hasNormals,
                                bool hasColors);
    bool ReplaceContent(unsigned long vertexCount,
                        std::vector<SDataDecl> declarations);
    void SetStreamModel(CPlugVertexStream *source);
    CPlugVertexStream *StreamModel(void) const;
    unsigned long GetStrideTotal(void) const;
    void AllocateFromStreams(CFastBuffer<SStreamOrDeclType> &sources,
                             unsigned long vertexCount);
    void AllocateFromSources(
            const std::vector<SStreamOrDeclType> &sources,
            unsigned long vertexCount);
    void SetMultAt(unsigned long destinationIndex,
                   const CPlugVertexStream *source,
                   const GmIso4 &transform,
                   GmBoxAligned *boundingBox);
    void CopyAndClearVisual(CPlugVisual *visual);

protected:
    void Release(void);
    void InitDeclFromDataDecl(void);
    void Alloc(void);
    const SDataDecl *DataFindSharedPtr(const SDataDecl *target) const;
    void StreamModelUpdateDataPtrs(void) const;

private:
    struct VertexStreamContent {
        unsigned long vertexCount = 0u;
        std::vector<SDataDecl> declarations;
    };

    const VertexStreamContent &ResolvedContent(void) const {
        return streamModel_ ? streamModel_->ResolvedContent() : ownedContent_;
    }
    static bool ContentIsValid(const VertexStreamContent &content);
    static void SortDeclarations(VertexStreamContent &content) {
        std::sort(content.declarations.begin(), content.declarations.end(),
                  [](const SDataDecl &left, const SDataDecl &right) {
                      return SDataDecl::SortByDecl(&left, &right) < 0;
                  });
    }
    static SDataDecl *FindDeclaration(VertexStreamContent &content,
                                      EPlugVDcl declaration,
                                      unsigned long *outIndex);
    static const SDataDecl *FindDeclaration(
            const VertexStreamContent &content,
            EPlugVDcl declaration,
            unsigned long *outIndex);
    bool WouldCreateModelCycle(const CPlugVertexStream *source) const;
    bool SupportsCanonicalVertices(int wantPositions,
                                   int wantNormals,
                                   int wantColors) const;
    bool CommitContent(VertexStreamContent content);
    void AdoptValidatedContent(VertexStreamContent content);
    void DetachModel(void);

    VertexStreamContent ownedContent_;
    CMwNodRef<CPlugVertexStream> streamModel_;
    bool isStatic_;
    bool dirtyVision_;
    bool dirty_;
    mutable CFastBuffer<GxVertex> canonicalVertexView_;
};
