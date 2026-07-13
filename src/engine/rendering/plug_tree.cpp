#include "engine/rendering/plug_tree.h"
#include <memory>

#include "engine/scene/plug_solid.h"
#include "engine/resources/system_fid_parameter_types.h"
#include "engine/resources/system_fid_parameters.h"
CFuncTree::CFuncTree(void) = default;
CFuncTree::~CFuncTree(void) = default;

CPlugTree::CPlugTree(void) {
    localIso.SetIdentity();
    SetInvalidBox();
}


void GmBoxAligned::SetInvalidForPlugTree(void) {
    center.x = 0.0f;
    center.y = 0.0f;
    center.z = 0.0f;
    halfExtents.x = -1.0f;
    halfExtents.y = -1.0f;
    halfExtents.z = -1.0f;
}

const CPlugTree::SFlags &CPlugTree::State(void) const {
    return state_;
}

void CPlugTree::ApplyLoadedState(const SFlags &state) {
    state_ = state;
}

void CPlugTree::SetCollisionEnabled(bool enabled) {
    state_.collisionEnabled = enabled;
    if (enabled) {
        EnableCollisionOnAncestors();
    }
}

void CPlugTree::EnableCollisionOnAncestors(void) {
    for (CPlugTree *cursor = this; cursor != nullptr; cursor = cursor->parent) {
        cursor->state_.collisionEnabled = true;
    }
}

void CPlugTree::SetModelInstance(bool enabled) {
    state_.modelInstance = enabled;
}

bool CPlugTree::IsModelInstance(void) const {
    return state_.modelInstance;
}

void CPlugTree::SetShadowCaster(bool enabled) {
    state_.castsShadows = enabled;
}

bool CPlugTree::IsShadowCaster(void) const {
    return state_.castsShadows;
}

bool CPlugTree::IsVisible(void) const {
    return state_.visible;
}

bool CPlugTree::UsesLocation(void) const {
    return state_.usesLocation;
}

void CPlugTree::SetIsPickableVisual(int enabled) {
    state_.pickableVisual = enabled != 0;
}

SPlugTreeOptimProperties CPlugTree::OptimizationProperties(void) const {
    return {
        state_.groupingBoundary,
        state_.groupingMarker,
        state_.requiresMatchingShaderState,
        state_.requiresMatchingMaterialState,
        state_.visible,
        state_.modelInstance,
        state_.castsShadows,
        state_.rooted,
    };
}

void CPlugTree::ApplyOptimizationProperties(
        const SPlugTreeOptimProperties &properties) {
    state_.groupingBoundary = properties.optimizationBoundary;
    state_.groupingMarker = properties.visualInheritance;
    state_.requiresMatchingShaderState = properties.shaderInheritance;
    state_.requiresMatchingMaterialState = properties.materialInheritance;
    state_.visible = properties.visible;
    state_.modelInstance = properties.modelInstance;
    state_.castsShadows = properties.castsShadows;
    state_.rooted = properties.rooted;
}

int GmBoxAligned::IsValidForPlugTreeRefresh(void) const {
    return !(0.0f > halfExtents.x);
}

CPlugSolid *CPlugTree::OwningSolid(void) const {
    return owningSolid;
}

void CPlugTree::InternalSetSolid(CPlugSolid *solid) {
    this->owningSolid = solid;
}

CPlugTree *CPlugTree::GetModelTree(void) const {
    CPlugTree *modelRoot = this->modelSolid->CollisionTree();
    if (modelRoot == nullptr) {
        return nullptr;
    }

    return modelRoot->GetPlugFromId(this->modelId);
}

void CPlugTree::DisconnectThisFromModel(
        int duplicatePolicy,
        unsigned long disconnectMask) {
    (void)duplicatePolicy;
    (void)disconnectMask;
    if (modelSolid != nullptr) {
        modelSolid.MwSetNod(nullptr);
    }
    modelId.SetInvalid();
}

void CPlugTree::RefreshThisFromModel(void) {
    CPlugTree *modelTree = this->GetModelTree();
    this->CopyFromModel(modelTree, 0);
    this->Generate(0);
}

void CPlugTree::DisconnectFromModel(int duplicatePolicy,
                                    unsigned long disconnectMask) {
    this->DisconnectThisFromModel(duplicatePolicy, disconnectMask);
    const u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = this->GetChild(childIndex);
        child->DisconnectFromModel(duplicatePolicy, disconnectMask);
    }
}

const CMwId *CPlugTree::MwGetId(void) const {
    return &id;
}

