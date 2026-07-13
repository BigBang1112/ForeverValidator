#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "format/vehicle_tuning/curve_reader.h"
#include "format/vehicle_tuning/chunk_schema.h"
#include <new>
class VehicleTuningCurveAssembly {
public:
    int ReadXValues(VehicleTuningArchiveCursor &cursor) {
        hasX = cursor.ReadFloatArray(&xKeys);
        return hasX;
    }

    int ReadYValuesAndMode(VehicleTuningArchiveCursor &cursor) {
        hasY = cursor.ReadFloatArray(&values);
        if (!hasY || !cursor.ReadNatural(&interpolationMode)) {
            return 0;
        }
        hasMode = 1;
        return 1;
    }

    int Install(VehicleTuningCurveRecord &record) const {
        if (!hasX || !hasY || !hasMode || xKeys.size() != values.size()) {
            return 0;
        }
        record.interpolationMode = interpolationMode;
        try {
            record.keys.reserve(xKeys.size());
            for (std::size_t i = 0u; i < xKeys.size(); ++i) {
                record.keys.push_back({xKeys[i], values[i]});
            }
        } catch (const std::bad_alloc &) {
            record.keys.clear();
            return 0;
        }
        return 1;
    }

private:
    int hasX = 0;
    int hasY = 0;
    int hasMode = 0;
    u32 interpolationMode = 0u;
    std::vector<float> xKeys;
    std::vector<float> values;
};

static int ReadFunctionCurve(
        VehicleTuningArchiveCursor &cursor,
        VehicleTuningCurveRecord *record) {
    VehicleTuningCurveAssembly assembly;
    for (;;) {
        u32 chunkId = 0u;
        if (!cursor.ReadArchiveWord(&chunkId)) {
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            return assembly.Install(*record);
        }
        if (chunkId == ArchiveChunkIdValue(FunctionCurveChunkId::XValues)) {
            if (!assembly.ReadXValues(cursor)) {
                return 0;
            }
        } else if (chunkId == ArchiveChunkIdValue(FunctionCurveChunkId::Id)) {
            if (!cursor.SkipNatural()) {
                return 0;
            }
        } else if (chunkId ==
                   ArchiveChunkIdValue(FunctionCurveChunkId::YValuesAndMode)) {
            if (!assembly.ReadYValuesAndMode(cursor)) {
                return 0;
            }
        } else {
            return 0;
        }
    }
}

static int ReadCurveReference(
        VehicleTuningArchiveCursor &cursor,
        u32 chunkId,
        u32 nodRefIndex,
        std::vector<VehicleTuningCurveRecord> *records) {
    u32 index = 0u;
    if (!cursor.ReadNatural(&index)) {
        return 0;
    }
    if (index == 0xffffffffu) {
        return 1;
    }
    u32 classId = 0u;
    if (!cursor.ReadArchiveWord(&classId) ||
        classId != ArchiveChunkIdValue(FunctionCurveChunkId::Root) ||
        records == 0 ||
        records->size() >= MaxVehicleTuningCurveCount) {
        return 0;
    }
    VehicleTuningCurveRecord record{};
    record.chunk = VehicleTuningChunkWord::FromArchiveWord(
            chunkId);
    record.nodRefIndex = nodRefIndex;
    if (!ReadFunctionCurve(cursor, &record)) {
        return 0;
    }
    try {
        records->push_back(record);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    return 1;
}

int ReadVehicleTuningCurveChunk(
        const VehicleTuningArchiveDocument &archive,
        size_t *offset,
        std::vector<VehicleTuningCurveRecord> *records,
        int *sawFacade) {
    VehicleTuningArchiveCursor cursor(&archive, *offset);
    u32 chunkId = 0u;
    if (!cursor.ReadArchiveWord(&chunkId)) {
        return 0;
    }
    if (chunkId == CMwNodArchiveFacadeSentinel) {
        *sawFacade = 1;
        *offset = cursor.Offset();
        return 1;
    }
    const VehicleTuningChunkSchema *schema =
            FindVehicleTuningChunkSchema(chunkId);
    if (schema == nullptr) {
        return 0;
    }
    u32 nodRefIndex = 0u;
    for (size_t i = 0u; i < schema->fieldCount; i++) {
        const VehicleTuningFieldKind field = schema->fields[i];
        if (field == VehicleTuningFieldKind::Real ||
            field == VehicleTuningFieldKind::Natural ||
            field == VehicleTuningFieldKind::Boolean ||
            field == VehicleTuningFieldKind::CmwId) {
            if (!cursor.SkipNatural()) {
                return 0;
            }
        } else if (field == VehicleTuningFieldKind::NodRef) {
            if (!ReadCurveReference(
                    cursor,
                    chunkId,
                    nodRefIndex,
                    records)) {
                return 0;
            }
            nodRefIndex++;
        } else if (field == VehicleTuningFieldKind::FloatArray) {
            if (!cursor.SkipFloatArray()) {
                return 0;
            }
        } else {
            return 0;
        }
    }
    *offset = cursor.Offset();
    return 1;
}
