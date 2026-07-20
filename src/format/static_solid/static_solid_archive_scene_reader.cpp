#include "format/static_solid/static_solid_archive_scene_reader.h"
#include "format/static_solid/static_solid_archive_chunk_dispatcher.h"
#include "format/static_solid/static_solid_archive_scene_payloads.h"
#include "format/static_solid/static_solid_archive_scene_traffic_graph_reader.h"
#include "format/archive/scene_object_archive_chunk_ids.h"
#include "format/archive/tmnf_archive_ids.h"
int CGameCtnReplayStaticSolidArchiveSceneReader::ParseSceneChunk(
    const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
    u32 classId, u32 chunkId) {
  if (context.byteStream == nullptr || context.cmwIdState == nullptr ||
      context.archiveNodeGraph == nullptr || context.nodeRefs == nullptr) {
    return 0;
  }
  const CSceneArchivePayloadContext sceneContext{
      context.cmwIdState,         context.archiveNodeGraph,
      context.externalFolders,    context.materialStore,
      context.archiveModels,      context.payload,
      context.currentArchiveNode, context.nodeRefs,
      context.nodeParser};
  if (classId == TMNF_CLASS_CSceneSector) {
    CSceneSectorArchivePayload sectorPayload(context.cmwIdState,
                                             context.nodeRefs);
    return sectorPayload.Read(context.byteStream, chunkId);
  }
  if (classId == TMNF_CLASS_CScene3d) {
    CScene3dArchivePayload scene3dPayload(sceneContext);
    return scene3dPayload.Read(context.byteStream, chunkId);
  }
  if (classId == TMNF_CLASS_CScenePoc || classId == TMNF_CLASS_CSceneLight ||
      classId == TMNF_CLASS_CSceneSoundSource ||
      classId == TMNF_CLASS_CSceneMobil ||
      classId == TMNF_CLASS_CSceneMobilLeaves ||
      classId == TMNF_CLASS_CSceneVehicleCar) {
    if (classId == TMNF_CLASS_CSceneMobilLeaves &&
        (IsCSceneMobilLeavesInfo1Chunk(chunkId) ||
         IsCSceneMobilLeavesInfo3Chunk(chunkId))) {
      CSceneMobilLeavesArchivePayload leavesPayload(context.nodeRefs);
      return leavesPayload.Chunk(context.byteStream, chunkId);
    }
    CSceneObjectOrMobilArchivePayload objectPayload(sceneContext);
    return objectPayload.Read(context.byteStream, chunkId);
  }
  if (classId == TMNF_CLASS_CSceneVehicleEnvironment) {
    CSceneVehicleEnvironmentArchivePayload environmentPayload(sceneContext);
    return environmentPayload.Read(context.byteStream, chunkId);
  }
  if (classId == TMNF_CLASS_CSceneObjectLink) {
    CSceneObjectLinkArchivePayload linkPayload(sceneContext);
    return linkPayload.Read(context.byteStream, chunkId);
  }
  if (classId == TMNF_CLASS_CSceneTrafficGraph) {
    CSceneTrafficGraphArchivePayload trafficGraphPayload(
        context.cmwIdState, context.nodeRefs, context.trafficGraphState,
        context.currentArchiveNode);
    return trafficGraphPayload.Chunk(context.byteStream, chunkId);
  }
  return 0;
}