CPlugTree::~CPlugTree(void) {
    if (parent != nullptr) {
        parent->DetachChildPtr(this);
    }
    this->DeleteAllChilds();
}

void CPlugTree::SetOwningSolidRecursive(CPlugSolid *solid) {
    InternalSetSolid(solid);
    const u32 childCount = GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        GetChild(childIndex)->SetOwningSolidRecursive(solid);
    }
}

int CPlugTree::IsVolatileChild(void) const {
    return !GetIsRooted();
}

void CPlugTree::SetShader(CPlugShader *shader) {
    CPlugShader *oldShader = this->shader;
    if (oldShader == nullptr || oldShader == shader) {
        return;
    }

    if (shader != nullptr) {
        shader->CheckGeometryRequirement();
    }

    this->shader.MwSetNod(shader);

    if (this->material != nullptr &&
        !this->material->DoesContainShader(shader, nullptr)) {
        this->material.MwSetNod(nullptr);
    }
}

void CPlugTree::SetMaterial(CPlugMaterial *material) {
    if (this->shader == nullptr || material == this->material) {
        return;
    }

    this->material.MwSetNod(material);

    if (material != nullptr) {
        CPlugShader *supportedShader = material->GetSupportedShader();
        this->shader.MwSetNod(supportedShader);
    }
}

void CPlugTree::InternalSetMwId(const CMwId &id) {
    this->id = id;
}

void CPlugTree::SetPlugId(const CMwId &id) {
    const int wasRooted = this->GetIsRooted();
    this->InternalSetMwId(id);
    if (wasRooted != 0 && this->owningSolid != nullptr) {
        this->owningSolid->InternalConnectSubTree(this);
    }
}

CPlugTree *CPlugTree::CopyCloneIdentityFromSource(
    const CPlugTree &source) {
    CPlugTree *mutableSource = const_cast<CPlugTree *>(&source);
    CopyFromModel(mutableSource, 1);
    InternalSetMwId(mutableSource->id);
    SetIsRooted(mutableSource->GetIsRooted());
    return this;
}

void CPlugTree::ApplyCurrentFidParametersToClone() {
    CSystemFidParameters outParameters;
    CFastBuffer<CMwNod::SManuallyLoadedFid> manuallyLoadedFids;
    const CSystemFidParameters &currentParameters =
            CSystemFidParameters::GetCurrentParameters();
    ApplyFidParameters(
            &currentParameters,
            &outParameters,
            manuallyLoadedFids);
}

CPlugShader *CPlugTree::Shader(void) const {
    return shader;
}

CPlugMaterial *CPlugTree::Material(void) const {
    return material;
}

CPlugTreeGenerator *CPlugTree::Generator(void) const {
    return generator;
}

CFuncTree *CPlugTree::FunctionTree(void) const {
    return funcTree;
}

CPlugTree *CPlugTree::ParentTree(void) const {
    return parent;
}

CPlugSolid *CPlugTree::ModelSolidReference(void) const {
    return modelSolid;
}

const CMwId &CPlugTree::ModelNodeId(void) const {
    return modelId;
}

const CMwId &CPlugTree::PlugId(void) const {
    return id;
}

void CPlugTree::SetTreeBounds(const GmBoxAligned &bounds) {
    box = bounds;
}

int CPlugTree::HasShader(void) const {
    return Shader() != nullptr;
}

void CPlugTree::MarkRenderBeforeFidParametrization(void) {
    state_.renderBeforeFidParametrization = true;
}

void CPlugTree::AddMatchedRenderBeforeParam(
        const CSystemFidParameters::SParam_Fid_Common &param,
        CSystemFidParameters *outParameters) {
    MarkRenderBeforeFidParametrization();
    if (outParameters != nullptr) {
        outParameters->AddParam(param);
    }
}

void CPlugTree::ApplySpecialRenderBeforeParams(
        const CSystemFidParameters &parameters,
        CSystemFidParameters *outParameters) {
    CPlugShader *activeShader = Shader();
    if (activeShader == nullptr) {
        return;
    }

    parameters.VisitDirectFidRemaps(
            [this, activeShader, outParameters](
                    const CSystemFidParameters::SParam_Fid &param) {
                if (!param.HasSpecialPackDesc()) {
                    return;
                }

                CSystemPackDesc *packDesc = param.PackDesc();
                const u32 renderBeforeCount =
                        activeShader->GetRenderBeforeCount();
                for (u32 renderBeforeIndex = 0u;
                     renderBeforeIndex < renderBeforeCount;
                     renderBeforeIndex++) {
                    if (activeShader->RenderBeforeEntryMatchesPackDesc(
                                renderBeforeIndex,
                                *packDesc)) {
                        AddMatchedRenderBeforeParam(param, outParameters);
                        break;
                    }
                }
            });
}

