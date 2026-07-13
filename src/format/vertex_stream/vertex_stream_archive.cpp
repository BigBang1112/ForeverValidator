#include "format/vertex_stream/vertex_stream_archive.h"
#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include "format/archive/archive_binary32.h"
#include "format/archive/archive_node_reference.h"
#include "format/archive/classic_archive.h"
#include "format/archive/classic_archive_wire.h"
#include "format/archive/archive_class_ids.h"
namespace {

constexpr std::uint32_t StreamFlagStatic = 1u << 0u;
constexpr std::uint32_t StreamFlagDirty = 1u << 1u;
constexpr std::uint32_t StreamFlagDirtyVision = 1u << 3u;

constexpr std::uint32_t DeclarationMask = 0x000001ffu;
constexpr std::uint32_t TypeMask = 0x0003fe00u;
constexpr std::uint32_t StrideMask = 0x07fc0000u;
constexpr std::uint32_t SpaceMask = 0x38000000u;
constexpr std::uint32_t OwnsStorageMask = 0x40000000u;
constexpr std::uint32_t SkipVisionMask = 0x80000000u;

constexpr std::size_t MaximumDeclarationCount = 22u;
constexpr unsigned long MaximumVertexCount = 0x10000000ul;
constexpr unsigned long CanonicalGxStride = 40ul;
constexpr unsigned long CanonicalNormalOffset = 12ul;
constexpr unsigned long CanonicalColorOffset = 24ul;
constexpr unsigned long NodeArchiveEnd = 0xfacade01ul;

class AttributeByteEncoder {
public:
    explicit AttributeByteEncoder(std::size_t reserveBytes = 0u) {
        bytes_.reserve(reserveBytes);
    }
    void U8(std::uint8_t value) { bytes_.push_back(value); }

    void U16(std::uint16_t value) {
        U8(static_cast<std::uint8_t>(value));
        U8(static_cast<std::uint8_t>(value >> 8u));
    }

    void U32(std::uint32_t value) {
        U16(static_cast<std::uint16_t>(value));
        U16(static_cast<std::uint16_t>(value >> 16u));
    }

    void Float(const float &value) { U32(TmnfArchiveBinary32::Encode(value)); }

