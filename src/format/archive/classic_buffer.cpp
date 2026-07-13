#include "engine/core/classic_buffer.h"
#include <algorithm>
#include <array>
#include <limits>

CClassicBuffer::CClassicBuffer(void) = default;

CClassicBuffer::~CClassicBuffer(void) = default;

int CClassicBuffer::IsStoringSecured(void) const {
    return 0;
}

int CClassicBuffer::IsSeekable(void) const {
    return 0;
}

void CClassicBuffer::SetCurOffset(unsigned long offset) {
    (void)offset;
}

const CFastStringInt *CClassicBuffer::GetFileName(void) {
    return nullptr;
}

int CClassicBuffer::ReadAll(void *data, unsigned long byteCount) {
    if (data == nullptr || byteCount == 0ul) {
        return 0;
    }

    auto *bytes = static_cast<unsigned char *>(data);
    unsigned long remaining = byteCount;
    while (remaining != 0ul) {
        const unsigned long readNow = Read(
                bytes + (byteCount - remaining),
                remaining);
        if (readNow == 0ul || readNow > remaining) {
            return 0;
        }
        remaining -= readNow;
    }
    return 1;
}

int CClassicBuffer::WriteAll(
        const void *data,
        unsigned long byteCount) {
    if (data == nullptr || byteCount == 0ul) {
        return 0;
    }

    const auto *bytes = static_cast<const unsigned char *>(data);
    unsigned long remaining = byteCount;
    while (remaining != 0ul) {
        const unsigned long writtenNow = Write(
                bytes + (byteCount - remaining),
                remaining);
        if (writtenNow == 0ul || writtenNow > remaining) {
            return 0;
        }
        remaining -= writtenNow;
    }
    return 1;
}

unsigned long CClassicBuffer::Skip(unsigned long byteCount) {
    if (byteCount == 0ul) {
        return 0ul;
    }

    if (IsSeekable() != 0) {
        const unsigned long oldPosition = GetCurOffset();
        if (oldPosition > ClassicBufferMaximumExtent ||
            byteCount > ClassicBufferMaximumExtent - oldPosition) {
            return 0ul;
        }
        SetCurOffset(oldPosition + byteCount);
        const unsigned long newPosition = GetCurOffset();
        return newPosition >= oldPosition ? newPosition - oldPosition : 0ul;
    }

    std::array<unsigned char, 4096> scratch{};
    unsigned long remaining = byteCount;
    while (remaining != 0ul) {
        const unsigned long batch = std::min<unsigned long>(
                remaining,
                static_cast<unsigned long>(scratch.size()));
        const unsigned long readNow = Read(scratch.data(), batch);
        if (readNow == 0ul || readNow > batch) {
            break;
        }
        remaining -= readNow;
    }
    return byteCount - remaining;
}

int CClassicBuffer::IsEqualBuffer(CClassicBuffer &other) {
    const unsigned long byteCount = GetActualSize();
    if (other.GetActualSize() != byteCount) {
        return 0;
    }

    if (GetCurOffset() != 0ul) {
        if (IsSeekable() == 0) {
            return 0;
        }
        SetCurOffset(0ul);
    }
    if (other.GetCurOffset() != 0ul) {
        if (other.IsSeekable() == 0) {
            return 0;
        }
        other.SetCurOffset(0ul);
    }

    std::array<unsigned char, 4096> left{};
    std::array<unsigned char, 4096> right{};
    unsigned long remaining = byteCount;
    while (remaining != 0ul) {
        const unsigned long batch = std::min<unsigned long>(
                remaining,
                static_cast<unsigned long>(left.size()));
        if (ReadAll(left.data(), batch) == 0 ||
            other.ReadAll(right.data(), batch) == 0 ||
            !std::equal(left.begin(), left.begin() + batch, right.begin())) {
            return 0;
        }
        remaining -= batch;
    }
    return 1;
}

int CClassicBuffer::CopyFrom(CClassicBuffer *source) {
    if (source == nullptr) {
        return 0;
    }

    std::array<unsigned char, 4096> bytes{};
    unsigned long remaining = source->GetActualSize();
    while (remaining != 0ul) {
        const unsigned long batch = std::min<unsigned long>(
                remaining,
                static_cast<unsigned long>(bytes.size()));
        if (source->ReadAll(bytes.data(), batch) == 0 ||
            WriteAll(bytes.data(), batch) == 0) {
            return 0;
        }
        remaining -= batch;
    }
    return 1;
}
