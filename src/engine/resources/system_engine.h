#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "engine/core/mw_engine.h"
struct CClassicBufferMemory;
struct CFastStringInt;
struct CMwNod;
class AssetAudience;
struct CSystemFid;
struct CSystemFidFile;
struct CSystemFidMemory;
struct CSystemFidParameters;
struct CSystemFids;
struct CSystemFidsDrive;
struct CSystemManagerFile;
struct CSystemPackManager;

struct CSystemEngine : CMwEngine {
    static constexpr std::uint32_t DriveSlotCount = 5u;
    static CSystemPackManager *s_PackManager;

    CSystemEngine(void);
    ~CSystemEngine(void) override;

    void UnbindFidNod(CMwNod *nod, CSystemFid *fid);
    void UnbindFid(CMwNod *nod);
    CSystemFids *GetLocationBuffer(void);
    void BindFidNod(CMwNod *nod, CSystemFid *fid);
    void AddFidNod(CMwNod *nod,
                   CSystemFid *fid,
                   const CSystemFidParameters &params);
    void RemoveFid(CSystemFid *fid);
    void RemoveAndDeleteFid(CSystemFid *fid);
    CSystemFid *FindFidResource(CSystemFidFile *fid);
    CSystemFidFile *FindFidFile(CSystemFidFile *fid);
    int FindDriveAndRelName(const CFastStringInt &filename,
                            CSystemFidsDrive *&outDrive,
                            CFastStringInt &outRelativeName);
    CSystemFidFile *FindFid(
            CSystemFids *location,
            const CFastStringInt &filename,
            int findWay);
    CSystemFidFile *FindFidFromBaseName(
            CSystemFids *location,
            const CFastStringInt &filename,
            const AssetAudience &audience);
    void DetachBuffer(CClassicBufferMemory *memoryBuffer);
    CSystemFid *FindFid(CSystemFid *fid);
    const char *GetDriveName(const CSystemFidsDrive *drive) const;

private:
    std::unique_ptr<CSystemManagerFile> managerFile;
    std::unique_ptr<CSystemFids> locationBuffer;
    std::array<std::unique_ptr<CSystemFidsDrive>, DriveSlotCount> drives{};
    std::vector<CMwNodRef<CSystemFidFile>> resourceFids;

    CSystemFidFile *FindMatchingFileFid(
            CSystemFids *location,
            CSystemFidFile *fid);
    CSystemFid *FindMatchingFid(
            CSystemFids *location,
            CSystemFid *fid);
    void BuildDriveName(std::uint32_t index,
                        CSystemFidsDrive *drive,
                        int hasDrivePrefix,
                        CFastStringInt &outName) const;
    int TryDriveForRelativeName(std::uint32_t index,
                                const CFastStringInt &filename,
                                int hasDrivePrefix,
                                CSystemFidsDrive *&outDrive,
                                CFastStringInt &outRelativeName) const;
};
