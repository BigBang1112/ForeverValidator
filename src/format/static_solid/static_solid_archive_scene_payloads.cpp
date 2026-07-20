#include "format/static_solid/static_solid_archive_scene_payloads.h"
#include <vector>

#include "format/archive/scene_archive_chunk_ids.h"
#include "format/archive/scene_object_archive_chunk_ids.h"
#include "format/archive/scene_object_link_archive_chunk_ids.h"
#include "format/archive/scene_sector_archive_chunk_ids.h"
#include "format/archive/scene_vehicle_car_archive_schema.h"
#include "format/archive/scene_vehicle_environment_archive_chunk_ids.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_class_info.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_node_graph.h"
#include "format/static_solid/static_solid_archive_primitive_reader.h"
#include "format/static_solid/static_solid_external_node_paths.h"
#include "format/archive/tmnf_archive_ids.h"
namespace {

int ReadDiscardedF32Values(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 count) {
    if (byteStream == nullptr) {
        return 0;
    }
    float value = 0.0f;
    for (u32 index = 0u; index < count; ++index) {
        if (!byteStream->ReadF32(&value)) {
            return 0;
        }
    }
    return 1;
}

int ReadDiscardedU32Values(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 count) {
    if (byteStream == nullptr) {
        return 0;
    }
    u32 value = 0u;
    for (u32 index = 0u; index < count; ++index) {
        if (!byteStream->ReadU32(&value)) {
            return 0;
        }
    }
    return 1;
}

class CSceneArchiveLocalNodeRef {
public:
    static constexpr u32 NullArchiveIndex =
            ArchiveNodeReference::InvalidIndex;
    static constexpr u32 DeferredIndex =
            ArchiveNodeReference::DeferredIndex;

    static CSceneArchiveLocalNodeRef Invalid() {
        return CSceneArchiveLocalNodeRef(
                ArchiveNodeReference::Invalid());
    }

    static CSceneArchiveLocalNodeRef FromArchiveIndex(u32 archiveIndex) {
        return CSceneArchiveLocalNodeRef(
                ArchiveNodeReference::FromIndex(archiveIndex));
    }

    static CSceneArchiveLocalNodeRef FromLocalNode(
            ArchiveNodeReference node) {
        return CSceneArchiveLocalNodeRef(node);
    }

    ArchiveNodeReference LocalNode() const {
        return node;
    }

    int IsNull() const {
        return node.Index() == NullArchiveIndex;
    }

    int IsDeferred() const {
        return node.Index() == DeferredIndex;
    }

private:
    explicit CSceneArchiveLocalNodeRef(
            ArchiveNodeReference node)
        : node(node) {}

    ArchiveNodeReference node;
};

class CSceneArchiveNodeRefPair {
public:
    int Read(CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
        ArchiveNodeReference modelNode =
                ArchiveNodeReference::Invalid();
        ArchiveNodeReference mobilNode =
                ArchiveNodeReference::Invalid();
        if (nodeRefs == nullptr ||
            !nodeRefs->ReadNodeRef(&modelNode) ||
            !nodeRefs->ReadNodeRef(&mobilNode)) {
            return 0;
        }
        model = CSceneArchiveLocalNodeRef::FromLocalNode(modelNode);
        mobil = CSceneArchiveLocalNodeRef::FromLocalNode(mobilNode);
        return 1;
    }

    ArchiveNodeReference Mobil() const {
        return mobil.LocalNode();
    }

private:
    CSceneArchiveLocalNodeRef model = CSceneArchiveLocalNodeRef::Invalid();
    CSceneArchiveLocalNodeRef mobil = CSceneArchiveLocalNodeRef::Invalid();
};

class CSceneMobilArchiveRef {
public:
    int Read(CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
        u32 encodedRef = CSceneArchiveLocalNodeRef::NullArchiveIndex;
        if (byteStream == nullptr || !byteStream->ReadU32(&encodedRef)) {
            return 0;
        }
        ref = CSceneArchiveLocalNodeRef::FromArchiveIndex(encodedRef);
        return 1;
    }

    int IsNull() const {
        return ref.IsNull();
    }

    int IsDeferred() const {
        return ref.IsDeferred();
    }

