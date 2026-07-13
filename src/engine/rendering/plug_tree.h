#pragma once

#include "engine/core/engine_types.h"
#include <memory>
#include <optional>
#include <vector>

#include "engine/core/cfast_buffer.h"
#include "engine/core/func_tree.h"
#include "engine/core/gm_types.h"
#include "engine/core/mw_id.h"
#include "engine/rendering/plug.h"
#include "engine/rendering/plug_material.h"
#include "engine/rendering/plug_shader.h"
#include "engine/physics/geometry/plug_surface.h"
#include "engine/rendering/plug_tree_optimization.h"
#include "engine/rendering/plug_visual.h"
#include "engine/resources/system_fid_parameters.h"
struct CPlugSolid;
struct CPlugTree;

struct CPlugTreeGenerator : CMwNod {
    ~CPlugTreeGenerator(void) override = default;
    virtual void GenerateTree(CPlugTree *tree) = 0;
    virtual void CleanTree(CPlugTree *tree) = 0;
    virtual void CopyGeneratedTree(const CPlugTree *source,
                                   CPlugTree *destination);
    virtual void GetOptimizedGroups(
            CFastBuffer<SPlugTreeOptimGroup *> &groups,
            const SPlugTreeOptimCriteria &criteria,
            const SPlugTreeOptimTravel &travel,
            CPlugTree *tree) = 0;
};

struct CPlugTreeGenSolid : CPlugTreeGenerator {
    CPlugTreeGenSolid(void) = default;
    ~CPlugTreeGenSolid(void) override = default;
    void GenerateTree(CPlugTree *tree) override;
    void CleanTree(CPlugTree *tree) override;
    void GetOptimizedGroups(CFastBuffer<SPlugTreeOptimGroup *> &groups,
                            const SPlugTreeOptimCriteria &criteria,
                            const SPlugTreeOptimTravel &travel,
                            CPlugTree *tree) override;

protected:
    CPlugTree *MergeSolidInTree(
            const CFastBuffer<CPlugTree *> &trees);
};

struct CPlugTree : CPlug {
    struct SFlags {
        bool groupingBoundary = false;
        bool visible = true;
        bool requiresMatchingShaderState = false;
        bool requiresMatchingMaterialState = false;
        bool modelInstance = false;
        bool collisionEnabled = false;
        bool groupingMarker = false;
        bool pickableVisual = true;
        bool renderBeforeFidParametrization = false;
        bool castsShadows = true;
        bool rooted = true;
        bool locationDirty = true;
        bool usesLocation = false;
    };

    class CIteratorTree;
    class CIteratorVisual;
    class CIteratorSurface;
    class CIteratorShader;
    class CIteratorMaterial;

    CPlugTree(void);
    ~CPlugTree(void) override;
    CPlugTree(const CPlugTree &) = delete;
    CPlugTree &operator=(const CPlugTree &) = delete;
    CPlugTree(CPlugTree &&) = delete;
    CPlugTree &operator=(CPlugTree &&) = delete;

    const SFlags &State(void) const;
    void ApplyLoadedState(const SFlags &state);
    void SetCollisionEnabled(bool enabled);
    void EnableCollisionOnAncestors(void);
    void SetModelInstance(bool enabled);
    bool IsModelInstance(void) const;
    void SetShadowCaster(bool enabled);
    bool IsShadowCaster(void) const;
    bool IsVisible(void) const;
    bool UsesLocation(void) const;
    void SetIsPickableVisual(int enabled);
    SPlugTreeOptimProperties OptimizationProperties(void) const;
    void ApplyOptimizationProperties(const SPlugTreeOptimProperties &properties);