    std::vector<std::uint8_t> Finish(void) { return std::move(bytes_); }

private:
    std::vector<std::uint8_t> bytes_;
};

struct RawDeclaration {
    EPlugVDcl declaration = EPlugVDcl_Position;
    EPlugVDclType type = EPlugVDclType_Float1;
    EPlugVDclSpace space = EPlugVDclSpace::Point;
    bool skipVision = false;
    bool ownsStorage = true;
    unsigned long stride = 0u;
    std::uint16_t ownerIndex = 0u;
    std::uint16_t byteOffset = 0u;
    std::vector<std::uint8_t> ownedBytes;
    std::size_t semanticBlock = std::numeric_limits<std::size_t>::max();
    bool blockOwner = false;
};

std::uint32_t PackDeclaration(const RawDeclaration &row) {
    return static_cast<std::uint32_t>(row.declaration) |
           (static_cast<std::uint32_t>(row.type) << 9u) |
           (static_cast<std::uint32_t>(row.stride) << 18u) |
           (static_cast<std::uint32_t>(row.space) << 27u) |
           (row.ownsStorage ? OwnsStorageMask : 0u) |
           (row.skipVision ? SkipVisionMask : 0u);
}

bool UnpackDeclaration(std::uint32_t packed, RawDeclaration &row) {
    row.declaration = static_cast<EPlugVDcl>(packed & DeclarationMask);
    row.type = static_cast<EPlugVDclType>((packed & TypeMask) >> 9u);
    row.stride = static_cast<unsigned long>((packed & StrideMask) >> 18u);
    row.space = static_cast<EPlugVDclSpace>((packed & SpaceMask) >> 27u);
    row.ownsStorage = (packed & OwnsStorageMask) != 0u;
    row.skipVision = (packed & SkipVisionMask) != 0u;
    return PlugVDclIsValid(row.declaration) &&
           PlugVDclTypeIsMaterialized(row.type) &&
           row.space == PlugVDclSpace(row.declaration) &&
           row.stride >= PlugVDclTypeByteCount(row.type) &&
           row.stride <= 0x1ffu;
}

void EncodeGxVertex(AttributeByteEncoder &writer, const GxVertex &vertex) {
    writer.Float(vertex.position.x);
    writer.Float(vertex.position.y);
    writer.Float(vertex.position.z);
    writer.Float(vertex.normal.x);
    writer.Float(vertex.normal.y);
    writer.Float(vertex.normal.z);
    writer.Float(vertex.color[0]);
    writer.Float(vertex.color[1]);
    writer.Float(vertex.color[2]);
    writer.Float(vertex.color[3]);
}

bool DecodeFloatAt(const std::vector<std::uint8_t> &bytes,
                   std::size_t offset,
                   float &value) {
    if (offset > bytes.size() || bytes.size() - offset < 4u) return false;
    TmnfArchiveBinary32::ReadLittleEndianInto(bytes.data() + offset, value);
    return true;
}

bool DecodeGxVertex(const std::vector<std::uint8_t> &bytes,
                    std::size_t offset,
                    GxVertex &vertex) {
    return DecodeFloatAt(bytes, offset + 0u, vertex.position.x) &&
           DecodeFloatAt(bytes, offset + 4u, vertex.position.y) &&
           DecodeFloatAt(bytes, offset + 8u, vertex.position.z) &&
           DecodeFloatAt(bytes, offset + 12u, vertex.normal.x) &&
           DecodeFloatAt(bytes, offset + 16u, vertex.normal.y) &&
           DecodeFloatAt(bytes, offset + 20u, vertex.normal.z) &&
           DecodeFloatAt(bytes, offset + 24u, vertex.color[0]) &&
           DecodeFloatAt(bytes, offset + 28u, vertex.color[1]) &&
           DecodeFloatAt(bytes, offset + 32u, vertex.color[2]) &&
           DecodeFloatAt(bytes, offset + 36u, vertex.color[3]);
}

template <typename Values>
void EncodeByteArray(AttributeByteEncoder &writer, const Values &values) {
    for (std::uint8_t value : values) writer.U8(value);
}

template <typename Values>
void EncodeShortArray(AttributeByteEncoder &writer, const Values &values) {
    for (auto value : values) writer.U16(static_cast<std::uint16_t>(value));
}

bool EncodeValue(AttributeByteEncoder &writer,
                 EPlugVDclType type,
                 const VertexAttributeValue &value) {
    switch (type) {
    case EPlugVDclType_Float1:
        writer.Float(std::get<float>(value));
        return true;
    case EPlugVDclType_Float2: {
        const GmVec2 &v = std::get<GmVec2>(value);
        writer.Float(v.x); writer.Float(v.y);
        return true;
    }
    case EPlugVDclType_Float3: {
        const GmVec3 &v = std::get<GmVec3>(value);
        writer.Float(v.x); writer.Float(v.y); writer.Float(v.z);
        return true;
    }
    case EPlugVDclType_Float4: {
        const GmVec4 &v = std::get<GmVec4>(value);
        writer.Float(v.x); writer.Float(v.y); writer.Float(v.z); writer.Float(v.w);
        return true;
    }
    case EPlugVDclType_ColorD3D:
        writer.U32(std::get<GxBGRAColor>(value).value);
        return true;
    case EPlugVDclType_UByte4:
        EncodeByteArray(writer, std::get<VertexUByte4>(value).value);
        return true;
    case EPlugVDclType_Short2:
        EncodeShortArray(writer, std::get<VertexShort2>(value).value);
        return true;
    case EPlugVDclType_Short4:
        EncodeShortArray(writer, std::get<VertexShort4>(value).value);
        return true;
    case EPlugVDclType_UByte4N:
        EncodeByteArray(writer, std::get<VertexUByte4N>(value).value);
        return true;
    case EPlugVDclType_Short2N:
        EncodeShortArray(writer, std::get<VertexShort2N>(value).value);
        return true;
    case EPlugVDclType_Short4N:
        EncodeShortArray(writer, std::get<VertexShort4N>(value).value);
        return true;
    case EPlugVDclType_UShort2N:
        EncodeShortArray(writer, std::get<VertexUShort2N>(value).value);
        return true;
    case EPlugVDclType_UShort4N:
        EncodeShortArray(writer, std::get<VertexUShort4N>(value).value);
        return true;
    case EPlugVDclType_UDec3:
        writer.U32(std::get<VertexUDec3>(value).value);
        return true;
    case EPlugVDclType_Dec3N:
        writer.U32(std::get<VertexDec3N>(value).value);
        return true;
    case EPlugVDclType_Half2:
        EncodeShortArray(writer, std::get<VertexHalf2>(value).value);
        return true;
    case EPlugVDclType_Half4:
        EncodeShortArray(writer, std::get<VertexHalf4>(value).value);
        return true;
    case EPlugVDclType_Unused:
        return false;
    }
    return false;
}

bool ReadU16At(const std::vector<std::uint8_t> &bytes,
               std::size_t offset,
               std::uint16_t &value) {
    if (offset > bytes.size() || bytes.size() - offset < 2u) return false;
    value = static_cast<std::uint16_t>(bytes[offset]) |
            static_cast<std::uint16_t>(bytes[offset + 1u] << 8u);
    return true;
}

bool ReadU32At(const std::vector<std::uint8_t> &bytes,
               std::size_t offset,
               std::uint32_t &value) {
    if (offset > bytes.size() || bytes.size() - offset < 4u) return false;
    value = TmnfArchiveBinary32::ReadU32LittleEndian(bytes.data() + offset);
    return true;
}

template <typename Array>
bool DecodeByteArray(const std::vector<std::uint8_t> &bytes,
                     std::size_t offset,
                     Array &out) {
    if (offset > bytes.size() || bytes.size() - offset < out.size()) return false;
    std::copy_n(bytes.begin() + offset, out.size(), out.begin());
    return true;
}

template <typename Array>
bool DecodeShortArray(const std::vector<std::uint8_t> &bytes,
                      std::size_t offset,
                      Array &out) {
    for (std::size_t index = 0u; index < out.size(); ++index) {
        std::uint16_t value = 0u;
        if (!ReadU16At(bytes, offset + index * 2u, value)) return false;
        out[index] = static_cast<typename Array::value_type>(value);
    }
    return true;
}

bool DecodeValue(const std::vector<std::uint8_t> &bytes,
                 std::size_t offset,
                 EPlugVDclType type,
                 VertexAttributeValue &outValue) {
    float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
    std::uint32_t word = 0u;
    switch (type) {
    case EPlugVDclType_Float1:
        if (!DecodeFloatAt(bytes, offset, a)) return false;
        outValue = a; return true;
    case EPlugVDclType_Float2:
        if (!DecodeFloatAt(bytes, offset, a) || !DecodeFloatAt(bytes, offset + 4u, b)) return false;
        outValue = GmVec2{a, b}; return true;
    case EPlugVDclType_Float3:
        if (!DecodeFloatAt(bytes, offset, a) || !DecodeFloatAt(bytes, offset + 4u, b) || !DecodeFloatAt(bytes, offset + 8u, c)) return false;
        outValue = GmVec3{a, b, c}; return true;
    case EPlugVDclType_Float4:
        if (!DecodeFloatAt(bytes, offset, a) || !DecodeFloatAt(bytes, offset + 4u, b) || !DecodeFloatAt(bytes, offset + 8u, c) || !DecodeFloatAt(bytes, offset + 12u, d)) return false;
        outValue = GmVec4{a, b, c, d}; return true;
    case EPlugVDclType_ColorD3D:
        if (!ReadU32At(bytes, offset, word)) return false;
        outValue = GxBGRAColor{word}; return true;
    case EPlugVDclType_UByte4: {
        VertexUByte4 value; if (!DecodeByteArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_Short2: {
        VertexShort2 value; if (!DecodeShortArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_Short4: {
        VertexShort4 value; if (!DecodeShortArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_UByte4N: {
        VertexUByte4N value; if (!DecodeByteArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_Short2N: {
        VertexShort2N value; if (!DecodeShortArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_Short4N: {
        VertexShort4N value; if (!DecodeShortArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_UShort2N: {
        VertexUShort2N value; if (!DecodeShortArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_UShort4N: {
        VertexUShort4N value; if (!DecodeShortArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_UDec3:
        if (!ReadU32At(bytes, offset, word)) return false;
        outValue = VertexUDec3{word}; return true;
    case EPlugVDclType_Dec3N:
        if (!ReadU32At(bytes, offset, word)) return false;
        outValue = VertexDec3N{word}; return true;
    case EPlugVDclType_Half2: {
        VertexHalf2 value; if (!DecodeShortArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_Half4: {
        VertexHalf4 value; if (!DecodeShortArray(bytes, offset, value.value)) return false;
        outValue = value; return true;
    }
    case EPlugVDclType_Unused:
        return false;
    }
    return false;
}

bool AppendCanonicalGxRows(
        const VertexStreamArchivePayload &payload,
        std::size_t semanticBlock,
        std::vector<RawDeclaration> &rows) {
    if (!payload.canonicalGx.has_value()) {
        return true;
    }
    const VertexStreamArchiveGxBlock &gx = *payload.canonicalGx;
    if (gx.vertices.size() != payload.vertexCount ||
        payload.vertexCount >
                std::numeric_limits<std::uint32_t>::max() / CanonicalGxStride) {
        return false;
    }

    AttributeByteEncoder writer(
            static_cast<std::size_t>(payload.vertexCount) * CanonicalGxStride);
    for (const GxVertex &vertex : gx.vertices) {
        EncodeGxVertex(writer, vertex);
    }
    rows.push_back({
        EPlugVDcl_Position,
        EPlugVDclType_Float3,
        EPlugVDclSpace::Point,
        gx.positionsSkipVision,
        true,
        CanonicalGxStride,
        0u,
        0u,
        writer.Finish(),
        semanticBlock,
        true,
    });
    if (gx.hasNormals) {
        rows.push_back({
            EPlugVDcl_Normal,
            EPlugVDclType_Float3,
            EPlugVDclSpace::Direction,
            gx.normalsSkipVision,
            false,
            CanonicalGxStride,
            0u,
            CanonicalNormalOffset,
            {},
            semanticBlock,
            false,
        });
    }
    if (gx.hasColors) {
        rows.push_back({
            EPlugVDcl_Color0,
            EPlugVDclType_Float4,
            EPlugVDclSpace::Invariant,
            gx.colorsSkipVision,
            false,
            CanonicalGxStride,
            0u,
            CanonicalColorOffset,
            {},
            semanticBlock,
            false,
        });
    }
    return true;
}

struct PreparedAttributeBlock {
    std::vector<const VertexStreamArchiveAttribute *> attributes;
    unsigned long stride = 0u;
    std::vector<std::uint8_t> interleaved;
};

std::optional<PreparedAttributeBlock> PrepareAttributeBlock(
        const VertexStreamArchivePayload &payload,
        const VertexStreamArchiveAttributeBlock &block) {
    if (block.attributes.empty()) {
        return std::nullopt;
    }
    PreparedAttributeBlock prepared;
    prepared.attributes.reserve(block.attributes.size());
    for (const VertexStreamArchiveAttribute &attribute : block.attributes) {
        if (!PlugVDclIsValid(attribute.declaration) ||
            !PlugVDclTypeIsMaterialized(attribute.type) ||
            attribute.space != PlugVDclSpace(attribute.declaration) ||
            attribute.values.Type() != attribute.type ||
            attribute.values.Count() != payload.vertexCount) {
            return std::nullopt;
        }
        prepared.attributes.push_back(&attribute);
    }
    std::sort(
            prepared.attributes.begin(), prepared.attributes.end(),
            [](const auto *left, const auto *right) {
                return static_cast<std::uint32_t>(left->declaration) <
                       static_cast<std::uint32_t>(right->declaration);
            });
    for (const auto *attribute : prepared.attributes) {
        const unsigned long width = PlugVDclTypeByteCount(attribute->type);
        if (width == 0u || prepared.stride > 0x1ffu - width) {
            return std::nullopt;
        }
        prepared.stride += width;
    }
    if (prepared.stride != 0u &&
        payload.vertexCount >
                std::numeric_limits<std::uint32_t>::max() / prepared.stride) {
        return std::nullopt;
    }
    prepared.interleaved.resize(
            static_cast<std::size_t>(payload.vertexCount) * prepared.stride);
    return prepared;
}

bool EncodeAttributeBlock(
        const VertexStreamArchivePayload &payload,
        std::size_t semanticBlock,
        PreparedAttributeBlock prepared,
        std::vector<RawDeclaration> &rows) {
    unsigned long byteOffset = 0u;
    for (std::size_t attributeIndex = 0u;
         attributeIndex < prepared.attributes.size(); ++attributeIndex) {
        const VertexStreamArchiveAttribute &attribute =
                *prepared.attributes[attributeIndex];
        const unsigned long width = PlugVDclTypeByteCount(attribute.type);
        try {
            for (unsigned long vertexIndex = 0u;
                 vertexIndex < payload.vertexCount; ++vertexIndex) {
                AttributeByteEncoder writer;
                if (!EncodeValue(
                            writer,
                            attribute.type,
                            attribute.values.ValueAt(vertexIndex))) {
                    return false;
                }
                std::vector<std::uint8_t> encoded = writer.Finish();
                if (encoded.size() != width) {
                    return false;
                }
                std::copy(
                        encoded.begin(), encoded.end(),
                        prepared.interleaved.begin() +
                                static_cast<std::size_t>(vertexIndex) *
                                        prepared.stride + byteOffset);
            }
        } catch (const std::bad_variant_access &) {
            return false;
        }
        rows.push_back({
            attribute.declaration,
            attribute.type,
            attribute.space,
            attribute.skipVision,
            attributeIndex == 0u,
            prepared.stride,
            0u,
            static_cast<std::uint16_t>(byteOffset),
            {},
            semanticBlock,
            attributeIndex == 0u,
        });
        byteOffset += width;
    }
    rows[rows.size() - prepared.attributes.size()].ownedBytes =
            std::move(prepared.interleaved);
    return true;
}

bool FinalizeRawRows(std::vector<RawDeclaration> &rows) {
    std::sort(rows.begin(), rows.end(), [](const auto &left, const auto &right) {
        return static_cast<std::uint32_t>(left.declaration) <
               static_cast<std::uint32_t>(right.declaration);
    });
    if (rows.size() > MaximumDeclarationCount) {
        return false;
    }
    for (std::size_t index = 1u; index < rows.size(); ++index) {
        if (rows[index - 1u].declaration == rows[index].declaration) {
            return false;
        }
    }
    for (RawDeclaration &row : rows) {
        if (row.ownsStorage) {
            continue;
        }
        const auto owner = std::find_if(
                rows.begin(), rows.end(), [&row](const RawDeclaration &candidate) {
                    return candidate.semanticBlock == row.semanticBlock &&
                           candidate.blockOwner;
                });
        if (owner == rows.end() ||
            static_cast<std::size_t>(owner - rows.begin()) >
                    std::numeric_limits<std::uint16_t>::max()) {
            return false;
        }
        row.ownerIndex = static_cast<std::uint16_t>(owner - rows.begin());
    }
    return true;
}

bool BuildRawRowsUnchecked(
        const VertexStreamArchivePayload &payload,
        std::vector<RawDeclaration> &rows) {
    if (payload.vertexCount > MaximumVertexCount || payload.streamModel) {
        return false;
    }
    rows.clear();
    std::size_t semanticBlock = 0u;
    if (!AppendCanonicalGxRows(payload, semanticBlock, rows)) {
        return false;
    }
    semanticBlock += payload.canonicalGx.has_value();
    for (const VertexStreamArchiveAttributeBlock &block : payload.attributeBlocks) {
        auto prepared = PrepareAttributeBlock(payload, block);
        if (!prepared.has_value() ||
            !EncodeAttributeBlock(payload, semanticBlock++, std::move(*prepared), rows)) {
            return false;
        }
    }
    return FinalizeRawRows(rows);
}

bool BuildRawRows(const VertexStreamArchivePayload &payload,
                  std::vector<RawDeclaration> &rows) {
    try {
        return BuildRawRowsUnchecked(payload, rows);
    } catch (const std::bad_alloc &) {
        return false;
    } catch (const std::length_error &) {
        return false;
    }
}

bool ResolveRawStorage(const std::vector<RawDeclaration> &rows,
                       const RawDeclaration &row,
                       const RawDeclaration *&owner,
                       std::size_t &baseOffset) {
    if (row.ownsStorage) {
        owner = &row;
        baseOffset = 0u;
        return true;
    }
    if (row.ownerIndex >= rows.size()) return false;
    owner = &rows[row.ownerIndex];
    baseOffset = row.byteOffset;
    return owner->ownsStorage && row.stride == owner->stride &&
           baseOffset + PlugVDclTypeByteCount(row.type) <= owner->stride;
}

bool ArchiveRawU32(CClassicArchive &archive, std::uint32_t &value) {
    TmnfFormat::ClassicArchiveWire::NaturalBytes bytes = archive.IsWriting()
            ? TmnfFormat::ClassicArchiveWire::EncodeNatural(value)
            : TmnfFormat::ClassicArchiveWire::NaturalBytes{};
    archive.DoData(bytes.data(), bytes.size());
    if (!archive.IsWriting() && archive.Good()) {
        value = TmnfFormat::ClassicArchiveWire::DecodeNatural(bytes.data());
    }
    return archive.Good();
}

bool ValidateRawRows(
        const std::vector<RawDeclaration> &rows,
        unsigned long vertexCount) {
    for (std::size_t index = 0u; index < rows.size(); ++index) {
        const RawDeclaration &row = rows[index];
        if (row.ownsStorage) {
            if ((row.stride != 0u &&
                 vertexCount > std::numeric_limits<std::size_t>::max() / row.stride) ||
                row.ownedBytes.size() !=
                        static_cast<std::size_t>(vertexCount) * row.stride) {
                return false;
            }
        } else {
            const RawDeclaration *owner = nullptr;
            std::size_t baseOffset = 0u;
            if (!ResolveRawStorage(rows, row, owner, baseOffset)) {
                return false;
            }
        }
        if (index != 0u &&
            static_cast<std::uint32_t>(rows[index - 1u].declaration) >=
                    static_cast<std::uint32_t>(row.declaration)) {
            return false;
        }
    }
    return true;
}

bool IsCanonicalGxLayout(
        const std::vector<RawDeclaration> &rows,
        std::size_t positionIndex) {
    for (const RawDeclaration &row : rows) {
        if (row.ownsStorage || row.ownerIndex != positionIndex) {
            continue;
        }
        const bool normal =
                row.declaration == EPlugVDcl_Normal &&
                row.type == EPlugVDclType_Float3 &&
                row.stride == CanonicalGxStride &&
                row.byteOffset == CanonicalNormalOffset;
        const bool color =
                row.declaration == EPlugVDcl_Color0 &&
                row.type == EPlugVDclType_Float4 &&
                row.stride == CanonicalGxStride &&
                row.byteOffset == CanonicalColorOffset;
        if (!normal && !color) {
            return false;
        }
    }
    return true;
}

bool CaptureCanonicalGx(
        const std::vector<RawDeclaration> &rows,
        unsigned long vertexCount,
        std::vector<bool> &captured,
        std::optional<VertexStreamArchiveGxBlock> &output) {
    const auto position = std::find_if(
            rows.begin(), rows.end(), [](const RawDeclaration &row) {
                return row.declaration == EPlugVDcl_Position &&
                       row.type == EPlugVDclType_Float3 && row.ownsStorage &&
                       row.stride == CanonicalGxStride;
            });
    if (position == rows.end()) {
        return true;
    }
    const std::size_t positionIndex = position - rows.begin();
    if (!IsCanonicalGxLayout(rows, positionIndex)) {
        return true;
    }

    VertexStreamArchiveGxBlock gx;
    gx.positionsSkipVision = position->skipVision;
    gx.vertices.resize(vertexCount);
    for (std::size_t index = 0u; index < gx.vertices.size(); ++index) {
        if (!DecodeGxVertex(
                    position->ownedBytes,
                    index * CanonicalGxStride,
                    gx.vertices[index])) {
            return false;
        }
    }
    captured[positionIndex] = true;
    for (std::size_t index = 0u; index < rows.size(); ++index) {
        const RawDeclaration &row = rows[index];
        if (row.ownsStorage || row.ownerIndex != positionIndex) {
            continue;
        }
        if (row.declaration == EPlugVDcl_Normal) {
            gx.hasNormals = true;
            gx.normalsSkipVision = row.skipVision;
            captured[index] = true;
        } else if (row.declaration == EPlugVDcl_Color0) {
            gx.hasColors = true;
            gx.colorsSkipVision = row.skipVision;
            captured[index] = true;
        }
    }
    output = std::move(gx);
    return true;
}

std::optional<VertexStreamArchiveAttribute> DecodeRawAttribute(
        const std::vector<RawDeclaration> &rows,
        const RawDeclaration &row,
        unsigned long vertexCount) {
    const RawDeclaration *owner = nullptr;
    std::size_t baseOffset = 0u;
    if (!ResolveRawStorage(rows, row, owner, baseOffset)) {
        return std::nullopt;
    }
    VertexAttributeValues values(row.type, vertexCount);
    for (unsigned long vertexIndex = 0u; vertexIndex < vertexCount; ++vertexIndex) {
        VertexAttributeValue value = 0.0f;
        if (!DecodeValue(
                    owner->ownedBytes,
                    baseOffset + static_cast<std::size_t>(vertexIndex) * owner->stride,
                    row.type,
                    value) ||
            !values.SetValueAt(vertexIndex, value)) {
            return std::nullopt;
        }
    }
    return VertexStreamArchiveAttribute{
        row.declaration,
        row.type,
        row.space,
        row.skipVision,
        std::move(values),
    };
}

bool CaptureAttributeBlocks(
        const std::vector<RawDeclaration> &rows,
        unsigned long vertexCount,
        std::vector<bool> &captured,
        std::vector<VertexStreamArchiveAttributeBlock> &blocks) {
    for (std::size_t ownerIndex = 0u; ownerIndex < rows.size(); ++ownerIndex) {
        if (captured[ownerIndex] || !rows[ownerIndex].ownsStorage) {
            continue;
        }
        VertexStreamArchiveAttributeBlock block;
        for (std::size_t rowIndex = 0u; rowIndex < rows.size(); ++rowIndex) {
            if (captured[rowIndex]) {
                continue;
            }
            const RawDeclaration &row = rows[rowIndex];
            if (rowIndex != ownerIndex &&
                (row.ownsStorage || row.ownerIndex != ownerIndex)) {
                continue;
            }
            auto attribute = DecodeRawAttribute(rows, row, vertexCount);
            if (!attribute.has_value()) {
                return false;
            }
            block.attributes.push_back(std::move(*attribute));
            captured[rowIndex] = true;
        }
        if (block.attributes.empty()) {
            return false;
        }
        blocks.push_back(std::move(block));
    }
    return std::all_of(captured.begin(), captured.end(), [](bool value) {
        return value;
    });
}

bool RawRowsToPayload(
        const std::vector<RawDeclaration> &rows,
        VertexStreamArchivePayload &payload) {
    if (!ValidateRawRows(rows, payload.vertexCount)) {
        return false;
    }
    std::vector<bool> captured(rows.size(), false);
    return CaptureCanonicalGx(
                   rows, payload.vertexCount, captured, payload.canonicalGx) &&
           CaptureAttributeBlocks(
                   rows, payload.vertexCount, captured, payload.attributeBlocks);
}

} // namespace

namespace {

bool ArchiveVertexStreamNode(
        CClassicArchive &archive,
        CPlugVertexStream &stream) {
    unsigned long chunkId = TMNF_CLASS_CPlugVertexStream;
    archive.DoNatural(&chunkId, 1u, 1);
    if (!archive.Good() || chunkId != TMNF_CLASS_CPlugVertexStream ||
        !VertexStreamArchiveCodec::Archive(archive, stream)) {
        return false;
    }

    unsigned long endMarker = NodeArchiveEnd;
    archive.DoNatural(&endMarker, 1u, 1);
    return archive.Good() && endMarker == NodeArchiveEnd;
}

} // namespace

void VertexStreamArchiveCodec::ArchiveReference(
        CClassicArchive &archive,
        CMwNod *&node,
        std::vector<CMwNodRef<CMwNod>> &references) {
    constexpr unsigned long MaximumReferenceCount = 0x100000ul;

    if (archive.IsWriting()) {
        if (node == nullptr) {
            unsigned long referenceIndex =
                    ArchiveNodeReference::Invalid().Index();
            archive.DoNatural(&referenceIndex, 1u, 0);
            return;
        }

        const auto existing = std::find_if(
                references.begin(),
                references.end(),
                [node](const CMwNodRef<CMwNod> &reference) {
                    return reference.Get() == node;
                });
        if (existing != references.end()) {
            unsigned long referenceIndex = static_cast<unsigned long>(
                    existing - references.begin());
            archive.DoNatural(&referenceIndex, 1u, 0);
            return;
        }

        auto *vertexStream = dynamic_cast<CPlugVertexStream *>(node);
        if (vertexStream == nullptr ||
            references.size() >= MaximumReferenceCount) {
            archive.Fail();
            return;
        }

        unsigned long referenceIndex =
                static_cast<unsigned long>(references.size());
        references.emplace_back(node);
        archive.DoNatural(&referenceIndex, 1u, 0);
        if (!archive.Good()) {
            return;
        }

        unsigned long archiveClassId = TMNF_CLASS_CPlugVertexStream;
        archive.DoNatural(&archiveClassId, 1u, 1);
        if (archive.Good() &&
            !ArchiveVertexStreamNode(archive, *vertexStream)) {
            archive.Fail();
        }
        return;
    }

    unsigned long referenceIndex = ArchiveNodeReference::Invalid().Index();
    archive.DoNatural(&referenceIndex, 1u, 0);
    if (!archive.Good()) {
        return;
    }
    const ArchiveNodeReference reference =
            referenceIndex == ArchiveNodeReference::InvalidIndex
                    ? ArchiveNodeReference::Invalid()
                    : ArchiveNodeReference::FromIndex(
                              static_cast<std::uint32_t>(referenceIndex));
    if (reference.IsInvalid()) {
        node = nullptr;
        return;
    }
    if (referenceIndex >= MaximumReferenceCount) {
        archive.Fail();
        return;
    }
    if (referenceIndex < references.size() && references[referenceIndex]) {
        node = references[referenceIndex].Get();
        return;
    }

    unsigned long archiveClassId = 0u;
    archive.DoNatural(&archiveClassId, 1u, 1);
    if (!archive.Good() || archiveClassId != TMNF_CLASS_CPlugVertexStream) {
        archive.Fail();
        return;
    }

    CMwNodRef<CPlugVertexStream> createdStream =
            MakeMwNod<CPlugVertexStream>();
    CMwNodRef<CMwNod> createdReference(createdStream.Get());
    if (referenceIndex >= references.size()) {
        references.resize(referenceIndex + 1u);
    }
    references[referenceIndex] = createdReference;
    node = createdReference.Get();

    if (!ArchiveVertexStreamNode(archive, *createdStream)) {
        archive.Fail();
        references[referenceIndex] = {};
        node = nullptr;
    }
}

bool VertexStreamArchiveCodec::Capture(
        const CPlugVertexStream &stream,
        VertexStreamArchivePayload &outPayload) {
    VertexStreamArchivePayload payload;
    payload.vertexCount = stream.VertexCount();
    payload.isStatic = stream.IsStatic();
    payload.dirtyVision = stream.DirtyVision();
    payload.isDirty = stream.IsDirty();
    payload.streamModel = stream.StreamModel();
    if (payload.streamModel) {
        outPayload = std::move(payload);
        return true;
    }

    const CPlugVertexStream::SDataDecl *position =
            stream.DataFindByDecl(EPlugVDcl_Position, nullptr);
    const CPlugVertexStream::SDataDecl *normal =
            stream.DataFindByDecl(EPlugVDcl_Normal, nullptr);
    const CPlugVertexStream::SDataDecl *color =
            stream.DataFindByDecl(EPlugVDcl_Color0, nullptr);
    const bool canonicalPosition = position != nullptr &&
            position->Type() == EPlugVDclType_Float3;
    const bool canonicalNormal = normal != nullptr &&
            normal->Type() == EPlugVDclType_Float3;
    const bool canonicalColor = color != nullptr &&
            color->Type() == EPlugVDclType_Float4;
    if (canonicalPosition) {
        std::vector<GxVertex> vertices = stream.CanonicalVertices(
                1, canonicalNormal ? 1 : 0, canonicalColor ? 1 : 0);
        if (vertices.size() != payload.vertexCount) return false;
        VertexStreamArchiveGxBlock gx;
        gx.vertices = std::move(vertices);
        gx.hasNormals = canonicalNormal;
        gx.hasColors = canonicalColor;
        gx.positionsSkipVision = position->SkipVision();
        gx.normalsSkipVision = canonicalNormal && normal->SkipVision();
        gx.colorsSkipVision = canonicalColor && color->SkipVision();
        payload.canonicalGx = std::move(gx);
    }

    for (const CPlugVertexStream::SDataDecl &declaration :
         stream.DataDeclarations()) {
        if ((canonicalPosition &&
             declaration.Declaration() == EPlugVDcl_Position) ||
            (canonicalNormal &&
             declaration.Declaration() == EPlugVDcl_Normal) ||
            (canonicalColor &&
             declaration.Declaration() == EPlugVDcl_Color0)) {
            continue;
        }
        VertexStreamArchiveAttributeBlock block;
        block.attributes.push_back({
            declaration.Declaration(), declaration.Type(),
            declaration.Space(), declaration.SkipVision(),
            declaration.Values(),
        });
        payload.attributeBlocks.push_back(std::move(block));
    }
    outPayload = std::move(payload);
    return true;
}

bool VertexStreamArchiveCodec::Restore(
        const VertexStreamArchivePayload &payload,
        CPlugVertexStream &stream) {
    if (payload.streamModel) {
        if (payload.canonicalGx.has_value() ||
            !payload.attributeBlocks.empty() ||
            payload.streamModel.Get() == &stream) {
            return false;
        }
        stream.SetStreamModel(payload.streamModel.Get());
        if (stream.StreamModel() != payload.streamModel.Get()) return false;
        stream.SetIsStatic(payload.isStatic ? 1 : 0);
        stream.SetDirtyVision(payload.dirtyVision ? 1 : 0);
        stream.SetDirty(payload.isDirty ? 1 : 0);
        return true;
    }

    if (payload.vertexCount > MaximumVertexCount) return false;

    std::vector<CPlugVertexStream::SDataDecl> declarations;
    if (payload.canonicalGx.has_value()) {
        const VertexStreamArchiveGxBlock &gx = *payload.canonicalGx;
        if (gx.vertices.size() != payload.vertexCount) return false;
        std::vector<GmVec3> positions(payload.vertexCount);
        std::vector<GmVec3> normals(gx.hasNormals ? payload.vertexCount : 0u);
        std::vector<GmVec4> colors(gx.hasColors ? payload.vertexCount : 0u);
        for (unsigned long index = 0u; index < payload.vertexCount; ++index) {
            positions[index] = gx.vertices[index].position;
            if (gx.hasNormals) normals[index] = gx.vertices[index].normal;
            if (gx.hasColors) {
                colors[index] = {gx.vertices[index].color[0],
                                 gx.vertices[index].color[1],
                                 gx.vertices[index].color[2],
                                 gx.vertices[index].color[3]};
            }
        }
        declarations.emplace_back(
                EPlugVDcl_Position, EPlugVDclType_Float3,
                EPlugVDclSpace::Point, gx.positionsSkipVision,
                VertexAttributeValues(std::move(positions)));
        if (gx.hasNormals) {
            declarations.emplace_back(
                    EPlugVDcl_Normal, EPlugVDclType_Float3,
                    EPlugVDclSpace::Direction, gx.normalsSkipVision,
                    VertexAttributeValues(std::move(normals)));
        }
        if (gx.hasColors) {
            declarations.emplace_back(
                    EPlugVDcl_Color0, EPlugVDclType_Float4,
                    EPlugVDclSpace::Invariant, gx.colorsSkipVision,
                    VertexAttributeValues(std::move(colors)));
        }
    }
    for (const VertexStreamArchiveAttributeBlock &archiveBlock :
         payload.attributeBlocks) {
        if (archiveBlock.attributes.empty()) return false;
        for (const VertexStreamArchiveAttribute &attribute :
             archiveBlock.attributes) {
            VertexAttributeValues values = attribute.values;
            if (!PlugVDclIsValid(attribute.declaration) ||
                !PlugVDclTypeIsMaterialized(attribute.type) ||
                attribute.space != PlugVDclSpace(attribute.declaration) ||
                values.Type() != attribute.type ||
                values.Count() != payload.vertexCount) {
                return false;
            }
            declarations.emplace_back(
                    attribute.declaration, attribute.type, attribute.space,
                    attribute.skipVision, std::move(values));
        }
    }
    if (!stream.ReplaceContent(
                payload.vertexCount, std::move(declarations))) {
        return false;
    }
    stream.SetIsStatic(payload.isStatic ? 1 : 0);
    stream.SetDirtyVision(payload.dirtyVision ? 1 : 0);
    stream.SetDirty(payload.isDirty ? 1 : 0);
    return true;
}

bool VertexStreamArchiveCodec::Archive(
        CClassicArchive &archive,
        CPlugVertexStream &stream) {
    try {
    VertexStreamArchivePayload payload;
    std::vector<RawDeclaration> rows;
    unsigned long vertexCount = 0u;
    std::uint32_t flags = 0u;

    if (archive.IsWriting()) {
        if (!Capture(stream, payload)) return false;
        vertexCount = payload.vertexCount;
        flags = (payload.isStatic ? StreamFlagStatic : 0u) |
                (payload.isDirty ? StreamFlagDirty : 0u) |
                (payload.dirtyVision ? StreamFlagDirtyVision : 0u);
    }

    archive.DoNatural(&vertexCount, 1u, 0);
    if (!ArchiveRawU32(archive, flags)) {
        return false;
    }
    if (!archive.IsWriting()) {
        payload.vertexCount = vertexCount;
        payload.isStatic = (flags & StreamFlagStatic) != 0u;
        payload.isDirty = (flags & StreamFlagDirty) != 0u;
        payload.dirtyVision = (flags & StreamFlagDirtyVision) != 0u;
    }

    archive.MwDoNodRef(payload.streamModel);
    if (!archive.Good()) return false;
    if (payload.streamModel) {
        return archive.IsWriting() || Restore(payload, stream);
    }
    if (vertexCount > MaximumVertexCount) return false;

    unsigned long declarationCount = 0u;
    if (archive.IsWriting()) {
        if (!BuildRawRows(payload, rows)) return false;
        declarationCount = rows.size();
    }
    archive.DoNatural(&declarationCount, 1u, 0);
    if (!archive.Good() || declarationCount > MaximumDeclarationCount) {
        return false;
    }
    if (!archive.IsWriting()) rows.resize(declarationCount);

    for (RawDeclaration &row : rows) {
        std::uint32_t packed = archive.IsWriting()
                ? PackDeclaration(row)
                : 0u;
        if (!ArchiveRawU32(archive, packed)) return false;
        if (!archive.IsWriting() &&
            !UnpackDeclaration(packed, row)) {
            return false;
        }
        if (row.ownsStorage) {
            if (row.stride != 0u &&
                vertexCount >
                        std::numeric_limits<std::uint32_t>::max() / row.stride) {
                return false;
            }
            const std::size_t byteCount =
                    static_cast<std::size_t>(vertexCount) * row.stride;
            if (!archive.IsWriting()) row.ownedBytes.resize(byteCount);
            archive.DoData(row.ownedBytes.data(), byteCount);
        } else {
            unsigned short link[2] = {row.ownerIndex, row.byteOffset};
            archive.DoNat16(&link[0], 1u, 0);
            archive.DoNat16(&link[1], 1u, 0);
            if (!archive.IsWriting()) {
                row.ownerIndex = link[0];
                row.byteOffset = link[1];
            }
        }
        if (!archive.Good()) return false;
    }

    if (archive.IsWriting()) return true;
    return RawRowsToPayload(rows, payload) && Restore(payload, stream);
    } catch (const std::bad_alloc &) {
        return false;
    } catch (const std::length_error &) {
        return false;
    }
}
