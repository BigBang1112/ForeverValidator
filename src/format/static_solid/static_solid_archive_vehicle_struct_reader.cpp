#include "format/static_solid/static_solid_archive_vehicle_struct_reader.h"

#include "format/archive/archive_class_ids.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_chunk_dispatcher.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_primitive_reader.h"
#include "format/static_solid/static_solid_archive_scene_payloads.h"
#include "format/vehicle_tuning/default_vehicle_archive_schema.h"

namespace {

constexpr u32 VehicleStructArrayCountLimit = 0x100000u;

int ReadCount(CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
              u32 *countOut) {
    return byteStream != nullptr && countOut != nullptr &&
           byteStream->ReadU32(countOut) &&
           *countOut <= VehicleStructArrayCountLimit;
}

int ReadVisualId(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState) {
    u32 flag = 0u;
    return byteStream != nullptr && cmwIdState != nullptr &&
           cmwIdState->ReadSkip(*byteStream) &&
           byteStream->ReadBool(&flag);
}

int ReadVisualArms(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 visualVehicleCount) {
    for (u32 vehicleIndex = 0u;
         vehicleIndex < visualVehicleCount;
         ++vehicleIndex) {
        u32 armCount = 0u;
        if (!ReadCount(byteStream, &armCount)) {
            return 0;
        }
        for (u32 armIndex = 0u; armIndex < armCount; ++armIndex) {
            if (!ReadVisualId(byteStream, cmwIdState) ||
                !ReadVisualId(byteStream, cmwIdState) ||
                !ReadVisualId(byteStream, cmwIdState) ||
                !byteStream->Skip(3u * sizeof(u32))) {
                return 0;
            }
        }
    }
    return 1;
}

int ReadVisualLights(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 visualVehicleCount) {
    for (u32 vehicleIndex = 0u;
         vehicleIndex < visualVehicleCount;
         ++vehicleIndex) {
        u32 lightCount = 0u;
        if (!ReadCount(byteStream, &lightCount)) {
            return 0;
        }
        for (u32 lightIndex = 0u; lightIndex < lightCount; ++lightIndex) {
            if (!ReadVisualId(byteStream, cmwIdState) ||
                !byteStream->Skip(sizeof(u32))) {
                return 0;
            }
        }
    }
    return 1;
}

int ReadVisualWheels(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 visualVehicleCount) {
    for (u32 vehicleIndex = 0u;
         vehicleIndex < visualVehicleCount;
         ++vehicleIndex) {
        u32 wheelCount = 0u;
        if (!ReadCount(byteStream, &wheelCount)) {
            return 0;
        }
        for (u32 wheelIndex = 0u; wheelIndex < wheelCount; ++wheelIndex) {
            for (u32 visualIdIndex = 0u;
                 visualIdIndex < 4u;
                 ++visualIdIndex) {
                if (!ReadVisualId(byteStream, cmwIdState)) {
                    return 0;
                }
            }
            if (!byteStream->Skip(2u * sizeof(u32))) {
                return 0;
            }
        }
    }
    return 1;
}

int ReadVisualVehicleIds(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 visualVehicleCount) {
    for (u32 vehicleIndex = 0u;
         vehicleIndex < visualVehicleCount;
         ++vehicleIndex) {
        for (u32 visualIdIndex = 0u;
             visualIdIndex < 4u;
             ++visualIdIndex) {
            if (!ReadVisualId(byteStream, cmwIdState)) {
                return 0;
            }
        }
        if (!byteStream->Skip(sizeof(u32))) {
            return 0;
        }
    }
    return 1;
}

int ReadCSceneVehicleStructArchiveChunk(
        const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
        u32 chunkId) {
    if (context.byteStream == nullptr || context.cmwIdState == nullptr ||
        context.nodeRefs == nullptr || context.vehicleStructState == nullptr) {
        return 0;
    }
    CSceneVehicleStructArchiveState &state = *context.vehicleStructState;
    switch (chunkId) {
        case ArchiveChunkIdValue(
                CSceneVehicleStructArchiveChunkId::SimulationWheelBuffer): {
            u32 wheelCount = 0u;
            if (!ReadCount(context.byteStream, &wheelCount)) {
                return 0;
            }
            for (u32 wheelIndex = 0u;
                 wheelIndex < wheelCount;
                 ++wheelIndex) {
                u32 firstFlag = 0u;
                u32 secondFlag = 0u;
                if (!context.byteStream->ReadBool(&firstFlag) ||
                    !context.byteStream->ReadBool(&secondFlag) ||
                    !context.cmwIdState->ReadSkip(*context.byteStream)) {
                    return 0;
                }
            }
            return 1;
        }
        case ArchiveChunkIdValue(
                CSceneVehicleStructArchiveChunkId::VisualVehicleCount):
            state.hasVisualVehicleCount =
                    ReadCount(context.byteStream, &state.visualVehicleCount);
            return state.hasVisualVehicleCount;
        case ArchiveChunkIdValue(
                CSceneVehicleStructArchiveChunkId::VisualArms):
            return state.hasVisualVehicleCount &&
                   ReadVisualArms(context.byteStream,
                                  context.cmwIdState,
                                  state.visualVehicleCount);
        case ArchiveChunkIdValue(
                CSceneVehicleStructArchiveChunkId::VisualLights):
            return state.hasVisualVehicleCount &&
                   ReadVisualLights(context.byteStream,
                                    context.cmwIdState,
                                    state.visualVehicleCount);
        case ArchiveChunkIdValue(
                CSceneVehicleStructArchiveChunkId::VisualWheels):
            return state.hasVisualVehicleCount &&
                   ReadVisualWheels(context.byteStream,
                                    context.cmwIdState,
                                    state.visualVehicleCount);
        case ArchiveChunkIdValue(
                CSceneVehicleStructArchiveChunkId::VisualIds):
            return state.hasVisualVehicleCount &&
                   ReadVisualVehicleIds(context.byteStream,
                                        context.cmwIdState,
                                        state.visualVehicleCount);
        case ArchiveChunkIdValue(
                CSceneVehicleStructArchiveChunkId::VehicleMaterialGroups):
        case ArchiveChunkIdValue(
                CSceneVehicleStructArchiveChunkId::VehicleEmitters):
            return CSceneArchiveNodeRefPayload::ReadFastBuffer(
                    context.byteStream, context.nodeRefs);
        case ArchiveChunkIdValue(
                CSceneVehicleStructArchiveChunkId::FeedbackCurves):
            return context.nodeRefs->ReadNodPtr(nullptr) &&
                   context.nodeRefs->ReadNodPtr(nullptr) &&
                   context.nodeRefs->ReadNodPtr(nullptr);
        default:
            return 0;
    }
}

int ReadCSceneVehicleMaterialGroupArchiveChunk(
        const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
        u32 chunkId) {
    u32 count = 0u;
    return chunkId == ArchiveChunkIdValue(
                              CSceneVehicleMaterialGroupArchiveChunkId::Root) &&
           ReadCount(context.byteStream, &count) &&
           context.byteStream->SkipCounted(count, sizeof(u32));
}

int ReadCSceneVehicleEmitterArchiveChunk(
        const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
        u32 chunkId) {
    if (context.byteStream == nullptr || context.nodeRefs == nullptr) {
        return 0;
    }
    u32 flag = 0u;
    switch (chunkId) {
        case ArchiveChunkIdValue(
                CSceneVehicleEmitterArchiveChunkId::Chunk002):
        case ArchiveChunkIdValue(
                CSceneVehicleEmitterArchiveChunkId::Chunk005):
            return context.byteStream->ReadBool(&flag);
        case ArchiveChunkIdValue(
                CSceneVehicleEmitterArchiveChunkId::Chunk003):
            return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(
                    context.byteStream, 6u);
        case ArchiveChunkIdValue(
                CSceneVehicleEmitterArchiveChunkId::Chunk004):
            return context.byteStream->Skip(sizeof(u32)) &&
                   context.nodeRefs->ReadNodPtr(nullptr) &&
                   context.nodeRefs->ReadNodPtr(nullptr) &&
                   context.nodeRefs->ReadNodPtr(nullptr) &&
                   context.byteStream->Skip(6u * sizeof(u32)) &&
                   CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipIso4(
                           context.byteStream) &&
                   CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(
                           context.byteStream, 14u);
        default:
            return 0;
    }
}

}  // namespace

void CSceneVehicleStructArchiveState::Reset() {
    *this = CSceneVehicleStructArchiveState{};
}

int CGameCtnReplayStaticSolidArchiveVehicleStructReader::ParseChunk(
        const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
        u32 classId,
        u32 chunkId) {
    switch (classId) {
        case TMNF_CLASS_CSceneVehicleStruct:
            return ReadCSceneVehicleStructArchiveChunk(context, chunkId);
        case TMNF_CLASS_CSceneVehicleMaterialGroup:
            return ReadCSceneVehicleMaterialGroupArchiveChunk(context, chunkId);
        case TMNF_CLASS_CSceneVehicleEmitter:
            return ReadCSceneVehicleEmitterArchiveChunk(context, chunkId);
        default:
            return 0;
    }
}
