#include "format/static_solid/static_solid_archive_payload_stream.h"
#include <memory>
#include <vector>

#include "format/archive/classic_buffer_crypted.h"
#include "format/archive/classic_archive_chunk_info.h"
#include "format/archive/scene_object_archive_chunk_ids.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_animation_motion_reader.h"
#include "format/static_solid/static_solid_archive_bitmap_reader.h"
#include "format/static_solid/static_solid_archive_chunk_dispatcher.h"
#include "format/static_solid/static_solid_archive_class_info.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_decode_progress.h"
#include "format/static_solid/static_solid_archive_feedback.h"
#include "format/static_solid/static_solid_archive_graph_writer.h"
#include "format/static_solid/static_solid_archive_node_ref_reader.h"
#include "format/static_solid/static_solid_archive_node_graph.h"
#include "format/static_solid/static_solid_archive_root_reader.h"
#include "format/static_solid/static_solid_archive_stream_roles.h"
#include "format/static_solid/static_solid_archive_visual_provider_state.h"
#include "format/static_solid/static_solid_archive_vehicle_struct_reader.h"
#include "format/static_solid/static_solid_archive_scene_traffic_graph_reader.h"
#include "format/static_solid/static_solid_descriptor_dependency_queue.h"
#include "format/static_solid/static_solid_external_node_paths.h"
#include "format/archive/tmnf_archive_ids.h"
static constexpr size_t StaticSolidArchiveSharedNameCacheCapacity = 128u;

namespace {

int RequiresCompleteArchiveSemantics(u32 classId) {
    return classId == TMNF_CLASS_CPlugMaterial ||
           classId == TMNF_CLASS_CPlugMaterialCustom ||
           classId == TMNF_CLASS_CPlugShaderApply ||
           classId == TMNF_CLASS_CPlugShaderPass ||
           classId == TMNF_CLASS_CPlugBitmapApply ||
           classId == TMNF_CLASS_CPlugBitmap ||
           classId == TMNF_CLASS_CPlugBitmapRenderWater;
}

}  // namespace