    virtual unsigned long GetChildCount(void) const;
    virtual CPlugTree *GetChild(unsigned long index) const;
    virtual CPlugTree *GetChildToRenderMip(void) const;
    virtual CPlugTree *GetLastChild(void) const;
    virtual const CMwId *MwGetId(void) const;
    int GetIsRooted(void) const;
    void SetIsRooted(int isRooted);
    void SetIsVisible(int isVisible);
    void SetUseLocation(int useLocation);
    void SetLocation(const GmIso4 &location);
    void SetTranslation(const GmVec3 &translation);
    virtual CPlugTree *GetPlugFromId(const CMwId &id);
    virtual CPlugTree *GetPlugFromModelId(const CMwId &modelId);
    CPlugTree *GetChildFromId(const CMwId &id) const;
    int FindTree(const CPlugTree *candidate) const;
    virtual void DeleteAllChilds(void);
    virtual void DeleteChildPtr(CPlugTree *child);
    virtual void SetChild(CPlugTree *child, unsigned long index);
    virtual CPlugTree *DetachChildPtr(CPlugTree *child);
    virtual CPlugTree *DetachChild(unsigned long index);
    virtual void DeleteChild(unsigned long index);
    virtual void AddChild(CPlugTree *child);
    void AddOwnedChild(std::unique_ptr<CPlugTree> child);
    virtual unsigned long GetChildIndex(CPlugTree *child);
    virtual void DeleteAllVolatileChilds(void);
    void InternalSetMwId(const CMwId &id);
    void SetPlugId(const CMwId &id);
    void GetThisToRootTransfo(GmIso4 &out,
                              int includeLocal,
                              CPlugTree *stopAt) const;
    int HideInvalidTrees(void);
    void RecursiveSetUnassigned(void);
    void RecursiveSetRooted(void);
    void SetGenerator(CPlugTreeGenerator *generator,
                      int updateGeneratorLinks);
    void SetSurface(CPlugSurface *surface);
    void SetFuncTree(CFuncTree *funcTree);
    void SetShader(CPlugShader *shader);
    void SetMaterial(CPlugMaterial *material);
    void SetVisual(CPlugVisual *visual,
                   CPlugShader *shader,
                   CPlugMaterial *material,
                   int updateProviderShader);
    unsigned long GetRecursiveTreeCount(int requireVisible,
                                        int requireRooted) const;
    virtual unsigned long GetRecursiveVertexCount(
            int requireVisible,
            int lastVisualMipOnly) const;
    unsigned long GetVolatileChildCount(void) const;
    unsigned long GetRootedChildCount(void) const;
    virtual void Generate(int flags);
    virtual void ApplyFidParameters(
            const CSystemFidParameters *currentParameters,
            CSystemFidParameters *outParameters,
            CFastBuffer<CMwNod::SManuallyLoadedFid> &manuallyLoadedFids);
    CPlugShader *Shader(void) const;
    CPlugMaterial *Material(void) const;
    CPlugTreeGenerator *Generator(void) const;
    CFuncTree *FunctionTree(void) const;
    CPlugTree *ParentTree(void) const;
    CPlugSolid *ModelSolidReference(void) const;
    const CMwId &ModelNodeId(void) const;
    const CMwId &PlugId(void) const;
    void SetTreeBounds(const GmBoxAligned &bounds);
    int HasShader(void) const;
    void MarkRenderBeforeFidParametrization(void);
    void AddMatchedRenderBeforeParam(
            const CSystemFidParameters::SParam_Fid_Common &param,
            CSystemFidParameters *outParameters);
    void ApplySpecialRenderBeforeParams(
            const CSystemFidParameters &parameters,
            CSystemFidParameters *outParameters);
    virtual void CopyFromModel(const CPlugTree *source, int deepCopy);
    void InternalSetSolid(CPlugSolid *solid);
    CPlugSolid *OwningSolid(void) const;
    void SetOwningSolidRecursive(CPlugSolid *solid);
    int IsVolatileChild(void) const;
    CPlugTree *GetModelTree(void) const;
    void RefreshThisFromModel(void);
    virtual void DisconnectThisFromModel(int duplicatePolicy,
                                         unsigned long disconnectMask);
    void DisconnectFromModel(int duplicatePolicy,
                             unsigned long disconnectMask);
    CPlugTree *CreateChildFromVisual(CPlugVisual *visual);
    using CPlugTreeCloneFactory = CPlugTree *(CPlugTree::*)(void) const;
    virtual CPlugTree *InternalRecursiveCreate(
            CPlugTreeCloneFactory cloneFactory,
            int includeResolvable,
            CPlugTree *outRoot) const;
    CPlugTree *CreateModelInstance(void) const;
    CPlugTree *DuplicateThis(void) const;
    CPlugTree *InternalCreateSolidModelInstance(void) const;
    CPlugTree *DuplicateRecursive(void) const;
    unsigned long GetAllTreeStart(void);
    unsigned long GetAllChildStart(void);
    CPlugTree *GetAllChildNext(unsigned long &cursor);
    virtual int UpdateBoundingBox(int flags);
    CPlugTree *GetAllTreeNext(unsigned long &cursor);
    int HasWorldBox(void) const;
    int AllowsSurfaceCollision(void) const;
    int HasLocalTransform(void) const;
    const GmBoxAligned &Box(void) const;
    const GmIso4 &LocalIso(void) const;
    CPlugSurface *Surface(void) const;
    CPlugVisual *Visual(void) const;
    void ComposeCollisionIso(const GmIso4 &parentIso,
                             GmIso4 &outIso) const;
    void GetTransformedCollisionBox(const GmIso4 &iso,
                                    GmBoxAligned &outBox) const;
    std::unique_ptr<CPlugTree> MakeTreeFromGroups(
            const std::vector<SPlugTreeOptimGroup *> &groups,
            const SPlugTreeOptimCriteria &criteria);
    bool ShouldForceSingleOptimGroup(
            const SPlugTreeOptimCriteria &criteria) const;
    virtual void GetOptimizedGroups(
            CFastBuffer<SPlugTreeOptimGroup *> &groups,
            const SPlugTreeOptimCriteria &criteria,
            const SPlugTreeOptimTravel &travel);
    virtual void GenerateOptimizedTree(
            CPlugTree *&outTree,
            const SPlugTreeOptimCriteria &criteria);
    void ApplyDecorationFlags(bool visible,
                              bool castsShadows,
                              bool collision,
                              bool propagateVisibleToChildren,
                              bool propagateCasterToChildren);

protected:
    void DeconnectAsChild(CPlugTree *child);
    void ConnectAsChild(CPlugTree *child, int notifyRoot);
    int IsVisualValid(void) const;
    CPlugSurface *CreateGroupSurface(SPlugTreeOptimGroup *group);
    CPlugVisual *CreateGroupVisual(SPlugTreeOptimGroup *group);
    int IsOptimizable(const SPlugTreeOptimCriteria &criteria) const;
    int IsEqual(SPlugTreeOptimGroup *group,
                const SPlugTreeOptimCriteria &criteria,
                const SPlugTreeOptimTravel &travel);
    CPlugTree *MakeGroupTree(SPlugTreeOptimGroup *group,
                             const SPlugTreeOptimCriteria &criteria);
    CPlugTree *InternalCreateSolidModelInstanceThis(void) const;
    virtual int GetDecorationBoundingBox(int flags,
                                         GmBoxAligned &out) const;

private:
    friend struct CPlugTreeVisualMip;

