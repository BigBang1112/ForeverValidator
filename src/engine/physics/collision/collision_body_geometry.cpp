// Collision-body locations and collision-tree transforms.

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include "engine/core/cmw_nod.h"
#include "engine/physics/geometry/geometry_helpers.h"
#include "engine/physics/collision/gm_collision_buffer.h"
#include "engine/physics/collision/hms_collision.h"
#include "engine/physics/collision/hms_collision_manager.h"
#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_dyna.h"
#include "engine/physics/dynamics/hms_item.h"
#include "engine/physics/world/hms_zone.h"
#include "engine/physics/geometry/physics_tolerances.h"
#include "engine/scene/plug_solid.h"
#include "engine/physics/geometry/plug_surface.h"
#include "engine/rendering/plug_tree.h"

u32 CHmsItem::CollisionGroup(void) const {
    return static_cast<u32>(properties.collisionGroup);
}

int CHmsItem::HasStaticCollisionTree(void) const {
    return properties.collisionStatic;
}

const GmIso4 *CHmsCorpus::GetLocation(void) const {
    if (this->dyna != 0) {
        queriedLocation.Set(
                dyna->WriteState().rotation,
                dyna->WriteState().position);
        return &queriedLocation;
    }
    return &this->localIso;
}

const GmIso4 *CHmsCorpus::LocationIso(void) const {
    if (dyna != 0) {
        queriedLocation.Set(
                dyna->CurrentState().rotation,
                dyna->CurrentState().position);
        return &queriedLocation;
    }
    return &localIso;
}

const GmIso4 *CHmsCorpus::VirtualLocationIso(void) {
    return LocationIso();
}

CPlugTree *CHmsCorpus::CollisionTree(void) const {
    return item->Solid()->CollisionTree();
}

int CPlugTree::HasWorldBox(void) const {
    return State().collisionEnabled;
}

int CPlugTree::AllowsSurfaceCollision(void) const {
    return State().collisionEnabled;
}

int CPlugTree::HasLocalTransform(void) const {
    return UsesLocation();
}

const GmBoxAligned &CPlugTree::Box(void) const {
    return box;
}

const GmIso4 &CPlugTree::LocalIso(void) const {
    return localIso;
}

CPlugSurface *CPlugTree::Surface(void) const {
    return surface;
}


void CPlugTree::ComposeCollisionIso(const GmIso4 &parentIso, GmIso4 &outIso) const {
    if (!HasLocalTransform()) {
        outIso = parentIso;
        return;
    }

    outIso = LocalIso();
    outIso.Mult(parentIso);
}

void CPlugTree::GetTransformedCollisionBox(const GmIso4 &iso, GmBoxAligned &outBox) const {
    outBox.SetMult(Box(), iso);
}