class StaticSolidArchiveStream :
        public CGameCtnReplayStaticSolidArchiveNodeRefReader,
        public CGameCtnReplayStaticSolidArchiveFeedbackSink,
        public CGameCtnReplayStaticSolidArchiveEmbeddedNodeParser,
        public CGameCtnReplayStaticSolidArchiveNodeParser {
public:
    int OpenDecodedPayload(
            const StaticSolidArchivePayload *payloadAsset,
            const unsigned char *decodedBytes,
            u32 decodedByteCount,
            StaticSolidArchiveLoadSession *store,
            CGameCtnReplayArchiveStaticModelCollection *archiveModelsIn,
            StaticSolidArchiveId selectedPayload);
    int OpenEncryptedFeedbackPayload(
            const StaticSolidArchivePayload *payloadAsset,
            CClassicBufferCrypted *reader,
            unsigned char *decodedBytes,
            StaticSolidArchiveLoadSession *store,
            CGameCtnReplayArchiveStaticModelCollection *archiveModelsIn,
            StaticSolidArchiveId selectedPayload);
    int ParseDecodedExternalRefDependencies(
            u32 decodedByteCount,
            CGameCtnReplayStaticSolidDescriptorDependencyQueue *dependencyQueue);
    int ParseEncryptedPayloadToEnd(
            u32 maxPayloadByteCount,
            CGameCtnReplayStaticSolidDecodedPayload *decoded,
            CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut);
    int ParseDecodedPayloadToEnd(
            u32 decodedByteCount,
            CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut);
    int ParseEncryptedReferenceTableHeader(
            u32 maxPayloadByteCount,
            CGameCtnReplayStaticSolidDecodedPayload *decoded);
private:
    CGameCtnReplayStaticSolidArchiveByteStream byteStream;
    StaticSolidArchiveLoadSession *materialStore = nullptr;
    CGameCtnReplayArchiveStaticModelCollection *archiveModels = nullptr;
    StaticSolidArchiveId payload =
            StaticSolidArchiveId::Invalid();
    CGameCtnReplayStaticSolidArchiveCMwIdState cmwIdState;
    StaticSolidArchiveVisualState visualProviderState;
    CSceneVehicleStructArchiveState vehicleStructState;
    CSceneTrafficGraphArchiveState trafficGraphState;
    CGameCtnReplayStaticSolidArchiveAnimationMotionState animationMotionState;
    ArchiveNodeReference currentArchiveNode =
            ArchiveNodeReference::Invalid();
    u32 currentArchiveClassId = 0u;
    u32 archiveNodeCountLimit = 0u;
    SceneDescriptorFolderPaths externalFolders;
    CGameCtnReplayStaticSolidArchiveNodeGraph archiveNodeGraph;
    CGameCtnReplayStaticSolidArchiveFeedback feedback;
    CGameCtnReplayStaticSolidArchiveDecodeProgress decodeProgress;

    int ApplyFeedbackValue(u32 value, int nested);
    int ApplyFeedback(u32 classId, int nested);
    int ApplyFeedbackU32(u32 value, int nested) override;
    int EmitNode(ArchiveNodeReference nodeRef,
                 u32 classId);
    int ReadNodeRef(
            ArchiveNodeReference *nodeRefOut);
    int ReadNodeRefWithInlineState(
            ArchiveNodeReference *nodeRefOut,
            int *isInternalOut) override;
    int ReadFidRef(ArchiveNodeReference *nodeRefOut) override;
    int ParseRootNodeArchive(u32 maxPayloadByteCount,
                             u32 *rootClassIdOut);
    int RequestExternalRefDescriptorDependencies(
            CGameCtnReplayStaticSolidDescriptorDependencyQueue *dependencyQueue) const;
    int SkipOrBreakUnknownChunk(u32 classId, u32 chunkId, int *breakArchiveOut);
    int ParseEmbeddedNodeArchive(u32 classId);
    int ParseNodeArchiveInternal(
            ArchiveNodeReference nodeRef,
            u32 classId,
            int nested,
            int applyFeedback);
    int ParseNodeArchive(ArchiveNodeReference nodeRef,
                         u32 classId,
                         int nested);
    int ParseNodeArchiveNoFeedback(
            ArchiveNodeReference nodeRef,
            u32 classId);
    int ParseChunkPayload(u32 classId, u32 chunkId);
};

int StaticSolidArchiveStream::ApplyFeedbackValue(u32 value, int nested) {
    return byteStream.ApplyFeedbackValue(feedback, value, nested);
}

int StaticSolidArchiveStream::ApplyFeedback(u32 classId, int nested) {
    const u32 feedback = CGameCtnReplayStaticSolidArchiveClassInfo::FeedbackClassId(classId);
    if (feedback == 0u) {
        return 1;
    }
    return ApplyFeedbackValue(feedback, nested);
}

int StaticSolidArchiveStream::ApplyFeedbackU32(u32 value, int nested) {
    return ApplyFeedbackValue(value, nested);
}

int StaticSolidArchiveStream::EmitNode(
        ArchiveNodeReference nodeRef,
        u32 classId) {
    CGameCtnReplayStaticSolidArchiveGraphWriter writer(
            materialStore != nullptr ? &materialStore->MutableArchiveGraph()
                                  : nullptr,
            payload);
    return writer.AppendNode(nodeRef, classId);
}

int StaticSolidArchiveStream::ReadNodeRef(
        ArchiveNodeReference *nodeRefOut) {
    return ReadNodeRefWithInlineState(nodeRefOut, nullptr);
}