    CGameCtnReplayStaticSolidArchiveNodeGraph::SceneMobilInternalRefKey
    InternalRefKey() const {
        return CGameCtnReplayStaticSolidArchiveNodeGraph::
                SceneMobilInternalRefKey::FromSceneObjectRef(
                        ref.LocalNode().Index());
    }

private:
    CSceneArchiveLocalNodeRef ref = CSceneArchiveLocalNodeRef::Invalid();
};

int ReadSceneArchiveNodePair(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        ArchiveNodeReference *mobilNodeOut) {
    CSceneArchiveNodeRefPair pair;
    if (!pair.Read(nodeRefs)) {
        return 0;
    }
    if (mobilNodeOut != nullptr) {
        *mobilNodeOut = pair.Mobil();
    }
    return 1;
}

int ReadSceneArchiveNodeRef(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        ArchiveNodeReference *nodeOut) {
    ArchiveNodeReference node =
            ArchiveNodeReference::Invalid();
    if (nodeRefs == nullptr || !nodeRefs->ReadNodeRef(&node)) {
        return 0;
    }
    if (nodeOut != nullptr) {
        *nodeOut = node;
    }
    return 1;
}

}  // namespace

int CSceneArchivePayloadContext::HasNodeArchiveCore() const {
    return cmwIdState != nullptr &&
           archiveNodeGraph != nullptr &&
           nodeRefs != nullptr;
}

int CSceneArchivePayloadContext::HasSceneObjectInstallTargets() const {
    return archiveNodeGraph != nullptr &&
           materialStore != nullptr &&
           archiveModels != nullptr;
}

int CSceneArchiveNodeRefPayload::ReadSingle(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    return nodeRefs != nullptr && nodeRefs->ReadNodPtr(nullptr);
}

int CSceneArchiveNodeRefPayload::ReadCountedArray(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    constexpr u32 kMaxRefCount = 0x100000u;
    u32 count = 0u;
    if (!byteStream->ReadU32(&count) ||
        count > kMaxRefCount) {
        return 0;
    }
    for (u32 i = 0u; i < count; i++) {
        if (!ReadSingle(nodeRefs)) {
            return 0;
        }
    }
    return 1;
}

int CSceneArchiveNodeRefPayload::ReadFastBuffer(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs) {
    if (byteStream == nullptr || nodeRefs == nullptr) {
        return 0;
    }
    u32 reservedWord = 0u;
    if (!byteStream->ReadU32(&reservedWord)) {
        return 0;
    }
    return ReadCountedArray(byteStream, nodeRefs);
}

