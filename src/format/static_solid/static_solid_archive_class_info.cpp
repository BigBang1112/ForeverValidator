#include "format/static_solid/static_solid_archive_class_info.h"
#include "engine/game/game_ctn_block_info.h"
#include "engine/game/game_skin.h"
#include <limits>
#include <optional>

#include "format/archive/scene_vehicle_car_archive_schema.h"
#include "format/vehicle_tuning/archive_ids.h"
#include "format/archive/archive_class_ids.h"
#include "format/pack/block_info_catalog/blockinfo_archive_chunk_ids.h"
#include "format/pack/block_info_catalog/blockunitinfo_archive_chunk_ids.h"
#include "format/materials/material_archive_schema.h"
#include "format/archive/scene_archive_chunk_ids.h"
#include "format/archive/scene_object_archive_chunk_ids.h"
#include "format/archive/scene_object_link_archive_chunk_ids.h"
#include "format/archive/scene_sector_archive_chunk_ids.h"
#include "format/archive/scene_vehicle_environment_archive_chunk_ids.h"
#include "format/static_solid/static_solid_archive_animation_motion_chunk_ids.h"
#include "format/static_solid/static_solid_archive_decorator_tree_chunk_ids.h"
#include "format/static_solid/static_solid_archive_hms_chunk_ids.h"
#include "format/static_solid/static_solid_archive_solid_chunk_ids.h"
#include "format/static_solid/static_solid_archive_tree_chunk_ids.h"
#include "format/static_solid/static_solid_archive_visual_surface_chunk_ids.h"
#include "format/archive/tmnf_archive_ids.h"
int CGameCtnReplayStaticSolidArchiveClassInfo::IsUnknownClassId(u32 classId) {
  return classId == std::numeric_limits<u32>::max();
}

int CGameCtnReplayStaticSolidArchiveClassInfo::IsZipFileClass(u32 classId) {
  return classId == TMNF_CLASS_CPlugFileZip ? 1 : 0;
}

int CGameCtnReplayStaticSolidArchiveClassInfo::IsBlockInfoClass(u32 classId) {
  return IsCGameCtnBlockInfoArchiveClass(classId);
}

u32 CGameCtnReplayStaticSolidArchiveClassInfo::FeedbackClassId(u32 classId) {
  /*
   * Inline node records identify the feedback base class before their chunk
   * stream. Map each concrete archive class to that required base class.
   */
  switch (classId) {
  case TMNF_CLASS_CGameCtnBlockInfo:
  case TMNF_CLASS_CGameCtnBlockInfoFlat:
  case TMNF_CLASS_CGameCtnBlockInfoFrontier:
  case TMNF_CLASS_CGameCtnBlockInfoClassic:
  case TMNF_CLASS_CGameCtnBlockInfoRoad:
  case TMNF_CLASS_CGameCtnBlockInfoClip:
  case TMNF_CLASS_CGameCtnBlockInfoPylon:
  case TMNF_CLASS_CGameCtnBlockInfoRectAsym:
  case TMNF_CLASS_CGameCtnBlock:
    return 0x24005000u;
  case TMNF_CLASS_CGameSkin:
  case TMNF_CLASS_CGameCtnBlockUnitInfo:
  case TMNF_CLASS_CSceneVehicleEnvironment:
  case TMNF_CLASS_CSceneSector:
  case TMNF_CLASS_CHmsItem:
  case TMNF_CLASS_CSceneObjectLink:
    return TMNF_CLASS_CMwNod;
  case TMNF_CLASS_GxLightAmbient:
  case TMNF_CLASS_GxLightDirectional:
    return TMNF_CLASS_GxLight;
  case TMNF_CLASS_CFuncKeysNatural:
    return TMNF_CLASS_CFuncKeys;
  case TMNF_CLASS_CFuncTreeSubVisualSequence:
    return TMNF_CLASS_CFuncTree;
  case ArchiveChunkIdValue(FunctionCurveChunkId::Root):
    return TMNF_CLASS_CFuncKeys;
  case ArchiveChunkIdValue(VehicleTuningContainerChunkId::Root):
    return TMNF_CLASS_CMwNod;
  case ArchiveChunkIdValue(VehicleTuningChunkId::Root):
    return ArchiveChunkIdValue(VehicleTuningBaseChunkId::Base);
  case TMNF_CLASS_CScene3d:
    return TMNF_CLASS_CScene;
  case TMNF_CLASS_CSceneMobil:
    return TMNF_CLASS_CSceneObject;
  case TMNF_CLASS_CSceneLight:
    return TMNF_CLASS_CSceneObject;
  case TMNF_CLASS_CSceneVehicleCar:
    return TMNF_CLASS_CSceneVehicle;
  case TMNF_CLASS_CSceneSoundSource:
    return TMNF_CLASS_CScenePoc;
  case TMNF_CLASS_CMotionPlayer:
    return TMNF_CLASS_CMotion;
  case TMNF_CLASS_CMotionCmdBase:
    return ArchiveChunkIdValue(CMotionCmdBaseArchiveChunkId::PeriodAndPhase);
  case TMNF_CLASS_CMotionShader:
    return TMNF_CLASS_CMotionShader;
  case TMNF_CLASS_CHmsZone:
  case TMNF_CLASS_CHmsZoneDynamic:
    return TMNF_CLASS_CHmsZone;
  case TMNF_CLASS_CHmsLight:
  case TMNF_CLASS_CHmsSoundSource:
    return TMNF_CLASS_CHmsPocEmitter;
  case TMNF_CLASS_CPlugSolid:
    return TMNF_CLASS_CPlug;
  case TMNF_CLASS_CPlugVisualIndexedTriangles:
    return TMNF_CLASS_CPlugVisualIndexed;
  case TMNF_CLASS_CPlugTreeLight:
    return TMNF_CLASS_CPlugTree;
  case TMNF_CLASS_CPlugSurface:
    return TMNF_CLASS_CPlug;
  case TMNF_CLASS_CPlugSurfaceGeom:
    return TMNF_CLASS_CPlug;
  case TMNF_CLASS_CPlugIndexBuffer:
    return TMNF_CLASS_CPlug;
  case TMNF_CLASS_CPlugVertexStream:
    return TMNF_CLASS_CPlug;
  case TMNF_CLASS_CPlugMaterial:
    return TMNF_CLASS_CPlug;
  case TMNF_CLASS_CPlugTree:
    return TMNF_CLASS_CPlug;
  case TMNF_CLASS_CPlugTreeVisualMip:
    return TMNF_CLASS_CPlugTree;
  default:
    return 0u;
  }
}