int StaticSolidArchiveStream::ReadNodeRefWithInlineState(
        ArchiveNodeReference *nodeRefOut,
        int *isInternalOut) {
    if (isInternalOut != nullptr) {
        *isInternalOut = 0;
    }
    u32 index = 0u;
    if (!byteStream.ReadU32(&index)) {
        return 0;
    }
    const ArchiveNodeReference nodeRef =
            ArchiveNodeReference::FromIndex(index);
    if (nodeRefOut != nullptr) {
        *nodeRefOut = nodeRef;
    }
    if (nodeRef.IsInvalid()) {
        return 1;
    }
    if (nodeRef.IsDeferred()) {
        return 1;
    }
    if (nodeRef.Index() > archiveNodeCountLimit) {
        return 0;
    }
    if (archiveNodeGraph.HasNode(nodeRef)) {
        const CGameCtnReplayStaticSolidArchiveNodeGraph::Node *node =
                archiveNodeGraph.FindNode(nodeRef);
        if (isInternalOut != nullptr && node != nullptr) {
            *isInternalOut = !node->IsExternal();
        }
        return 1;
    }
    if (!byteStream.Ensure(byteStream.Offset() + 4u)) {
        return 0;
    }
    u32 nextWord = 0u;
    if (!byteStream.PeekU32(&nextWord)) {
        return 0;
    }
    const u32 nextClass =
            TMNF_CPlugTreeCanonicalChunkIdFromArchiveWord(nextWord);
    if (!CGameCtnReplayStaticSolidArchiveClassInfo::IsKnownNodeClass(nextClass)) {
        if (isInternalOut != nullptr) {
            *isInternalOut = 1;
        }
        return 1;
    }
    u32 rawClassId = 0u;
    if (!byteStream.ReadU32(&rawClassId) ||
        !archiveNodeGraph.EnsureNodeCapacity(
                nodeRef.Index())) {
        return 0;
    }
    const u32 classId =
            TMNF_CPlugTreeCanonicalChunkIdFromArchiveWord(rawClassId);
    archiveNodeGraph.MarkInlineNode(nodeRef, classId);
    if (isInternalOut != nullptr) {
        *isInternalOut = 1;
    }
    return ParseNodeArchive(nodeRef, classId, 1);
}

int StaticSolidArchiveStream::ReadFidRef(
        ArchiveNodeReference *nodeRefOut) {
    u32 index = 0u;
    if (!byteStream.ReadU32(&index)) {
        return 0;
    }
    const ArchiveNodeReference nodeRef =
            ArchiveNodeReference::FromIndex(index);
    if (nodeRefOut != nullptr) {
        *nodeRefOut = nodeRef;
    }
    if (nodeRef.IsInvalid()) {
        return 1;
    }
    if (!nodeRef.IsValid() || nodeRef.Index() > archiveNodeCountLimit) {
        return 0;
    }
    const CGameCtnReplayStaticSolidArchiveNodeGraph::Node *node =
            archiveNodeGraph.FindNode(nodeRef);
    return node != nullptr && node->IsExternal();
}

int StaticSolidArchiveStream::SkipOrBreakUnknownChunk(
        u32 classId,
        u32 chunkId,
        int *breakArchiveOut) {
    /*
     * An unrecognized chunk may be followed by a SKIP marker and byte count.
     * Any other following word terminates the current node archive after that
     * word has been consumed. Packed mobil archives use this form for legacy
     * control tails before facade-family records.
     */
    (void)classId;
    (void)chunkId;

    u32 magic = 0u;
    if (breakArchiveOut != nullptr) {
        *breakArchiveOut = 0;
    }
    if (!byteStream.ReadU32(&magic)) {
        if (breakArchiveOut != nullptr) {
            *breakArchiveOut = 1;
        }
        return 1;
    }
    if (magic == 0x534b4950u) {
        u32 size = 0u;
        return byteStream.ReadU32(&size) &&
               byteStream.Skip(size);
    }
    if (breakArchiveOut != nullptr) {
        *breakArchiveOut = 1;
    }
    return 1;
}

