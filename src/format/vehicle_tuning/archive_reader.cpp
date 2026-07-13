#include "format/vehicle_tuning/archive_reader.h"
#include "format/archive/archive_binary32.h"
#include <new>
int VehicleTuningArchiveCursor::ReadArchiveWord(u32 *out) {
    return ReadNatural(out);
}

int VehicleTuningArchiveCursor::ReadNatural(u32 *out) {
    if (archive == nullptr || out == nullptr) {
        return 0;
    }
    int ok = 1;
    *out = archive->ReadU32At(offset, &ok);
    if (!ok) {
        return 0;
    }
    offset += 4u;
    return 1;
}

int VehicleTuningArchiveCursor::ReadReal(float *out) {
    if (archive == nullptr || out == nullptr) {
        return 0;
    }
    int ok = 1;
    *out = archive->ReadF32At(offset, &ok);
    if (!ok) {
        return 0;
    }
    offset += 4u;
    return 1;
}

int VehicleTuningArchiveCursor::SkipNatural() {
    if (archive == nullptr) {
        return 0;
    }
    return archive->SkipU32(&offset);
}

int VehicleTuningArchiveCursor::SkipNodRef() {
    if (archive == nullptr) {
        return 0;
    }
    return archive->SkipNodRef(&offset);
}

int VehicleTuningArchiveCursor::SkipFloatArray(u32 maxCount) {
    if (archive == nullptr) {
        return 0;
    }
    return archive->SkipFloatArray(&offset, maxCount);
}

int VehicleTuningArchiveCursor::ReadFloatArray(
        std::vector<float> *values,
        u32 maxCount) {
    if (archive == nullptr ||
        values == nullptr) {
        return 0;
    }
    values->clear();
    int ok = 1;
    const u32 count = archive->ReadU32At(offset, &ok);
    if (!ok ||
        count > maxCount ||
        offset > archive->Size() ||
        archive->Size() - offset < 4u ||
        count > (archive->Size() - offset - 4u) / 4u) {
        return 0;
    }
    try {
        values->reserve(count);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    offset += 4u;
    for (u32 i = 0u; i < count; i++) {
        const float value = archive->ReadF32At(offset, &ok);
        if (!ok) {
            values->clear();
            return 0;
        }
        values->push_back(value);
        offset += 4u;
    }
    return 1;
}

u32 VehicleTuningArchiveDocument::ReadU32At(size_t offset, int *ok) const {
    if (offset + 4u > bytes.size()) {
        *ok = 0;
        return 0u;
    }
    return ((u32)bytes[offset]) |
           ((u32)bytes[offset + 1u] << 8u) |
           ((u32)bytes[offset + 2u] << 16u) |
           ((u32)bytes[offset + 3u] << 24u);
}

float VehicleTuningArchiveDocument::ReadF32At(size_t offset, int *ok) const {
    return TmnfArchiveBinary32::Decode(ReadU32At(offset, ok));
}

int VehicleTuningArchiveDocument::SkipU32(size_t *offset) const {
    if (*offset + 4u > bytes.size()) {
        return 0;
    }
    *offset += 4u;
    return 1;
}

int VehicleTuningArchiveDocument::SkipCmwId(size_t *offset) const {
    return SkipU32(offset);
}

int VehicleTuningArchiveDocument::SkipFloatArray(size_t *offset,
                                               u32 maxCount) const {
    int ok = 1;
    const u32 count = ReadU32At(*offset, &ok);
    if (!ok ||
        count > maxCount ||
        *offset + 4u + (size_t)count * 4u > bytes.size()) {
        return 0;
    }
    *offset += 4u + (size_t)count * 4u;
    return 1;
}

int VehicleTuningArchiveDocument::SkipNodRef(size_t *offset) const {
    int ok = 1;
    const u32 index = ReadU32At(*offset, &ok);
    *offset += 4u;
    if (!ok) {
        return 0;
    }
    if (index == 0xffffffffu) {
        return 1;
    }
    const u32 classId = ReadU32At(*offset, &ok);
    *offset += 4u;
    if (!ok || classId != ArchiveChunkIdValue(FunctionCurveChunkId::Root)) {
        return 0;
    }
    for (;;) {
        const u32 chunkId = ReadU32At(*offset, &ok);
        if (!ok) {
            return 0;
        }
        *offset += 4u;
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            return 1;
        }
        if (chunkId == ArchiveChunkIdValue(FunctionCurveChunkId::XValues)) {
            if (!SkipFloatArray(offset)) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(FunctionCurveChunkId::Id)) {
            if (!SkipCmwId(offset)) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(FunctionCurveChunkId::YValuesAndMode)) {
            if (!SkipFloatArray(offset) || !SkipU32(offset)) {
                return 0;
            }
        } else {
            return 0;
        }
    }
}

