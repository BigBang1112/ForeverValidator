#pragma once

#include <optional>
#include <vector>

#include "engine/rendering/plug_visual.h"
#include "engine/rendering/plug_vertex_stream.h"
struct CClassicArchive;

struct VertexStreamArchiveAttribute {
    EPlugVDcl declaration = EPlugVDcl_Position;
    EPlugVDclType type = EPlugVDclType_Float1;
    EPlugVDclSpace space = EPlugVDclSpace::Point;
    bool skipVision = false;
    VertexAttributeValues values;
};

struct VertexStreamArchiveAttributeBlock {
    std::vector<VertexStreamArchiveAttribute> attributes;
};

struct VertexStreamArchiveGxBlock {
    std::vector<GxVertex> vertices;
    bool hasNormals = false;
    bool hasColors = false;
    bool positionsSkipVision = false;
    bool normalsSkipVision = false;
    bool colorsSkipVision = false;
};

struct VertexStreamArchivePayload {
    unsigned long vertexCount = 0u;
    bool isStatic = true;
    bool dirtyVision = false;
    bool isDirty = true;
    CMwNodRef<CPlugVertexStream> streamModel;
    std::optional<VertexStreamArchiveGxBlock> canonicalGx;
    std::vector<VertexStreamArchiveAttributeBlock> attributeBlocks;
};

class VertexStreamArchiveCodec {
public:
    static void ArchiveReference(
            CClassicArchive &archive,
            CMwNod *&node,
            std::vector<CMwNodRef<CMwNod>> &references);
    static bool Capture(const CPlugVertexStream &stream,
                        VertexStreamArchivePayload &outPayload);
    static bool Restore(const VertexStreamArchivePayload &payload,
                        CPlugVertexStream &stream);
    static bool Archive(CClassicArchive &archive,
                        CPlugVertexStream &stream);
};
