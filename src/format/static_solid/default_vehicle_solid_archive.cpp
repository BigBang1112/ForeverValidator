#include "format/static_solid/default_vehicle_solid_archive.h"
#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

#include "format/pack/installed/plug_file_pack.h"
#include "format/static_solid/static_scene_archive_loader.h"
namespace {

constexpr std::string_view DefaultVehicleSolidPath =
        "Vehicles\\Media\\Solid\\41B4559784BFBD1D35DAA5A70EBB9C5B97";

struct DefaultVehicleWheelSurface {
    std::size_t wheelIndex;
    VehicleWheelSurfaceId surfaceId;
    VehicleCollisionRole collisionRole;
};

std::optional<DefaultVehicleWheelSurface> WheelSurfaceForTreeName(
        const char *name) {
    if (name == nullptr) {
        return std::nullopt;
    }

    struct NamedWheelRole {
        std::string_view name;
        VehicleCollisionRole role;
    };
    static constexpr std::array<NamedWheelRole,
                                OfficialVehicleWheelCount> SurfaceRoles{
            NamedWheelRole{"FLSurf", VehicleCollisionRole::FrontLeftWheel},
            NamedWheelRole{"FRSurf", VehicleCollisionRole::FrontRightWheel},
            NamedWheelRole{"RRSurf", VehicleCollisionRole::RearRightWheel},
            NamedWheelRole{"RLSurf", VehicleCollisionRole::RearLeftWheel},
    };
    for (std::size_t wheelIndex = 0u;
         wheelIndex < SurfaceRoles.size();
         ++wheelIndex) {
        if (name == SurfaceRoles[wheelIndex].name) {
            return DefaultVehicleWheelSurface{
                    wheelIndex,
                    VehicleWheelSurfaceIdForIndex(wheelIndex),
                    SurfaceRoles[wheelIndex].role,
            };
        }
    }
    return std::nullopt;
}

std::optional<VehicleCollisionRole> CollisionRoleForTreeName(
        const char *name) {
    if (name == nullptr) {
        return std::nullopt;
    }
    struct NamedBodyRole {
        std::string_view name;
        VehicleCollisionRole role;
    };
    static constexpr std::array<NamedBodyRole, 4u> BodyRoles{
            NamedBodyRole{"BodySurf", VehicleCollisionRole::BodyLongitudinal},
            NamedBodyRole{"BodySurf01", VehicleCollisionRole::BodyMain},
            NamedBodyRole{"BodySurf02", VehicleCollisionRole::BodyRear},
            NamedBodyRole{"BodySurf04", VehicleCollisionRole::BodyFront},
    };
    for (const NamedBodyRole &body : BodyRoles) {
        if (name == body.name) {
            return body.role;
        }
    }
    const std::optional<DefaultVehicleWheelSurface> wheel =
            WheelSurfaceForTreeName(name);
    return wheel.has_value()
            ? std::optional<VehicleCollisionRole>(wheel->collisionRole)
            : std::nullopt;
}

bool AddTreeDefinition(
        const CGameCtnReplayStaticSolidArchiveNamedTree &namedTree,
        ReplayVehicleSolidDefinition &definitions) {
    const std::optional<DefaultVehicleWheelSurface> wheelSurface =
            WheelSurfaceForTreeName(namedTree.Name());
    const std::optional<VehicleCollisionRole> collisionRole =
            CollisionRoleForTreeName(namedTree.Name());
    if (!collisionRole.has_value()) {
        return true;
    }
    if (!namedTree.HasSurfaceGeometry()) {
        return false;
    }

    const CGameCtnReplayStaticSolidArchiveTreeStateDefinition *state =
            namedTree.State();
    const CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition *geom =
            namedTree.SurfaceGeom();
    if (state == nullptr || geom == nullptr) {
        return false;
    }

    const std::uint16_t materialId = geom->MaterialId();
    if (materialId >= EPlugSurfaceMaterialId_Count) {
        return false;
    }

    if (!definitions.collisionModel.SetShape(
            *collisionRole,
            VehicleCollisionShapeDefinition{
                state->LocalTransform(),
                geom->BoundingBox(),
                GmLocalMaterialIndex::FromIndex(materialId),
                static_cast<EPlugSurfaceMaterialId>(materialId),
            })) {
        return false;
    }

    if (!wheelSurface.has_value()) {
        return true;
    }
    if (wheelSurface->wheelIndex >= definitions.wheels.size() ||
        definitions.wheels[wheelSurface->wheelIndex].has_value()) {
        return false;
    }

    VehicleWheelDefinition wheel;
    wheel.surfaceId = wheelSurface->surfaceId;
    wheel.rollingRadius = geom->BoundingBox().halfExtents.y;
    wheel.restSurfacePose = state->LocalTransform();
    wheel.forceApplicationPoint = wheel.restSurfacePose.Translation();
    wheel.initialSurfacePoint = wheel.forceApplicationPoint;
    wheel.collisionRole = wheelSurface->collisionRole;
    definitions.wheels[wheelSurface->wheelIndex] = std::move(wheel);
    return true;
}

bool ExtractWheelDefinitions(
        const StaticSolidArchiveLoadSession &archive,
        ReplayVehicleSolidDefinition &definitions) {
    const StaticSolidArchiveId payload =
            StaticSolidArchiveId::FromIndex(0u);
    const CGameCtnReplayStaticSolidArchiveGraph &graph =
            archive.ArchiveGraph();
    if (!graph.ForEachNamedTree(
                payload,
                [&](const CGameCtnReplayStaticSolidArchiveNamedTree
                            &namedTree) {
                    return AddTreeDefinition(
                            namedTree,
                            definitions);
                })) {
        return false;
    }
    return definitions.IsComplete();
}

}  // namespace

std::optional<ReplayVehicleSolidDefinition>
DefaultVehicleSolidArchive::LoadFromPack(
        CPlugFilePack &pack) {
    const CPlugFileFidContainer_SFileDesc *file =
            pack.FindFileDescByPath(DefaultVehicleSolidPath.data());
    if (file == nullptr) {
        return std::nullopt;
    }

    StaticSolidArchiveLoadSession archive;
    if (!archive.InstallPackSource(pack)) {
        return std::nullopt;
    }

    CGameCtnReplayStaticSolidDecodedPayload decodedPayload;
    CGameCtnReplayStaticSolidArchiveDecodeStats stats;
    if (!archive.DecodePackFilePayloadWithStreamFeedback(
            pack,
            *file,
            0u,
            1u,
            DefaultVehicleSolidPath.data(),
            DefaultVehicleSolidPath.data(),
            &decodedPayload,
            &stats) ||
        !decodedPayload.IsReady() ||
        decodedPayload.ByteCount() != file->uncompressedSize) {
        return std::nullopt;
    }

    ReplayVehicleSolidDefinition definitions;
    if (!ExtractWheelDefinitions(archive, definitions)) {
        return std::nullopt;
    }
    return std::optional<ReplayVehicleSolidDefinition>(
            std::move(definitions));
}
