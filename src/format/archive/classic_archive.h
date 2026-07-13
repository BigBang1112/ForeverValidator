#pragma once

#include <cstdint>
#include <string>

#include "engine/core/cmw_nod.h"
struct CClassicBuffer;
struct CFastString;

struct CClassicArchive {
    CClassicArchive(void);
    virtual ~CClassicArchive(void);

    bool IsWriting(void) const;
    void SetWriting(bool writing);
    bool IsTextMode(void) const;
    void SetTextMode(bool textMode);
    bool IsMemoryArchive(void) const;
    void SetMemoryArchive(bool memoryArchive);

    CClassicBuffer *ArchiveBuffer(void);
    const CClassicBuffer *ArchiveBuffer(void) const;
    void SetArchiveBuffer(CClassicBuffer *buffer);
    void ClearArchiveBuffer(void);

    void ReadNatural(
            unsigned long *values,
            unsigned long count,
            int asMask);
    void WriteData(
            const void *data,
            unsigned long byteCount);
    void WriteNatural(
            const unsigned long *values,
            unsigned long count,
            int asMask);
    void WriteMask(
            const unsigned long *values,
            unsigned long count);
    void DoData(void *data, unsigned long byteCount);
    void DoNatural(
            unsigned long *values,
            unsigned long count,
            int asMask);
    void DoNat16(
            unsigned short *values,
            unsigned long count,
            int asMask);
    void DoNat8(
            unsigned char *values,
            unsigned long count,
            int asMask);
    void DoString(CFastString *values, unsigned long count);
    virtual void DoNodPtr(CMwNod *&nod);

    template <typename T>
    void MwDoNodRef(CMwNodRef<T> &reference) {
        CMwNod *nod = reference.Get();
        DoNodPtr(nod);
        if (!Good()) {
            return;
        }
        T *typed = dynamic_cast<T *>(nod);
        if (nod != nullptr && typed == nullptr) {
            Fail();
            return;
        }
        reference.MwSetNod(typed);
    }

    bool Good(void) const;
    void Fail(void);
    void SkipData(unsigned long byteCount);

protected:
    int ReadLine(void);
    void WriteLine(void);

private:
    enum class Direction : std::uint8_t {
        Reading,
        Writing,
    };

    enum class Representation : std::uint8_t {
        Binary,
        Text,
    };

    enum class Destination : std::uint8_t {
        External,
        Memory,
    };

    int ReadArchiveByte(char *out);
    int WriteArchiveBytes(const void *data, unsigned long byteCount);
    int ReadNaturalText(
            unsigned long *values,
            unsigned long count,
            int asMask);
    void WriteNaturalText(
            const unsigned long *values,
            unsigned long count,
            int asMask);

    CClassicBuffer *activeBuffer = nullptr;
    Direction direction = Direction::Reading;
    Representation representation = Representation::Binary;
    Destination destination = Destination::External;
    std::string textLine;
    bool archiveFailed = false;
};