int StaticSolidArchiveStream::ParseEmbeddedNodeArchive(u32 classId) {
    // Embedded motion commands start directly with their node body: nested
    // class feedback followed by the command's chunk-mask stream.
    if (!ApplyFeedback(classId, 1)) {
        return 0;
    }
    for (;;) {
        u32 chunkId = 0u;
        if (!byteStream.ReadU32(&chunkId)) {
            decodeProgress.MarkFailure(classId,
                                       chunkId,
                                       byteStream.Offset(),
                                       byteStream.CompressedRead());
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            return 1;
        }
        const u32 info = CGameCtnReplayStaticSolidArchiveClassInfo::ChunkInfo(classId, chunkId);
        const CClassicArchiveChunkInfo chunkInfo(info);
        if (info == 0u ||
            (((info & 1u) == 0u) &&
             chunkInfo.HasSkipPayload())) {
            int breakArchive = 0;
            if (!SkipOrBreakUnknownChunk(
                        classId,
                        chunkId,
                        &breakArchive)) {
                decodeProgress.MarkFailure(classId,
                                           chunkId,
                                           byteStream.Offset(),
                                           byteStream.CompressedRead());
                return 0;
            }
            if (breakArchive) {
                return 1;
            }
            continue;
        }
        if (info != 0xffffffffu &&
            chunkInfo.HasSkipPayload() &&
            !byteStream.Skip(8u)) {
            decodeProgress.MarkFailure(classId,
                                       chunkId,
                                       byteStream.Offset(),
                                       byteStream.CompressedRead());
            return 0;
        }
        if (!ParseChunkPayload(classId, chunkId)) {
            decodeProgress.MarkFailure(classId,
                                       chunkId,
                                       byteStream.Offset(),
                                       byteStream.CompressedRead());
            return 0;
        }
    }
}

int StaticSolidArchiveStream::ParseChunkPayload(u32 classId, u32 chunkId) {
    CGameCtnReplayStaticSolidArchiveChunkDispatchContext context{
            &byteStream,
            &cmwIdState,
            &feedback,
            &decodeProgress,
            &archiveNodeGraph,
            &visualProviderState,
            &vehicleStructState,
            &trafficGraphState,
            &animationMotionState,
            &externalFolders,
            materialStore,
            archiveModels,
            payload,
            currentArchiveNode,
            this,
            this,
            this,
            this};
    return CGameCtnReplayStaticSolidArchiveChunkDispatcher::ParseChunk(
            context,
            classId,
            chunkId);
}

