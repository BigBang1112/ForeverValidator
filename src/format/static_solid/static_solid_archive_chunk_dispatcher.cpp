#include "format/static_solid/static_solid_archive_chunk_dispatcher.h"
#include "engine/game/game_ctn_block_info.h"
#include "format/vehicle_tuning/archive_ids.h"
#include "format/static_solid/static_solid_archive_animation_motion_reader.h"
#include "format/static_solid/static_solid_archive_blockinfo_reader.h"
#include "format/static_solid/static_solid_archive_bitmap_reader.h"
#include "format/static_solid/static_solid_archive_decorator_tree_reader.h"
#include "format/static_solid/static_solid_archive_hms_reader.h"
#include "format/static_solid/static_solid_archive_material_reader.h"
#include "format/static_solid/static_solid_archive_scene_reader.h"
#include "format/static_solid/static_solid_archive_shader_reader.h"
#include "format/static_solid/static_solid_archive_solid_reader.h"
#include "format/static_solid/static_solid_archive_tree_reader.h"
#include "format/static_solid/static_solid_archive_vehicle_tuning_reader.h"
#include "format/static_solid/static_solid_archive_vehicle_struct_reader.h"
#include "format/static_solid/static_solid_archive_visual_surface_reader.h"
#include "format/archive/tmnf_archive_ids.h"
enum class StaticSolidArchiveNodeFamily {
  Unsupported,
  BlockInfo,
  VisualSurface,
  VehicleTuning,
  VehicleStruct,
  AnimationMotion,
  Scene,
  Hms,
  Solid,
  Tree,
  Material,
  Bitmap,
  Shader,
  DecoratorTree,
};