void CPlugTree::ApplyFidParameters(
        const CSystemFidParameters *currentParameters,
        CSystemFidParameters *outParameters,
        CFastBuffer<CMwNod::SManuallyLoadedFid> &manuallyLoadedFids) {
    (void)manuallyLoadedFids;
    if (currentParameters != nullptr && HasShader()) {
        ApplySpecialRenderBeforeParams(*currentParameters, outParameters);
    }
}

void CPlugTree::GetThisToRootTransfo(GmIso4 &out,
                                                int includeLocal,
                                                CPlugTree *stopAt) const {
    if (this->parent != nullptr && this->parent != stopAt) {
        GmIso4 parentToRoot;
        this->parent->GetThisToRootTransfo(parentToRoot, 1, stopAt);
        if (includeLocal != 0 &&
            state_.usesLocation) {
            out.SetMult(this->localIso, parentToRoot);
        } else {
            out = parentToRoot;
        }
        return;
    }

    if (includeLocal != 0 &&
        state_.usesLocation) {
        out = this->localIso;
        return;
    }

    out.SetIdentity();
}

int CPlugTree::IsVisualValid(void) const {
    CPlugVisual *provider = this->visual;
    if (provider == nullptr) {
        return 0;
    }

    return provider->HasRenderableGeometry();
}

int CPlugTree::HideInvalidTrees(void) {
    if (!state_.visible) {
        return 0;
    }

    int anyVisibleChild = 0;
    const u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        if (this->GetChild(childIndex)->HideInvalidTrees()) {
            anyVisibleChild = 1;
        }
    }

    int shouldHide;
    if (this->visual != nullptr) {
        shouldHide = this->IsVisualValid() == 0;
    } else {
        shouldHide = anyVisibleChild == 0;
    }
    if (shouldHide) {
        state_.visible = false;
    }
    return state_.visible;
}

void CPlugTree::SetSurface(CPlugSurface *surface) {
    this->surface.MwSetNod(surface);
}

void CPlugTree::SetFuncTree(CFuncTree *funcTree) {
    this->funcTree.MwSetNod(funcTree);
}

void CPlugTree::SetVisual(
        CPlugVisual *visual,
        CPlugShader *shader,
        CPlugMaterial *material,
        int updateProviderShader) {
    CPlugTree *self = this;
    if (visual == self->visual &&
        (shader == nullptr || shader == self->shader) &&
        (material == nullptr || material == self->material)) {
        return;
    }

    self->visual.MwSetNod(visual);
    CPlugVisual *provider = self->visual;
    if (provider == nullptr) {
        self->shader.MwSetNod(nullptr);
        self->material.MwSetNod(nullptr);
        return;
    }

    if (material != nullptr) {
        self->material.MwSetNod(material);
        CPlugShader *supportedShader = material->GetSupportedShader();
        self->shader.MwSetNod(supportedShader);
    } else {
        if (shader != nullptr) {
            if (shader != self->shader) {
                shader->CheckGeometryRequirement();
                self->shader.MwSetNod(shader);
            }
        } else if (self->shader != nullptr) {
            if (auto *apply = dynamic_cast<CPlugShaderApply *>(
                        self->shader.Get())) {
                apply->RemoveLayers();
            } else {
                self->shader->RemovePasses();
            }
        } else {
            CPlugShader *defaultShader = static_cast<CPlugShaderApply *>(
                    CPlugShaderApply::MwNewCPlugShaderApply());
            self->shader.MwSetNod(defaultShader);
        }

        if (self->material != nullptr &&
            !self->material->DoesContainShader(self->shader, nullptr)) {
            self->material.MwSetNod(nullptr);
        }
    }

    if (self->shader == nullptr) {
        return;
    }

    if (updateProviderShader) {
        provider->SetDefaultShaderProperties(self->shader);
    }

}

CPlugTree *CPlugTree::CreateChildFromVisual(
        CPlugVisual *visual) {
    auto child = std::make_unique<CPlugTree>();
    if (visual != nullptr) {
        child->SetVisual(visual, nullptr, nullptr, 0);
    }
    CPlugTree *result = child.get();
    AddOwnedChild(std::move(child));
    return result;
}