int StaticSolidArchiveStream::ParseNodeArchiveInternal(
        ArchiveNodeReference nodeRef,
        u32 classId,
        int nested,
        int applyFeedback) {
    if (applyFeedback &&
        !ApplyFeedback(classId, nested)) {
        return 0;
    }
    if (!EmitNode(nodeRef, classId)) {
        return 0;
    }
    decodeProgress.RecordArchiveNode(classId);
    const ArchiveNodeReference savedArchiveNode =
            currentArchiveNode;
    const u32 savedArchiveClassId = currentArchiveClassId;
    currentArchiveNode = nodeRef;
    currentArchiveClassId = classId;
    if (classId == TMNF_CLASS_CPlugFileGen) {
        const int parsed =
                CGameCtnReplayStaticSolidArchiveBitmapReader::
                        ParseCPlugFileGenPayload(&byteStream, this);
        currentArchiveNode = savedArchiveNode;
        currentArchiveClassId = savedArchiveClassId;
        return parsed;
    }
    for (;;) {
        u32 nextChunkId = 0u;
        if (!byteStream.PeekU32(&nextChunkId)) {
            currentArchiveNode = savedArchiveNode;
            currentArchiveClassId = savedArchiveClassId;
            return 0;
        }
        const u32 ownerChunkId =
                NormalizePackedCSceneObjectOrMobilArchiveChunkId(nextChunkId);
        if (savedArchiveClassId != 0u &&
            CGameCtnReplayStaticSolidArchiveClassInfo::ChunkInfo(
                    classId, nextChunkId) == 0u) {
            if (CGameCtnReplayStaticSolidArchiveClassInfo::ChunkInfo(
                        savedArchiveClassId, ownerChunkId) != 0u) {
                currentArchiveNode = savedArchiveNode;
                currentArchiveClassId = savedArchiveClassId;
                return 1;
            }
        }
        u32 chunkId = 0u;
        if (!byteStream.ReadU32(&chunkId)) {
            currentArchiveNode = savedArchiveNode;
            currentArchiveClassId = savedArchiveClassId;
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            currentArchiveNode = savedArchiveNode;
            currentArchiveClassId = savedArchiveClassId;
            return 1;
        }
        const u32 info = CGameCtnReplayStaticSolidArchiveClassInfo::ChunkInfo(classId, chunkId);
        const CClassicArchiveChunkInfo chunkInfo(info);
        if (info == 0u ||
            (((info & 1u) == 0u) &&
             chunkInfo.HasSkipPayload())) {
            if (classId == TMNF_CLASS_CMotionEmitterLeaves ||
                RequiresCompleteArchiveSemantics(classId)) {
                decodeProgress.MarkFailure(classId,
                                           chunkId,
                                           byteStream.Offset(),
                                           byteStream.CompressedRead());
                currentArchiveNode = savedArchiveNode;
                currentArchiveClassId = savedArchiveClassId;
                return 0;
            }
            int breakArchive = 0;
            if (!SkipOrBreakUnknownChunk(
                        classId,
                        chunkId,
                        &breakArchive)) {
                decodeProgress.MarkFailure(classId,
                                           chunkId,
                                           byteStream.Offset(),
                                           byteStream.CompressedRead());
                currentArchiveNode = savedArchiveNode;
                currentArchiveClassId = savedArchiveClassId;
                return 0;
            }
            if (breakArchive) {
                currentArchiveNode = savedArchiveNode;
                currentArchiveClassId = savedArchiveClassId;
                return 1;
            }
            continue;
        }
        if (info != 0xffffffffu &&
            chunkInfo.HasSkipPayload() &&
            !byteStream.Skip(8u)) {
            decodeProgress.MarkFailure(classId,
                                       chunkId,
                                       byteStream.Offset(),
                                       byteStream.CompressedRead());
            currentArchiveNode = savedArchiveNode;
            currentArchiveClassId = savedArchiveClassId;
            return 0;
        }
        if (classId == TMNF_CLASS_CPlugSolid) {
            chunkId = TMNF_CPlugSolidCanonicalChunkIdFromArchiveWord(chunkId);
        } else if (classId == TMNF_CLASS_CPlugTree ||
                   classId == TMNF_CLASS_CPlugTreeVisualMip ||
                   classId == TMNF_CLASS_CPlugTreeLight) {
            chunkId = TMNF_CPlugTreeCanonicalChunkIdFromArchiveWord(chunkId);
        }
        if (!ParseChunkPayload(classId, chunkId)) {
            decodeProgress.MarkFailure(classId,
                                       chunkId,
                                       byteStream.Offset(),
                                       byteStream.CompressedRead());
            currentArchiveNode = savedArchiveNode;
            currentArchiveClassId = savedArchiveClassId;
            return 0;
        }
    }
}

int StaticSolidArchiveStream::ParseNodeArchive(
        ArchiveNodeReference nodeRef,
        u32 classId,
        int nested) {
    return ParseNodeArchiveInternal(nodeRef, classId, nested, 1);
}

int StaticSolidArchiveStream::ParseNodeArchiveNoFeedback(
        ArchiveNodeReference nodeRef,
        u32 classId) {
    return ParseNodeArchiveInternal(nodeRef, classId, 1, 0);
}

int StaticSolidArchiveStream::OpenDecodedPayload(
        const StaticSolidArchivePayload *payloadAsset,
        const unsigned char *decodedBytes,
        u32 decodedByteCount,
        StaticSolidArchiveLoadSession *store,
        CGameCtnReplayArchiveStaticModelCollection *archiveModelsIn,
        StaticSolidArchiveId selectedPayload) {
    if (decodedBytes == nullptr) {
        return 0;
    }
    cmwIdState.ResetSharedNameCache(
            StaticSolidArchiveSharedNameCacheCapacity);
    vehicleStructState.Reset();
    trafficGraphState.Reset();
    animationMotionState.Reset();
    currentArchiveNode = ArchiveNodeReference::Invalid();
    currentArchiveClassId = 0u;
    archiveNodeCountLimit = 0u;
    materialStore = store;
    archiveModels = archiveModelsIn;
    payload = selectedPayload;
    return byteStream.OpenDecodedPayload(payloadAsset,
                                         decodedBytes,
                                         decodedByteCount);
}