CSceneMobilReferencePayload::CSceneMobilReferencePayload(
        CGameCtnReplayStaticSolidArchiveNodeGraph *archiveNodeGraph,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : archiveNodeGraph_(archiveNodeGraph),
          nodeRefs_(nodeRefs) {}

int CSceneMobilReferencePayload::ReadBeforeIso(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        CGameCtnReplayStaticSolidArchiveNodeParser *nodeParser) {
    if (byteStream == nullptr || archiveNodeGraph_ == nullptr ||
        nodeParser == nullptr) {
        return 0;
    }
    u32 index = 0u;
    if (!byteStream->ReadU32(&index)) {
        return 0;
    }
    const ArchiveNodeReference nodeRef =
            ArchiveNodeReference::FromIndex(index);
    const CGameCtnReplayStaticSolidArchiveNodeGraph::Node *knownNode =
            archiveNodeGraph_->FindNode(nodeRef);
    if (nodeRef.IsInvalid() ||
        (knownNode != nullptr && knownNode->IsPresent() &&
         !knownNode->IsExternal())) {
        return 1;
    }
    if (!byteStream->Ensure(byteStream->Offset() + 4u)) {
        return 0;
    }
    u32 nextWord = 0u;
    if (!byteStream->PeekU32(&nextWord)) {
        return 0;
    }
    const u32 nextClass =
            TMNF_CPlugTreeCanonicalChunkIdFromArchiveWord(nextWord);
    if (CGameCtnReplayStaticSolidArchiveClassInfo::IsKnownNodeClass(nextClass)) {
        u32 rawClassId = 0u;
        if (!byteStream->ReadU32(&rawClassId) ||
            !archiveNodeGraph_->EnsureNodeCapacity(index)) {
            return 0;
        }
        const u32 classId =
                TMNF_CPlugTreeCanonicalChunkIdFromArchiveWord(rawClassId);
        archiveNodeGraph_->MarkInlineNode(nodeRef, classId);
        return nodeParser->ParseNodeArchiveInternal(nodeRef, classId, 1, 1);
    }
    const u32 searchStart = byteStream->Offset();
    const u32 payloadUncompressedSize = byteStream->PayloadUncompressedSize();
    if (payloadUncompressedSize < 56u ||
        searchStart > payloadUncompressedSize - 56u) {
        return 0;
    }
    for (u32 cursor = searchStart;
         cursor <= payloadUncompressedSize - 56u;
         cursor++) {
        if (!byteStream->Ensure(cursor + 56u)) {
            return 0;
        }
        u32 nextChunk = 0u;
        if (!byteStream->PeekU32At(cursor + 52u, &nextChunk)) {
            return 0;
        }
        if (CGameCtnReplayStaticSolidArchiveClassInfo::
                    WordIsSceneObjectLinkChunk(nextChunk)) {
            return byteStream->Skip(cursor - byteStream->Offset());
        }
    }
    return 0;
}

int CSceneMobilReferencePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        int internalRefMode,
        ArchiveNodeReference *mobilNodeOut) {
    if (byteStream == nullptr || archiveNodeGraph_ == nullptr ||
        nodeRefs_ == nullptr) {
        return 0;
    }
    if (mobilNodeOut != nullptr) {
        *mobilNodeOut =
                ArchiveNodeReference::Invalid();
    }
    if (!internalRefMode) {
        return ReadSceneArchiveNodePair(nodeRefs_, mobilNodeOut);
    }
    CSceneMobilArchiveRef ref;
    if (!ref.Read(byteStream)) {
        return 0;
    }
    if (ref.IsNull()) {
        return 1;
    }
    if (ref.IsDeferred()) {
        return ReadSceneArchiveNodeRef(nodeRefs_, mobilNodeOut);
    }
    if (!byteStream->Ensure(byteStream->Offset() + 4u)) {
        return 0;
    }
    u32 nextWord = 0u;
    if (!byteStream->PeekU32(&nextWord)) {
        return 0;
    }
    const u32 nextClass =
            TMNF_CPlugTreeCanonicalChunkIdFromArchiveWord(nextWord);
    if (!CGameCtnReplayStaticSolidArchiveClassInfo::IsKnownNodeClass(nextClass)) {
        if (mobilNodeOut != nullptr) {
            *mobilNodeOut =
                    archiveNodeGraph_->SceneMobilRefNode(ref.InternalRefKey());
        }
        return 1;
    }
    ArchiveNodeReference mobilNode =
            ArchiveNodeReference::Invalid();
    const int ok = ReadSceneArchiveNodePair(nodeRefs_, &mobilNode) &&
                   archiveNodeGraph_->RememberSceneMobilRef(
                           ref.InternalRefKey(),
                           mobilNode);
    if (ok && mobilNodeOut != nullptr) {
        *mobilNodeOut = mobilNode;
    }
    return ok;
}

CSceneObjectPlacementArchivePayload::CSceneObjectPlacementArchivePayload(
        const CSceneArchivePayloadContext &context)
        : context_(context) {}

