#include "format/materials/material_archive_decoder.h"
#include "format/materials/material_archive_schema.h"
#include "format/materials/material_surface_definition_format_access.h"
#include "format/archive/archive_node_reference.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/mw_id_archive_codec.h"
#include "format/archive/tmnf_gbx_body_reader.h"
#include "format/static_solid/static_solid_archive_shader_chunk_ids.h"
#include <new>
#include <vector>
namespace {

constexpr u32 CPlugMaterialCustomChunkIntArray = 0x0903a004u;
constexpr u32 CPlugMaterialCustomChunkBitmaps = 0x0903a006u;
constexpr u32 CPlugMaterialCustomChunkGpuFx = 0x0903a00au;
constexpr u32 CPlugMaterialCustomChunkBitmapSkip = 0x0903a00cu;
constexpr u32 CPlugMaterialCustomChunkFlags = 0x0903a00du;
constexpr u32 CPlugMaterialCustomChunkFloats = 0x0903a00fu;
constexpr u32 CPlugMaterialCustomChunkLegacySkip = 0x0903a011u;
constexpr u32 MaxMaterialArchiveArrayCount = 0x100000u;
constexpr u32 MaxMaterialArchiveStringBytes = 0x10000u;

u32 ReadU32(const unsigned char *bytes) {
    return static_cast<u32>(bytes[0]) |
           (static_cast<u32>(bytes[1]) << 8u) |
           (static_cast<u32>(bytes[2]) << 16u) |
           (static_cast<u32>(bytes[3]) << 24u);
}

class MaterialArchiveCursor;

enum class MaterialArchiveIdEncodingMode : u32 {
    Unknown = 0u,
    TextTagged = 1u,
    InlineNames = 2u,
    SharedNames = 3u,
};

class MaterialArchiveIdState {
public:
    bool SkipText(MaterialArchiveCursor &cursor);

private:
    MaterialArchiveIdEncodingMode mode_ =
            MaterialArchiveIdEncodingMode::Unknown;
    u32 sharedNameCount_ = 0u;
};

class MaterialArchiveCursor {
public:
    MaterialArchiveCursor(const unsigned char *bytes,
                          u32 byteCount,
                          const GbxBodyReferenceTable &references)
            : bytes_(bytes),
              byteCount_(byteCount),
              offset_(references.bodyOffset),
              references_(references) {
        try {
            internalNodesSeen_.resize(
                    static_cast<std::size_t>(references.nodeCount) + 1u,
                    0u);
            ready_ = true;
        } catch (const std::bad_alloc &) {
            ready_ = false;
        }
    }

    bool ReadWord(u32 &value) {
        if (Remaining() < 4u) {
            return false;
        }
        value = ReadU32(bytes_ + offset_);
        offset_ += 4u;
        return true;
    }

    bool ReadByte(unsigned char &value) {
        if (Remaining() == 0u) {
            return false;
        }
        value = bytes_[offset_++];
        return true;
    }

    bool SkipWord() {
        u32 ignored = 0u;
        return ReadWord(ignored);
    }

    bool SkipBytes(u32 byteCount) {
        if (byteCount > Remaining()) {
            return false;
        }
        offset_ += byteCount;
        return true;
    }

    bool SkipString() {
        u32 byteCount = 0u;
        return ReadWord(byteCount) &&
               byteCount <= MaxMaterialArchiveStringBytes &&
               SkipBytes(byteCount);
    }

    bool SkipNodeReference(bool *isNullReference);
    bool SkipNodeReference();
    bool SkipFidReference();
    bool SkipDeviceSets(u32 chunk);
    bool SkipSizedBlock();
    bool SkipCPlugShaderApplyArchive();
    bool SkipCPlugBitmapApplyArchive();
    bool SkipCPlugMaterialCustomArchive();

    bool IsReady() const {
        return ready_;
    }

    u32 Remaining() const {
        return offset_ <= byteCount_ ? byteCount_ - offset_ : 0u;
    }

    u32 PeekWord() const {
        return Remaining() >= 4u ? ReadU32(bytes_ + offset_) : 0u;
    }

private:
    const GbxBodyExternalReference *ExternalReference(u32 nodeIndex) const;
    bool SkipFormatArray();
    bool SkipNodeReferenceArray();

