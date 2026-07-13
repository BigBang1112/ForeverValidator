#include "format/archive/classic_buffer_crypted.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

#include "format/archive/classic_archive_wire.h"
namespace {

using BlowfishBlock = std::array<unsigned char, 8>;
namespace ArchiveWire = TmnfFormat::ClassicArchiveWire;

BlowfishBlock ReverseBlowfishWords(const BlowfishBlock &source) {
    return {{source[3], source[2], source[1], source[0],
             source[7], source[6], source[5], source[4]}};
}

std::array<unsigned char, 16> KeyBytes(const SNat128 &key) {
    std::array<unsigned char, 16> bytes{};
    for (unsigned long wordIndex = 0ul; wordIndex < key.words.size();
         ++wordIndex) {
        const std::uint32_t word = key.words[wordIndex];
        const unsigned long byteIndex = wordIndex * 4ul;
        const ArchiveWire::NaturalBytes encoded =
                ArchiveWire::EncodeNatural(word);
        std::copy(encoded.begin(), encoded.end(), bytes.begin() + byteIndex);
    }
    return bytes;
}

} // namespace

CClassicBufferCrypted::CClassicBufferCrypted(ECipher cipher, int storing)
        : cipher_(cipher), storing_(storing != 0) {
}

CClassicBufferCrypted::~CClassicBufferCrypted(void) = default;

std::unique_ptr<CClassicBufferCrypted>
CClassicBufferCrypted::CreateBlowfishReaderForMemory(
        const unsigned char *bytes,
        unsigned long byteCount,
        unsigned long ciphertextOffset,
        const SNat128 &key) {
    if (bytes == nullptr || ciphertextOffset < BlowfishBlockByteCount ||
        ciphertextOffset > byteCount) {
        return nullptr;
    }

    auto source = std::make_unique<CClassicBufferMemory>();
    if (source->WriteAll(bytes, byteCount) == 0) {
        return nullptr;
    }
    source->SetCurOffset(0ul);

    auto reader = std::make_unique<CClassicBufferCrypted>(
            ECipher_BlowfishCbc, 0);
    reader->ownedSource_ = std::move(source);
    reader->Blowfish_InitForReading(
            reader->ownedSource_.get(), key, ciphertextOffset);
    return reader->error_ ? nullptr : std::move(reader);
}

void CClassicBufferCrypted::Blowfish_InitForReading(
        CClassicBuffer *source,
        const SNat128 &key,
        unsigned long ciphertextOffset) {
    source_ = source;
    keyValue_ = key;
    ciphertextOffset_ = ciphertextOffset;
    error_ = source_ == nullptr ||
            source_->IsSeekable() == 0 ||
            ciphertextOffset_ < BlowfishBlockByteCount ||
            ciphertextOffset_ > source_->GetActualSize();
    if (error_) {
        return;
    }

    const std::array<unsigned char, 16> keyBytes = KeyBytes(keyValue_);
    BF_set_key(&blowfishKey_,
               static_cast<int>(keyBytes.size()),
               keyBytes.data());

    source_->SetCurOffset(ciphertextOffset_ - BlowfishBlockByteCount);
    if (source_->ReadAll(initialIv_.data(), BlowfishBlockByteCount) == 0) {
        error_ = true;
        return;
    }
    actualSize_ = source_->GetActualSize() - ciphertextOffset_;
    ResetReadState();
}

void CClassicBufferCrypted::ResetReadState(void) {
    if (source_ == nullptr) {
        error_ = true;
        return;
    }
    source_->SetCurOffset(ciphertextOffset_);
    currentIv_ = initialIv_;
    plainBlock_.fill(0u);
    plainBlockOffset_ = BlowfishBlockByteCount;
    pageOffset_ = FeedbackPageByteCount;
    logicalOffset_ = 0ul;
    feedbackLow_ = 0ul;
    feedbackHigh_ = 0ul;
    error_ = false;
}

bool CClassicBufferCrypted::ApplyPageFeedbackIfNeeded(void) {
    if (pageOffset_ < FeedbackPageByteCount) {
        return true;
    }
    const ArchiveWire::NaturalBytes low = ArchiveWire::EncodeNatural(
            static_cast<std::uint32_t>(feedbackLow_));
    const ArchiveWire::NaturalBytes high = ArchiveWire::EncodeNatural(
            static_cast<std::uint32_t>(feedbackHigh_));
    for (unsigned long index = 0ul; index < low.size(); ++index) {
        currentIv_[index] ^= low[index];
        currentIv_[index + low.size()] ^= high[index];
    }
    feedbackLow_ = 0ul;
    feedbackHigh_ = 0ul;
    pageOffset_ = 0ul;
    return true;
}

