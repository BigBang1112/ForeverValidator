#include "simulation/runtime/replay_vehicle_collision_model.h"
#include <new>
#include <utility>
#include <vector>

#include "engine/physics/geometry/gm_surface.h"
#include "engine/rendering/plug_material.h"
#include "engine/physics/geometry/plug_surface.h"
#include "engine/rendering/plug_tree.h"
namespace {

CPlugTree::SFlags VehicleCollisionTreeState(bool visible) {
    CPlugTree::SFlags state;
    state.visible = visible;
    state.modelInstance = true;
    state.collisionEnabled = true;
    state.usesLocation = true;
    return state;
}

const GmIso4 &IdentityIso() {
    static const GmIso4 identity = [] {
        GmIso4 result;
        result.SetIdentity();
        return result;
    }();
    return identity;
}

std::unique_ptr<CPlugTree> BuildShapeTree(
        const VehicleCollisionShapeDefinition &definition) {
    auto shapeTree = std::make_unique<CPlugTree>();
    shapeTree->ApplyLoadedState(VehicleCollisionTreeState(false));
    shapeTree->SetLocation(definition.localPose);

    CMwNodRef<CPlugSurface> surface = MakeMwNod<CPlugSurface>();
    CMwNodRef<CPlugSurfaceGeom> geometry = MakeMwNod<CPlugSurfaceGeom>();
    std::unique_ptr<GmSurf> collisionSurface;
    if (definition.surfaceType ==
        static_cast<std::uint32_t>(GmSurf::EGmSurfType::Sphere)) {
        auto sphere = std::make_unique<GmSurfSphere>();
        sphere->radius = definition.localBounds.halfExtents.y;
        collisionSurface = std::move(sphere);
    } else if (definition.surfaceType ==
               static_cast<std::uint32_t>(GmSurf::EGmSurfType::Ellipsoid)) {
        auto ellipsoid = std::make_unique<GmSurfEllipsoid>();
        ellipsoid->radii = definition.localBounds.halfExtents;
        collisionSurface = std::move(ellipsoid);
    } else if (definition.surfaceType ==
               static_cast<std::uint32_t>(GmSurf::EGmSurfType::Box)) {
        auto box = std::make_unique<GmSurfBox>();
        box->center = definition.localBounds.center;
        box->halfExtents = definition.localBounds.halfExtents;
        collisionSurface = std::move(box);
    } else {
        return nullptr;
    }
    collisionSurface->material = GmLocalMaterialIndex::FromIndex(0u);
    geometry->SetGmSurf(std::move(collisionSurface));

    CMwNodRef<CPlugMaterial> material = MakeMwNod<CPlugMaterial>();
    material->SetSurfaceMaterialId(definition.surfaceMaterial);
    surface->SetGeometry(geometry.Get());
    surface->AttachSingleMaterial(*material);
    shapeTree->SetSurface(surface.Get());
    shapeTree->UpdateBoundingBox(0u);
    return shapeTree;
}

}  // namespace

bool ReplayVehicleCollisionModel::Build(
        const VehicleCollisionModelDefinition &definition) {
    tree.reset();
    shapes.fill(nullptr);
    if (!definition.IsComplete()) {
        return false;
    }

    try {
        auto root = std::make_unique<CPlugTree>();
        root->ApplyLoadedState(VehicleCollisionTreeState(true));
        root->SetLocation(IdentityIso());
        const std::vector<VehicleCollisionShapeEntry> &entries =
                definition.ShapesInArchiveOrder();
        std::vector<CPlugTree *> builtShapes(entries.size(), nullptr);
        for (std::size_t index = 0u; index < entries.size(); index++) {
            const VehicleCollisionShapeEntry &entry = entries[index];
            std::unique_ptr<CPlugTree> shape =
                    BuildShapeTree(entry.shape);
            if (shape == nullptr) {
                return false;
            }
            CPlugTree *shapePtr = shape.get();
            if (entry.parentShapeIndex.has_value()) {
                const std::size_t parentIndex = *entry.parentShapeIndex;
                if (parentIndex >= index ||
                    builtShapes[parentIndex] == nullptr) {
                    return false;
                }
                builtShapes[parentIndex]->AddOwnedChild(std::move(shape));
            } else {
                root->AddOwnedChild(std::move(shape));
            }
            builtShapes[index] = shapePtr;
            if (entry.wheelRole.has_value()) {
                shapes[VehicleCollisionRoleIndex(*entry.wheelRole)] = shapePtr;
            }
        }
        root->UpdateBoundingBox(0u);
        tree = std::move(root);
    } catch (const std::bad_alloc &) {
        tree.reset();
        shapes.fill(nullptr);
        return false;
    }
    return true;
}

CPlugTree *ReplayVehicleCollisionModel::Shape(
        VehicleCollisionRole role) const {
    const std::size_t index = VehicleCollisionRoleIndex(role);
    return index < shapes.size() ? shapes[index] : nullptr;
}

std::unique_ptr<CPlugTree> ReplayVehicleCollisionModel::ReleaseTree() {
    if (tree != nullptr) {
        tree->EnableCollisionOnAncestors();
    }
    return std::move(tree);
}
