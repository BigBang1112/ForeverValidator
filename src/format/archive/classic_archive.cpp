#include "format/archive/classic_archive.h"
#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <limits>

#include "format/archive/classic_archive_wire.h"
#include "engine/core/classic_buffer.h"
#include "engine/core/fast_string.h"
namespace {

namespace ArchiveWire = TmnfFormat::ClassicArchiveWire;

static_assert(std::numeric_limits<unsigned char>::digits == 8,
              "CClassicArchive requires eight-bit bytes");
static_assert(std::numeric_limits<unsigned long>::digits >= 32,
              "CClassicArchive naturals require at least 32 bits");

}  // namespace

bool CClassicArchive::IsWriting(void) const {
    return direction == Direction::Writing;
}

void CClassicArchive::SetWriting(bool writingArchive) {
    direction = writingArchive ? Direction::Writing : Direction::Reading;
}

bool CClassicArchive::IsTextMode(void) const {
    return representation == Representation::Text;
}

void CClassicArchive::SetTextMode(bool textArchive) {
    representation = textArchive
            ? Representation::Text
            : Representation::Binary;
}

bool CClassicArchive::IsMemoryArchive(void) const {
    return destination == Destination::Memory;
}

void CClassicArchive::SetMemoryArchive(bool isMemoryArchive) {
    destination = isMemoryArchive
            ? Destination::Memory
            : Destination::External;
}

CClassicBuffer *CClassicArchive::ArchiveBuffer(void) {
    return activeBuffer;
}

const CClassicBuffer *CClassicArchive::ArchiveBuffer(void) const {
    return activeBuffer;
}

void CClassicArchive::SetArchiveBuffer(CClassicBuffer *buffer) {
    activeBuffer = buffer;
}

void CClassicArchive::ClearArchiveBuffer(void) {
    activeBuffer = nullptr;
}

int CClassicArchive::ReadArchiveByte(char *out) {
    return activeBuffer->ReadAll(out, 1ul);
}

int CClassicArchive::WriteArchiveBytes(
        const void *data,
        unsigned long byteCount) {
    return activeBuffer->WriteAll(data, byteCount);
}

int CClassicArchive::ReadNaturalText(
        unsigned long *values,
        unsigned long count,
        int asMask) {
    for (unsigned long index = 0ul; index < count; ++index) {
        ReadLine();
        std::uint32_t value = 0u;
        (void)std::sscanf(
                textLine.c_str(),
                asMask ? "%" SCNx32 : "%" SCNu32,
                &value);
        values[index] = static_cast<unsigned long>(value);
    }
    return 1;
}

void CClassicArchive::WriteNaturalText(
        const unsigned long *values,
        unsigned long count,
        int asMask) {
    char line[4096];
    for (unsigned long index = 0ul; index < count; ++index) {
        if (values[index] > std::numeric_limits<std::uint32_t>::max()) {
            Fail();
            return;
        }
        const std::uint32_t value = static_cast<std::uint32_t>(values[index]);
        const int length = asMask
                ? std::snprintf(line, sizeof(line), "%" PRIx32, value)
                : std::snprintf(line, sizeof(line), "%" PRIu32, value);
        if (length < 0 ||
            static_cast<std::size_t>(length) >= sizeof(line)) {
            Fail();
            return;
        }
        textLine.assign(line, static_cast<std::size_t>(length));
        WriteLine();
    }
}

CClassicArchive::CClassicArchive(void) = default;

CClassicArchive::~CClassicArchive(void) = default;

void CClassicArchive::WriteLine(void) {
    textLine.append("\r\n");
    if (WriteArchiveBytes(
                textLine.data(),
                static_cast<unsigned long>(textLine.size())) == 0) {
        Fail();
    }
}

int CClassicArchive::ReadLine(void) {
    char c = 0;
    textLine.clear();
    for (;;) {
        if (!ReadArchiveByte(&c)) {
            return 0;
        }
        if (c == '|') {
            if (!ReadArchiveByte(&c)) {
                return 0;
            }
            if (c == '\r') {
                if (!ReadArchiveByte(&c)) {
                    return 0;
                }
                return 1;
            }
        }

        if (c == '\n' && !textLine.empty() && textLine.back() == '\r') {
            textLine.pop_back();
            return 0;
        }
        if (textLine.size() >= 0xfffu) {
            return 0;
        }
        textLine.push_back(c);
    }
}

