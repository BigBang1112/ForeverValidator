#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <openssl/blowfish.h>

#include "engine/core/classic_buffer.h"
#include "engine/core/engine_types.h"
// Encrypted classic-buffer stream with owned storage and ordinary C++ lifetime.
class CClassicBufferCrypted : public CClassicBuffer {
public:
    enum ECipher : unsigned long {
        ECipher_BlowfishCbc = 0ul,
        ECipher_BlowfishCtr = 1ul,
        ECipher_Multiplexed = 2ul,
    };

    CClassicBufferCrypted(ECipher cipher, int storing);
    ~CClassicBufferCrypted(void) override;

    CClassicBufferCrypted(const CClassicBufferCrypted &) = delete;
    CClassicBufferCrypted &operator=(const CClassicBufferCrypted &) = delete;

    static std::unique_ptr<CClassicBufferCrypted>
    CreateBlowfishReaderForMemory(
            const unsigned char *bytes,
            unsigned long byteCount,
            unsigned long ciphertextOffset,
            const SNat128 &key);

    void Blowfish_InitForReading(
            CClassicBuffer *source,
            const SNat128 &key,
            unsigned long ciphertextOffset);

    unsigned long Read(void *destination,
                       unsigned long byteCount) override;
    unsigned long Write(const void *source,
                        unsigned long byteCount) override;
    int Close(void) override;
    void SetCurOffset(unsigned long offset) override;
    unsigned long GetCurOffset(void) const override;
    unsigned long GetActualSize(void) const override;
    int IsStoringSecured(void) const override;
    int IsSeekable(void) const override;
    const CFastStringInt *GetFileName(void) override;

    void MixPageFeedback(const unsigned char *payload,
                         unsigned long byteCount);
    void MixPageFeedbackWord(unsigned long value);
    int Error(void) const;

protected:
    unsigned long BlowfishCBC_Read(void *destination,
                                   unsigned long byteCount);
    unsigned long BlowfishCBC_Write(const void *source,
                                    unsigned long byteCount);

private:
    static constexpr unsigned long BlowfishBlockByteCount = 8ul;
    static constexpr unsigned long FeedbackPageByteCount = 0x100ul;

    void ResetReadState(void);
    bool ApplyPageFeedbackIfNeeded(void);
    bool ReadNextBlock(void);
    void DecryptReversedBlock(
            const std::array<unsigned char, BlowfishBlockByteCount> &cipher,
            std::array<unsigned char, BlowfishBlockByteCount> &plain) const;

    ECipher cipher_;
    bool storing_;
    CClassicBuffer *source_ = nullptr;
    std::unique_ptr<CClassicBufferMemory> ownedSource_;
    unsigned long ciphertextOffset_ = 0ul;
    unsigned long actualSize_ = 0ul;
    SNat128 keyValue_{};
    BF_KEY blowfishKey_{};
    std::array<unsigned char, BlowfishBlockByteCount> initialIv_{};
    std::array<unsigned char, BlowfishBlockByteCount> currentIv_{};
    std::array<unsigned char, BlowfishBlockByteCount> plainBlock_{};
    unsigned long plainBlockOffset_ = BlowfishBlockByteCount;
    unsigned long pageOffset_ = FeedbackPageByteCount;
    unsigned long logicalOffset_ = 0ul;
    std::uint32_t feedbackLow_ = 0u;
    std::uint32_t feedbackHigh_ = 0u;
    bool error_ = false;
};
