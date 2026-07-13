#include "engine/core/classic_buffer.h"
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include <new>

namespace {

constexpr unsigned long ReusableCapacityLimit = 0x100000ul;

std::vector<std::unique_ptr<CClassicBufferMemory>> &BufferMemoryPool(void) {
    static std::vector<std::unique_ptr<CClassicBufferMemory>> buffers;
    return buffers;
}

}  // namespace

CClassicBufferMemory::CClassicBufferMemory(void) = default;

CClassicBufferMemory::~CClassicBufferMemory(void) = default;

CClassicBufferMemory *sBufferMemoryGetNew(void) {
    auto &pool = BufferMemoryPool();
    if (pool.empty()) {
        try {
            return std::make_unique<CClassicBufferMemory>().release();
        } catch (const std::bad_alloc &) {
            return nullptr;
        }
    }

    std::unique_ptr<CClassicBufferMemory> buffer = std::move(pool.back());
    pool.pop_back();
    buffer->Empty();
    return buffer.release();
}

void sBufferMemoryDeleteAll(void) {
    BufferMemoryPool().clear();
}

void sBufferMemoryFree(CClassicBufferMemory *&memoryBuffer) {
    std::unique_ptr<CClassicBufferMemory> owned(memoryBuffer);
    memoryBuffer = nullptr;
    if (!owned || owned->GetAllocatedSize() >= ReusableCapacityLimit) {
        return;
    }

    owned->PrepareForPoolReuse();
    BufferMemoryPool().push_back(std::move(owned));
}

CClassicBufferMemory::CClassicBufferMemory(
        CClassicBufferMemory &&other) noexcept {
    MoveStorageFrom(other);
}

CClassicBufferMemory &CClassicBufferMemory::operator=(
        CClassicBufferMemory &&other) noexcept {
    if (this != &other) {
        MoveStorageFrom(other);
    }
    return *this;
}

const unsigned char *CClassicBufferMemory::Data(void) const {
    if (const auto *owned = std::get_if<OwnedBytes>(&storage)) {
        return owned->data();
    }
    return std::get<AttachedBytes>(storage).data;
}

unsigned char *CClassicBufferMemory::Data(void) {
    return MutableData();
}

unsigned char *CClassicBufferMemory::MutableData(void) {
    if (auto *owned = std::get_if<OwnedBytes>(&storage)) {
        return owned->data();
    }
    return std::get<AttachedBytes>(storage).data;
}

unsigned long CClassicBufferMemory::Capacity(void) const {
    if (const auto *owned = std::get_if<OwnedBytes>(&storage)) {
        return static_cast<unsigned long>(owned->size());
    }
    return std::get<AttachedBytes>(storage).extent;
}

bool CClassicBufferMemory::EnsureCapacity(
        unsigned long requestedCapacity) {
    if (requestedCapacity <= Capacity()) {
        return true;
    }
    if (requestedCapacity > ClassicBufferMaximumExtent ||
        requestedCapacity >
                static_cast<unsigned long>(
                        std::numeric_limits<std::size_t>::max())) {
        return false;
    }

    OwnedBytes replacement(
            static_cast<std::size_t>(requestedCapacity),
            0u);
    if (logicalSize != 0ul) {
        const unsigned char *source = Data();
        if (source == nullptr || logicalSize > requestedCapacity) {
            return false;
        }
        std::copy_n(source, logicalSize, replacement.data());
    }
    storage = std::move(replacement);
    growthQuantum = std::max(growthQuantum, requestedCapacity);
    return true;
}

bool CClassicBufferMemory::ComputeGrowthCapacity(
        unsigned long requestedCapacity,
        unsigned long &capacityOut) const {
    const unsigned long quantum = growthQuantum != 0ul
            ? growthQuantum
            : DefaultGrowthQuantum;
    const unsigned long quotient = requestedCapacity / quantum;
    if (quotient == ClassicBufferMaximumExtent) {
        return false;
    }
    const unsigned long multiplier = quotient + 1ul;
    if (multiplier > ClassicBufferMaximumExtent / quantum) {
        return false;
    }
    capacityOut = multiplier * quantum;
    return capacityOut >= requestedCapacity;
}

void CClassicBufferMemory::ResetToOwnedEmpty(void) {
    storage.emplace<OwnedBytes>();
    logicalSize = 0ul;
    position = 0ul;
    growthQuantum = DefaultGrowthQuantum;
}

void CClassicBufferMemory::MoveStorageFrom(
        CClassicBufferMemory &source) noexcept {
    storage = std::move(source.storage);
    logicalSize = source.logicalSize;
    position = source.position;
    growthQuantum = source.growthQuantum;
    source.ResetToOwnedEmpty();
}

void CClassicBufferMemory::PrepareForPoolReuse(void) {
    if ((std::holds_alternative<AttachedBytes>(storage))) {
        ResetToOwnedEmpty();
    }
}

void CClassicBufferMemory::ZeroGapBefore(unsigned long endOffset) {
    if (logicalSize < position && position <= endOffset) {
        unsigned char *bytes = MutableData();
        std::fill(bytes + logicalSize, bytes + position, 0u);
    }
}

