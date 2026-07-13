#include "engine/resources/system_pack_desc.h"
#include "engine/resources/plug_file_zip.h"
#include "engine/resources/system_fid.h"
#include "engine/resources/system_fid_file.h"
#include "engine/resources/system_fids.h"
#include "engine/resources/system_file_name.h"
#include "engine/resources/system_pack_manager.h"
namespace {

constexpr SNat128 SpecialPackChecksum{{1u, 0u, 0u, 0u}};

}

CSystemPackDesc::CSystemPackDesc(
        const SNat128 &packChecksum,
        const CFastStringInt &packName,
        CSystemFidFile *packFid,
        SSystemTime *time)
        : checksum(packChecksum), name(packName) {
    (void)time;
    if (packFid != nullptr) {
        sourceFid = *packFid;
    }
}

CSystemPackDesc::~CSystemPackDesc(void) = default;

int CSystemPackDesc::IsChecksumSpecial(void) const {
    return checksum == SpecialPackChecksum;
}

void CSystemPackDesc::TouchLastTimeOfUse(void) {
    if (manager == nullptr) {
        ++lastUseSequence;
        return;
    }
    lastUseSequence = manager->NotePackUse();
    manager->MarkDirty();
}

bool CSystemPackDesc::HasSourceFid(void) const {
    return sourceFid.has_value();
}

bool CSystemPackDesc::ReferencesFid(const CSystemFid *candidate) const {
    return SourceFid() == candidate;
}

CSystemFids *CSystemPackDesc::CacheBase(void) const {
    if (CSystemFidFile *fid = SourceFid()) {
        return fid->Location();
    }
    return nullptr;
}

int CSystemPackDesc::PackFidIsZip(void) const {
    CSystemFidFile *fid = SourceFid();
    return fid != nullptr &&
           CSystemFileName::IsExtension(fid->FileName(), ".zip") != 0;
}

void CSystemPackDesc::GetContents(
        CSystemFids *&outFids,
        CSystemFid *&outFid) {
    outFids = nullptr;
    outFid = nullptr;
    CSystemFidFile *fid = SourceFid();
    if (fid == nullptr) {
        return;
    }
    if (contents) {
        outFids = contents->ContentsFolder();
        return;
    }
    if (!PackFidIsZip()) {
        outFid = fid;
        return;
    }

    CMwNod *loadedNod = nullptr;
    if (fid->LoadNode(loadedNod) == 0) {
        return;
    }
    (contents.MwSetNod((dynamic_cast<CPlugFileZip *>(loadedNod))));
    if (!contents) {
        return;
    }
    if (contents->ContentsMissing()) {
        contents->InstallFids(fid->Location(), 1);
    }
    outFids = contents->ContentsFolder();
}
