#include "format/materials/material_archive_decoder.h"
#include "format/materials/material_archive_schema.h"
#include "format/materials/material_surface_definition_format_access.h"
#include "format/archive/archive_class_ids.h"
#include "format/archive/tmnf_gbx_body_reader.h"
namespace {

u32 ReadU32(const unsigned char *bytes) {
    return static_cast<u32>(bytes[0]) |
           (static_cast<u32>(bytes[1]) << 8u) |
           (static_cast<u32>(bytes[2]) << 16u) |
           (static_cast<u32>(bytes[3]) << 24u);
}

class MaterialArchiveCursor {
public:
    MaterialArchiveCursor(const unsigned char *bytes,
                          u32 byteCount,
                          u32 offset)
            : bytes_(bytes), byteCount_(byteCount), offset_(offset) {}

    bool ReadWord(u32 &value) {
        if (Remaining() < 4u) {
            return false;
        }
        value = ReadU32(bytes_ + offset_);
        offset_ += 4u;
        return true;
    }

    bool SkipWord() {
        u32 ignored = 0u;
        return ReadWord(ignored);
    }

    bool SkipNodeReference(bool *isNull);
    bool SkipNodeReference();
    bool SkipDeviceSets(u32 chunk);
    bool SkipToNextMaterialChunk();
    bool SkipSizedBlock();
    bool SkipInlineCustomPayload();
    bool SkipInlineArchivePayload();

    u32 Remaining() const {
        return offset_ <= byteCount_ ? byteCount_ - offset_ : 0u;
    }

    u32 PeekWord() const {
        return Remaining() >= 4u ? ReadU32(bytes_ + offset_) : 0u;
    }

    u32 Position() const {
        return offset_;
    }

    bool Seek(u32 offset) {
        if (offset > byteCount_) {
            return false;
        }
        offset_ = offset;
        return true;
    }

private:
    bool SkipFormatArray();

    const unsigned char *bytes_ = nullptr;
    u32 byteCount_ = 0u;
    u32 offset_ = 0u;
};

bool MaterialArchiveCursor::SkipInlineCustomPayload() {
    if (Remaining() < 4u) {
        return false;
    }
    for (u32 cursor = offset_; cursor <= byteCount_ - 4u; ++cursor) {
        const u32 word = ReadU32(bytes_ + cursor);
        if (word == CMwNodArchiveFacadeSentinel) {
            offset_ = cursor + 4u;
            return true;
        }
        if (IsMaterialArchiveChunk(word)) {
            offset_ = cursor;
            return true;
        }
    }
    return false;
}

bool MaterialArchiveCursor::SkipInlineArchivePayload() {
    if (Remaining() < 4u) {
        return false;
    }
    for (u32 cursor = offset_; cursor <= byteCount_ - 4u; ++cursor) {
        if (ReadU32(bytes_ + cursor) == CMwNodArchiveFacadeSentinel) {
            offset_ = cursor + 4u;
            return true;
        }
    }
    return false;
}

bool MaterialArchiveCursor::SkipNodeReference(bool *isNull) {
    u32 nodeIndex = 0u;
    if (!ReadWord(nodeIndex)) {
        return false;
    }
    const bool nullReference = nodeIndex == UINT32_MAX;
    if (isNull != nullptr) {
        *isNull = nullReference;
    }
    if (nullReference || Remaining() < 4u ||
        IsMaterialArchiveChunk(PeekWord())) {
        return true;
    }
    if (PeekWord() == TMNF_CLASS_CPlugMaterialCustom) {
        if (!SkipWord()) {
            return false;
        }
    }
    return SkipInlineCustomPayload();
}

bool MaterialArchiveCursor::SkipNodeReference() {
    return SkipNodeReference(nullptr);
}

bool MaterialArchiveCursor::SkipFormatArray() {
    u32 count = 0u;
    if (!ReadWord(count) || count > Remaining() / 4u) {
        return false;
    }
    offset_ += count * 4u;
    return true;
}

bool MaterialArchiveCursor::SkipDeviceSets(u32 chunk) {
    u32 count = 0u;
    if (!ReadWord(count) || count > 0x10000000u) {
        return false;
    }
    for (u32 index = 0u; index < count; ++index) {
        u32 hasInlineShader = 0u;
        if (!SkipWord() || !ReadWord(hasInlineShader)) {
            return false;
        }
        if (hasInlineShader != 0u) {
            if (!SkipInlineArchivePayload()) {
                return false;
            }
        } else if (!SkipNodeReference()) {
            return false;
        }
        if (MaterialDeviceSetHasShaderRefs(chunk) &&
            (!SkipNodeReference() || !SkipNodeReference())) {
            return false;
        }
    }
    return !MaterialDeviceSetHasFormats(chunk) || SkipFormatArray();
}

bool MaterialArchiveCursor::SkipToNextMaterialChunk() {
    while (Remaining() >= 4u) {
        const u32 word = PeekWord();
        if (word == CMwNodArchiveFacadeSentinel ||
            IsMaterialArchiveChunk(word)) {
            return true;
        }
        offset_ += 4u;
    }
    return false;
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
    u32 bodyOffset = 0u;
    if (bytes == nullptr ||
        !GbxBodyOffsetReader::TryParse(
                bytes, byteCount, &classId, &bodyOffset) ||
        classId != TMNF_CLASS_CPlugMaterial) {
        return std::nullopt;
    }

    MaterialArchiveCursor cursor(bytes, byteCount, bodyOffset);
    u32 surfaceFlags = MaterialArchiveDefaultSurfaceFlags;
    for (;;) {
        u32 chunk = 0u;
        if (!cursor.ReadWord(chunk)) {
            return std::nullopt;
        }
        if (chunk == CMwNodArchiveFacadeSentinel) {
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
            if (!cursor.SkipNodeReference(&materialModelIsNull)) {
                return std::nullopt;
            }
            if (materialModelIsNull) {
                // CPlugMaterial stores renderer-only device data before the
                // independent physical surface-flags chunks. Nested shader
                // archives are not needed for physics; if their detailed
                // shape is unknown, resume at the next aligned material chunk.
                const u32 deviceSetsOffset = cursor.Position();
                if (!cursor.SkipDeviceSets(chunk) &&
                    (!cursor.Seek(deviceSetsOffset) ||
                     !cursor.SkipToNextMaterialChunk())) {
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
        if (cursor.Remaining() >= 4u &&
            cursor.PeekWord() != MaterialArchiveSkipBlockMarker) {
            return MaterialSurfaceDefinitionFormatAccess::FromArchiveFlags(
                    surfaceFlags);
        }
        return std::nullopt;
    }
}