int StaticSolidArchiveStream::OpenEncryptedFeedbackPayload(
        const StaticSolidArchivePayload *payloadAsset,
        CClassicBufferCrypted *reader,
        unsigned char *decodedBytes,
        StaticSolidArchiveLoadSession *store,
        CGameCtnReplayArchiveStaticModelCollection *archiveModelsIn,
        StaticSolidArchiveId selectedPayload) {
    if (reader == nullptr || decodedBytes == nullptr) {
        return 0;
    }
    cmwIdState.ResetSharedNameCache(
            StaticSolidArchiveSharedNameCacheCapacity);
    vehicleStructState.Reset();
    trafficGraphState.Reset();
    animationMotionState.Reset();
    currentArchiveNode = ArchiveNodeReference::Invalid();
    currentArchiveClassId = 0u;
    archiveNodeCountLimit = 0u;
    materialStore = store;
    archiveModels = archiveModelsIn;
    payload = selectedPayload;
    return byteStream.OpenEncryptedFeedbackPayload(payloadAsset,
                                                   reader,
                                                   decodedBytes,
                                                   &feedback);
}

int StaticSolidArchiveStream::ParseRootNodeArchive(
        u32 maxPayloadByteCount,
        u32 *rootClassIdOut) {
    if (rootClassIdOut == nullptr) {
        return 0;
    }
    *rootClassIdOut = 0u;
    CGameCtnReplayStaticSolidArchiveRootHeader header;
    if (!CGameCtnReplayStaticSolidArchiveRootReader::ReadGbxRootArchive(
                &byteStream,
                &archiveNodeGraph,
                &externalFolders,
                maxPayloadByteCount,
                &header) ||
        !archiveNodeGraph.EnsureNodeCapacity(0u)) {
        return 0;
    }
    archiveNodeGraph.MarkInlineNode(
            ArchiveNodeReference::FromIndex(0u),
            header.classId);
    archiveNodeCountLimit = header.numNodes;
    *rootClassIdOut = header.classId;
    decodeProgress.MarkBodyOffsetParsed();
    return ParseNodeArchive(ArchiveNodeReference::FromIndex(0u),
                            header.classId,
                            0);
}

int StaticSolidArchiveStream::ParseDecodedExternalRefDependencies(
        u32 decodedByteCount,
        CGameCtnReplayStaticSolidDescriptorDependencyQueue *dependencyQueue) {
    CGameCtnReplayStaticSolidArchiveRootHeader header;
    if (!CGameCtnReplayStaticSolidArchiveRootReader::ReadGbxRootArchive(
                &byteStream,
                &archiveNodeGraph,
                &externalFolders,
                decodedByteCount,
                &header)) {
        return 0;
    }
    return RequestExternalRefDescriptorDependencies(dependencyQueue);
}

int StaticSolidArchiveStream::RequestExternalRefDescriptorDependencies(
        CGameCtnReplayStaticSolidDescriptorDependencyQueue *dependencyQueue) const {
    if (materialStore == nullptr || dependencyQueue == nullptr) {
        return 0;
    }
    const CPlugFilePack *pack = materialStore->ExternalPack();
    if (pack == nullptr) {
        return 0;
    }
    std::vector<CGameCtnReplayStaticSolidExternalNodeRef> nodeRefs;
    if (!archiveNodeGraph.CopyExternalNodeRefs(&nodeRefs)) {
        return 0;
    }
    return dependencyQueue->RequestExternalRefDescriptors(
            externalFolders,
            *pack,
            *materialStore,
            nodeRefs);
}

