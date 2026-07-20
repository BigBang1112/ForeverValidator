#include "engine/physics/dynamics/hms_corpus.h"

#include "engine/physics/dynamics/hms_item.h"
#include "engine/physics/world/hms_zone.h"
#include "engine/rendering/plug_shader.h"
#include "engine/rendering/plug_tree.h"
#include "engine/rendering/plug_visual.h"
#include "engine/scene/plug_solid.h"
#include "format/archive/archive_class_ids.h"

int CHmsCorpus::WaterGetPlaneEqInZone(GmVec4 &plane) {
    CPlugTree *root = CollisionTree();
    if (root == nullptr) {
        return 0;
    }

    CPlugTree::CIteratorShader shaders(
            root, CPlugTree::CIteratorTree::Mode_IncludeRoot);
    while (shaders.HasNext()) {
        CPlugTree *tree = nullptr;
        CPlugShader *shader = shaders.GetNextShader(&tree);
        CPlugVisual *provider = tree != nullptr ? tree->Visual() : nullptr;
        if (shader == nullptr || provider == nullptr ||
            (shader->ArchiveFlags() & 0x00c00000u) != 0x00800000u ||
            shader->FindBitmapRenderByClassId(
                    TMNF_CLASS_CPlugBitmapRenderWater,
                    nullptr,
                    nullptr) == nullptr) {
            continue;
        }

        GmIso4 treeToWorld;
        tree->GetThisToRootTransfo(treeToWorld, 1, nullptr);
        const GmIso4 *corpusLocation = GetLocation();
        if (corpusLocation == nullptr) {
            return 0;
        }
        treeToWorld.Mult(*corpusLocation);

        plane = {0.0f, 1.0f, 0.0f,
                 -provider->BoundingBox().center.y};
        plane.PlaneEqMult(treeToWorld);
        return 1;
    }
    return 0;
}

void Zone_UpdateWaterHeights(CHmsCorpus *corpus) {
    if (corpus == nullptr || corpus->OwningZone() == nullptr) {
        return;
    }
    GmVec4 plane{};
    if (!corpus->WaterGetPlaneEqInZone(plane)) {
        return;
    }
    (void)corpus->OwningZone()->AppendWaterPlaneEq(plane);
}