void CClassicArchive::ReadNatural(
        unsigned long *values,
        unsigned long count,
        int asMask) {
    if (values == nullptr && count != 0ul) {
        Fail();
        return;
    }
    if (IsTextMode()) {
        (void)ReadNaturalText(values, count, asMask);
        return;
    }
    if (activeBuffer == nullptr) {
        Fail();
        return;
    }
    for (unsigned long index = 0ul; index < count; ++index) {
        ArchiveWire::NaturalBytes bytes{};
        if (activeBuffer->ReadAll(
                    bytes.data(), ArchiveWire::NaturalByteCount) == 0) {
            Fail();
            return;
        }
        values[index] = static_cast<unsigned long>(
                ArchiveWire::DecodeNatural(bytes.data()));
    }
}

void CClassicArchive::WriteData(
        const void *data,
        unsigned long byteCount) {
    if (byteCount != 0ul) {
        (void)WriteArchiveBytes(data, byteCount);
    }
}

void CClassicArchive::WriteNatural(
        const unsigned long *values,
        unsigned long count,
        int asMask) {
    if (values == nullptr && count != 0ul) {
        Fail();
        return;
    }
    if (IsTextMode()) {
        WriteNaturalText(values, count, asMask);
        return;
    }
    if (activeBuffer == nullptr) {
        Fail();
        return;
    }
    for (unsigned long index = 0ul; index < count; ++index) {
        if (values[index] > std::numeric_limits<std::uint32_t>::max()) {
            Fail();
            return;
        }
        const ArchiveWire::NaturalBytes bytes = ArchiveWire::EncodeNatural(
                static_cast<std::uint32_t>(values[index]));
        if (WriteArchiveBytes(
                    bytes.data(), ArchiveWire::NaturalByteCount) == 0) {
            Fail();
            return;
        }
    }
}

void CClassicArchive::WriteMask(
        const unsigned long *values,
        unsigned long count) {
    WriteNatural(values, count, 1);
}

void CClassicArchive::DoData(void *data, unsigned long byteCount) {
    if (!Good() || (data == nullptr && byteCount != 0ul) ||
        activeBuffer == nullptr) {
        Fail();
        return;
    }
    if (byteCount == 0ul) {
        return;
    }
    const int ok = IsWriting()
            ? activeBuffer->WriteAll(data, byteCount)
            : activeBuffer->ReadAll(data, byteCount);
    if (ok == 0) {
        Fail();
    }
}

void CClassicArchive::DoNatural(
        unsigned long *values,
        unsigned long count,
        int asMask) {
    if (!Good() || (values == nullptr && count != 0ul)) {
        Fail();
        return;
    }
    for (unsigned long index = 0ul; index < count && Good(); ++index) {
        if (IsWriting() &&
            values[index] > std::numeric_limits<std::uint32_t>::max()) {
            Fail();
            return;
        }
        unsigned long value = values[index];
        if (IsTextMode()) {
            if (IsWriting()) {
                WriteNatural(&value, 1ul, asMask);
            } else {
                ReadNatural(&value, 1ul, asMask);
            }
        } else if (IsWriting()) {
            ArchiveWire::NaturalBytes bytes = ArchiveWire::EncodeNatural(
                    static_cast<std::uint32_t>(value));
            DoData(bytes.data(), ArchiveWire::NaturalByteCount);
        } else {
            ArchiveWire::NaturalBytes bytes{};
            DoData(bytes.data(), ArchiveWire::NaturalByteCount);
            value = static_cast<unsigned long>(
                    ArchiveWire::DecodeNatural(bytes.data()));
        }
        if (!IsWriting()) {
            values[index] = value;
        }
    }
}

