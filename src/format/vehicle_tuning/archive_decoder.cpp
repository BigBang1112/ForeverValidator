#include "format/vehicle_tuning/scalar_reader.h"
#include "format/vehicle_tuning/archive_decoder.h"
#include <array>
#include <vector>
#include <new>

static int ReadTransmissionArray(
        const VehicleTuningArchiveDocument &archive,
        size_t *offset,
        TransmissionArrayKind source,
        TransmissionArray *record);

enum class M6FullArchiveSlotKind : u32 {
    Scalar,
    NodRef,
    GearSpeedRatio,
    UpshiftThreshold,
    DownshiftThreshold,
};

static constexpr std::array<M6FullArchiveSlotKind, 33u> M6FullArchiveSlots = {{
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::NodRef,
    M6FullArchiveSlotKind::NodRef,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::NodRef,
    M6FullArchiveSlotKind::NodRef,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::Scalar,
    M6FullArchiveSlotKind::GearSpeedRatio,
    M6FullArchiveSlotKind::UpshiftThreshold,
    M6FullArchiveSlotKind::DownshiftThreshold,
    M6FullArchiveSlotKind::Scalar,
}};

static int SkipScalarSlot(size_t size, size_t *offset) {
    if (*offset + 4u > size) {
        return 0;
    }
    *offset += 4u;
    return 1;
}

static int ReadFullTransmissionSlot(
        const VehicleTuningArchiveDocument &archive,
        size_t *offset,
        M6FullArchiveSlotKind slot,
        std::vector<TransmissionArray> *records) {
    switch (slot) {
        case M6FullArchiveSlotKind::Scalar:
            return SkipScalarSlot(archive.Size(), offset);
        case M6FullArchiveSlotKind::NodRef:
            return archive.SkipNodRef(offset);
        case M6FullArchiveSlotKind::GearSpeedRatio:
        case M6FullArchiveSlotKind::UpshiftThreshold:
        case M6FullArchiveSlotKind::DownshiftThreshold: {
            TransmissionArrayKind source =
                    TransmissionArrayKind::GearSpeedRatio;
            if (slot == M6FullArchiveSlotKind::UpshiftThreshold) {
                source = TransmissionArrayKind::UpshiftThreshold;
            } else if (slot == M6FullArchiveSlotKind::DownshiftThreshold) {
                source = TransmissionArrayKind::DownshiftThreshold;
            }
            TransmissionArray record;
            if (!ReadTransmissionArray(
                        archive,
                        offset,
                        source,
                        &record)) {
                return 0;
            }
            records->push_back(record);
            return 1;
        }
    }
    return 0;
}

static int ReadTransmissionArray(
        const VehicleTuningArchiveDocument &archive,
        size_t *offset,
        TransmissionArrayKind source,
        TransmissionArray *record) {
    int ok = 1;
    u32 count = archive.ReadU32At(*offset, &ok);
    if (!ok ||
        *offset > archive.Size() ||
        archive.Size() - *offset < 4u ||
        count > (archive.Size() - *offset - 4u) / 4u) {
        return 0;
    }
    *record = TransmissionArray{};
    record->source = source;
    try {
        record->values.reserve(count);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    *offset += 4u;
    for (u32 i = 0u; i < count; i++) {
        const float value = archive.ReadF32At(
                *offset,
                &ok);
        if (!ok) {
            return 0;
        }
        record->values.push_back(value);
        *offset += 4u;
    }
    return 1;
}

static int ReadRpmArrayChunk(
        const VehicleTuningArchiveDocument &archive,
        size_t offset,
        std::vector<TransmissionArray> *records) {
    int ok = 1;
    if (archive.ReadU32At(offset, &ok) !=
            ArchiveChunkIdValue(VehicleTuningChunkId::M6FloatBuffer) ||
        !ok ||
        records == nullptr) {
        return 0;
    }
    offset += 4u;
    TransmissionArray record;
    if (!ReadTransmissionArray(
            archive,
            &offset,
            TransmissionArrayKind::RpmWanted,
            &record)) {
        return 0;
    }
    records->push_back(record);
    return 1;
}

static int ReadFullTransmissionArrays(
        const VehicleTuningArchiveDocument &archive,
        size_t offset,
        std::vector<TransmissionArray> *records) {
    int ok = 1;
    if (archive.ReadU32At(offset, &ok) !=
            ArchiveChunkIdValue(VehicleTuningChunkId::M6Full) ||
        !ok ||
        records == nullptr) {
        return 0;
    }
    offset += 4u;
    for (M6FullArchiveSlotKind slot : M6FullArchiveSlots) {
        if (!ReadFullTransmissionSlot(
                    archive,
                    &offset,
                    slot,
                    records)) {
            return 0;
        }
    }
    return 1;
}

int ReadVehicleTuningScalars(
        const VehicleTuningArchiveDocument &archive,
        std::vector<VehicleTuningScalarChunk> &outChunks) {
    outChunks.clear();
    try {
        outChunks.reserve(MaxVehicleTuningChunkCount);
    } catch (const std::bad_alloc &) {
        return 3;
    }

    VehicleTuningScalarChunk baseChunk{};
    baseChunk.chunk = VehicleTuningChunkWord::
            FromArchiveWord(ArchiveChunkIdValue(
                    VehicleTuningBaseChunkId::Base));
    outChunks.push_back(baseChunk);

    size_t offset = archive.ActiveStart();
    int sawFacade = 0;
    while (offset < archive.ActiveEnd() &&
           outChunks.size() < MaxVehicleTuningChunkCount) {
        VehicleTuningScalarChunk chunk{};
        if (!ReadVehicleTuningScalarChunk(
                archive,
                &offset,
                &chunk)) {
            outChunks.clear();
            return 1;
        }
        outChunks.push_back(chunk);
        if (chunk.chunk.IsFacadeSentinel()) {
            sawFacade = 1;
            break;
        }
    }
    if (!sawFacade) {
        outChunks.clear();
        return 2;
    }
    return 0;
}

int ReadVehicleTransmissionArrays(
        const VehicleTuningArchiveDocument &archive,
        std::vector<TransmissionArray> &outRecords) {
    outRecords.clear();
    try {
        outRecords.reserve(4u);
    } catch (const std::bad_alloc &) {
        return 2;
    }

    size_t offset057 = 0u;
    size_t offset05d = 0u;
    if (!archive.FindActiveChunk(
                VehicleTuningChunkId::M6FloatBuffer,
                &offset057) ||
        !archive.FindActiveChunk(
                VehicleTuningChunkId::M6Full,
                &offset05d) ||
        !ReadRpmArrayChunk(archive, offset057, &outRecords) ||
        !ReadFullTransmissionArrays(archive, offset05d, &outRecords)) {
        outRecords.clear();
        return 1;
    }
    if (outRecords.size() != 4u) {
        outRecords.clear();
        return 3;
    }
    return 0;
}
