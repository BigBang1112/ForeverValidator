#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <variant>
#include <vector>

inline constexpr unsigned long ClassicBufferMaximumExtent =
        static_cast<unsigned long>(
                std::numeric_limits<std::uint32_t>::max());
inline constexpr unsigned long ClassicBufferEndOffset =
        ClassicBufferMaximumExtent;
inline constexpr unsigned long ClassicBufferReadError =
        ClassicBufferMaximumExtent;

struct CFastStringInt;
struct CClassicBufferMemory;

struct CClassicBuffer {
    enum class EMode : unsigned long {
        Read = 1ul,
        Write = 2ul,
    };

    CClassicBuffer(void);
    virtual ~CClassicBuffer(void);

    CClassicBuffer(const CClassicBuffer &) = delete;
    CClassicBuffer &operator=(const CClassicBuffer &) = delete;

    virtual unsigned long Read(
            void *destination,
            unsigned long byteCount) = 0;
    virtual unsigned long Write(
            const void *source,
            unsigned long byteCount) = 0;
    virtual int Close(void) = 0;
    virtual int IsStoringSecured(void) const;
    virtual unsigned long GetCurOffset(void) const = 0;
    virtual unsigned long GetActualSize(void) const = 0;
    virtual int IsSeekable(void) const;
    virtual void SetCurOffset(unsigned long offset);
    virtual const CFastStringInt *GetFileName(void);

    void AddCompressedBlock(
            const CClassicBufferMemory &memoryBuffer);
    int WriteAll(const void *data, unsigned long byteCount);
    unsigned long Skip(unsigned long byteCount);
    int ReadAll(void *data, unsigned long byteCount);
    int IsEqualBuffer(CClassicBuffer &other);
    int CopyFrom(CClassicBuffer *source);
    CClassicBufferMemory *CreateUncompressedBlock(void);
};

struct CClassicBufferMemory : CClassicBuffer {
    CClassicBufferMemory(void);
    ~CClassicBufferMemory(void) override;

    CClassicBufferMemory(const CClassicBufferMemory &) = delete;
    CClassicBufferMemory &operator=(const CClassicBufferMemory &) = delete;
    CClassicBufferMemory(CClassicBufferMemory &&other) noexcept;
    CClassicBufferMemory &operator=(CClassicBufferMemory &&other) noexcept;

    unsigned long Read(
            void *destination,
            unsigned long byteCount) override;
    unsigned long Write(
            const void *source,
            unsigned long byteCount) override;
    int Close(void) override;
    unsigned long GetCurOffset(void) const override;
    unsigned long GetActualSize(void) const override;
    int IsSeekable(void) const override;
    void SetCurOffset(unsigned long offset) override;

    virtual void AdvanceOffset(unsigned long byteCount);
    virtual unsigned long GetAllocatedSize(void) const;

    void PreAlloc(unsigned long requestedCapacity);
    void Attach(void *bytes, unsigned long byteCount);
    void CopyAndDetachBufferMemory(CClassicBufferMemory &source);
    void Reset(void);
    void Empty(void);
    void EmptyAndFreeMemory(void);
    int IsEqualBuffer(const CClassicBufferMemory &other) const;
    unsigned long WriteVoid(unsigned long byteCount);
    unsigned long WriteCopy(
            CClassicBuffer *source,
            unsigned long byteCount);

    unsigned char *Data(void);
    const unsigned char *Data(void) const;

private:
    friend void sBufferMemoryFree(CClassicBufferMemory *&memoryBuffer);

    struct AttachedBytes {
        unsigned char *data = nullptr;
        unsigned long extent = 0ul;
    };

    using OwnedBytes = std::vector<unsigned char>;
    using Storage = std::variant<OwnedBytes, AttachedBytes>;

    static constexpr unsigned long DefaultGrowthQuantum = 32ul;

    Storage storage;
    unsigned long logicalSize = 0ul;
    unsigned long position = 0ul;
    unsigned long growthQuantum = DefaultGrowthQuantum;

    unsigned char *MutableData(void);
    unsigned long Capacity(void) const;
    bool EnsureCapacity(unsigned long requestedCapacity);
    bool ComputeGrowthCapacity(
            unsigned long requestedCapacity,
            unsigned long &capacityOut) const;
    void MoveStorageFrom(CClassicBufferMemory &source) noexcept;
    void PrepareForPoolReuse(void);
    void ResetToOwnedEmpty(void);
    void ZeroGapBefore(unsigned long endOffset);
};

CClassicBufferMemory *sBufferMemoryGetNew(void);
void sBufferMemoryDeleteAll(void);
void sBufferMemoryFree(CClassicBufferMemory *&memoryBuffer);
