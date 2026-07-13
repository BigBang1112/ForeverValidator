#include "format/static_solid/static_solid_archive_vehicle_tuning_reader.h"
#include <string.h>

#include "format/vehicle_tuning/chunk_schema.h"
#include "format/vehicle_tuning/archive_ids.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_primitive_reader.h"
int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ApplyVehicleTuningIdFeedback(
        const char *tuningId,
        CGameCtnReplayStaticSolidArchiveFeedbackSink *feedbackSink) {
    if (tuningId == nullptr || strlen(tuningId) <= 3u) {
        return 1;
    }
    if (feedbackSink == nullptr) {
        return 0;
    }
    const u32 feedback = static_cast<u32>(
            static_cast<unsigned char>(tuningId[0])) |
            (static_cast<u32>(static_cast<unsigned char>(tuningId[1])) << 8u) |
            (static_cast<u32>(static_cast<unsigned char>(tuningId[2])) << 16u) |
            (static_cast<u32>(static_cast<unsigned char>(tuningId[3])) << 24u);
    return feedbackSink->ApplyFeedbackU32(feedback, 1);
}

int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ParseFastBufferNodPtrs(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    u32 ignoredReservedWord = 0u;
    u32 count = 0u;
    if (!byteStream->ReadU32(&ignoredReservedWord) ||
        !byteStream->ReadU32(&count)) {
        return 0;
    }
    if (count > 0x100000u) {
        return 0;
    }
    for (u32 i = 0u; i < count; i++) {
        if (!nodeRefs->ReadNodPtr(nullptr)) {
            return 0;
        }
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ParseSceneVehicleTuningBaseChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CGameCtnReplayStaticSolidArchiveFeedbackSink *feedbackSink) {
    if (byteStream == nullptr || cmwIdState == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    char tuningId[64];
    tuningId[0] = '\0';
    if (!cmwIdState->ReadText(*byteStream, tuningId, sizeof(tuningId)) ||
        !nodeRefs->ReadNodPtr(nullptr) ||
        !CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 1u) ||
        !ApplyVehicleTuningIdFeedback(tuningId,
                                      feedbackSink)) {
        return 0;
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ParseFuncKeysRealChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 chunkId) {
    if (byteStream == nullptr || cmwIdState == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CLASS_CFuncKeys:
            return byteStream->ReadStringSkip();
        case ArchiveChunkIdValue(FunctionCurveChunkId::XValues):
        case ArchiveChunkIdValue(FunctionCurveChunkId::Root):
            return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream);
        case 0x05002002u:
            return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 2u);
        case ArchiveChunkIdValue(FunctionCurveChunkId::Id):
            return cmwIdState->ReadSkip(*byteStream);
        case ArchiveChunkIdValue(FunctionCurveChunkId::YValuesAndMode):
            return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream) &&
                   byteStream->Skip(4u);
        default:
            return 0;
    }
}

int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ParseSceneVehicleTuningsChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (chunkId != ArchiveChunkIdValue(VehicleTuningContainerChunkId::Root)) {
        return 0;
    }
    return ParseFastBufferNodPtrs(byteStream, nodeRefs) &&
           byteStream->Skip(4u);
}

int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ParseCarTuningSchemaChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || cmwIdState == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    const VehicleTuningChunkSchema *schema =
            FindVehicleTuningChunkSchema(chunkId);
    if (schema == nullptr) {
        return 0;
    }
    for (size_t i = 0u; i < schema->fieldCount; i++) {
        switch (schema->fields[i]) {
            case VehicleTuningFieldKind::Real:
            case VehicleTuningFieldKind::Natural:
            case VehicleTuningFieldKind::Boolean:
                if (!byteStream->Skip(4u)) {
                    return 0;
                }
                break;
            case VehicleTuningFieldKind::NodRef:
                if (!nodeRefs->ReadNodPtr(nullptr)) {
                    return 0;
                }
                break;
            case VehicleTuningFieldKind::FloatArray:
                if (!CGameCtnReplayStaticSolidArchivePrimitiveReader::
                            SkipFloatBuffer(byteStream)) {
                    return 0;
                }
                break;
            case VehicleTuningFieldKind::CmwId:
                if (!cmwIdState->ReadSkip(*byteStream)) {
                    return 0;
                }
                break;
            default:
                return 0;
        }
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ParseLegacyCarTuningM6CompositeChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 2u) &&
           nodeRefs->ReadNodPtr(nullptr) &&
           nodeRefs->ReadNodPtr(nullptr) &&
           CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 9u) &&
           byteStream->Skip(4u) &&
           CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 2u) &&
           byteStream->Skip(4u) &&
           CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 4u) &&
           nodeRefs->ReadNodPtr(nullptr) &&
           nodeRefs->ReadNodPtr(nullptr) &&
           CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 6u) &&
           CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream) &&
           CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream) &&
           CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream) &&
           byteStream->Skip(4u);
}

