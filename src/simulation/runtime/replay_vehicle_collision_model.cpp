#include "simulation/runtime/replay_vehicle_collision_model.h"
#include <utility>

#include "engine/physics/geometry/gm_surface.h"
#include "engine/rendering/plug_material.h"
#include "engine/physics/geometry/plug_surface.h"
#include "engine/rendering/plug_tree.h"
#include <new>
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
    auto ellipsoid = std::make_unique<GmSurfEllipsoid>();
    ellipsoid->material = definition.localMaterial;
    ellipsoid->radii = definition.localBounds.halfExtents;
    geometry->SetGmSurf(std::move(ellipsoid));

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
        for (VehicleCollisionRole role :
             VehicleCollisionRolesInDetectorOrder) {
            const VehicleCollisionShapeDefinition *shapeDefinition =
                    definition.Shape(role);
            if (shapeDefinition == nullptr) {
                return false;
            }
            std::unique_ptr<CPlugTree> shape =
                    BuildShapeTree(*shapeDefinition);
            CPlugTree *shapePtr = shape.get();
            root->AddOwnedChild(std::move(shape));
            shapes[VehicleCollisionRoleIndex(role)] = shapePtr;
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
