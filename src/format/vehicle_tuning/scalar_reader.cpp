#include "format/vehicle_tuning/scalar_reader.h"
#include "format/vehicle_tuning/chunk_schema.h"
int ReadVehicleTuningScalarChunk(
        const VehicleTuningArchiveDocument &archive,
        size_t *offset,
        VehicleTuningScalarChunk *out) {
    VehicleTuningArchiveCursor cursor(&archive, *offset);
    u32 rawChunk = 0u;
    if (!cursor.ReadArchiveWord(&rawChunk)) {
        return 0;
    }
    out->chunk = VehicleTuningChunkWord::FromArchiveWord(
            rawChunk);
    if (out->chunk.IsFacadeSentinel()) {
        *offset = cursor.Offset();
        return 1;
    }
    const VehicleTuningChunkSchema *schema =
            FindVehicleTuningChunkSchema(
                    out->chunk.ValueForFormatLookup());
    if (schema == nullptr) {
        return 0;
    }
    for (size_t i = 0u; i < schema->fieldCount; i++) {
        const VehicleTuningFieldKind field = schema->fields[i];
        if (field == VehicleTuningFieldKind::Real) {
            if (out->reals.size() >= MaxVehicleTuningChunkRealCount) {
                return 0;
            }
            float value = 0.0f;
            if (!cursor.ReadReal(&value)) {
                return 0;
            }
            out->reals.push_back(value);
        } else if (field == VehicleTuningFieldKind::Natural ||
                   field == VehicleTuningFieldKind::Boolean) {
            if (out->naturals.size() >=
                MaxVehicleTuningChunkNaturalCount) {
                return 0;
            }
            uint32_t value = 0u;
            if (!cursor.ReadNatural(&value)) {
                return 0;
            }
            out->naturals.push_back(value);
        } else if (field == VehicleTuningFieldKind::NodRef) {
            if (!cursor.SkipNodRef()) {
                return 0;
            }
        } else if (field == VehicleTuningFieldKind::FloatArray) {
            if (!cursor.SkipFloatArray()) {
                return 0;
            }
        } else if (field == VehicleTuningFieldKind::CmwId) {
            if (!cursor.SkipNatural()) {
                return 0;
            }
        } else {
            return 0;
        }
    }
    *offset = cursor.Offset();
    return 1;
}
