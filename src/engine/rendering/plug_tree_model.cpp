#include "engine/rendering/plug_tree.h"
#include <memory>

#include "engine/scene/plug_solid.h"
#include "engine/resources/system_fid.h"
#include "engine/resources/system_fid_parameters.h"
namespace {

std::unique_ptr<CPlugTree> MakeEmptyTreeLike(const CPlugTree &source) {
    if (dynamic_cast<const CPlugTreeVisualMip *>(&source) != nullptr) {
        return std::make_unique<CPlugTreeVisualMip>();
    }
    return std::make_unique<CPlugTree>();
}

}  // namespace

CPlugTree *CPlugTree::CreateModelInstance(void) const {
    std::unique_ptr<CPlugTree> clone = MakeEmptyTreeLike(*this);
    clone->modelSolid = owningSolid;

    if (parent != nullptr || id.IsLocalName()) {
        clone->modelId = id;
    } else {
        clone->modelId.SetInvalid();
    }

    clone->CopyFromModel(this, 1);
    clone->ApplyCurrentFidParametersToClone();
    return clone.release();
}

CPlugTree *CPlugTree::InternalCreateSolidModelInstanceThis(void) const {
    std::unique_ptr<CPlugTree> clone = MakeEmptyTreeLike(*this);
    clone->CopyCloneIdentityFromSource(*this);
    clone->ApplyCurrentFidParametersToClone();
    return clone.release();
}

CPlugTree *CPlugTree::DuplicateThis(void) const {
    std::unique_ptr<CPlugTree> clone;
    if (modelSolid != nullptr) {
        CPlugTree *modelTree = GetModelTree();
        if (modelTree == nullptr) {
            return nullptr;
        }
        clone.reset(modelTree->CreateModelInstance());
    } else {
        clone = MakeEmptyTreeLike(*this);
        clone->CopyFromModel(this, 1);
    }
    clone->InternalSetMwId(id);
    clone->SetIsRooted(GetIsRooted());
    clone->ApplyCurrentFidParametersToClone();
    return clone.release();
}

CPlugTree *CPlugTree::DuplicateRecursive(void) const {
    CPlugTree *clone = InternalRecursiveCreate(
            &CPlugTree::DuplicateThis,
            1,
            nullptr);
    clone->Generate(0);
    clone->UpdateBoundingBox(1);
    return clone;
}

CPlugTree *CPlugTree::InternalRecursiveCreate(
    CPlugTreeCloneFactory cloneFactory,
    int includeResolvable,
    CPlugTree *outRoot) const {
    std::unique_ptr<CPlugTree> ownedRoot;
    CPlugTree *root = outRoot;
    if (root == nullptr) {
        ownedRoot.reset((this->*cloneFactory)());
        root = ownedRoot.get();
        if (root == nullptr) {
            return nullptr;
        }
    }

    const u32 childCount = static_cast<u32>(CPlugTree::GetChildCount());
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = CPlugTree::GetChild(childIndex);
        const int childIsRooted = child->GetIsRooted();
        if ((includeResolvable != 0) != (childIsRooted != 0)) {
            continue;
        }

        std::unique_ptr<CPlugTree> childClone(
                child->InternalRecursiveCreate(
                        cloneFactory, includeResolvable, nullptr));
        if (!childClone) {
            return nullptr;
        }
        root->AddOwnedChild(std::move(childClone));
    }

    return ownedRoot ? ownedRoot.release() : root;
}

CPlugTree *CPlugTree::InternalCreateSolidModelInstance(void) const {
    return InternalRecursiveCreate(
            &CPlugTree::InternalCreateSolidModelInstanceThis,
            1,
            nullptr);
}

namespace {
CMwNod *GetParametrizedNodFromModel(
        CMwNod *modelNod,
        CPlugTree *tree) {
    if (modelNod == nullptr) {
        return nullptr;
    }

    CSystemFid *fid = modelNod->fid;
    if (fid == nullptr) {
        return modelNod;
    }

    CMwNod *remappedNod = nullptr;
    CSystemFidParameters::RemappedLoadFromFid(
            remappedNod,
            fid,
            tree);
    return remappedNod;
}
}

CPlugSurface *CPlugTree::CloneSurfaceForModelCopy(const CPlugSurface *source) {
    if (source == nullptr) {
        return nullptr;
    }
    auto clone = std::make_unique<CPlugSurface>();
    if (source->GeometryNode() != nullptr) {
        clone->SetGeometry(source->GeometryNode());
    }

    const u32 materialCount = source->MaterialCount();
    if (!clone->SetMaterialCount(materialCount)) {
        return nullptr;
    }
    for (u32 materialIndex = 0; materialIndex < materialCount; materialIndex++) {
        CPlugMaterial *sourceMaterial = source->MaterialAt(materialIndex);
        CPlugMaterial *remappedMaterial = dynamic_cast<CPlugMaterial *>(
                GetParametrizedNodFromModel(sourceMaterial, this));
        if (remappedMaterial != nullptr) {
            clone->SetMaterialAt(materialIndex, remappedMaterial);
        }
    }
    return clone.release();
}

void CPlugTree::CopyFromModel(
        const CPlugTree *source,
        int deepCopy) {
    CPlugTree *self = this;
    if (source == nullptr) {
        return;
    }

    (void)source->generator;

    if (source->material != nullptr) {
        self->SetVisual(
                source->visual,
                nullptr,
                source->material,
                0);
    } else {
        self->SetVisual(
                source->visual,
                source->shader,
                nullptr,
                0);
    }

    CPlugSurface *surfaceClone = self->CloneSurfaceForModelCopy(source->surface);
    self->SetSurface(surfaceClone);
    self->SetGenerator(nullptr, 1);

    CFuncTree *funcTree = dynamic_cast<CFuncTree *>(
            GetParametrizedNodFromModel(source->funcTree, self));
    self->SetFuncTree(funcTree);

    self->state_.requiresMatchingShaderState =
            source->state_.requiresMatchingShaderState;
    self->state_.requiresMatchingMaterialState =
            source->state_.requiresMatchingMaterialState;
    self->state_.groupingBoundary = source->state_.groupingBoundary;
    self->state_.castsShadows = source->state_.castsShadows;
    self->state_.rooted = source->state_.rooted;
    if (deepCopy != 0) {
        self->state_.locationDirty = true;
        self->state_.usesLocation = source->state_.usesLocation;
        self->localIso = source->localIso;
        self->state_.visible = source->state_.visible;
        self->state_.collisionEnabled = source->state_.collisionEnabled;
        self->state_.modelInstance = source->state_.modelInstance;
        self->state_.renderBeforeFidParametrization =
                source->state_.renderBeforeFidParametrization;
    }

    self->UpdateBoundingBox(0);
}
