#pragma once

#include <utility>
#include <vector>

#include "engine/core/engine_types.h"
struct CSystemFid;
struct CMwNod;

struct CMwNod {
    struct SManuallyLoadedFid {
        CSystemFid *fid = nullptr;
    };

    CMwNod(void) = default;
    virtual ~CMwNod(void);
    virtual void SetIdName(const char *idName);
    virtual void MwIsKilled(CMwNod *killedNod);
    virtual void OnNodLoaded(void);

    unsigned long MwAddRef(void);
    unsigned long MwForceRef(unsigned long forcedRefCount);
    unsigned long MwRelease(void);
    void DependantSendMwIsKilled(void);
    void MwAddDependant(CMwNod *dependant) const;
    void MwSubDependant(CMwNod *dependant) const;
    void MwSubDependantSafe(CMwNod *dependant) const;
    void MwFinalSubDependant(CMwNod *releasedNod) const;

    CSystemFid *fid = nullptr;

private:
    friend struct CSystemFids;

    u32 refCount = 0u;
    unsigned long fidsTravelInfo = 0ul;
    mutable std::vector<CMwNod *> dependants;
};

template <typename T>
class CMwNodRef {
public:
    CMwNodRef() = default;
    explicit CMwNodRef(T *nod) { MwSetNod(nod); }
    CMwNodRef(const CMwNodRef &other) { MwSetNod(other.nod_); }
    CMwNodRef(CMwNodRef &&other) noexcept : nod_(other.nod_) {
        other.nod_ = nullptr;
    }
    ~CMwNodRef() { MwSetNod(nullptr); }

    CMwNodRef &operator=(const CMwNodRef &other) {
        if (this != &other) {
            MwSetNod(other.nod_);
        }
        return *this;
    }

    CMwNodRef &operator=(CMwNodRef &&other) noexcept {
        if (this != &other) {
            MwSetNod(nullptr);
            nod_ = other.nod_;
            other.nod_ = nullptr;
        }
        return *this;
    }

    CMwNodRef &operator=(T *nod) {
        MwSetNod(nod);
        return *this;
    }

    void MwSetNod(T *nod) {
        if (nod == nod_) {
            return;
        }
        if (nod != nullptr) {
            nod->MwAddRef();
        }
        T *old = nod_;
        if (old != nullptr) {
            old->MwRelease();
        }
        nod_ = nod;
    }

    T *Get(void) const { return nod_; }
    T *operator->(void) const { return nod_; }
    T &operator*(void) const { return *nod_; }
    operator T *(void) const { return nod_; }
    explicit operator bool(void) const { return nod_ != nullptr; }

private:
    T *nod_ = nullptr;
};

template <typename T, typename... Args>
CMwNodRef<T> MakeMwNod(Args &&...args) {
    return CMwNodRef<T>(new T(std::forward<Args>(args)...));
}