void CClassicBufferCrypted::DecryptReversedBlock(
        const std::array<unsigned char, BlowfishBlockByteCount> &cipher,
        std::array<unsigned char, BlowfishBlockByteCount> &plain) const {
    const BlowfishBlock reversedInput = ReverseBlowfishWords(cipher);
    BlowfishBlock reversedPlain{};
    BF_ecb_encrypt(reversedInput.data(),
                   reversedPlain.data(),
                   &blowfishKey_,
                   BF_DECRYPT);
    plain = ReverseBlowfishWords(reversedPlain);
}

bool CClassicBufferCrypted::ReadNextBlock(void) {
    if (!ApplyPageFeedbackIfNeeded() || source_ == nullptr) {
        error_ = true;
        return false;
    }

    BlowfishBlock cipher{};
    if (source_->ReadAll(cipher.data(), BlowfishBlockByteCount) == 0) {
        error_ = true;
        return false;
    }
    DecryptReversedBlock(cipher, plainBlock_);
    for (unsigned long index = 0ul; index < BlowfishBlockByteCount;
         ++index) {
        plainBlock_[index] ^= currentIv_[index];
    }
    currentIv_ = cipher;
    plainBlockOffset_ = 0ul;
    return true;
}

unsigned long CClassicBufferCrypted::BlowfishCBC_Read(
        void *destination,
        unsigned long byteCount) {
    if (byteCount == 0ul) {
        return 0ul;
    }
    if (destination == nullptr || cipher_ != ECipher_BlowfishCbc ||
        storing_ || byteCount > actualSize_ - std::min(actualSize_, logicalOffset_)) {
        error_ = true;
        return ClassicBufferReadError;
    }

    auto *output = static_cast<unsigned char *>(destination);
    unsigned long written = 0ul;
    while (written < byteCount) {
        if (plainBlockOffset_ == BlowfishBlockByteCount &&
            !ReadNextBlock()) {
            return ClassicBufferReadError;
        }
        const unsigned long available =
                BlowfishBlockByteCount - plainBlockOffset_;
        const unsigned long count = std::min(
                byteCount - written, available);
        std::copy_n(plainBlock_.begin() + plainBlockOffset_,
                    count,
                    output + written);
        plainBlockOffset_ += count;
        pageOffset_ += count;
        logicalOffset_ += count;
        written += count;
    }
    return written;
}

unsigned long CClassicBufferCrypted::BlowfishCBC_Write(
        const void *source,
        unsigned long byteCount) {
    (void)source;
    (void)byteCount;
    error_ = true;
    return 0ul;
}

unsigned long CClassicBufferCrypted::Read(
        void *destination,
        unsigned long byteCount) {
    return BlowfishCBC_Read(destination, byteCount);
}

unsigned long CClassicBufferCrypted::Write(
        const void *source,
        unsigned long byteCount) {
    return BlowfishCBC_Write(source, byteCount);
}

int CClassicBufferCrypted::Close(void) {
    if (source_ != nullptr) {
        source_->Close();
    }
    source_ = nullptr;
    ownedSource_.reset();
    return 1;
}

void CClassicBufferCrypted::SetCurOffset(unsigned long offset) {
    if (offset == 0ul) {
        ResetReadState();
        return;
    }
    error_ = true;
}

unsigned long CClassicBufferCrypted::GetCurOffset(void) const {
    return logicalOffset_;
}

unsigned long CClassicBufferCrypted::GetActualSize(void) const {
    return actualSize_;
}

int CClassicBufferCrypted::IsStoringSecured(void) const {
    return storing_ ? 1 : 0;
}

int CClassicBufferCrypted::IsSeekable(void) const {
    return 0;
}

const CFastStringInt *CClassicBufferCrypted::GetFileName(void) {
    return source_ != nullptr ? source_->GetFileName() : nullptr;
}

void CClassicBufferCrypted::MixPageFeedback(
        const unsigned char *payload,
        unsigned long byteCount) {
    if (payload == nullptr) {
        return;
    }
    for (unsigned long index = 0ul; index < byteCount; ++index) {
        const std::uint32_t low = static_cast<std::uint32_t>(feedbackLow_);
        const std::uint32_t high = static_cast<std::uint32_t>(feedbackHigh_);
        const std::uint32_t mixed =
                static_cast<std::uint32_t>(payload[index]) | 0xaau;
        feedbackLow_ = mixed ^ (low << 13u) ^ (high >> 19u);
        feedbackHigh_ = (low >> 19u) | (high << 13u);
    }
}

void CClassicBufferCrypted::MixPageFeedbackWord(unsigned long value) {
    const ArchiveWire::NaturalBytes bytes = ArchiveWire::EncodeNatural(
            static_cast<std::uint32_t>(value));
    MixPageFeedback(bytes.data(), bytes.size());
}

int CClassicBufferCrypted::Error(void) const {
    return error_ ? 1 : 0;
}
