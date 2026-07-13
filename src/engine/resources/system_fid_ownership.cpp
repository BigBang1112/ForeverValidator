#include "engine/resources/system_fid.h"
#include "engine/resources/system_engine.h"
#include "engine/resources/system_fid_memory.h"
#include "engine/resources/system_fid_parameters.h"
#include "engine/resources/system_fids.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

CSystemFid *CSystemFid::ParametrizedGetFid(
        const CSystemFidParameters &params) const {
    CSystemFid *loadable = ParametrizedGetLoadableFid();
    CSystemFid *filterFid = params.HasNonDefaultParameters()
            ? loadable
            : nullptr;

    if (params.IncludesFid(loadable->parameters, filterFid, 0) != 0) {
        return loadable;
    }

    for (const std::unique_ptr<CSystemFid> &ownedChild :
         loadable->parametrizedFids) {
        CSystemFid *child = ownedChild.get();
        if (params.IncludesFid(child->parameters, filterFid, 0) != 0) {
            return child;
        }
    }
    return nullptr;
}

void CSystemFid::ParametrizedAddFid(CSystemFid *fid) {
    if (fid == nullptr) {
        return;
    }
    CSystemFid *loadable = ParametrizedGetLoadableFid();
    if (fid == loadable ||
        std::any_of(
                loadable->parametrizedFids.begin(),
                loadable->parametrizedFids.end(),
                [fid](const std::unique_ptr<CSystemFid> &candidate) {
                    return candidate.get() == fid;
                })) {
        return;
    }
    fid->DetachFromLoadableParent();
    loadable->parametrizedFids.emplace_back(fid);
    fid->loadableFid = loadable;
}

const CFastBuffer<CSystemFid *> &
CSystemFid::ParametrizedGetAllSubFids(void) const {
    parametrizedFidView.clear();
    parametrizedFidView.reserve(
            static_cast<unsigned long>(parametrizedFids.size()));
    for (const std::unique_ptr<CSystemFid> &child : parametrizedFids) {
        parametrizedFidView.push_back(child.get());
    }
    return parametrizedFidView;
}

CMwNod *CSystemFid::GetNod(
        const CSystemFidParameters &params) const {
    CSystemFid *fid = ParametrizedGetFid(params);
    return fid != nullptr ? fid->nod : nullptr;
}

void CSystemFid::CopyFromFid(CSystemFid *other) {
    if (other->nod != nullptr) {
        SetNod(other->nod);
    } else {
        nod = nullptr;
    }
    parameters = other->parameters;
    location = other->location;
}

std::unique_ptr<CSystemFid> CSystemFid::ReleaseParametrizedFid(
        CSystemFid *fid) {
    const auto found = std::find_if(
            parametrizedFids.begin(),
            parametrizedFids.end(),
            [fid](const std::unique_ptr<CSystemFid> &candidate) {
                return candidate.get() == fid;
            });
    if (found == parametrizedFids.end()) {
        return nullptr;
    }
    std::unique_ptr<CSystemFid> released = std::move(*found);
    parametrizedFids.erase(found);
    released->loadableFid = nullptr;
    return released;
}

void CSystemFid::DetachFromLoadableParent(void) {
    CSystemFid *parent = loadableFid;
    if (parent == nullptr) {
        return;
    }

    std::unique_ptr<CSystemFid> released =
            parent->ReleaseParametrizedFid(this);
    if (released != nullptr) {
        (void)released.release();
    }
    loadableFid = nullptr;
}

CSystemFidMemory::CSystemFidMemory(void) = default;

CSystemFidMemory::~CSystemFidMemory(void) = default;

int CSystemFidMemory::IsEqualFid(CSystemFid *other) {
    CSystemFidMemory *memoryFid = dynamic_cast<CSystemFidMemory *>(other);
    if (memoryFid == nullptr || !CSystemFid::IsEqualFid(other)) {
        return 0;
    }
    return MemoryBuffer() == memoryFid->MemoryBuffer();
}

void CSystemFidMemory::CopyFromFid(CSystemFid *other) {
    if (other == this) {
        CSystemFid::CopyFromFid(other);
        return;
    }

    CSystemFidMemory *source = dynamic_cast<CSystemFidMemory *>(other);
    CClassicBufferMemory *sourceBuffer =
            source != nullptr ? source->MemoryBuffer() : nullptr;
    if (OwnsMemoryBuffer() && MemoryBuffer() == sourceBuffer &&
        sourceBuffer != nullptr) {
        throw std::logic_error(
                "an owned memory buffer cannot become its own observer");
    }

    CSystemFid::CopyFromFid(other);
    BorrowMemoryBuffer(sourceBuffer);
}

CClassicBufferMemory *CSystemFidMemory::MemoryBuffer(void) {
    return memoryBuffer.has_value() ? memoryBuffer->Get() : nullptr;
}

const CClassicBufferMemory *CSystemFidMemory::MemoryBuffer(void) const {
    return memoryBuffer.has_value() ? memoryBuffer->Get() : nullptr;
}

void CSystemFidMemory::BorrowMemoryBuffer(CClassicBufferMemory *buffer) {
    if (OwnsMemoryBuffer() && MemoryBuffer() == buffer && buffer != nullptr) {
        throw std::logic_error(
                "an owned memory buffer cannot become its own observer");
    }
    if (buffer == nullptr) {
        memoryBuffer.reset();
    } else {
        memoryBuffer.emplace(*buffer);
    }
}

void CSystemFidMemory::OwnMemoryBuffer(
        std::unique_ptr<CClassicBufferMemory> buffer) {
    if (buffer == nullptr) {
        memoryBuffer.reset();
    } else {
        memoryBuffer.emplace(std::move(buffer));
    }
}

bool CSystemFidMemory::OwnsMemoryBuffer(void) const {
    return memoryBuffer.has_value() && memoryBuffer->IsOwned();
}

void CSystemEngine::RemoveAndDeleteFid(CSystemFid *fid) {
    if (fid == nullptr) {
        return;
    }

    std::unique_ptr<CSystemFid> ownedFid;
    if (fid->location != nullptr) {
        ownedFid = fid->location->ReleaseOwnedLeave(fid);
    }

    while (!fid->parametrizedFids.empty()) {
        CSystemFid *child = fid->parametrizedFids.back().get();
        RemoveFid(child);
        fid->ReleaseParametrizedFid(child);
    }
    RemoveFid(fid);
    if (ownedFid == nullptr) {
        delete fid;
    }
}