int CSceneObjectPlacementArchivePayload::Append(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        ArchiveNodeReference mobilNode,
        const float sceneIso[12]) {
    if (byteStream == nullptr || context_.archiveNodeGraph == nullptr) {
        return 0;
    }
    const StaticSolidArchivePayload *payloadAsset =
            byteStream->PayloadAsset();
    if (context_.materialStore == nullptr || context_.archiveModels == nullptr ||
        payloadAsset == nullptr || sceneIso == nullptr ||
        !mobilNode.IsValid()) {
        return 1;
    }
    const ArchiveNodeReference solidNode =
            context_.archiveNodeGraph->SolidForSceneMobil(mobilNode);
    const ArchiveNodeReference treeNode =
            context_.archiveNodeGraph->TreeForSolid(context_.materialStore,
                                            context_.payload,
                                            solidNode);
    ArchiveNodeReference modelFidNode =
            ArchiveNodeReference::Invalid();
    if (solidNode.IsValid() && !treeNode.IsValid()) {
        modelFidNode = context_.archiveNodeGraph->ModelFidForSolid(solidNode);
    }
    CGameCtnReplayStaticSolidExternalNodePathResult modelPath;
    if (modelFidNode.IsValid()) {
        (void)CGameCtnReplayStaticSolidExternalNodePathResolver::
                ResolveSelectedPath(
                        context_.externalFolders,
                        context_.materialStore->ExternalPack(),
                        context_.archiveNodeGraph->ExternalNodeRef(modelFidNode),
                        1,
                        &modelPath);
    }
    if (!solidNode.IsValid() ||
        (!treeNode.IsValid() &&
         !modelPath.HasSelectedPath())) {
        return 1;
    }

    const char *sceneObjectId = nullptr;
    if (const CGameCtnReplayStaticSolidArchiveNodeGraph::Node *mobilGraphNode =
                context_.archiveNodeGraph->FindNode(mobilNode)) {
        if (!mobilGraphNode->Name().empty()) {
            sceneObjectId = mobilGraphNode->Name().c_str();
        }
    }

    StaticSceneArchiveModelRecord archiveModel;
    std::optional<CHmsItem::Properties> itemProperties;
    if (const HmsItemArchiveWords *state =
                context_.archiveNodeGraph->HmsItemStateForSceneMobil(
                        mobilNode)) {
        itemProperties = DecodeHmsItemArchiveState(*state);
    }
    if (modelPath.HasSelectedPath()) {
        if (!archiveModel.ConfigureFromSceneObject(
                    sceneIso,
                    treeNode.Index(),
                    sceneObjectId,
                    modelPath.SelectedPath(),
                    itemProperties ? &*itemProperties : nullptr)) {
            return 0;
        }
    } else {
        if (!archiveModel.ConfigureFromSceneObject(
                    sceneIso,
                    treeNode.Index(),
                    sceneObjectId,
                    payloadAsset->SelectedDescriptorPath(),
                    itemProperties ? &*itemProperties : nullptr)) {
            return 0;
        }
    }
    return context_.archiveModels->AppendDecodedSceneObject(archiveModel);
}

CSceneObjectBufferArchivePayload::CSceneObjectBufferArchivePayload(
        const CSceneArchivePayloadContext &context)
        : context_(context) {}

int CSceneObjectBufferArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        int mobilBuffer) {
    if (byteStream == nullptr || context_.archiveNodeGraph == nullptr || context_.nodeRefs == nullptr) {
        return 0;
    }
    constexpr u32 kMaxObjectCount = 0x100000u;
    if (mobilBuffer) {
        u32 count = 0u;
        if (!byteStream->ReadU32(&count) ||
            count > kMaxObjectCount) {
            return 0;
        }
        std::vector<ArchiveNodeReference> mobilNodes;
        if (count != 0u) {
            mobilNodes.assign(
                    count,
                    ArchiveNodeReference::Invalid());
        }
        CSceneMobilReferencePayload mobilRefs(context_.archiveNodeGraph, context_.nodeRefs);
        for (u32 i = 0u; i < count; i++) {
            if (!mobilRefs.Read(byteStream, 1, &mobilNodes[i])) {
                return 0;
            }
        }
        u32 locCount = 0u;
        if (!byteStream->ReadU32(&locCount) ||
            locCount > kMaxObjectCount) {
            return 0;
        }
        CSceneObjectPlacementArchivePayload placements(context_);
        for (u32 i = 0u; i < locCount; i++) {
            ArchiveNodeReference objectNode =
                    ArchiveNodeReference::Invalid();
            float sceneIso[12]{};
            if (!ReadSceneArchiveNodeRef(context_.nodeRefs, &objectNode) ||
                !CGameCtnReplayStaticSolidArchivePrimitiveReader::ReadIso4(
                        byteStream,
                        sceneIso)) {
                return 0;
            }
            const ArchiveNodeReference mobilNode =
                    i < count
                            ? mobilNodes[i]
                            : objectNode;
            if (!placements.Append(byteStream, mobilNode, sceneIso)) {
                return 0;
            }
        }
        return 1;
    }
    if (!CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                           context_.nodeRefs)) {
        return 0;
    }
    u32 locCount = 0u;
    if (!byteStream->ReadU32(&locCount) ||
        locCount > kMaxObjectCount) {
        return 0;
    }
    for (u32 i = 0u; i < locCount; i++) {
        if (!CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) ||
            !CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipMat3(byteStream) ||
            !byteStream->Skip(3u * 4u)) {
            return 0;
        }
    }
    return 1;
}

