#pragma once

#include <memory>

#include "engine/core/classic_buffer.h"
struct CFastStringInt;

struct CSystemFile : CClassicBuffer {
    struct CReadCallback {
        virtual void Callback(unsigned long long totalByteCount) = 0;
    };

    CSystemFile(void);
    ~CSystemFile(void) override;

    CSystemFile(const CSystemFile &) = delete;
    CSystemFile &operator=(const CSystemFile &) = delete;

    void Flush(void);
    unsigned long Write(
            const void *source,
            unsigned long byteCount) override;
    int SetOffset(unsigned long offset);
    unsigned long GetLength(void) const;
    unsigned long Read(
            void *destination,
            unsigned long byteCount) override;
    int Open(
            const CFastStringInt &filename,
            CClassicBuffer::EMode openMode,
            int secured,
            int truncate,
            int asynchronous,
            int shareRead);
    int Close(void) override;
    int IsStoringSecured(void) const override;
    unsigned long GetCurOffset(void) const override;
    unsigned long GetActualSize(void) const override;
    void SetCurOffset(unsigned long offset) override;
    const CFastStringInt *GetFileName(void) override;
    unsigned long GetOffset(void) const;
    int IsSeekable(void) const override;

    static void ReadCallbackSet(
            CReadCallback *callback,
            unsigned long stepByteSize);
    static void ReadCallbackByteReaded(unsigned long byteCount);

private:
    class Impl;
    std::unique_ptr<Impl> implementation;
    static CReadCallback *s_ReadCallback;
    static unsigned long s_ReadStepByteSize;
    static unsigned long long s_TotalReadByteCount;
    static unsigned long long s_TotalReadByteCountSentToCallback;
    static bool s_IsInReadCallback;
};