unsigned long CClassicBufferMemory::Read(
        void *destination,
        unsigned long byteCount) {
    if (destination == nullptr || byteCount == 0ul ||
        position > logicalSize || byteCount > logicalSize - position) {
        return 0ul;
    }

    std::copy_n(
            Data() + position,
            byteCount,
            static_cast<unsigned char *>(destination));
    position += byteCount;
    return byteCount;
}

unsigned long CClassicBufferMemory::Write(
        const void *source,
        unsigned long byteCount) {
    if (source == nullptr || byteCount == 0ul ||
        byteCount > ClassicBufferMaximumExtent - position) {
        return 0ul;
    }

    const unsigned long endOffset = position + byteCount;
    if (endOffset > Capacity()) {
        unsigned long newCapacity = 0ul;
        if (!ComputeGrowthCapacity(endOffset, newCapacity) ||
            !EnsureCapacity(newCapacity)) {
            return 0ul;
        }
    }

    ZeroGapBefore(endOffset);
    std::copy_n(
            static_cast<const unsigned char *>(source),
            byteCount,
            MutableData() + position);
    position = endOffset;
    logicalSize = std::max(logicalSize, endOffset);
    return byteCount;
}

int CClassicBufferMemory::Close(void) {
    Reset();
    return 1;
}

unsigned long CClassicBufferMemory::GetCurOffset(void) const {
    return position;
}

unsigned long CClassicBufferMemory::GetActualSize(void) const {
    return logicalSize;
}

int CClassicBufferMemory::IsSeekable(void) const {
    return 1;
}

void CClassicBufferMemory::SetCurOffset(unsigned long offset) {
    if (offset > ClassicBufferMaximumExtent) {
        return;
    }
    position = offset == ClassicBufferEndOffset ? logicalSize : offset;
}

void CClassicBufferMemory::AdvanceOffset(unsigned long byteCount) {
    if (byteCount <= ClassicBufferMaximumExtent - position) {
        position += byteCount;
    }
}

unsigned long CClassicBufferMemory::GetAllocatedSize(void) const {
    return Capacity();
}

void CClassicBufferMemory::PreAlloc(unsigned long requestedCapacity) {
    if (EnsureCapacity(requestedCapacity) && !(std::holds_alternative<AttachedBytes>(storage))) {
        growthQuantum = std::max(growthQuantum, Capacity());
    }
}

void CClassicBufferMemory::Attach(
        void *bytes,
        unsigned long byteCount) {
    if (bytes == nullptr) {
        ResetToOwnedEmpty();
        return;
    }

    storage.emplace<AttachedBytes>(AttachedBytes{
            static_cast<unsigned char *>(bytes),
            byteCount});
    logicalSize = byteCount;
    position = 0ul;
    growthQuantum = 0ul;
}

void CClassicBufferMemory::CopyAndDetachBufferMemory(
        CClassicBufferMemory &source) {
    if (this != &source) {
        MoveStorageFrom(source);
    }
}

void CClassicBufferMemory::Reset(void) {
    position = 0ul;
}

void CClassicBufferMemory::Empty(void) {
    position = 0ul;
    logicalSize = 0ul;
}

void CClassicBufferMemory::EmptyAndFreeMemory(void) {
    ResetToOwnedEmpty();
}

int CClassicBufferMemory::IsEqualBuffer(
        const CClassicBufferMemory &other) const {
    if (logicalSize != other.logicalSize) {
        return 0;
    }
    if (logicalSize == 0ul) {
        return 1;
    }
    return std::equal(
            Data(),
            Data() + logicalSize,
            other.Data()) ? 1 : 0;
}

unsigned long CClassicBufferMemory::WriteVoid(
        unsigned long byteCount) {
    if (byteCount == 0ul ||
        byteCount > ClassicBufferMaximumExtent - position) {
        return 0ul;
    }

    const unsigned long endOffset = position + byteCount;
    if (endOffset > Capacity()) {
        unsigned long newCapacity = 0ul;
        if (!ComputeGrowthCapacity(endOffset, newCapacity) ||
            !EnsureCapacity(newCapacity)) {
            return 0ul;
        }
    }

    ZeroGapBefore(endOffset);
    std::fill(
            MutableData() + position,
            MutableData() + endOffset,
            0u);
    position = endOffset;
    logicalSize = std::max(logicalSize, endOffset);
    return byteCount;
}

unsigned long CClassicBufferMemory::WriteCopy(
        CClassicBuffer *source,
        unsigned long byteCount) {
    if (source == nullptr || byteCount == 0ul ||
        byteCount > ClassicBufferMaximumExtent - position) {
        return 0ul;
    }

    const unsigned long requestedEnd = position + byteCount;
    if (requestedEnd > Capacity()) {
        unsigned long newCapacity = 0ul;
        if (!ComputeGrowthCapacity(requestedEnd, newCapacity) ||
            !EnsureCapacity(newCapacity)) {
            return 0ul;
        }
    }

    ZeroGapBefore(requestedEnd);
    const unsigned long copied = source->Read(
            MutableData() + position,
            byteCount);
    if (copied > byteCount) {
        return 0ul;
    }
    position += copied;
    logicalSize = std::max(logicalSize, position);
    return copied;
}
