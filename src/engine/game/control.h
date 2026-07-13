#pragma once

#include <string>
#include <vector>

#include "engine/scene/scene_mobil.h"
struct CMwStack;
class CControlContainer;

class CSceneToy : public CSceneMobil {
public:
    CSceneToy(void);
    ~CSceneToy(void) override;
};

class CControlBase : public CSceneToy {
public:
    CControlBase(void);
    ~CControlBase(void) override;
    int CreateStack(CMwNod *nod, const char *stack);
    virtual int CreateStack_MwStack(CMwNod *nod, CMwStack *stack);
    static void RecursiveConnect(CControlBase *control,
                                 CMwNod *nod,
                                 int connect);

protected:
    friend class CControlContainer;
    CMwNod *boundNod = nullptr;
    std::string stackExpression_;
    CControlContainer *parentControl = nullptr;
    bool stackBindingEnabled = true;
};

class CControlContainer : public CControlBase {
public:
    CControlContainer(void);
    ~CControlContainer(void) override;
    virtual unsigned long AddChild(CControlBase *child,
                                   const GmVec3 &location);
    unsigned long RemoveChild(CControlBase *child);
    virtual unsigned long InternalRemoveChild(CControlBase *child,
                                              int removeFromScene);
    void RemoveAllChilds(void);
    virtual void ChangeChildIndex(unsigned long oldIndex,
                                  unsigned long newIndex);

protected:
    void ReleaseChilds(void);

private:
    friend class CControlBase;
    std::vector<CMwNodRef<CControlBase>> childControls;
};