int StaticSolidArchiveStream::ParseEncryptedPayloadToEnd(
        u32 maxPayloadByteCount,
        CGameCtnReplayStaticSolidDecodedPayload *decoded,
        CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut) {
    if (decoded == nullptr) {
        return 0;
    }
    int ok = 0;
    u32 rootClassId = 0u;
    if (ParseRootNodeArchive(maxPayloadByteCount, &rootClassId) &&
        byteStream.Offset() <= maxPayloadByteCount) {
        const int completeRoot =
                RequiresCompleteArchiveSemantics(rootClassId);
        ok = (!completeRoot ||
              byteStream.Offset() == maxPayloadByteCount) &&
             (completeRoot ||
              byteStream.Skip(maxPayloadByteCount - byteStream.Offset())) &&
             byteStream.Produced() == maxPayloadByteCount &&
             byteStream.FinishExactPayload() &&
             decoded->TrimToByteCount(byteStream.Produced());
    }
    decodeProgress.CaptureStats(feedback, statsOut);
    return ok;
}

int StaticSolidArchiveStream::ParseEncryptedReferenceTableHeader(
        u32 maxPayloadByteCount,
        CGameCtnReplayStaticSolidDecodedPayload *decoded) {
    if (decoded == nullptr) {
        return 0;
    }
    CGameCtnReplayStaticSolidArchiveRootHeader header;
    if (!CGameCtnReplayStaticSolidArchiveRootReader::ReadGbxRootArchive(
                &byteStream,
                &archiveNodeGraph,
                &externalFolders,
                maxPayloadByteCount,
                &header)) {
        return 0;
    }
    return decoded->TrimToByteCount(byteStream.Produced());
}

int StaticSolidArchiveStream::ParseDecodedPayloadToEnd(
        u32 decodedByteCount,
        CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut) {
    int ok = 0;
    u32 rootClassId = 0u;
    if (ParseRootNodeArchive(decodedByteCount, &rootClassId) &&
        byteStream.Offset() <= decodedByteCount) {
        const int completeRoot =
                RequiresCompleteArchiveSemantics(rootClassId);
        ok = (!completeRoot || byteStream.Offset() == decodedByteCount) &&
             (completeRoot ||
              byteStream.Skip(decodedByteCount - byteStream.Offset())) &&
             byteStream.Offset() == decodedByteCount &&
             byteStream.Failed() == 0;
    }
    decodeProgress.CaptureStats(feedback, statsOut);
    return ok;
}

int CGameCtnReplayStaticSolidArchivePayloadReader::EnqueueExternalRefDependencies(
        const StaticSolidArchivePayload &payloadAsset,
        const unsigned char *decodedBytes,
        u32 decodedByteCount,
        StaticSolidArchiveLoadSession *store,
        CGameCtnReplayStaticSolidDescriptorDependencyQueue *dependencyQueue) {
    if (decodedBytes == nullptr || store == nullptr ||
        dependencyQueue == nullptr || decodedByteCount < 17u) {
        return 0;
    }
    StaticSolidArchiveStream stream{};
    if (!stream.OpenDecodedPayload(&payloadAsset,
                                   decodedBytes,
                                   decodedByteCount,
                                   store,
                                   nullptr,
                                           StaticSolidArchiveId::
                                           Invalid())) {
        return 0;
    }
    return stream.ParseDecodedExternalRefDependencies(decodedByteCount,
                                                      dependencyQueue);
}