void CClassicArchive::DoString(
        CFastString *values,
        unsigned long count) {
    if (!Good() || (values == nullptr && count != 0ul) ||
        activeBuffer == nullptr) {
        Fail();
        return;
    }
    for (unsigned long index = 0ul; index < count && Good(); ++index) {
        CFastString &value = values[index];
        if (!IsTextMode()) {
            unsigned long length = value.Length();
            DoNatural(&length, 1ul, 0);
            if (!Good() ||
                length > ArchiveWire::MaximumStringByteCount) {
                Fail();
                return;
            }
            if (IsWriting()) {
                DoData(value.MutableData(), length);
            } else {
                value.SetLength(static_cast<std::uint32_t>(length));
                DoData(value.MutableData(), length);
            }
            continue;
        }

        if (IsWriting()) {
            const char *text = value.Data();
            for (unsigned long offset = 0ul; offset < value.Length(); ++offset) {
                const bool escapedNewline = text[offset] == '\r' &&
                        offset + 1ul < value.Length() &&
                        text[offset + 1ul] == '\n';
                if (text[offset] == '|' || escapedNewline) {
                    char escape = '|';
                    DoData(&escape, 1ul);
                }
                char ch = text[offset];
                DoData(&ch, 1ul);
            }
            char newline[2] = {'\r', '\n'};
            DoData(newline, 2ul);
            continue;
        }

        std::string decoded;
        for (;;) {
            const int continued = ReadLine();
            decoded.append(textLine);
            if (continued == 0) {
                break;
            }
            decoded.append("\r\n");
        }
        value.SetString(
                static_cast<std::uint32_t>(decoded.size()),
                decoded.data());
    }
}

void CClassicArchive::DoNat16(
        unsigned short *values,
        unsigned long count,
        int asMask) {
    if (!Good() || (values == nullptr && count != 0ul)) {
        Fail();
        return;
    }
    for (unsigned long index = 0ul; index < count && Good(); ++index) {
        unsigned long value = values[index];
        if (IsTextMode()) {
            DoNatural(&value, 1ul, asMask);
        } else if (IsWriting()) {
            ArchiveWire::SmallNaturalBytes bytes =
                    ArchiveWire::EncodeSmallNatural(
                            static_cast<std::uint16_t>(value));
            DoData(bytes.data(), bytes.size());
        } else {
            ArchiveWire::SmallNaturalBytes bytes{};
            DoData(bytes.data(), bytes.size());
            value = static_cast<unsigned long>(
                    ArchiveWire::DecodeSmallNatural(bytes.data()));
        }
        if (!IsWriting() && Good()) {
            if (value > std::numeric_limits<unsigned short>::max()) {
                Fail();
                return;
            }
            values[index] = static_cast<unsigned short>(value);
        }
    }
}

void CClassicArchive::DoNat8(
        unsigned char *values,
        unsigned long count,
        int asMask) {
    if (!Good() || (values == nullptr && count != 0ul)) {
        Fail();
        return;
    }
    if (!IsTextMode()) {
        DoData(values, count);
        return;
    }
    for (unsigned long index = 0ul; index < count && Good(); ++index) {
        unsigned long value = values[index];
        DoNatural(&value, 1ul, asMask);
        if (!IsWriting() && Good()) {
            if (value > std::numeric_limits<unsigned char>::max()) {
                Fail();
                return;
            }
            values[index] = static_cast<unsigned char>(value);
        }
    }
}

void CClassicArchive::DoNodPtr(CMwNod *&nod) {
    (void)nod;
}

bool CClassicArchive::Good(void) const {
    return !archiveFailed;
}

void CClassicArchive::Fail(void) {
    archiveFailed = true;
}

void CClassicArchive::SkipData(unsigned long byteCount) {
    if (!Good() || activeBuffer == nullptr) {
        Fail();
        return;
    }
    if (!IsWriting()) {
        (void)activeBuffer->Skip(byteCount);
        return;
    }
    unsigned char zeros[4096]{};
    while (byteCount != 0ul && Good()) {
        const unsigned long batch = std::min<unsigned long>(
                byteCount,
                sizeof(zeros));
        DoData(zeros, batch);
        byteCount -= batch;
    }
}
