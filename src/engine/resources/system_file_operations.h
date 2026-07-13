#pragma once

#include <cstdint>
#include <memory>

#include "engine/core/classic_buffer.h"

struct CFastStringInt;

namespace tmnf::platform {

struct SystemFileMetadata {
    bool exists = false;
    bool readOnly = true;
    std::uint64_t byteSize = 0u;
};

class SystemFileHandle {
public:
    virtual ~SystemFileHandle() = default;

    virtual void Flush() = 0;
    virtual unsigned long Write(
            const void *source,
            unsigned long byteCount) = 0;
    virtual int SetOffset(unsigned long offset) = 0;
    virtual unsigned long GetLength() const = 0;
    virtual unsigned long Read(
            void *destination,
            unsigned long byteCount) = 0;
    virtual int Open(
            const CFastStringInt &filename,
            CClassicBuffer::EMode openMode,
            int secured,
            int truncate,
            int asynchronous,
            int shareRead) = 0;
    virtual int Close() = 0;
    virtual int IsStoringSecured() const = 0;
    virtual unsigned long GetOffset() const = 0;
    virtual bool NeedsClose() const = 0;
};

class SystemFileOperations {
public:
    virtual ~SystemFileOperations() = default;

    virtual std::unique_ptr<SystemFileHandle> CreateHandle() const = 0;
    virtual SystemFileMetadata ReadMetadata(
            const CFastStringInt &fullName) const = 0;
};

SystemFileOperations *GetSystemFileOperations() noexcept;
void SetSystemFileOperations(SystemFileOperations *operations) noexcept;

}  // namespace tmnf::platform