    CPlugSolid *owningSolid = nullptr;
    CMwId id;
    CMwNodRef<CPlugSolid> modelSolid;
    CMwId modelId;
    CPlugTree *parent = nullptr;
    GmBoxAligned box{};
    GmIso4 localIso{};
    CMwNodRef<CPlugSurface> surface;
    CMwNodRef<CPlugVisual> visual;
    CMwNodRef<CPlugShader> shader;
    CMwNodRef<CPlugMaterial> material;
    CMwNodRef<CPlugTreeGenerator> generator;
    CMwNodRef<CFuncTree> funcTree;

    void SetInvalidBox(void) {
        box.SetInvalidForPlugTree();
    }
    int EmptyLeafRefreshReturn(void) const {
        return !state_.visible;
    }
    CPlugSurface *CloneSurfaceForModelCopy(const CPlugSurface *source);
    CPlugTree *CopyCloneIdentityFromSource(const CPlugTree &source);
    void ApplyCurrentFidParametersToClone(void);
    void PropagateVisibleToSubtree(bool visible);
    void PropagateShadowCasterToSubtree(bool castsShadows);
    void FoldChildState(const CPlugTree &child);

    SFlags state_{};
    std::vector<std::unique_ptr<CPlugTree>> children;
};

class CPlugTree::CIteratorTree {
public:
    enum EMode : unsigned long {
        Mode_IncludeRoot = 0u,
        Mode_ExcludeRoot = 1u,
        Mode_SkipRootedSubtrees = 2u,
    };

    CIteratorTree(CPlugTree *tree, EMode mode);
    CPlugTree *GetNextTree(void);
    void ResetItTree(CPlugTree *tree, EMode mode);
    bool HasNext(void) const;

protected:
    CIteratorTree(void) = default;
    CPlugTree *CurrentTree(void) const {
        return currentTree;
    }

    template <typename Resource>
    void ResetToResource(
            CPlugTree *tree,
            EMode modeValue,
            CMwNodRef<Resource> CPlugTree::*member) {
        ResetItTree(tree, modeValue);
        while (HasNext() && !(CurrentTree()->*member)) {
            GetNextTree();
        }
    }

