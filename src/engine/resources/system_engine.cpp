#include "engine/resources/system_engine.h"
#include <algorithm>

#include "engine/resources/system_fid.h"
#include "engine/resources/system_fid_file.h"
#include "engine/resources/system_fid_memory.h"
#include "engine/resources/system_fid_parameters.h"
#include "engine/resources/system_fids.h"
#include "engine/resources/system_manager_file.h"
CSystemPackManager *CSystemEngine::s_PackManager = nullptr;

CMwEngine::CMwEngine(void) = default;
CMwEngine::~CMwEngine(void) = default;

CSystemEngine::CSystemEngine(void)
        : managerFile(std::make_unique<CSystemManagerFile>()),
          locationBuffer(std::make_unique<CSystemFids>()) {
    for (std::unique_ptr<CSystemFidsDrive> &drive : drives) {
        drive = std::make_unique<CSystemFidsDrive>();
    }
    for (std::unique_ptr<CSystemFidsDrive> &drive : drives) {
        drive->SetLogicalPrefix(GetDriveName(drive.get()));
    }
}

CSystemEngine::~CSystemEngine(void) = default;

void CSystemEngine::UnbindFidNod(CMwNod *nod, CSystemFid *fid) {
    if (nod != nullptr && fid != nullptr) {
        fid->DetachNod(nod);
    }
}

void CSystemEngine::UnbindFid(CMwNod *nod) {
    if (nod != nullptr && nod->fid != nullptr) {
        UnbindFidNod(nod, nod->fid);
    }
}

void CSystemEngine::BindFidNod(CMwNod *nod, CSystemFid *fid) {
    if (nod == nullptr || fid == nullptr) {
        return;
    }
    if (nod->fid != nullptr) {
        UnbindFidNod(nod, nod->fid);
    }
    fid->SetNod(nod);
}

CSystemFids *CSystemEngine::GetLocationBuffer(void) {
    return locationBuffer.get();
}

void CSystemEngine::DetachBuffer(CClassicBufferMemory *memoryBuffer) {
    CSystemFids *bufferLocation = GetLocationBuffer();
    if (bufferLocation == nullptr) {
        return;
    }
    CSystemFidMemory *matchingFid = nullptr;
    for (const auto &entry : bufferLocation->leaves) {
        CSystemFid *candidate = entry.Get();
        auto *memoryFid = dynamic_cast<CSystemFidMemory *>(candidate);
        if (memoryFid != nullptr &&
            memoryFid->MemoryBuffer() == memoryBuffer) {
            matchingFid = memoryFid;
            break;
        }
    }
    if (matchingFid == nullptr) {
        return;
    }
    std::unique_ptr<CSystemFid> ownedFid =
            bufferLocation->ReleaseOwnedLeave(matchingFid);
    if (ownedFid == nullptr) {
        bufferLocation->RemoveLeaveSafe(matchingFid);
    }
    if (matchingFid->nod != nullptr) {
        UnbindFidNod(matchingFid->nod, matchingFid);
    }
    if (ownedFid == nullptr) {
        delete matchingFid;
    }
}

void CSystemEngine::AddFidNod(
        CMwNod *nod,
        CSystemFid *fid,
        const CSystemFidParameters &params) {
    if (fid == nullptr || fid->GetNod(params) != nullptr) {
        return;
    }
    BindFidNod(nod, fid);
    fid->parameters = params;
    if (fid->location != nullptr) {
        fid->location->AddLeaveIfNot(fid);
    }
}

CSystemFidFile *CSystemEngine::FindMatchingFileFid(
        CSystemFids *location,
        CSystemFidFile *fid) {
    if (location == nullptr) {
        return nullptr;
    }
    const auto found = std::find_if(
            location->leaves.begin(), location->leaves.end(),
            [fid](const auto &entry) {
                CSystemFid *candidate = entry.Get();
                return candidate != nullptr && candidate->IsEqualFid(fid);
            });
    return found != location->leaves.end()
            ? dynamic_cast<CSystemFidFile *>(found->Get())
            : nullptr;
}

CSystemFid *CSystemEngine::FindMatchingFid(
        CSystemFids *location,
        CSystemFid *fid) {
    if (location == nullptr) {
        return nullptr;
    }
    const auto found = std::find_if(
            location->leaves.begin(), location->leaves.end(),
            [fid](const auto &entry) {
                CSystemFid *candidate = entry.Get();
                return candidate != nullptr && candidate->IsEqualFid(fid);
            });
    return found != location->leaves.end() ? found->Get() : nullptr;
}

void CSystemEngine::RemoveFid(CSystemFid *fid) {
    if (fid == nullptr) {
        return;
    }
    if (fid->location != nullptr) {
        fid->location->RemoveLeaveSafe(fid);
    }
    UnbindFid(fid->nod);
}

CSystemFid *CSystemEngine::FindFidResource(CSystemFidFile *fid) {
    if (fid == nullptr || !fid->resourceIndex.has_value()) {
        return nullptr;
    }
    const std::size_t index = *fid->resourceIndex;
    return index < resourceFids.size()
            ? resourceFids[index].Get()
            : nullptr;
}

CSystemFidFile *CSystemEngine::FindFidFile(CSystemFidFile *fid) {
    return fid != nullptr ? FindMatchingFileFid(fid->location, fid) : nullptr;
}

CSystemFid *CSystemEngine::FindFid(CSystemFid *fid) {
    if (fid == nullptr) {
        return nullptr;
    }
    if (auto *fileFid = dynamic_cast<CSystemFidFile *>(fid)) {
        return fileFid->resourceFile
                ? FindFidResource(fileFid)
                : FindFidFile(fileFid);
    }
    return FindMatchingFid(fid->location, fid);
}

CSystemFidFile *CSystemEngine::FindFid(
        CSystemFids *location,
        const CFastStringInt &filename,
        int findWay) {
    (void)findWay;
    if (location != nullptr) {
        return dynamic_cast<CSystemFidFile *>(
                location->FindFid(
                        filename,
                        findWay,
                        CSystemFids::FindWay_ChildrenOnly));
    }
    CFastStringInt relativeName;
    CSystemFidsDrive *drive = nullptr;
    return FindDriveAndRelName(filename, drive, relativeName) != 0
            ? dynamic_cast<CSystemFidFile *>(
                      drive->FindFid(
                              relativeName,
                              findWay,
                              CSystemFids::FindWay_ChildrenOnly))
            : nullptr;
}

CSystemFidFile *CSystemEngine::FindFidFromBaseName(
        CSystemFids *location,
        const CFastStringInt &filename,
        const AssetAudience &audience) {
    if (location != nullptr) {
        return dynamic_cast<CSystemFidFile *>(
                location->FindFidFromBaseName(filename, audience));
    }
    CFastStringInt relativeName;
    CSystemFidsDrive *drive = nullptr;
    return FindDriveAndRelName(filename, drive, relativeName) != 0
            ? dynamic_cast<CSystemFidFile *>(
                      drive->FindFidFromBaseName(
                              relativeName, audience))
            : nullptr;
}
