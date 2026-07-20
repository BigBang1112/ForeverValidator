#include "format/static_solid/static_solid_archive_scene_traffic_graph_reader.h"

#include <new>

#include "format/archive/scene_traffic_graph_archive_chunk_ids.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include "format/static_solid/static_solid_archive_node_ref_reader.h"

namespace {

constexpr u32 TrafficGraphBufferCountLimit = 0x100000u;
constexpr u32 TrafficGraphHubWireSize = 2u * sizeof(std::uint16_t);
constexpr u32 TrafficGraphEdgePrefixWireSize =
        6u * sizeof(std::uint16_t) + 3u * sizeof(u32);
constexpr u32 TrafficGraphEdgeSuffixWireSize = 3u * sizeof(u32);

}  // namespace

void CSceneTrafficGraphArchiveState::Reset() {
    nodeHubCounts_.clear();
}

u32 CSceneTrafficGraphArchiveState::HubCount(
        ArchiveNodeReference nodeRef) const {
    const auto entry = nodeHubCounts_.find(nodeRef.Index());
    return entry != nodeHubCounts_.end() ? entry->second : 0u;
}

int CSceneTrafficGraphArchiveState::ReplaceHubCount(
        ArchiveNodeReference nodeRef,
        u32 hubCount) {
    if (!nodeRef.IsValid()) {
        return 0;
    }
    const auto entry = nodeHubCounts_.find(nodeRef.Index());
    if (entry != nodeHubCounts_.end()) {
        entry->second = hubCount;
        return 1;
    }
    try {
        nodeHubCounts_.emplace(nodeRef.Index(), hubCount);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    return 1;
}

CSceneTrafficGraphArchivePayload::CSceneTrafficGraphArchivePayload(
        CGameCtnReplayStaticSolidArchiveCMwIdState *cmwIdState,
        CGameCtnReplayStaticSolidArchiveNodeRefReader *nodeRefs,
        CSceneTrafficGraphArchiveState *state,
        ArchiveNodeReference currentArchiveNode)
        : cmwIdState_(cmwIdState),
          nodeRefs_(nodeRefs),
          state_(state),
          currentArchiveNode_(currentArchiveNode) {}

int CSceneTrafficGraphArchivePayload::ReadHubsAndEdges(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    u32 hubCount = 0u;
    u32 edgeCount = 0u;
    if (byteStream == nullptr || nodeRefs_ == nullptr || state_ == nullptr ||
        !currentArchiveNode_.IsValid() ||
        !byteStream->ReadU32(&hubCount) ||
        !byteStream->ReadU32(&edgeCount) ||
        hubCount > TrafficGraphBufferCountLimit ||
        edgeCount > TrafficGraphBufferCountLimit ||
        !byteStream->SkipCounted(hubCount, TrafficGraphHubWireSize)) {
        return 0;
    }
    for (u32 edgeIndex = 0u; edgeIndex < edgeCount; ++edgeIndex) {
        if (!byteStream->Skip(TrafficGraphEdgePrefixWireSize) ||
            !nodeRefs_->ReadNodPtr(nullptr) ||
            !byteStream->Skip(TrafficGraphEdgeSuffixWireSize)) {
            return 0;
        }
    }
    return state_->ReplaceHubCount(currentArchiveNode_, hubCount);
}

int CSceneTrafficGraphArchivePayload::ReadLegacyHubRecords(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream) {
    if (byteStream == nullptr || cmwIdState_ == nullptr ||
        nodeRefs_ == nullptr || state_ == nullptr ||
        !currentArchiveNode_.IsValid()) {
        return 0;
    }
    const u32 hubCount = state_->HubCount(currentArchiveNode_);
    for (u32 hubIndex = 0u; hubIndex < hubCount; ++hubIndex) {
        u32 nodeIndex = ArchiveNodeReference::InvalidIndex;
        u32 natural = 0u;
        u32 flag = 0u;
        if (!nodeRefs_->ReadNodPtr(&nodeIndex)) {
            return 0;
        }
        if (nodeIndex != ArchiveNodeReference::InvalidIndex &&
            (!cmwIdState_->ReadSkip(*byteStream) ||
             !byteStream->ReadU32(&natural) ||
             !byteStream->ReadBool(&flag))) {
            return 0;
        }
    }
    return 1;
}

int CSceneTrafficGraphArchivePayload::Chunk(
        CGameCtnReplayStaticSolidArchiveByteStream *byteStream,
        u32 chunkId) {
    switch (static_cast<CSceneTrafficGraphArchiveChunkId>(chunkId)) {
    case CSceneTrafficGraphArchiveChunkId::EmptyOld0:
    case CSceneTrafficGraphArchiveChunkId::EmptyOld1:
        return byteStream != nullptr;
    case CSceneTrafficGraphArchiveChunkId::HubLegacy0:
    case CSceneTrafficGraphArchiveChunkId::HubLegacy1:
    case CSceneTrafficGraphArchiveChunkId::HubLegacy2:
        return ReadLegacyHubRecords(byteStream);
    case CSceneTrafficGraphArchiveChunkId::HubsAndEdges:
        return ReadHubsAndEdges(byteStream);
    }
    return 0;
}