    template <typename Resource>
    Resource *GetNextResource(
            CPlugTree **outTree,
            CMwNodRef<Resource> CPlugTree::*member) {
        CPlugTree *tree = GetNextTree();
        while (HasNext() && !(CurrentTree()->*member)) {
            GetNextTree();
        }
        if (outTree != nullptr) {
            *outTree = tree;
        }
        return tree != nullptr ? (tree->*member).Get() : nullptr;
    }

private:
    void AdvanceAfter(CPlugTree *tree);

    std::vector<unsigned long> childIndexStack;
    CPlugTree *currentTree = nullptr;
    EMode mode = Mode_IncludeRoot;
};

class CPlugTree::CIteratorVisual : public CIteratorTree {
public:
    CIteratorVisual(CPlugTree *tree, EMode mode);
    void ResetItVisual(CPlugTree *tree, EMode mode);
    CPlugVisual *GetNextVisual(CPlugTree **outTree);
};

class CPlugTree::CIteratorSurface : public CIteratorTree {
public:
    CIteratorSurface(CPlugTree *tree, EMode mode);
    void ResetItSurface(CPlugTree *tree, EMode mode);
    CPlugSurface *GetNextSurface(CPlugTree **outTree);
};

class CPlugTree::CIteratorShader : public CIteratorTree {
public:
    CIteratorShader(CPlugTree *tree, EMode mode);
    void ResetItShader(CPlugTree *tree, EMode mode);
    CPlugShader *GetNextShader(CPlugTree **outTree);
};

class CPlugTree::CIteratorMaterial : public CIteratorTree {
public:
    CIteratorMaterial(CPlugTree *tree, EMode mode);
    void ResetItMaterial(CPlugTree *tree, EMode mode);
    CPlugMaterial *GetNextMaterial(CPlugTree **outTree);
};

struct CPlugTreeVisualMip : CPlugTree {
    CPlugTreeVisualMip(void);
    ~CPlugTreeVisualMip(void) override;
    unsigned long GetChildCount(void) const override;
    CPlugTree *GetChild(unsigned long index) const override;
    CPlugTree *GetChildToRenderMip(void) const override;
    unsigned long GetChildIndex(CPlugTree *child) override;
    void DeleteAllChilds(void) override;
    void DeleteChildPtr(CPlugTree *child) override;
    void SetChild(CPlugTree *child, unsigned long index) override;
    CPlugTree *DetachChildPtr(CPlugTree *child) override;
    CPlugTree *DetachChild(unsigned long index) override;
    void DeleteChild(unsigned long index) override;
    void SetLevelTree(unsigned long index, CPlugTree *tree);
    void SetOwnedLevelTree(unsigned long index,
                           std::unique_ptr<CPlugTree> tree);
    unsigned long LevelCount(void) const;
    CPlugTree *LevelTree(unsigned long index) const;
    float LevelFarZ(unsigned long index) const;
    void AddLevel(CPlugTree *tree, float farZ);
    void AddOwnedLevel(std::unique_ptr<CPlugTree> tree, float farZ);
    void SetLevelFarZ(unsigned long index, float farZ);
    unsigned long SetQualityFromMinZ(float minZ);
    void SetQuality(unsigned long quality);
    void GetMipOptimizedGroups(int mip,
                               CFastBuffer<SPlugTreeOptimGroup *> &groups,
                               const SPlugTreeOptimCriteria &criteria,
                               const SPlugTreeOptimTravel &travel);
    void GetOptimizedGroups(CFastBuffer<SPlugTreeOptimGroup *> &groups,
                            const SPlugTreeOptimCriteria &criteria,
                            const SPlugTreeOptimTravel &travel) override;
    unsigned long GetRecursiveVertexCount(
            int requireVisible,
            int lastVisualMipOnly) const override;
    void DisconnectThisFromModel(int duplicatePolicy,
                                 unsigned long disconnectMask) override;

protected:
    void SetQualityInternal(unsigned long quality);
    void SetDistributionFromFarZs(void);
    CPlugTree *InternalRecursiveCreate(CPlugTreeCloneFactory cloneFactory,
                                       int includeResolvable,
                                       CPlugTree *outRoot) const override;
    void CopyFromModel(const CPlugTree *source, int deepCopy) override;

private:
    struct MipLevel {
        std::unique_ptr<CPlugTree> tree;
        float farZ = 0.0f;
    };

    std::vector<MipLevel> mipLevels;
    std::optional<unsigned long> currentLevel;
};