static StaticSolidArchiveNodeFamily
classify_static_solid_archive_node_family(u32 classId, u32 chunkId) {
  if (classId == TMNF_CLASS_CGameCtnBlockUnitInfo ||
      classId == TMNF_CLASS_CGameCtnBlockInfo ||
      classId == TMNF_CLASS_CGameCtnBlockInfoFlat ||
      classId == TMNF_CLASS_CGameCtnBlockInfoFrontier ||
      classId == TMNF_CLASS_CGameCtnBlockInfoClassic ||
      classId == TMNF_CLASS_CGameCtnBlockInfoRoad ||
      classId == TMNF_CLASS_CGameCtnBlockInfoClip ||
      classId == TMNF_CLASS_CGameCtnBlockInfoSlope ||
      classId == TMNF_CLASS_CGameCtnBlockInfoPylon ||
      classId == TMNF_CLASS_CGameCtnBlockInfoRectAsym ||
      classId == TMNF_CLASS_CGameCtnBlock) {
    return StaticSolidArchiveNodeFamily::BlockInfo;
  }
  if (classId == TMNF_CLASS_GxLightAmbient ||
      classId == TMNF_CLASS_GxLightDirectional ||
      classId == TMNF_CLASS_CPlugVisualIndexedTriangles ||
      classId == TMNF_CLASS_CPlugVisualSprite ||
      classId == TMNF_CLASS_CPlugVisualGrid ||
      classId == TMNF_CLASS_CPlugSurface ||
      classId == TMNF_CLASS_CPlugSurfaceGeom ||
      classId == TMNF_CLASS_CPlugIndexBuffer ||
      classId == TMNF_CLASS_CPlugDecoratorSolid) {
    return StaticSolidArchiveNodeFamily::VisualSurface;
  }
  if (classId == ArchiveChunkIdValue(FunctionCurveChunkId::Root) ||
      classId == ArchiveChunkIdValue(VehicleTuningContainerChunkId::Root) ||
      classId == ArchiveChunkIdValue(VehicleTuningChunkId::Root)) {
    return StaticSolidArchiveNodeFamily::VehicleTuning;
  }
  if (classId == TMNF_CLASS_CSceneVehicleStruct ||
      classId == TMNF_CLASS_CSceneVehicleMaterialGroup ||
      classId == TMNF_CLASS_CSceneVehicleEmitter) {
    return StaticSolidArchiveNodeFamily::VehicleStruct;
  }
  if (classId == TMNF_CLASS_CFuncKeysNatural ||
      classId == TMNF_CLASS_CFuncSkel ||
      classId == TMNF_CLASS_CFuncKeysSkel ||
      classId == TMNF_CLASS_CFuncTreeSubVisualSequence ||
      classId == TMNF_CLASS_CMotions ||
      classId == TMNF_CLASS_CMotionPlayer ||
      classId == TMNF_CLASS_CMotionSkel ||
      classId == TMNF_CLASS_CMotionEmitterLeaves ||
      classId == TMNF_CLASS_CMotionManagerLeaves ||
      classId == TMNF_CLASS_CMotionWeather ||
      classId == TMNF_CLASS_CMotionDayTime ||
      classId == TMNF_CLASS_CMotionCmdBase ||
      classId == TMNF_CLASS_CMotionShader) {
    return StaticSolidArchiveNodeFamily::AnimationMotion;
  }
  if (classId == TMNF_CLASS_CScenePoc || classId == TMNF_CLASS_CScene3d ||
      classId == TMNF_CLASS_CSceneSector || classId == TMNF_CLASS_CSceneLight ||
      classId == TMNF_CLASS_CSceneSoundSource ||
      classId == TMNF_CLASS_CSceneMobil ||
      classId == TMNF_CLASS_CSceneMobilLeaves ||
      classId == TMNF_CLASS_CSceneVehicleCar ||
      classId == TMNF_CLASS_CSceneVehicleEnvironment ||
      classId == TMNF_CLASS_CSceneObjectLink ||
      classId == TMNF_CLASS_CSceneTrafficGraph) {
    return StaticSolidArchiveNodeFamily::Scene;
  }
  if (classId == TMNF_CLASS_CHmsItem || classId == TMNF_CLASS_CHmsZone ||
      classId == TMNF_CLASS_CHmsZoneDynamic ||
      classId == TMNF_CLASS_CHmsSoundSource ||
      classId == TMNF_CLASS_CHmsLight) {
    return StaticSolidArchiveNodeFamily::Hms;
  }
  if (classId == TMNF_CLASS_CPlugSolid) {
    return StaticSolidArchiveNodeFamily::Solid;
  }
  if (classId == TMNF_CLASS_CPlugTree ||
      classId == TMNF_CLASS_CPlugTreeVisualMip ||
      classId == TMNF_CLASS_CPlugTreeLight) {
    return StaticSolidArchiveNodeFamily::Tree;
  }
  if (classId == TMNF_CLASS_CPlugMaterial ||
      classId == TMNF_CLASS_CPlugMaterialCustom) {
    return StaticSolidArchiveNodeFamily::Material;
  }
  if (classId == TMNF_CLASS_CPlugBitmap ||
      classId == TMNF_CLASS_CPlugBitmapRenderWater) {
    return StaticSolidArchiveNodeFamily::Bitmap;
  }
  if (classId == TMNF_CLASS_CPlugShaderApply ||
      classId == TMNF_CLASS_CPlugShaderPass ||
      classId == TMNF_CLASS_CPlugBitmapApply) {
    return StaticSolidArchiveNodeFamily::Shader;
  }
  if (classId == TMNF_CLASS_CPlugDecoratorTree &&
      chunkId >= TMNF_CLASS_CPlugDecoratorTree && chunkId <= 0x090a2009u) {
    return StaticSolidArchiveNodeFamily::DecoratorTree;
  }
  return StaticSolidArchiveNodeFamily::Unsupported;
}