int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ParseLegacyCarTuningChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    /*
     * These legacy fixed-width layouts contain no tuning fields used by the
     * simulation. Chunks with modeled fields belong in VehicleTuningChunkSchema.
     */
    switch (chunkId) {
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyBodyTuningThreeReals): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 3u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyGroundFeedbackFourReals): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 4u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6ReverseAndTurboImpulse): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 2u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::AirborneTorque): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 2u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacySolidInertiaNatural): return byteStream->Skip(4u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyWheelForceSingleReal): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 1u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyForceModelNineReals): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 9u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyForceModelFiveRealsA):
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyForceModelFiveRealsB): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 5u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::ForceModelTwoRealsA):
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyForceModelTwoRealsB): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 2u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::ContactImpulseSingleRealA):
        case ArchiveChunkIdValue(VehicleTuningChunkId::ContactImpulseSingleRealB): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 1u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyContactImpulseSixReals): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 6u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyAirControlSingleReal): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 1u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyAirControlTwoReals): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 2u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyM5AccelThreeReals): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 3u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::TurboDuration): return byteStream->Skip(4u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::Model4AuxCurveRef): return nodeRefs->ReadNodPtr(nullptr);
        case ArchiveChunkIdValue(VehicleTuningChunkId::Model5AuxCurveRefA):
        case ArchiveChunkIdValue(VehicleTuningChunkId::Model5AuxCurveRefB): return nodeRefs->ReadNodPtr(nullptr);
        case ArchiveChunkIdValue(VehicleTuningChunkId::LegacyM6SlipFiveReals): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 5u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::WaterResponseAuxCurveRef): return nodeRefs->ReadNodPtr(nullptr);
        case ArchiveChunkIdValue(VehicleTuningChunkId::ForceFeedbackDivisorLegacy): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 2u);
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6ReverseBurnoutForceThresholdLegacy): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 2u) &&
                                  nodeRefs->ReadNodPtr(nullptr);
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6EmptyLegacyPayload): return 1;
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6CompositeBurnoutPayload):
            return ParseLegacyCarTuningM6CompositeChunk(byteStream,
                                                       nodeRefs);
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6FloatBufferSet): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream) &&
                                  CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream) &&
                                  CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream) &&
                                  CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream);
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6FloatBufferLegacySingle): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipFloatBuffer(byteStream);
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6Mat6AuxSlideRealA):
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6Mat6AuxSlideRealB):
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6Mat6AuxSlideRealC):
        case ArchiveChunkIdValue(VehicleTuningChunkId::M6Mat6AuxSlideRealD): return CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipReals(byteStream, 1u);
        default: return 0;
    }
}

int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ParseSceneVehicleCarTuningChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CGameCtnReplayStaticSolidArchiveFeedbackSink *feedbackSink) {
    if (byteStream == nullptr || cmwIdState == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    if (chunkId == ArchiveChunkIdValue(VehicleTuningBaseChunkId::Base)) {
        return ParseSceneVehicleTuningBaseChunk(byteStream,
                                               cmwIdState,
                                               nodeRefs,
                                               feedbackSink);
    }
    if (FindVehicleTuningChunkSchema(chunkId) != nullptr) {
        return ParseCarTuningSchemaChunk(byteStream,
                                        cmwIdState,
                                        chunkId,
                                        nodeRefs);
    }
    return ParseLegacyCarTuningChunk(byteStream, chunkId, nodeRefs);
}

int CGameCtnReplayStaticSolidArchiveVehicleTuningReader::ParseVehicleTuningChunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        u32 classId,
        u32 chunkId,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CGameCtnReplayStaticSolidArchiveFeedbackSink *feedbackSink) {
    if (classId == ArchiveChunkIdValue(FunctionCurveChunkId::Root)) {
        return ParseFuncKeysRealChunk(byteStream, cmwIdState, chunkId);
    }
    if (classId == ArchiveChunkIdValue(VehicleTuningContainerChunkId::Root)) {
        return ParseSceneVehicleTuningsChunk(byteStream,
                                            chunkId,
                                            nodeRefs);
    }
    if (classId == ArchiveChunkIdValue(VehicleTuningChunkId::Root)) {
        return ParseSceneVehicleCarTuningChunk(byteStream,
                                              cmwIdState,
                                              chunkId,
                                              nodeRefs,
                                              feedbackSink);
    }
    return 0;
}