int CGameCtnReplayStaticSolidArchivePayloadReader::DecodeWithStreamFeedback(
        const SNat128 &key,
        const StaticSolidArchivePayload &payloadAsset,
        StaticSolidArchiveRawBytes rawPayload,
        StaticSolidArchiveLoadSession *materialStore,
        CGameCtnReplayArchiveStaticModelCollection *archiveModels,
        StaticSolidArchiveId payload,
        CGameCtnReplayStaticSolidDecodedPayload *decodedOut,
        CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut) {
    if (decodedOut != nullptr) {
        decodedOut->Clear();
    }
    if (statsOut != nullptr) {
        statsOut->Clear();
    }
    if (!rawPayload.IsAvailable() || decodedOut == nullptr ||
        !payloadAsset.IsEncrypted() ||
        payloadAsset.UncompressedByteCount() == 0u ||
        rawPayload.ByteCount() < payloadAsset.RawByteCount()) {
        return 0;
    }
    CGameCtnReplayStaticSolidDecodedPayload decoded;
    if (!decoded.ResizeForDecode(payloadAsset.UncompressedByteCount())) {
        return 0;
    }

    auto reader = CClassicBufferCrypted::CreateBlowfishReaderForMemory(
            rawPayload.Bytes(),
            rawPayload.ByteCount(),
            8ul,
            key);
    if (reader == nullptr) {
        return 0;
    }

    StaticSolidArchiveStream stream{};
    if (!stream.OpenEncryptedFeedbackPayload(&payloadAsset,
                                             reader.get(),
                                             decoded.MutableBytes(),
                                             materialStore,
                                             archiveModels,
                                             payload)) {
        return 0;
    }

    if (!stream.ParseEncryptedPayloadToEnd(payloadAsset.UncompressedByteCount(),
                                           &decoded,
                                           statsOut)) {
        return 0;
    }
    decodedOut->TakeFrom(&decoded);
    return 1;
}

int CGameCtnReplayStaticSolidArchivePayloadReader::
DecodeReferenceTablePrefixWithStreamFeedback(
        const SNat128 &key,
        const StaticSolidArchivePayload &payloadAsset,
        StaticSolidArchiveRawBytes rawPayload,
        CGameCtnReplayStaticSolidDecodedPayload *decodedOut) {
    if (decodedOut != nullptr) {
        decodedOut->Clear();
    }
    if (!rawPayload.IsAvailable() || decodedOut == nullptr ||
        !payloadAsset.IsEncrypted() ||
        payloadAsset.UncompressedByteCount() == 0u ||
        rawPayload.ByteCount() < payloadAsset.RawByteCount()) {
        return 0;
    }
    CGameCtnReplayStaticSolidDecodedPayload decoded;
    if (!decoded.ResizeForDecode(payloadAsset.UncompressedByteCount())) {
        return 0;
    }
    auto reader = CClassicBufferCrypted::CreateBlowfishReaderForMemory(
            rawPayload.Bytes(), rawPayload.ByteCount(), 8ul, key);
    if (reader == nullptr) {
        return 0;
    }
    StaticSolidArchiveStream stream{};
    if (!stream.OpenEncryptedFeedbackPayload(
                &payloadAsset,
                reader.get(),
                decoded.MutableBytes(),
                nullptr,
                nullptr,
                StaticSolidArchiveId::Invalid()) ||
        !stream.ParseEncryptedReferenceTableHeader(
                payloadAsset.UncompressedByteCount(), &decoded)) {
        return 0;
    }
    decodedOut->TakeFrom(&decoded);
    return 1;
}

int CGameCtnReplayStaticSolidArchivePayloadReader::ParseDecodedIntoStore(
        const StaticSolidArchivePayload &payloadAsset,
        const unsigned char *decodedBytes,
        u32 decodedByteCount,
        StaticSolidArchiveLoadSession *materialStore,
        CGameCtnReplayArchiveStaticModelCollection *archiveModels,
        StaticSolidArchiveId payload,
        CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut) {
    if (decodedBytes == nullptr || materialStore == nullptr ||
        decodedByteCount != payloadAsset.UncompressedByteCount()) {
        return 0;
    }
    if (statsOut != nullptr) {
        statsOut->Clear();
    }

    StaticSolidArchiveStream stream{};
    if (!stream.OpenDecodedPayload(&payloadAsset,
                                   decodedBytes,
                                   decodedByteCount,
                                   materialStore,
                                   archiveModels,
                                   payload)) {
        return 0;
    }

    return stream.ParseDecodedPayloadToEnd(payloadAsset.UncompressedByteCount(),
                                           statsOut);
}