CScene3dObjectBuffersArchivePayload::CScene3dObjectBuffersArchivePayload(
        const CSceneArchivePayloadContext &context)
        : context_(context) {}

int CScene3dObjectBuffersArchivePayload::ReadTrailingMemoryArchiveChunks(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    CScene3dArchivePayload scene3d(context_);
    return scene3d.Read(
                   byteStream,
                   ArchiveChunkIdValue(
                           CScene3dArchiveChunkId::TrafficGraphPaths)) &&
           scene3d.Read(
                   byteStream,
                   ArchiveChunkIdValue(
                           CScene3dArchiveChunkId::SingleNodRef));
}

int CScene3dObjectBuffersArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    const u32 mode = CScene3dObjectBufferArchiveMode(chunkId);
    if (!CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                           context_.nodeRefs)) {
        return 0;
    }
    if (mode == 1u &&
        !CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                           context_.nodeRefs)) {
        return 0;
    }
    CSceneObjectBufferArchivePayload objectBuffer(context_);
    u32 version = 0u;
    if (mode >= 3u &&
        (!byteStream->ReadU32(&version) ||
         !objectBuffer.Read(byteStream, 1) ||
         !objectBuffer.Read(byteStream, 0) ||
         !objectBuffer.Read(byteStream, 0) ||
         !objectBuffer.Read(byteStream, 0) ||
         !objectBuffer.Read(byteStream, 0) ||
         !objectBuffer.Read(byteStream, 0))) {
        return 0;
    }
    if (mode < 3u &&
        (!objectBuffer.Read(byteStream, 0) ||
         !objectBuffer.Read(byteStream, 0) ||
         !objectBuffer.Read(byteStream, 0) ||
         !objectBuffer.Read(byteStream, 0) ||
         !objectBuffer.Read(byteStream, 0) ||
         !objectBuffer.Read(byteStream, 0))) {
        return 0;
    }
    if (mode >= 2u &&
        !CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                           context_.nodeRefs)) {
        return 0;
    }
    if (!CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                           context_.nodeRefs)) {
        return 0;
    }
    return ReadTrailingMemoryArchiveChunks(byteStream);
}

CSceneSectorArchivePayload::CSceneSectorArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs) {}

int CSceneSectorArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    switch (chunkId) {
        case TMNF_CLASS_CSceneSector:
            return CSceneArchiveNodeRefPayload::ReadSingle(nodeRefs_) &&
                   CSceneArchiveNodeRefPayload::ReadSingle(nodeRefs_);
        case ArchiveChunkIdValue(CSceneSectorArchiveChunkId::Iso):
            return CGameCtnReplayStaticSolidArchivePrimitiveReader::
                    SkipIso4(byteStream);
        case ArchiveChunkIdValue(CSceneSectorArchiveChunkId::Id):
            return cmwIdState_ != nullptr && cmwIdState_->ReadSkip(*byteStream);
        case ArchiveChunkIdValue(CSceneSectorArchiveChunkId::BoxAlignedOld1):
        case ArchiveChunkIdValue(CSceneSectorArchiveChunkId::BoxAligned):
            return CGameCtnReplayStaticSolidArchivePrimitiveReader::
                    SkipBoxAligned(byteStream);
        default:
            return 0;
    }
}

CScene3dArchivePayload::CScene3dArchivePayload(
        const CSceneArchivePayloadContext &context)
        : context_(context) {}

int CScene3dArchivePayload::ReadRefLocBuffer(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    constexpr u32 kMaxLocCount = 0x100000u;
    u32 count = 0u;
    if (!byteStream->ReadU32(&count) ||
        count > kMaxLocCount) {
        return 0;
    }
    for (u32 i = 0u; i < count; i++) {
        if (!CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) ||
            !CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) ||
            !CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipIso4(
                    byteStream)) {
            return 0;
        }
    }
    return 1;
}

int CScene3dArchivePayload::ReadTrafficPairLocs(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    constexpr u32 kMaxTrafficCount = 0x100000u;
    u32 count = 0u;
    if (!byteStream->ReadU32(&count) ||
        count > kMaxTrafficCount) {
        return 0;
    }
    for (u32 i = 0u; i < (count >> 1u); i++) {
        if (!CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) ||
            !context_.cmwIdState->ReadSkip(*byteStream) ||
            !byteStream->Skip(4u) ||
            !CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) ||
            !context_.cmwIdState->ReadSkip(*byteStream) ||
            !byteStream->Skip(4u) ||
            !CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipIso4(
                    byteStream) ||
            !CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipIso4(
                    byteStream)) {
            return 0;
        }
    }
    return 1;
}