int VehicleTuningArchiveDocument::ScanTuningChunkOffsets(
        std::vector<size_t> *offsets) const {
    offsets->clear();
    try {
        offsets->reserve(tuningCount);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    int ok = 1;
    for (size_t offset = bodyOffset; offset + 56u <= bytes.size(); offset++) {
        const u32 chunkId = ReadU32At(offset, &ok);
        const u32 nextChunkId = ReadU32At(offset + 52u, &ok);
        if (!ok) {
            return 0;
        }
        if (chunkId != ArchiveChunkIdValue(VehicleTuningChunkId::Root) ||
            nextChunkId != ArchiveChunkIdValue(VehicleTuningChunkId::Next)) {
            continue;
        }
        offsets->push_back(offset);
        if (offsets->size() > MaxVehicleTuningCount) {
            return 0;
        }
    }
    return 1;
}

int VehicleTuningArchiveDocument::Load(
        const std::vector<unsigned char> &sourceBytes) {
    if (sourceBytes.empty()) {
        return 1;
    }

    bytes = sourceBytes;
    bodyOffset = 0u;
    activeStart = 0u;
    activeEnd = 0u;
    activeIndex = 0u;
    tuningCount = 0u;

    const size_t size = bytes.size();
    int ok = 1;
    if (size < 25u ||
        bytes[0] != 'G' ||
        bytes[1] != 'B' ||
        bytes[2] != 'X') {
        return 2;
    }
    const u32 classId = ReadU32At(9u, &ok);
    const u32 headerSize = ReadU32At(13u, &ok);
    const size_t headerEnd = 17u + (size_t)headerSize;
    if (!ok ||
        classId != ArchiveChunkIdValue(VehicleTuningContainerChunkId::Root) ||
        headerEnd + 8u > size) {
        return 2;
    }
    const u32 externalCount = ReadU32At(headerEnd + 4u, &ok);
    if (!ok || externalCount != 0u) {
        return 2;
    }
    bodyOffset = headerEnd + 8u;

    const u32 rootChunk = ReadU32At(bodyOffset, &ok);
    const u32 formatMarker = ReadU32At(bodyOffset + 4u, &ok);
    const u32 parsedTuningCount = ReadU32At(bodyOffset + 8u, &ok);
    if (!ok ||
        rootChunk != ArchiveChunkIdValue(VehicleTuningContainerChunkId::Root) ||
        formatMarker != NodeReferenceArrayArchiveFormatMarker ||
        parsedTuningCount == 0u ||
        parsedTuningCount > MaxVehicleTuningCount) {
        return 3;
    }
    if (size < 8u ||
        ReadU32At(size - 4u, &ok) != CMwNodArchiveFacadeSentinel) {
        return 4;
    }
    const u32 parsedActiveIndex = ReadU32At(size - 8u, &ok);
    if (!ok || parsedActiveIndex >= parsedTuningCount) {
        return 5;
    }

    tuningCount = parsedTuningCount;
    activeIndex = parsedActiveIndex;

    std::vector<size_t> tuningOffsets;
    if (!ScanTuningChunkOffsets(&tuningOffsets) ||
        tuningOffsets.size() != (size_t)tuningCount) {
        return 7;
    }

    activeStart = tuningOffsets[activeIndex];
    activeEnd = activeIndex + 1u < tuningCount
        ? tuningOffsets[activeIndex + 1u]
        : size - 8u;
    return 0;
}

int VehicleTuningArchiveDocument::FindActiveChunk(
        VehicleTuningChunkId chunkId,
        size_t *outOffset) const {
    if (outOffset == nullptr) {
        return 0;
    }
    const u32 archiveChunkId = ArchiveChunkIdValue(chunkId);
    int ok = 1;
    for (size_t offset = activeStart; offset + 4u <= activeEnd; offset++) {
        if (ReadU32At(offset, &ok) == archiveChunkId && ok) {
            *outOffset = offset;
            return 1;
        }
    }
    return 0;
}
