#ifndef TMNF_FORMAT_VEHICLE_TUNING_ARCHIVE_READER_H
#define TMNF_FORMAT_VEHICLE_TUNING_ARCHIVE_READER_H

#include "engine/core/engine_types.h"
#include <stddef.h>
#include <vector>

#include "format/vehicle_tuning/archive_ids.h"
constexpr u32 MaxVehicleTuningCount = 256u;

class VehicleTuningArchiveDocument;

class VehicleTuningArchiveCursor {
public:
    VehicleTuningArchiveCursor(const VehicleTuningArchiveDocument *archive,
                               size_t offset)
        : archive(archive), offset(offset) {}

    int ReadArchiveWord(u32 *out);
    int ReadNatural(u32 *out);
    int ReadReal(float *out);
    int SkipNatural();
    int SkipNodRef();
    int SkipFloatArray(u32 maxCount = 4096u);
    int ReadFloatArray(std::vector<float> *values,
                       u32 maxCount = 4096u);

    size_t Offset() const { return offset; }

private:
    const VehicleTuningArchiveDocument *archive = nullptr;
    size_t offset = 0u;
};

class VehicleTuningArchiveDocument {
public:
    int Load(const std::vector<unsigned char> &sourceBytes);

    u32 ReadU32At(size_t offset, int *ok) const;
    float ReadF32At(size_t offset, int *ok) const;

    int SkipU32(size_t *offset) const;
    int SkipCmwId(size_t *offset) const;
    int SkipFloatArray(size_t *offset, u32 maxCount = 4096u) const;
    int SkipNodRef(size_t *offset) const;

    int FindActiveChunk(VehicleTuningChunkId chunkId,
                        size_t *outOffset) const;
    size_t Size() const { return bytes.size(); }
    size_t ActiveStart() const { return activeStart; }
    size_t ActiveEnd() const { return activeEnd; }

private:
    int ScanTuningChunkOffsets(std::vector<size_t> *offsets) const;

    std::vector<unsigned char> bytes;
    size_t bodyOffset = 0u;
    size_t activeStart = 0u;
    size_t activeEnd = 0u;
    u32 activeIndex = 0u;
    u32 tuningCount = 0u;
};

#endif