    const unsigned char *bytes_ = nullptr;
    u32 byteCount_ = 0u;
    u32 offset_ = 0u;
    const GbxBodyReferenceTable &references_;
    std::vector<unsigned char> internalNodesSeen_;
    bool ready_ = false;
};

bool MaterialArchiveIdState::SkipText(MaterialArchiveCursor &cursor) {
    u32 word = 0u;
    if (!cursor.ReadWord(word)) {
        return false;
    }
    if (mode_ == MaterialArchiveIdEncodingMode::Unknown &&
        word >= 1u && word <= 3u) {
        mode_ = static_cast<MaterialArchiveIdEncodingMode>(word);
        if (!cursor.ReadWord(word)) {
            return false;
        }
    }
    if (mode_ == MaterialArchiveIdEncodingMode::TextTagged) {
        return word == 0u || (word <= 3u && cursor.SkipString());
    }

    const TmnfFormat::ArchiveIdentifierWord id =
            TmnfFormat::CMwIdArchiveCodec::ParseWord(word);
    if (!id.IsNamed()) {
        return true;
    }
    if (mode_ == MaterialArchiveIdEncodingMode::SharedNames &&
        id.payload != 0u) {
        return id.payload <= sharedNameCount_;
    }
    if (mode_ == MaterialArchiveIdEncodingMode::Unknown &&
        id.payload != 0u) {
        return false;
    }
    if (!cursor.SkipString()) {
        return false;
    }
    if (mode_ == MaterialArchiveIdEncodingMode::SharedNames) {
        if (sharedNameCount_ >= MaxMaterialArchiveArrayCount) {
            return false;
        }
        ++sharedNameCount_;
    }
    return true;
}

const GbxBodyExternalReference *MaterialArchiveCursor::ExternalReference(
        u32 nodeIndex) const {
    for (const GbxBodyExternalReference &reference :
         references_.externalReferences) {
        if (reference.nodeIndex == nodeIndex) {
            return &reference;
        }
    }
    return nullptr;
}

bool MaterialArchiveCursor::SkipNodeReference(
        bool *isNullReference) {
    u32 nodeIndex = 0u;
    if (!ReadWord(nodeIndex)) {
        return false;
    }
    const bool isNull = nodeIndex == UINT32_MAX;
    if (isNullReference != nullptr) {
        *isNullReference = isNull;
    }
    if (isNull) {
        return true;
    }
    if (nodeIndex == 0u || nodeIndex > references_.nodeCount) {
        return false;
    }
    if (ExternalReference(nodeIndex) != nullptr) {
        return true;
    }
    if (nodeIndex >= internalNodesSeen_.size()) {
        return false;
    }
    if (internalNodesSeen_[nodeIndex] != 0u) {
        return true;
    }
    u32 classId = 0u;
    if (!ReadWord(classId)) {
        return false;
    }
    internalNodesSeen_[nodeIndex] = 1u;
    switch (classId) {
    case TMNF_CLASS_CPlugMaterialCustom:
        return SkipCPlugMaterialCustomArchive();
    case TMNF_CLASS_CPlugShaderApply:
        return SkipCPlugShaderApplyArchive();
    case TMNF_CLASS_CPlugBitmapApply:
        return SkipCPlugBitmapApplyArchive();
    default:
        return false;
    }
}

bool MaterialArchiveCursor::SkipNodeReference() {
    return SkipNodeReference(nullptr);
}

bool MaterialArchiveCursor::SkipFidReference() {
    u32 nodeIndex = 0u;
    if (!ReadWord(nodeIndex)) {
        return false;
    }
    if (nodeIndex == UINT32_MAX) {
        return true;
    }
    return nodeIndex != 0u &&
           nodeIndex != ArchiveNodeReference::DeferredIndex &&
           nodeIndex <= references_.nodeCount &&
           ExternalReference(nodeIndex) != nullptr;
}

bool MaterialArchiveCursor::SkipFormatArray() {
    u32 count = 0u;
    if (!ReadWord(count) || count > Remaining() / 4u) {
        return false;
    }
    offset_ += count * 4u;
    return true;
}

bool MaterialArchiveCursor::SkipNodeReferenceArray() {
    u32 count = 0u;
    if (!ReadWord(count) || count > Remaining() / 4u) {
        return false;
    }
    for (u32 index = 0u; index < count; ++index) {
        if (!SkipNodeReference()) {
            return false;
        }
    }
    return true;
}

bool MaterialArchiveCursor::SkipCPlugShaderApplyArchive() {
    for (;;) {
        u32 chunk = 0u;
        if (!ReadWord(chunk)) {
            return false;
        }
        if (chunk == CMwNodArchiveFacadeSentinel) {
            return true;
        }
        switch (chunk) {
        case ArchiveChunkIdValue(
                CPlugShaderArchiveChunkId::LegacyCustomShader): {
            u32 customShaderCount = 0u;
            if (!SkipNodeReference() || !SkipNodeReferenceArray() ||
                !SkipNodeReference() || !ReadWord(customShaderCount) ||
                customShaderCount > Remaining() / 4u) {
                return false;
            }
            for (u32 index = 0u; index < customShaderCount; ++index) {
                if (!SkipNodeReference()) {
                    return false;
                }
            }
            break;
        }
        case ArchiveChunkIdValue(
                CPlugShaderArchiveChunkId::RequirementsWithNat16):
            if (!SkipBytes(12u) || !SkipNodeReference() || !SkipBytes(2u)) {
                return false;
            }
            break;
        case ArchiveChunkIdValue(
                CPlugShaderGenericArchiveChunkId::Material):
            if (!SkipBytes(0x58u)) {
                return false;
            }
            break;
        case ArchiveChunkIdValue(
                CPlugShaderApplyArchiveChunkId::TextureApplyRefs):
            if (!SkipNodeReferenceArray()) {
                return false;
            }
            break;
        case ArchiveChunkIdValue(
                CPlugShaderApplyArchiveChunkId::ApplyField):
            if (!SkipBytes(4u)) {
                return false;
            }
            break;
        case ArchiveChunkIdValue(
                CPlugShaderApplyArchiveChunkId::ApplyFields):
            if (!SkipBytes(8u)) {
                return false;
            }
            break;
        default:
            return false;
        }
    }
}

bool MaterialArchiveCursor::SkipCPlugBitmapApplyArchive() {
    MaterialArchiveIdState ids;
    u32 chunk = 0u;
    unsigned char hasTransform = 0u;
    return ReadWord(chunk) &&
           chunk == ArchiveChunkIdValue(
                            CPlugBitmapSamplerArchiveChunkId::Bitmap) &&
           ids.SkipText(*this) && SkipNodeReference() && SkipBytes(8u) &&
           ReadWord(chunk) &&
           chunk == ArchiveChunkIdValue(
                            CPlugBitmapAddressArchiveChunkId::Transform) &&
           SkipBytes(4u) && SkipNodeReference() && ReadByte(hasTransform) &&
           hasTransform <= 1u &&
           (hasTransform == 0u || SkipBytes(24u)) && ReadWord(chunk) &&
           chunk == ArchiveChunkIdValue(
                            CPlugBitmapAddressArchiveChunkId::BumpEnvScale) &&
           SkipBytes(4u) && ReadWord(chunk) &&
           chunk == ArchiveChunkIdValue(
                            CPlugBitmapApplyArchiveChunkId::ApplyFlags) &&
           SkipBytes(4u) && ReadWord(chunk) &&
           chunk == CMwNodArchiveFacadeSentinel;
}

bool MaterialArchiveCursor::SkipCPlugMaterialCustomArchive() {
    MaterialArchiveIdState ids;
    for (;;) {
        u32 chunk = 0u;
        if (!ReadWord(chunk)) {
            return false;
        }
        if (chunk == CMwNodArchiveFacadeSentinel) {
            return true;
        }
        switch (chunk) {
        case CPlugMaterialCustomChunkIntArray: {
            u32 count = 0u;
            if (!ReadWord(count) || count > MaxMaterialArchiveArrayCount ||
                !SkipBytes(count * 4u)) {
                return false;
            }
            break;
        }
        case CPlugMaterialCustomChunkBitmaps: {
            u32 count = 0u;
            if (!ReadWord(count) || count > MaxMaterialArchiveArrayCount) {
                return false;
            }
            for (u32 index = 0u; index < count; ++index) {
                if (!ids.SkipText(*this) || !SkipWord() ||
                    !SkipNodeReference()) {
                    return false;
                }
            }
            break;
        }
        case CPlugMaterialCustomChunkGpuFx:
            for (u32 arrayIndex = 0u; arrayIndex < 2u; ++arrayIndex) {
                u32 count = 0u;
                if (!ReadWord(count) || count > MaxMaterialArchiveArrayCount) {
                    return false;
                }
                for (u32 index = 0u; index < count; ++index) {
                    u32 componentCount = 0u;
                    u32 registerCount = 0u;
                    u32 enabled = 0u;
                    if (!ids.SkipText(*this) ||
                        !ReadWord(componentCount) ||
                        !ReadWord(registerCount) || !ReadWord(enabled) ||
                        enabled > 1u ||
                        componentCount > MaxMaterialArchiveArrayCount ||
                        registerCount > MaxMaterialArchiveArrayCount ||
                        (componentCount != 0u &&
                         registerCount > UINT32_MAX / componentCount) ||
                        componentCount * registerCount > UINT32_MAX / 4u ||
                        !SkipBytes(componentCount * registerCount * 4u)) {
                        return false;
                    }
                }
            }
            break;
        case CPlugMaterialCustomChunkBitmapSkip: {
            u32 count = 0u;
            if (!ReadWord(count) || count > MaxMaterialArchiveArrayCount) {
                return false;
            }
            for (u32 index = 0u; index < count; ++index) {
                u32 enabled = 0u;
                if (!ids.SkipText(*this) || !ReadWord(enabled) ||
                    enabled > 1u) {
                    return false;
                }
            }
            break;
        }
        case CPlugMaterialCustomChunkFlags: {
            u32 flags = 0u;
            if (!ReadWord(flags) || !SkipBytes(12u) ||
                ((flags & 1u) != 0u && !SkipBytes(4u))) {
                return false;
            }
            break;
        }
        case CPlugMaterialCustomChunkFloats:
            if (!SkipSizedBlock()) {
                return false;
            }
            break;
        case CPlugMaterialCustomChunkLegacySkip: {
            u32 marker = 0u;
            u32 byteCount = 0u;
            if (!ReadWord(marker) || marker != MaterialArchiveSkipBlockMarker ||
                !ReadWord(byteCount) || byteCount != 4u ||
                !SkipBytes(byteCount)) {
                return false;
            }
            break;
        }
        default:
            return false;
        }
    }
}

bool MaterialArchiveCursor::SkipDeviceSets(u32 chunk) {
    u32 count = 0u;
    if (!ReadWord(count) || count > 0x10000000u) {
        return false;
    }
    for (u32 index = 0u; index < count; ++index) {
        u32 activeShaderIsFid = 0u;
        if (!SkipWord() || !ReadWord(activeShaderIsFid) ||
            activeShaderIsFid > 1u ||
            (activeShaderIsFid != 0u
                     ? !SkipFidReference()
                     : !SkipNodeReference())) {
            return false;
        }
        if (MaterialDeviceSetHasShaderRefs(chunk) &&
            (!SkipFidReference() || !SkipFidReference())) {
            return false;
        }
    }
    return !MaterialDeviceSetHasFormats(chunk) || SkipFormatArray();
}

bool MaterialArchiveCursor::SkipSizedBlock() {
    u32 marker = 0u;
    u32 size = 0u;
    if (!ReadWord(marker) || marker != MaterialArchiveSkipBlockMarker ||
        !ReadWord(size) || size > Remaining()) {
        return false;
    }
    offset_ += size;
    return true;
}

}  // namespace