int CGameCtnReplayStaticSolidArchiveClassInfo::IsKnownNodeClass(u32 classId) {
  classId = TMNF_CPlugTreeCanonicalChunkIdFromArchiveWord(classId);
  switch (classId) {
  case TMNF_CLASS_CGameCtnBlockUnitInfo:
  case TMNF_CLASS_CGameCtnBlockInfo:
  case TMNF_CLASS_CGameCtnBlockInfoFlat:
  case TMNF_CLASS_CGameCtnBlockInfoFrontier:
  case TMNF_CLASS_CGameCtnBlockInfoClassic:
  case TMNF_CLASS_CGameCtnBlockInfoRoad:
  case TMNF_CLASS_CGameCtnBlockInfoClip:
  case TMNF_CLASS_CGameCtnBlockInfoPylon:
  case TMNF_CLASS_CGameCtnBlockInfoRectAsym:
  case TMNF_CLASS_CGameCtnBlock:
  case TMNF_CLASS_GxLightAmbient:
  case TMNF_CLASS_GxLightDirectional:
  case TMNF_CLASS_CFuncKeysNatural:
  case TMNF_CLASS_CFuncTreeSubVisualSequence:
  case ArchiveChunkIdValue(FunctionCurveChunkId::Root):
  case TMNF_CLASS_CScenePoc:
  case TMNF_CLASS_CSceneLight:
  case TMNF_CLASS_CSceneSoundSource:
  case TMNF_CLASS_CSceneMobil:
  case TMNF_CLASS_CSceneObjectLink:
  case TMNF_CLASS_CSceneVehicleCar:
  case TMNF_CLASS_CScene3d:
  case TMNF_CLASS_CSceneSector:
  case ArchiveChunkIdValue(VehicleTuningChunkId::Root):
  case ArchiveChunkIdValue(VehicleTuningContainerChunkId::Root):
  case TMNF_CLASS_CSceneVehicleEnvironment:
  case TMNF_CLASS_CMotionCmdBase:
  case TMNF_CLASS_CMotionShader:
  case TMNF_CLASS_CMotionPlayer:
  case TMNF_CLASS_CMotionDayTime:
  case TMNF_CLASS_CHmsZone:
  case TMNF_CLASS_CHmsZoneDynamic:
  case TMNF_CLASS_CHmsItem:
  case TMNF_CLASS_CHmsLight:
  case TMNF_CLASS_CHmsSoundSource:
  case TMNF_CLASS_CPlugSolid:
  case TMNF_CLASS_CPlugVisualIndexedTriangles:
  case TMNF_CLASS_CPlugTree:
  case TMNF_CLASS_CPlugTreeLight:
  case TMNF_CLASS_CPlugSurface:
  case TMNF_CLASS_CPlugSurfaceGeom:
  case TMNF_CLASS_CPlugIndexBuffer:
  case TMNF_CLASS_CPlugMaterial:
  case TMNF_CLASS_CPlugTreeVisualMip:
  case TMNF_CLASS_CPlugDecoratorTree:
  case TMNF_CLASS_CPlugDecoratorSolid:
    return 1;
  default:
    return FeedbackClassId(classId) != 0u;
  }
}