int CScene3dArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || context_.cmwIdState == nullptr || context_.nodeRefs == nullptr) {
        return 0;
    }
    u32 value = 0u;
    switch (chunkId) {
        case ArchiveChunkIdValue(CSceneArchiveChunkId::Root):
        case ArchiveChunkIdValue(CSceneArchiveChunkId::EmptyOld0):
        case ArchiveChunkIdValue(CSceneArchiveChunkId::OldSerializeMobils0):
        case ArchiveChunkIdValue(CSceneArchiveChunkId::OldSerializeMobils1):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld0):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld1):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld2):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld3):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::EmptyOld4):
            return 1;
        case ArchiveChunkIdValue(CSceneArchiveChunkId::RefBuffer):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs);
        case ArchiveChunkIdValue(CSceneArchiveChunkId::FormatArray):
            return byteStream->ReadU32(&value) &&
                   value <= 0x100000u &&
                   byteStream->SkipCounted(value, 4u);
        case ArchiveChunkIdValue(CSceneArchiveChunkId::MotionManagers):
            return CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                                     context_.nodeRefs);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::Root):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::SectorBuffer):
            return CSceneArchiveNodeRefPayload::ReadCountedArray(
                    byteStream,
                    context_.nodeRefs);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::SectorLocBuffer):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::FieldBuffer):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectLocBuffer):
            return ReadRefLocBuffer(byteStream);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::PathBuffer):
            return CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                                     context_.nodeRefs);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::DefaultSectorIso):
            return byteStream->Skip(4u) &&
                   CGameCtnReplayStaticSolidArchivePrimitiveReader::SkipIso4(
                           byteStream);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::OldMood):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   byteStream->ReadStringSkip();
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::NaturalField):
            return byteStream->Skip(4u);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::TrafficPairLocs):
            return ReadTrafficPairLocs(byteStream);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::TrafficGraphPaths):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                                     context_.nodeRefs);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::Fog):
            return byteStream->Skip(16u);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::NodRef):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::NodRefBuffer):
            return (CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
           CSceneArchiveNodeRefPayload::ReadCountedArray((byteStream),
                                                               context_.nodeRefs));
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::FrustumIso):
            return byteStream->Skip(4u) &&
                   CGameCtnReplayStaticSolidArchivePrimitiveReader::
                           SkipFrustumIso4(byteStream);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::SingleNodRef):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs);
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV1):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV2):
        case ArchiveChunkIdValue(CScene3dArchiveChunkId::ObjectBuffersV3): {
            CScene3dObjectBuffersArchivePayload objectBuffers(context_);
            return objectBuffers.Read(byteStream, chunkId);
        }
        default:
            return 0;
    }
}

CSceneObjectOrMobilArchivePayload::CSceneObjectOrMobilArchivePayload(
        const CSceneArchivePayloadContext &context)
        : context_(context) {}

int CSceneObjectOrMobilArchivePayload::ReadSceneObjectId(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    if (!context_.archiveNodeGraph->HasNode(context_.currentArchiveNode)) {
        return context_.cmwIdState->ReadSkip(*byteStream);
    }
    char nodeNameBuffer[CGameCtnReplayStaticMobilModelNameCapacity]{};
    if (!context_.cmwIdState->ReadText(*byteStream,
                               nodeNameBuffer,
                               sizeof(nodeNameBuffer))) {
        return 0;
    }
    return context_.archiveNodeGraph->SetNodeName(context_.currentArchiveNode, nodeNameBuffer);
}

