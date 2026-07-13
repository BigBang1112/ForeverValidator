#include "engine/resources/system_file.h"

#include <mutex>
#include <new>
#include <utility>

#include "engine/core/fast_string.h"
#include "engine/resources/system_file_operations.h"

namespace {

class ActiveFlagGuard {
public:
    explicit ActiveFlagGuard(bool &flag) : activeFlag(flag) {
        activeFlag = true;
    }

    ~ActiveFlagGuard() {
        activeFlag = false;
    }

    ActiveFlagGuard(const ActiveFlagGuard &) = delete;
    ActiveFlagGuard &operator=(const ActiveFlagGuard &) = delete;

private:
    bool &activeFlag;
};

class ReadProgressBatcher {
public:
    bool ShouldDispatch(unsigned long requestedByteCount) {
        std::lock_guard<std::mutex> lock(mutex);
        requestedBytes += requestedByteCount;
        if (requestedBytes <= DispatchThreshold) {
            return false;
        }
        requestedBytes = 0u;
        return true;
    }

private:
    static constexpr unsigned long long DispatchThreshold = 0x40000u;

    std::mutex mutex;
    unsigned long long requestedBytes = 0u;
};

ReadProgressBatcher &FileReadProgressBatcher() {
    static ReadProgressBatcher batcher;
    return batcher;
}

}  // namespace

class CSystemFile::Impl {
public:
    CFastStringInt logicalFileName;
    std::unique_ptr<tmnf::platform::SystemFileHandle> handle;
    bool readInProgress = false;
};

CSystemFile::CReadCallback *CSystemFile::s_ReadCallback = nullptr;
unsigned long CSystemFile::s_ReadStepByteSize = 0u;
unsigned long long CSystemFile::s_TotalReadByteCount = 0u;
unsigned long long CSystemFile::s_TotalReadByteCountSentToCallback = 0u;
bool CSystemFile::s_IsInReadCallback = false;

CSystemFile::CSystemFile(void)
        : implementation(std::make_unique<Impl>()) {}

CSystemFile::~CSystemFile(void) {
    if (implementation != nullptr && implementation->handle != nullptr &&
        implementation->handle->NeedsClose()) {
        Close();
    }
}

void CSystemFile::Flush(void) {
    if (implementation->handle != nullptr) {
        implementation->handle->Flush();
    }
}

unsigned long CSystemFile::Write(
        const void *source,
        unsigned long byteCount) {
    return implementation->handle != nullptr
            ? implementation->handle->Write(source, byteCount)
            : 0u;
}

int CSystemFile::SetOffset(unsigned long offset) {
    return implementation->handle != nullptr
            ? implementation->handle->SetOffset(offset)
            : 0;
}

unsigned long CSystemFile::GetLength(void) const {
    return implementation->handle != nullptr
            ? implementation->handle->GetLength()
            : 0u;
}

unsigned long CSystemFile::Read(
        void *destination,
        unsigned long byteCount) {
    if (implementation->handle == nullptr || implementation->readInProgress ||
        (destination == nullptr && byteCount != 0u)) {
        return 0u;
    }

    ActiveFlagGuard readGuard(implementation->readInProgress);
    const bool dispatchProgress =
            FileReadProgressBatcher().ShouldDispatch(byteCount);
    const unsigned long readByteCount =
            implementation->handle->Read(destination, byteCount);
    if (dispatchProgress) {
        ReadCallbackByteReaded(readByteCount);
    }
    return readByteCount;
}

int CSystemFile::Open(
        const CFastStringInt &filename,
        CClassicBuffer::EMode openMode,
        int secured,
        int truncate,
        int asynchronous,
        int shareRead) {
    implementation->logicalFileName = filename;
    if (implementation->handle == nullptr) {
        tmnf::platform::SystemFileOperations *operations =
                tmnf::platform::GetSystemFileOperations();
        if (operations == nullptr) {
            return 0;
        }
        try {
            implementation->handle = operations->CreateHandle();
        } catch (const std::bad_alloc &) {
            return 0;
        }
        if (implementation->handle == nullptr) {
            return 0;
        }
    }
    return implementation->handle->Open(
            filename,
            openMode,
            secured,
            truncate,
            asynchronous,
            shareRead);
}

int CSystemFile::Close(void) {
    return implementation->handle != nullptr
            ? implementation->handle->Close()
            : 1;
}

int CSystemFile::IsStoringSecured(void) const {
    return implementation->handle != nullptr
            ? implementation->handle->IsStoringSecured()
            : 0;
}

unsigned long CSystemFile::GetCurOffset(void) const {
    return GetOffset();
}

unsigned long CSystemFile::GetActualSize(void) const {
    return GetLength();
}

void CSystemFile::SetCurOffset(unsigned long offset) {
    SetOffset(offset);
}

const CFastStringInt *CSystemFile::GetFileName(void) {
    return &implementation->logicalFileName;
}

unsigned long CSystemFile::GetOffset(void) const {
    return implementation->handle != nullptr
            ? implementation->handle->GetOffset()
            : 0u;
}

int CSystemFile::IsSeekable(void) const {
    return 1;
}

void CSystemFile::ReadCallbackSet(
        CReadCallback *callback,
        unsigned long stepByteSize) {
    s_ReadCallback = callback;
    s_ReadStepByteSize = stepByteSize;
    s_TotalReadByteCount = 0u;
    s_TotalReadByteCountSentToCallback = 0u;
}

void CSystemFile::ReadCallbackByteReaded(unsigned long byteCount) {
    if (s_ReadCallback == nullptr) {
        return;
    }

    s_TotalReadByteCount += byteCount;
    const unsigned long long total = s_TotalReadByteCount;
    if (total <= s_TotalReadByteCountSentToCallback + s_ReadStepByteSize) {
        return;
    }

    s_TotalReadByteCountSentToCallback = total;
    if (s_IsInReadCallback) {
        return;
    }

    ActiveFlagGuard callbackGuard(s_IsInReadCallback);
    CReadCallback *const callback = s_ReadCallback;
    callback->Callback(total);
}