int CGameCtnReplayStaticSolidArchiveClassInfo::WordIsSceneObjectLinkChunk(
    u32 word) {
  return IsCSceneObjectLinkArchiveChunk(word) ||
         word == CMwNodArchiveFacadeSentinel;
}

namespace {

std::optional<u32> BlockChunkInfo(u32 classId, u32 chunkId) {
  if (classId == TMNF_CLASS_CGameCtnBlockUnitInfo) {
    if (IsCGameCtnBlockUnitInfoInfo3Chunk(chunkId)) {
      return 3u;
    }
    if (IsCGameCtnBlockUnitInfoSkippedChunk(chunkId)) {
      return 0x13u;
    }
    return 0u;
  }
  if (IsCGameCtnBlockInfoArchiveClass(classId)) {
    switch (chunkId) {
    case TMNF_CGameCtnCollectorChunk_0301A006:
    case TMNF_CGameCtnCollectorChunk_0301A007:
    case TMNF_CGameCtnCollectorChunk_LoadableNodRefAndId:
    case TMNF_CGameCtnCollectorChunk_CMwId0301A00A:
    case TMNF_CGameCtnCollectorChunk_SGameCtnIdentifier:
      return 3u;
    default:
      if (IsCGameCtnBlockInfoInfo1Chunk(chunkId)) {
        return 1u;
      }
      if (IsCGameCtnBlockInfoInfo3Chunk(chunkId)) {
        return 3u;
      }
      if (chunkId == TMNF_CLASS_CGameCtnBlockInfoRoad) {
        return classId == TMNF_CLASS_CGameCtnBlockInfoRoad ? 3u : 0u;
      }
      if (chunkId ==
          ArchiveChunkIdValue(CGameCtnBlockInfoClipArchiveChunkId::Id)) {
        return classId == TMNF_CLASS_CGameCtnBlockInfoClip ? 3u : 0u;
      }
      if (chunkId == TMNF_CLASS_CGameCtnBlockInfoPylon) {
        return classId == TMNF_CLASS_CGameCtnBlockInfoPylon ? 3u : 0u;
      }
      return 0u;
    }
  }
  return std::nullopt;
}

std::optional<u32> LightCurveAndTuningChunkInfo(u32 classId, u32 chunkId) {
  if (classId == TMNF_CLASS_GxLightAmbient) {
    if (IsGxLightInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsGxLightAmbientInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_GxLightDirectional) {
    if (IsGxLightDirectionalInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsGxLightDirectionalInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == ArchiveChunkIdValue(FunctionCurveChunkId::Root)) {
    if (IsCFuncKeysInfo1Chunk(chunkId) ||
        chunkId == ArchiveChunkIdValue(FunctionCurveChunkId::Root)) {
      return 1u;
    }
    if (IsCFuncKeysInfo3Chunk(chunkId) ||
        chunkId == ArchiveChunkIdValue(FunctionCurveChunkId::YValuesAndMode)) {
      return 3u;
    }
    return 0u;
  }
  if (classId == ArchiveChunkIdValue(VehicleTuningContainerChunkId::Root)) {
    return chunkId == ArchiveChunkIdValue(VehicleTuningContainerChunkId::Root)
               ? 3u
               : 0u;
  }
  if (classId == ArchiveChunkIdValue(VehicleTuningChunkId::Root)) {
    if (chunkId == ArchiveChunkIdValue(VehicleTuningBaseChunkId::Base)) {
      return 3u;
    }
    if (chunkId < ArchiveChunkIdValue(VehicleTuningChunkId::Root) ||
        chunkId > 0x0a029069u) {
      return 0u;
    }
    switch (chunkId) {
    case 0x0a029003u:
    case 0x0a029006u:
    case 0x0a029008u:
    case 0x0a02900au:
    case 0x0a02900cu:
    case 0x0a02900fu:
    case 0x0a029011u:
    case 0x0a029012u:
    case 0x0a029015u:
    case 0x0a029016u:
    case 0x0a02901cu:
    case 0x0a029021u:
    case 0x0a029022u:
    case 0x0a029025u:
    case 0x0a02902fu:
    case 0x0a029045u:
    case 0x0a029048u:
    case 0x0a02904au:
    case 0x0a02904bu:
    case 0x0a02904cu:
    case 0x0a029050u:
    case 0x0a029054u:
    case 0x0a029055u:
    case 0x0a02905eu:
    case 0x0a029066u:
    case 0x0a029067u:
    case 0x0a029069u:
      return 1u;
    default:
      return 3u;
    }
  }
  if (classId == TMNF_CLASS_CFuncKeysNatural ||
      classId == TMNF_CLASS_CFuncTreeSubVisualSequence) {
    if (IsCFuncKeysInfo1Chunk(chunkId)) {
      return 1u;
    }
    if (IsCFuncKeysInfo3Chunk(chunkId)) {
      return 3u;
    }
    if (classId == TMNF_CLASS_CFuncKeysNatural) {
      return IsCFuncKeysNaturalInfo3Chunk(chunkId) ? 3u : 0u;
    }
    if (IsCFuncTreeSubVisualSequenceInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCFuncTreeSubVisualSequenceInfo3Chunk(chunkId) ? 3u : 0u;
  }
  return std::nullopt;
}

std::optional<u32> SceneChunkInfo(u32 classId, u32 chunkId) {
  if (classId == TMNF_CLASS_CSceneSector) {
    if (IsCSceneSectorInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCSceneSectorInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CScene3d) {
    if (IsCSceneInfo1Chunk(chunkId) || IsCScene3dInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCSceneInfo3Chunk(chunkId) || IsCScene3dInfo3Chunk(chunkId) ? 3u
                                                                        : 0u;
  }
  if (classId == TMNF_CLASS_CScenePoc || classId == TMNF_CLASS_CSceneLight ||
      classId == TMNF_CLASS_CSceneSoundSource ||
      classId == TMNF_CLASS_CSceneMobil ||
      classId == TMNF_CLASS_CSceneVehicleCar) {
    if (IsCSceneObjectInfo1Chunk(chunkId)) {
      return 1u;
    }
    if (IsCSceneObjectInfo3Chunk(chunkId)) {
      return 3u;
    }
    if (classId == TMNF_CLASS_CScenePoc || classId == TMNF_CLASS_CSceneLight ||
        classId == TMNF_CLASS_CSceneSoundSource) {
      return chunkId == TMNF_CLASS_CScenePoc ||
                     (classId == TMNF_CLASS_CSceneLight &&
                      chunkId == TMNF_CLASS_CSceneLight) ||
                     (classId == TMNF_CLASS_CSceneSoundSource &&
                      chunkId == TMNF_CLASS_CSceneSoundSource)
                 ? 3u
                 : 0u;
    }
    if (classId == TMNF_CLASS_CSceneMobil) {
      if (IsCSceneMobilInfo1Chunk(chunkId)) {
        return 1u;
      }
      return IsCSceneMobilInfo3Chunk(chunkId) ? 3u : 0u;
    }
    if (chunkId == TMNF_CLASS_CSceneVehicle ||
        IsCSceneVehicleCarInfo3Chunk(chunkId)) {
      return 3u;
    }
    return IsCSceneVehicleCarInfo1Chunk(chunkId) ? 1u : 0u;
  }
  if (classId == TMNF_CLASS_CSceneVehicleEnvironment) {
    if (IsCSceneVehicleEnvironmentInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCSceneVehicleEnvironmentInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CSceneObjectLink) {
    if (IsCSceneObjectLinkInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCSceneObjectLinkInfo3Chunk(chunkId) ? 3u : 0u;
  }
  return std::nullopt;
}

std::optional<u32> MotionChunkInfo(u32 classId, u32 chunkId) {
  if (classId == TMNF_CLASS_CMotionPlayer) {
    if (IsCMotionPlayerInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCMotionPlayerInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CMotionDayTime) {
    return IsCMotionDayTimeInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CMotionCmdBase) {
    if (IsCMotionCmdBaseInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCMotionCmdBaseInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CMotionShader) {
    return IsCMotionShaderInfo3Chunk(chunkId) ? 3u : 0u;
  }
  return std::nullopt;
}

std::optional<u32> HmsChunkInfo(u32 classId, u32 chunkId) {
  if (IsCHmsZoneArchiveClass(classId)) {
    if (IsCHmsZoneInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCHmsZoneInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CHmsLight) {
    if (IsCHmsLightInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCHmsLightInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CHmsItem) {
    if (IsCHmsItemInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCHmsItemInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CHmsSoundSource) {
    if (IsCHmsSoundSourceInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCHmsSoundSourceInfo3Chunk(chunkId) ? 3u : 0u;
  }
  return std::nullopt;
}

std::optional<u32> PlugChunkInfo(u32 classId, u32 chunkId) {
  if (classId == TMNF_CLASS_CPlugSolid) {
    if (IsCPlugSolidRequiredPayloadChunk(chunkId)) {
      return 3u;
    }
    return IsCPlugSolidOptionalPayloadChunk(chunkId) ? 1u : 0u;
  }
  if (classId == TMNF_CLASS_CPlugTreeLight) {
    if (IsCPlugTreeLightBaseArchiveChunk(chunkId)) {
      return 1u;
    }
    if (chunkId ==
        ArchiveChunkIdValue(CPlugTreeLightArchiveChunkId::LightRef)) {
      return 3u;
    }
  }
  if (classId == TMNF_CLASS_CPlugTreeVisualMip) {
    if (IsCPlugTreeVisualMipBaseArchiveChunk(chunkId)) {
      return 1u;
    }
    if (chunkId ==
        ArchiveChunkIdValue(CPlugTreeVisualMipArchiveChunkId::ChildLinks)) {
      return 3u;
    }
  }
  if (classId == TMNF_CLASS_CPlugTree ||
      classId == TMNF_CLASS_CPlugTreeVisualMip ||
      classId == TMNF_CLASS_CPlugTreeLight) {
    if (IsCPlugTreeOptionalPayloadChunk(chunkId)) {
      return 1u;
    }
    return IsCPlugTreeRequiredPayloadChunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CPlugVisualIndexedTriangles) {
    if (IsCPlugVisualIndexedTrianglesInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCPlugVisualIndexedTrianglesInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CPlugSurface) {
    return chunkId == TMNF_CLASS_CPlugSurface ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CPlugSurfaceGeom) {
    if (IsCPlugSurfaceGeomInfo1Chunk(chunkId)) {
      return 1u;
    }
    return IsCPlugSurfaceGeomInfo3Chunk(chunkId) ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CPlugIndexBuffer) {
    return chunkId == TMNF_CLASS_CPlugIndexBuffer ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CPlugMaterial) {
    if (IsRequiredMaterialPayloadChunk(chunkId)) {
      return 3u;
    }
    return IsOptionalMaterialPayloadChunk(chunkId) ? 1u : 0u;
  }
  if (classId == TMNF_CLASS_CPlugDecoratorSolid) {
    return chunkId == TMNF_CLASS_CPlugDecoratorSolid ? 3u : 0u;
  }
  if (classId == TMNF_CLASS_CPlugDecoratorTree) {
    return IsCPlugDecoratorTreeArchiveChunk(chunkId) ? 3u : 0u;
  }
  return std::nullopt;
}

} // namespace

u32 CGameCtnReplayStaticSolidArchiveClassInfo::ChunkInfo(u32 classId,
                                                         u32 chunkId) {
  if (chunkId == 0xffffffffu) {
    return 0xffffffffu;
  }
  if (const std::optional<u32> info = BlockChunkInfo(classId, chunkId)) {
    return *info;
  }
  if (const std::optional<u32> info =
          LightCurveAndTuningChunkInfo(classId, chunkId)) {
    return *info;
  }
  if (const std::optional<u32> info = SceneChunkInfo(classId, chunkId)) {
    return *info;
  }
  if (const std::optional<u32> info = MotionChunkInfo(classId, chunkId)) {
    return *info;
  }
  if (const std::optional<u32> info = HmsChunkInfo(classId, chunkId)) {
    return *info;
  }
  if (const std::optional<u32> info = PlugChunkInfo(classId, chunkId)) {
    return *info;
  }
  return 0u;
}
