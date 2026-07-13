#include "format/archive/system_archive_nod.h"
#include <memory>

#include "engine/resources/plug_file.h"
#include "engine/physics/geometry/plug_surface.h"
#include "engine/rendering/plug_visual.h"
#include "engine/resources/system_fid.h"
#include "engine/resources/system_fid_file.h"
#include "engine/resources/system_fid_memory.h"
#include "engine/resources/system_file_name.h"
#include "format/vertex_stream/vertex_stream_archive.h"
#include <new>
CSystemArchiveNod::CSystemArchiveNod(void) {
    SetMemoryArchive(true);
}

CSystemArchiveNod::~CSystemArchiveNod(void) = default;

int CSystemArchiveNod::DoFidLoadRefs(
        EArchive loadMode,
        CClassicBuffer *buffer) {
    if (fid == nullptr) {
        return 0;
    }
    fid->ClearAssetType();
    archiveMode = loadMode;

    if (buffer != nullptr) {
        SetArchiveBuffer(buffer);
        const int ok = DoLoadAllRef();
        ClearArchiveBuffer();
        return ok != 0;
    }

    if (auto *memoryFid = dynamic_cast<CSystemFidMemory *>(fid)) {
        CClassicBufferMemory *memory = memoryFid->MemoryBuffer();
        if (memory == nullptr) {
            return 0;
        }
        SetArchiveBuffer(memory);
        const int ok = DoLoadAllRef();
        ClearArchiveBuffer();
        return ok != 0;
    }

    auto *loadableFid = dynamic_cast<CSystemFidFile *>(
            fid->ParametrizedGetLoadableFid());
    if (loadableFid == nullptr) {
        return 0;
    }
    const CFastStringInt &filename = loadableFid->FileName();
    if (CSystemFileName::IsExtensionGbx(filename) != 0) {
        CClassicBuffer *openedBuffer = fid->BufferOpen(
                CClassicBuffer::EMode::Read, true);
        if (openedBuffer == nullptr) {
            return 0;
        }
        SetArchiveBuffer(openedBuffer);
        const int ok = DoLoadAllRef();
        fid->BufferClose(openedBuffer);
        ClearArchiveBuffer();
        return ok;
    }

    fid->SetAssetType(CPlugFile::GetAssetTypeFromFid(loadableFid));
    return 1;
}

int CSystemArchiveNod::DoLoadAllRef(void) {
    return 0;
}

int CSystemArchiveNod::DoFidLoadFile(CMwNod *&outNod) {
    outNod = nullptr;
    return 0;
}

int CSystemArchiveNod::AddInternalRef(
        CMwNod *nod,
        unsigned long &outRefIndex,
        const char *contextName) {
    (void)contextName;
    for (u32 index = 0u; index < internalRefs.size(); ++index) {
        if (internalRefs[index].Get() == nod) {
            outRefIndex = index;
            return 0;
        }
    }
    outRefIndex = static_cast<unsigned long>(internalRefs.size());
    internalRefs.emplace_back(nod);
    return 1;
}

void CSystemArchiveNod::DoNodPtr(CMwNod *&nod) {
    VertexStreamArchiveCodec::ArchiveReference(*this, nod, internalRefs);
}

int CSystemArchiveNod::Compare(
        CMwNod *leftNod,
        CMwNod *rightNod,
        int &isEqualOut) {
    isEqualOut = leftNod == rightNod ? 1 : 0;
    return 1;
}

int CSystemArchiveNod::Duplicate(CMwNod *&target, int branchSelector) {
    if (branchSelector != 0 || target == nullptr) {
        return 0;
    }
    try {
        if (auto *visual = dynamic_cast<CPlugVisual *>(target)) {
            std::unique_ptr<CPlugVisual> duplicate = visual->CloneVisual();
            if (duplicate != nullptr) {
                target = duplicate.release();
                return 1;
            }
        }
        if (auto *surface = dynamic_cast<CPlugSurface *>(target)) {
            std::unique_ptr<CPlugSurface> duplicate =
                    surface->CloneMeshSurface();
            if (duplicate != nullptr) {
                target = duplicate.release();
                return 1;
            }
        }
    } catch (const std::bad_alloc &) {
        return 0;
    }
    return 0;
}

int CSystemArchiveNod::DoLoadResource(
        unsigned long resourceId,
        CMwNod *&outNod) {
    (void)resourceId;
    outNod = nullptr;
    return 0;
}

int CSystemArchiveNod::LoadResource(
        unsigned long resourceId,
        CMwNod *&outNod) {
    CSystemArchiveNod archive;
    return archive.DoLoadResource(resourceId, outNod);
}

int CSystemArchiveNod::DoLoadFromFid(CMwNod *&outNod) {
    return DoFidLoadFile(outNod);
}

int CSystemArchiveNod::LoadFromFid(
        CMwNod *&outNod,
        CSystemFid *activeFid,
        EArchive mode) {
    CSystemArchiveNod archive;
    archive.fid = activeFid;
    archive.archiveMode = mode;
    return archive.DoLoadFromFid(outNod);
}

int CSystemFid::LoadNode(CMwNod *&outNode) {
    return CSystemArchiveNod::LoadFromFid(
            outNode,
            this,
            CSystemArchiveNod::Archive_Default);
}
