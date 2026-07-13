#pragma once

#include "engine/core/engine_types.h"
#include <memory>

#include "engine/core/mw_id.h"
#include "engine/rendering/plug.h"
#include "engine/scene/plug_physical_object.h"
#include "engine/rendering/plug_tree.h"
struct CHmsItem;

struct CPlugSolid : CPlug {
    static CMwId s_ParamId_PackBrotherShaders;

    CPlugSolid(void);
    ~CPlugSolid(void) override;
    virtual void CreateDefaultData(void);
    CPlugTree *CollisionTree(void) const;
    CPlugPhysicalObject &Physical(void) { return physical_; }
    const CPlugPhysicalObject &Physical(void) const { return physical_; }
    u32 NextGeneratedPlugId(void) const { return nextGeneratedPlugId_; }
    void SetNextGeneratedPlugId(u32 nextId) { nextGeneratedPlugId_ = nextId; }
    CPlugTree *GetPlugFromId(const CMwId &id) const;
    void DisconnectFromModel(int keepResolvedTree);
    void InternalDisconnectSubTree(CPlugTree *tree);
    void InternalConnectSubTree(CPlugTree *tree);
    void GivePlugId(CMwId &idOut) const;
    void MakeTreeIdsUnique(void);
    void SetTree(CPlugTree *tree, int refreshFlags);
    void SetOwnedTree(std::unique_ptr<CPlugTree> tree, int refreshFlags);
    void AddTree(CPlugTree *tree);
    void SetUseModel(int useModel);
    CPlugSolid *CreateModelInstance(void);

protected:
    void SetModel(CPlugSolid *model);

private:
    CPlugPhysicalObject physical_;
    mutable u32 nextGeneratedPlugId_ = 1u;
    std::unique_ptr<CPlugTree> collisionTree_;
    CMwNodRef<CPlugSolid> model_;
    float exclusionEllipsoidRadius_ = 0.0f;
    bool useModel = true;
};