int CSceneObjectOrMobilArchivePayload::ReadVehicleRefCluster(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    switch (chunkId) {
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::Root):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::WheelIdsOld):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::WheelVisualIdsOld):
            return CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                                     context_.nodeRefs);
        case ArchiveChunkIdValue(
                CSceneVehicleCarArchiveChunkId::MaterialRefsAndNaturalOld):
            return CSceneArchiveNodeRefPayload::ReadFastBuffer(byteStream,
                                                                     context_.nodeRefs) &&
                   byteStream->Skip(4u);
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::TuningContainer):
        case ArchiveChunkIdValue(
                CSceneVehicleCarArchiveChunkId::TuningContainerDiscardOld):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::MaterialContainer):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::VehicleStructRef):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs);
        case ArchiveChunkIdValue(
                CSceneVehicleCarArchiveChunkId::TuningContainerAndVectorOld0):
        case ArchiveChunkIdValue(
                CSceneVehicleCarArchiveChunkId::TuningContainerAndVectorOld1):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   byteStream->Skip(12u);
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::ThreeNodeRefsOld):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs);
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::SpeedParamsOld):
            return byteStream->Skip(16u);
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::TwoMaterialRefsOld):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs);
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::PhysicalParams):
            return byteStream->Skip(32u);
        default:
            return 0;
    }
}

int CSceneObjectOrMobilArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || context_.cmwIdState == nullptr ||
        context_.archiveNodeGraph == nullptr || context_.nodeRefs == nullptr) {
        return 0;
    }
    switch (chunkId) {
        case TMNF_CLASS_CMwNod:
        case TMNF_CLASS_CSceneObject:
        case TMNF_CLASS_CSceneMobil:
        case ArchiveChunkIdValue(CSceneMobilArchiveChunkId::EmptyOld0):
        case ArchiveChunkIdValue(CSceneMobilArchiveChunkId::EmptyOld1):
        case ArchiveChunkIdValue(CSceneMobilArchiveChunkId::BoolMarker):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::EmptyOld0):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::EmptyOld1):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::EmptyOld2):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::EmptyOld3):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::EmptyOld4):
        case ArchiveChunkIdValue(CSceneVehicleCarArchiveChunkId::EmptyOld5):
            return 1;
        case ArchiveChunkIdValue(CSceneObjectArchiveChunkId::Id):
            return ReadSceneObjectId(byteStream);
        case ArchiveChunkIdValue(CSceneObjectArchiveChunkId::BoolFlag):
        case ArchiveChunkIdValue(CSceneObjectArchiveChunkId::Natural):
        case TMNF_CLASS_CScenePoc:
            return byteStream->Skip(4u);
        case ArchiveChunkIdValue(CSceneObjectArchiveChunkId::MotionRef):
        case ArchiveChunkIdValue(CSceneMobilArchiveChunkId::MessageHandlerRef):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs);
        case TMNF_CLASS_CSceneLight:
            return context_.nodeParser != nullptr &&
                   context_.nodeParser->ParseNodeArchiveInternal(
                           context_.currentArchiveNode,
                           TMNF_CLASS_CHmsLight,
                           1,
                           1);
        case TMNF_CLASS_CSceneSoundSource:
            return context_.nodeParser != nullptr &&
                   context_.nodeParser->ParseNodeArchiveInternal(
                           context_.currentArchiveNode,
                           TMNF_CLASS_CHmsSoundSource,
                           1,
                           1);
        case ArchiveChunkIdValue(CSceneMobilArchiveChunkId::ChildObjects):
            return CSceneArchiveNodeRefPayload::ReadCountedArray(byteStream,
                                                                       context_.nodeRefs);
        case ArchiveChunkIdValue(CSceneMobilArchiveChunkId::HmsItemData):
            return context_.nodeParser != nullptr &&
                   context_.nodeParser->ParseNodeArchiveInternal(
                           context_.currentArchiveNode,
                           TMNF_CLASS_CHmsItem,
                           1,
                           1);
        case TMNF_CLASS_CSceneVehicle:
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs);
        default:
            return ReadVehicleRefCluster(byteStream, chunkId);
    }
}

CSceneMobilLeavesArchivePayload::CSceneMobilLeavesArchivePayload(
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs)
        : nodeRefs_(nodeRefs) {}