std::optional<MaterialSurfaceDefinition> DecodeMaterialArchive(
        const unsigned char *bytes,
        u32 byteCount) {
    u32 classId = 0u;
    GbxBodyReferenceTable references;
    if (bytes == nullptr ||
        !GbxBodyOffsetReader::TryParseWithReferences(
                bytes, byteCount, &classId, &references) ||
        classId != TMNF_CLASS_CPlugMaterial) {
        return std::nullopt;
    }

    MaterialArchiveCursor cursor(bytes, byteCount, references);
    if (!cursor.IsReady()) {
        return std::nullopt;
    }
    u32 surfaceFlags = MaterialArchiveDefaultSurfaceFlags;
    for (;;) {
        u32 chunk = 0u;
        if (!cursor.ReadWord(chunk)) {
            return std::nullopt;
        }
        if (chunk == CMwNodArchiveFacadeSentinel) {
            if (cursor.Remaining() != 0u) {
                return std::nullopt;
            }
            return MaterialSurfaceDefinitionFormatAccess::FromArchiveFlags(
                    surfaceFlags);
        }
        if (IsMaterialNodeReferenceChunk(chunk)) {
            if (!cursor.SkipNodeReference()) {
                return std::nullopt;
            }
            continue;
        }
        if (IsMaterialDeviceSetChunk(chunk)) {
            bool materialModelIsNull = false;
            if (!cursor.SkipNodeReference(
                        &materialModelIsNull)) {
                return std::nullopt;
            }
            if (materialModelIsNull) {
                if (!cursor.SkipDeviceSets(chunk)) {
                    return std::nullopt;
                }
            }
            continue;
        }
        if (chunk == MaterialArchiveChunkValue(
                             MaterialArchiveChunk::SurfaceFlagsReplace) ||
            chunk == MaterialArchiveChunkValue(
                             MaterialArchiveChunk::SurfaceFlagsOr) ||
            chunk == MaterialArchiveChunkValue(
                             MaterialArchiveChunk::SurfaceFlags)) {
            if (!cursor.ReadWord(surfaceFlags)) {
                return std::nullopt;
            }
            surfaceFlags = NormalizeMaterialSurfaceFlags(surfaceFlags, chunk);
            continue;
        }
        if (chunk == MaterialArchiveChunkValue(
                             MaterialArchiveChunk::FourByteLegacyPayload)) {
            if (!cursor.SkipWord()) {
                return std::nullopt;
            }
            continue;
        }
        if (cursor.Remaining() >= 8u &&
            cursor.PeekWord() == MaterialArchiveSkipBlockMarker) {
            if (!cursor.SkipSizedBlock()) {
                return std::nullopt;
            }
            continue;
        }
        return std::nullopt;
    }
}