int CGameCtnReplayStaticSolidArchiveChunkDispatcher::ParseChunk(
    const CGameCtnReplayStaticSolidArchiveChunkDispatchContext &context,
    u32 classId, u32 chunkId) {
  if (chunkId == 0xffffffffu) {
    return 1;
  }
  switch (classify_static_solid_archive_node_family(classId, chunkId)) {
  case StaticSolidArchiveNodeFamily::BlockInfo:
    return CGameCtnReplayStaticSolidArchiveBlockInfoReader::ParseBlockInfoChunk(
        context.byteStream, context.cmwIdState, classId, chunkId,
        context.nodeRefs);
  case StaticSolidArchiveNodeFamily::VisualSurface:
    return CGameCtnReplayStaticSolidArchiveVisualSurfaceReader::
        ParseVisualSurfaceChunk(context, classId, chunkId);
  case StaticSolidArchiveNodeFamily::VehicleTuning:
    return CGameCtnReplayStaticSolidArchiveVehicleTuningReader::
        ParseVehicleTuningChunk(context.byteStream, context.cmwIdState, classId,
                                chunkId, context.nodeRefs,
                                context.feedbackSink);
  case StaticSolidArchiveNodeFamily::VehicleStruct:
    return CGameCtnReplayStaticSolidArchiveVehicleStructReader::ParseChunk(
        context, classId, chunkId);
  case StaticSolidArchiveNodeFamily::AnimationMotion:
    return CGameCtnReplayStaticSolidArchiveAnimationMotionReader::
        ParseAnimationMotionChunk(context.byteStream, context.cmwIdState,
                                  classId, chunkId, context.nodeRefs,
                                  context.embeddedNodeParser,
                                  context.animationMotionState,
                                  context.currentArchiveNode);
  case StaticSolidArchiveNodeFamily::Scene:
    return CGameCtnReplayStaticSolidArchiveSceneReader::ParseSceneChunk(
        context, classId, chunkId);
  case StaticSolidArchiveNodeFamily::Hms:
    return CGameCtnReplayStaticSolidArchiveHmsReader::ParseHmsChunk(
        context.byteStream, context.archiveNodeGraph,
        context.currentArchiveNode.Index(), classId, chunkId, context.nodeRefs);
  case StaticSolidArchiveNodeFamily::Solid:
    return CGameCtnReplayStaticSolidArchiveSolidReader::ParseCPlugSolidChunk(
        context, chunkId);
  case StaticSolidArchiveNodeFamily::Tree:
    return CGameCtnReplayStaticSolidArchiveTreeReader::ParseTreeChunk(
        context, classId, chunkId);
  case StaticSolidArchiveNodeFamily::Material:
    return CGameCtnReplayStaticSolidArchiveMaterialReader::
        ParseCPlugMaterialChunk(
            context.byteStream, context.cmwIdState, classId,
            context.materialStore, context.payload,
            context.currentArchiveNode.Index(), chunkId, context.nodeRefs);
  case StaticSolidArchiveNodeFamily::Bitmap:
    if (classId == TMNF_CLASS_CPlugBitmap) {
      return CGameCtnReplayStaticSolidArchiveBitmapReader::
          ParseCPlugBitmapChunk(context.byteStream, chunkId, context.nodeRefs);
    }
    return CGameCtnReplayStaticSolidArchiveBitmapReader::
        ParseCPlugBitmapRenderWaterChunk(
            context.byteStream, chunkId, context.nodeRefs);
  case StaticSolidArchiveNodeFamily::Shader:
    return CGameCtnReplayStaticSolidArchiveShaderReader::
        ParseCPlugShaderChunk(context.byteStream, context.cmwIdState,
                             classId, chunkId, context.nodeRefs);
  case StaticSolidArchiveNodeFamily::DecoratorTree:
    return CGameCtnReplayStaticSolidArchiveDecoratorTreeReader::
        ParseCPlugDecoratorTreeChunk(context, chunkId);
  case StaticSolidArchiveNodeFamily::Unsupported:
    return 0;
  }
  return 0;
}