int CSceneMobilLeavesArchivePayload::Chunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || nodeRefs_ == nullptr) {
        return 0;
    }
    if (!IsCSceneMobilLeavesInfo1Chunk(chunkId) &&
        !IsCSceneMobilLeavesInfo3Chunk(chunkId)) {
        return 0;
    }
    if (!CSceneArchiveNodeRefPayload::ReadSingle(nodeRefs_)) {
        return 0;
    }
    switch (chunkId) {
        case ArchiveChunkIdValue(CSceneMobilLeavesArchiveChunkId::Legacy):
            return ReadDiscardedF32Values(byteStream, 4u) &&
                   ReadDiscardedU32Values(byteStream, 1u) &&
                   ReadDiscardedF32Values(byteStream, 15u);
        case ArchiveChunkIdValue(
                CSceneMobilLeavesArchiveChunkId::
                        LegacyWithoutFinalScalar):
            return ReadDiscardedF32Values(byteStream, 2u) &&
                   ReadDiscardedU32Values(byteStream, 1u) &&
                   ReadDiscardedF32Values(byteStream, 13u);
        case ArchiveChunkIdValue(
                CSceneMobilLeavesArchiveChunkId::LegacyWithFinalScalar):
            return ReadDiscardedF32Values(byteStream, 2u) &&
                   ReadDiscardedU32Values(byteStream, 1u) &&
                   ReadDiscardedF32Values(byteStream, 14u);
        case ArchiveChunkIdValue(CSceneMobilLeavesArchiveChunkId::Root):
            return ReadDiscardedF32Values(byteStream, 2u) &&
                   ReadDiscardedU32Values(byteStream, 2u) &&
                   ReadDiscardedF32Values(byteStream, 14u);
        default:
            return 0;
    }
}

CSceneVehicleEnvironmentArchivePayload::
        CSceneVehicleEnvironmentArchivePayload(
        const CSceneArchivePayloadContext &context)
        : context_(context) {}

int CSceneVehicleEnvironmentArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (chunkId == TMNF_CLASS_CSceneVehicleEnvironment) {
        return byteStream->Skip(4u);
    }
    if (chunkId ==
        ArchiveChunkIdValue(
                CSceneVehicleEnvironmentArchiveChunkId::MaterialRef)) {
        return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs);
    }
    if (chunkId ==
        ArchiveChunkIdValue(
                CSceneVehicleEnvironmentArchiveChunkId::MaterialRefArray)) {
        return CSceneArchiveNodeRefPayload::ReadCountedArray(
                byteStream, context_.nodeRefs);
    }
    return 0;
}

CSceneObjectLinkArchivePayload::CSceneObjectLinkArchivePayload(
        const CSceneArchivePayloadContext &context)
        : context_(context) {}

int CSceneObjectLinkArchivePayload::Read(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    if (byteStream == nullptr || context_.cmwIdState == nullptr ||
        context_.archiveNodeGraph == nullptr || context_.nodeRefs == nullptr) {
        return 0;
    }
    u32 value = 0u;
    switch (chunkId) {
        case ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::Root):
        case ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::LegacyRoot):
            return CSceneArchiveNodeRefPayload::ReadSingle(context_.nodeRefs) &&
                   byteStream->Skip(48u) &&
                   byteStream->ReadBool(&value);
        case ArchiveChunkIdValue(
                CSceneObjectLinkArchiveChunkId::MobilOrObjectRefBeforeIso): {
            if (!byteStream->ReadBool(&value)) {
                return 0;
            }
            CSceneMobilReferencePayload mobilRefs(context_.archiveNodeGraph, context_.nodeRefs);
            return (value != 0u
                        ? mobilRefs.ReadBeforeIso(byteStream, context_.nodeParser)
                        : CSceneArchiveNodeRefPayload::ReadSingle(
                                  context_.nodeRefs)) &&
                   byteStream->Skip(48u) &&
                   byteStream->ReadBool(&value);
        }
        case ArchiveChunkIdValue(
                CSceneObjectLinkArchiveChunkId::LegacyPositionAndActiveFlag):
            return byteStream->Skip(6u * 4u) &&
                   byteStream->ReadBool(&value);
        case ArchiveChunkIdValue(
                CSceneObjectLinkArchiveChunkId::LegacyActiveFlag):
            return byteStream->ReadBool(&value);
        case ArchiveChunkIdValue(
                CSceneObjectLinkArchiveChunkId::MobilTreeIdAndInstallFlags):
            return context_.cmwIdState->ReadSkip(*byteStream) &&
                   byteStream->ReadBool(&value) &&
                   byteStream->ReadBool(&value);
        case ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::ArchivedTreeId):
            return context_.cmwIdState->ReadSkip(*byteStream);
        default:
            return 0;
    }
}
