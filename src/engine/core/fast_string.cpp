#include "engine/core/fast_string.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <utility>

namespace {

uint32_t CheckedStrCpyWide(
        const char16_t *source,
        char16_t *destination,
        uint32_t maxCount) {
    uint32_t written = 0u;
    uint32_t read = 0u;
    while (read < maxCount) {
        const char16_t ch = source[read++];
        if (ch == 0u) {
            break;
        }
        if ((ch >= 0x20u && ch != 0x7fu) ||
            ch == 0x09u ||
            ch == 0x0au ||
            ch == 0x0du) {
            destination[written++] = ch;
        }
    }
    return written;
}

uint32_t CheckedStrCpyAnsiToWide(
        const char *source,
        char16_t *destination,
        uint32_t maxCount) {
    uint32_t written = 0u;
    uint32_t read = 0u;
    while (read < maxCount) {
        const uint8_t ch = static_cast<uint8_t>(source[read++]);
        if (ch == 0u) {
            break;
        }
        if ((ch >= 0x20u && ch != 0x7fu) ||
            ch == 0x09u ||
            ch == 0x0au ||
            ch == 0x0du) {
            destination[written++] = ch;
        }
    }
    return written;
}

char16_t FoldAsciiCase(char16_t ch) {
    if (ch >= u'A' && ch <= u'Z') {
        return static_cast<char16_t>(ch + (u'a' - u'A'));
    }
    return ch;
}

int CompareWide(const char16_t *left, const char16_t *right) {
    for (;;) {
        const char16_t leftCh = *left++;
        const char16_t rightCh = *right++;
        if (leftCh != rightCh) {
            return leftCh < rightCh ? -1 : 1;
        }
        if (leftCh == 0u) {
            return 0;
        }
    }
}

int CompareWidePrefix(
        const char16_t *left,
        const char16_t *right,
        uint32_t count) {
    for (uint32_t index = 0u; index < count; index++) {
        const char16_t leftCh = left[index];
        const char16_t rightCh = right[index];
        if (leftCh != rightCh) {
            return leftCh < rightCh ? -1 : 1;
        }
        if (leftCh == 0u) {
            return 0;
        }
    }
    return 0;
}

int CompareWideNoCase(const char16_t *left, const char16_t *right) {
    for (;;) {
        const char16_t leftCh = FoldAsciiCase(*left++);
        const char16_t rightCh = FoldAsciiCase(*right++);
        if (leftCh != rightCh) {
            return leftCh < rightCh ? -1 : 1;
        }
        if (leftCh == 0u) {
            return 0;
        }
    }
}

int CompareWidePrefixNoCase(
        const char16_t *left,
        const char16_t *right,
        uint32_t count) {
    for (uint32_t index = 0u; index < count; index++) {
        const char16_t leftCh = FoldAsciiCase(left[index]);
        const char16_t rightCh = FoldAsciiCase(right[index]);
        if (leftCh != rightCh) {
            return leftCh < rightCh ? -1 : 1;
        }
        if (leftCh == 0u) {
            return 0;
        }
    }
    return 0;
}

int CompareAsciiNoCase(std::string_view left, std::string_view right) {
    const std::size_t commonLength = std::min(left.size(), right.size());
    for (std::size_t index = 0u; index < commonLength; ++index) {
        const unsigned char leftChar = static_cast<unsigned char>(left[index]);
        const unsigned char rightChar = static_cast<unsigned char>(right[index]);
        const int foldedLeft = std::tolower(leftChar);
        const int foldedRight = std::tolower(rightChar);
        if (foldedLeft != foldedRight) {
            return foldedLeft < foldedRight ? -1 : 1;
        }
    }
    if (left.size() == right.size()) {
        return 0;
    }
    return left.size() < right.size() ? -1 : 1;
}

}  // namespace

SStringParam::SStringParam(const char *text)
        : text_(text != nullptr ? text : "") {}

SStringParam::SStringParam(const char *data, u32 length)
        : text_(data != nullptr ? std::string(data, length) : std::string()) {}

SStringParam::SStringParam(std::string_view text)
        : text_(text) {}

std::string_view SStringParam::View(void) const {
    return text_;
}

const char *SStringParam::Data(void) const {
    return text_.c_str();
}

u32 SStringParam::Length(void) const {
    return static_cast<u32>(text_.size());
}

SStringParamInt::SStringParamInt(const wchar_t *text) {
    if (text == nullptr) {
        return;
    }
    while (*text != 0) {
        text_.push_back(static_cast<char16_t>(*text++));
    }
}

SStringParamInt::SStringParamInt(const CFastStringInt &text)
        : text_(text.String()) {}

SStringParamInt::SStringParamInt(
        const char16_t *data,
        u32 length,
        u32 sanitize) {
    if (data == nullptr || length == 0u) {
        return;
    }
    if (sanitize == 0u) {
        text_.assign(data, length);
        return;
    }
    text_.resize(length);
    const u32 copied = CheckedStrCpyWide(data, text_.data(), length);
    text_.resize(copied);
}

SStringParamInt::SStringParamInt(std::u16string_view text)
        : text_(text) {}

const char16_t *SStringParamInt::Data(void) const {
    return text_.c_str();
}

u32 SStringParamInt::Length(void) const {
    return static_cast<u32>(text_.size());
}

std::u16string_view SStringParamInt::View(void) const {
    return text_;
}

unsigned long ReadNextUtf8Char(const char *&text) {
    for (;;) {
        const uint8_t lead = static_cast<uint8_t>(*text);
        if (lead == 0u) {
            ++text;
            return 0u;
        }
        if (lead < 0x80u) {
            ++text;
            return lead;
        }

        unsigned int length = 0u;
        uint8_t leadMask = 0u;
        uint8_t minimumSecond = 0x80u;
        if (lead >= 0xc2u && lead <= 0xdfu) {
            length = 2u;
            leadMask = 0x1fu;
        } else if (lead >= 0xe0u && lead <= 0xefu) {
            length = 3u;
            leadMask = 0x0fu;
            minimumSecond = lead == 0xe0u ? 0xa0u : 0x80u;
        } else if (lead >= 0xf0u && lead <= 0xf7u) {
            length = 4u;
            leadMask = 0x07u;
            minimumSecond = lead == 0xf0u ? 0x90u : 0x80u;
        } else if (lead >= 0xf8u && lead <= 0xfbu) {
            length = 5u;
            leadMask = 0x03u;
            minimumSecond = lead == 0xf8u ? 0x88u : 0x80u;
        } else if (lead >= 0xfcu && lead <= 0xfdu) {
            length = 6u;
            leadMask = 0x01u;
            minimumSecond = lead == 0xfcu ? 0x84u : 0x80u;
        }

        bool valid = length != 0u;
        if (valid) {
            const uint8_t second = static_cast<uint8_t>(text[1]);
            valid = second >= minimumSecond && second <= 0xbfu;
        }
        for (unsigned int index = 2u; valid && index < length; ++index) {
            const uint8_t continuation = static_cast<uint8_t>(text[index]);
            valid = continuation >= 0x80u && continuation <= 0xbfu;
        }
        if (!valid) {
            if (length == 2u) {
                const uint8_t second = static_cast<uint8_t>(text[1]);
                const bool retrySecond =
                        second == 0x09u || second == 0x0au ||
                        (second >= 0x20u && second <= 0x2fu) ||
                        (second >= 0x3au && second <= 0x3fu) ||
                        (second >= 0x7bu && second <= 0x7du);
                if (second != 0u && !retrySecond) {
                    text += 2;
                    continue;
                }
            }
            ++text;
            continue;
        }

        unsigned long codePoint = lead & leadMask;
        for (unsigned int index = 1u; index < length; ++index) {
            codePoint = (codePoint << 6u) |
                    (static_cast<uint8_t>(text[index]) & 0x3fu);
        }
        text += length;
        if (codePoint != 0xfeffu) {
            return codePoint;
        }
    }
}

unsigned long ConcatUtf8Char(CFastString &text, unsigned long codePoint) {
    if (codePoint == 0u) {
        return 0u;
    }
    if (codePoint > 0x7ffffffful) {
        codePoint = 0xfffdu;
    }

    unsigned int length;
    if (codePoint < 0x80u) {
        length = 1u;
    } else if (codePoint < 0x800u) {
        length = 2u;
    } else if (codePoint < 0x10000u) {
        length = 3u;
    } else if (codePoint < 0x200000u) {
        length = 4u;
    } else if (codePoint < 0x4000000u) {
        length = 5u;
    } else {
        length = 6u;
    }

    char encoded[6]{};
    unsigned long remaining = codePoint;
    for (unsigned int index = length - 1u; index != 0u; --index) {
        encoded[index] = static_cast<char>(0x80u | (remaining & 0x3fu));
        remaining >>= 6u;
    }
    static constexpr uint8_t PrefixByLength[] = {
        0u, 0u, 0xc0u, 0xe0u, 0xf0u, 0xf8u, 0xfcu,
    };
    encoded[0] = static_cast<char>(PrefixByLength[length] | remaining);
    text.Append(std::string_view(encoded, length));
    return length;
}

const CFastString CFastString::s_Null;

CFastString::CFastString(void) = default;

CFastString::CFastString(const char *text)
        : value(text != nullptr ? text : "") {}

CFastString::CFastString(const SStringParam &text)
        : value(text.View()) {}

CFastString::CFastString(const CFastString &source) = default;

CFastString::~CFastString(void) = default;

uint32_t CFastString::Length(void) const {
    return static_cast<uint32_t>(value.size());
}

const char *CFastString::Data(void) const {
    return value.c_str();
}

char *CFastString::MutableData(void) {
    return value.data();
}

std::string_view CFastString::View(void) const {
    return value;
}

const std::string &CFastString::String(void) const {
    return value;
}

SStringParam CFastString::Param(void) const {
    return {Data(), Length()};
}

bool CFastString::Empty(void) const {
    return value.empty();
}

void CFastString::Assign(std::string_view text) {
    value = std::string(text);
}

void CFastString::Append(std::string_view suffix) {
    if (!suffix.empty()) {
        value.append(std::string(suffix));
    }
}

void CFastString::Prepend(std::string_view prefix) {
    if (!prefix.empty()) {
        value.insert(0u, std::string(prefix));
    }
}

void CFastString::SetString(unsigned long textLength, const char *text) {
    if (textLength == 0u || text == nullptr) {
        Assign({});
        return;
    }
    value.assign(text, textLength);
}

void CFastString::SetString(const SStringParam &source) {
    Assign(source.View());
}

void CFastString::SetLength(unsigned long newLength) {
    value.resize(newLength);
}

void CFastString::Concat(const SStringParam &suffix) {
    Append(suffix.View());
}

void CFastString::Concat(char ch) {
    value.push_back(ch);
}

void CFastString::ConcatBefore(const SStringParam &prefix) {
    Prepend(prefix.View());
}

int CFastString::Compare(
        const SStringParam &other,
        unsigned long maxCount) const {
    if (maxCount == 0u) {
        return View().compare(other.View());
    }
    if (maxCount > Length() || maxCount > other.Length()) {
        return -1;
    }
    return std::strncmp(Data(), other.Data(), maxCount);
}

int CFastString::CompareNoCase(
        const SStringParam &other,
        unsigned long maxCount) const {
    if (maxCount == 0u) {
        return CompareAsciiNoCase(View(), other.View());
    }
    if (maxCount > Length() || maxCount > other.Length()) {
        return -1;
    }
    return CompareAsciiNoCase(
            View().substr(0u, maxCount),
            other.View().substr(0u, maxCount));
}

CFastStringInt::CFastStringInt(void) = default;

CFastStringInt::CFastStringInt(const SStringParam &source) {
    SetString(source);
}

CFastStringInt::CFastStringInt(const wchar_t *source) {
    if (source == nullptr) {
        return;
    }
    while (*source != 0) {
        Concat(static_cast<unsigned long>(*source++));
    }
}

CFastStringInt::CFastStringInt(const CFastStringInt &source) = default;

CFastStringInt::~CFastStringInt(void) = default;

uint32_t CFastStringInt::Length(void) const {
    return static_cast<uint32_t>(value.size());
}

const CFastStringInt::CodeUnit *CFastStringInt::Data(void) const {
    return value.c_str();
}

CFastStringInt::CodeUnit *CFastStringInt::MutableData(void) {
    return value.data();
}

std::u16string_view CFastStringInt::View(void) const {
    return value;
}

const std::u16string &CFastStringInt::String(void) const {
    return value;
}

SStringParamInt CFastStringInt::Param(u32 sanitize) const {
    return {Data(), Length(), sanitize};
}

bool CFastStringInt::Empty(void) const {
    return value.empty();
}

void CFastStringInt::Clear(void) {
    value.clear();
}

void CFastStringInt::Assign(std::u16string_view text) {
    value = Storage(text);
}

void CFastStringInt::Assign(Storage text) {
    value = std::move(text);
}

void CFastStringInt::Append(std::u16string_view suffix) {
    if (!suffix.empty()) {
        value.append(Storage(suffix));
    }
}

CFastStringInt::Storage CFastStringInt::Release(void) {
    Storage released = std::move(value);
    value.clear();
    return released;
}

void CFastStringInt::SetLength(unsigned long newLength) {
    value.resize(newLength);
}

void CFastStringInt::SetString(const SStringParamInt &source) {
    if (source.Length() == 0u) {
        Clear();
        return;
    }
    value.assign(source.View());
}

void CFastStringInt::SetString(const SStringParam &source) {
    const char *sourceText = source.Data();
    uint32_t declaredLength = source.Length();
    if (sourceText != nullptr && declaredLength >= 3u &&
        static_cast<uint8_t>(sourceText[0]) == 0xefu &&
        static_cast<uint8_t>(sourceText[1]) == 0xbbu &&
        static_cast<uint8_t>(sourceText[2]) == 0xbfu) {
        sourceText += 3;
        declaredLength -= 3u;
        SetUtf8({sourceText, declaredLength});
        return;
    }
    value.resize(declaredLength);
    if (declaredLength != 0u) {
        const uint32_t copied = CheckedStrCpyAnsiToWide(
                sourceText,
                MutableData(),
                declaredLength);
        value.resize(copied);
    }
}

void CFastStringInt::SetUtf8(const SStringParam &source) {
    Clear();
    if (source.Length() == 0u) {
        return;
    }

    std::string bounded(source.View());
    bounded.append(6u, '\0');
    const char *cursor = bounded.data();
    const char *end = cursor + source.Length();
    while (cursor < end) {
        const unsigned long codePoint = ReadNextUtf8Char(cursor);
        if (codePoint == 0u) {
            break;
        }
        if (codePoint == 0xfeffu || codePoint == 0x7fu) {
            continue;
        }
        if (codePoint < 0x20u &&
            codePoint != 0x09u &&
            codePoint != 0x0au &&
            codePoint != 0x0du) {
            continue;
        }
        Concat(codePoint);
    }
}

void CFastStringInt::GetAscii(CFastString &outAscii) const {
    std::string ascii;
    ascii.reserve(value.size());
    for (CodeUnit ch : value) {
        if (ch == 0u) {
            break;
        }
        ascii.push_back(ch <= 0x7fu ? static_cast<char>(ch) : '_');
    }
    outAscii.Assign(ascii);
}

void CFastStringInt::GetUtf8(
        CFastString &outUtf8,
        int withByteOrderMark) const {
    outUtf8.Assign({});
    if (Empty()) {
        return;
    }
    if (withByteOrderMark != 0) {
        ConcatUtf8Char(outUtf8, 0xfeffu);
    }
    for (CodeUnit codeUnit : value) {
        if (codeUnit == 0u) {
            break;
        }
        if (codeUnit != 0xfeffu) {
            ConcatUtf8Char(outUtf8, codeUnit);
        }
    }
}

void CFastStringInt::GetUtf8OrAscii(CFastString &outText) const {
    outText.Assign({});
    bool needsUtf8Marker = false;
    for (CodeUnit codeUnit : value) {
        if (codeUnit == 0u) {
            break;
        }
        if (codeUnit == 0xfeffu) {
            continue;
        }
        needsUtf8Marker |= codeUnit > 0x7fu;
        ConcatUtf8Char(outText, codeUnit);
    }
    if (needsUtf8Marker) {
        outText.Prepend("\xef\xbb\xbf");
    }
}

void CFastStringInt::Concat(const SStringParamInt &suffix) {
    const uint32_t declaredSuffixLength = suffix.Length();
    if (declaredSuffixLength == 0u) {
        return;
    }
    const Storage sourceCopy(suffix.View());
    Storage filtered(declaredSuffixLength, static_cast<CodeUnit>(0u));
    const uint32_t copied = CheckedStrCpyWide(
            sourceCopy.data(),
            filtered.data(),
            declaredSuffixLength);
    filtered.resize(copied);
    value.append(filtered);
}

void CFastStringInt::Concat(const SStringParam &suffix) {
    CFastStringInt converted(suffix);
    Concat(converted.Param());
}

void CFastStringInt::Concat(unsigned long ch) {
    if (ch < 0x20u && ch != 0x09u && ch != 0x0au && ch != 0x0du) {
        return;
    }
    if (ch == 0x7fu) {
        return;
    }
    value.push_back(static_cast<CodeUnit>(ch));
}

void CFastStringInt::ConcatBefore(const SStringParamInt &prefix) {
    const uint32_t prefixLength = prefix.Length();
    if (prefixLength == 0u) {
        return;
    }
    const Storage sourceCopy(prefix.View());
    Storage filtered(prefixLength, static_cast<CodeUnit>(0u));
    const uint32_t copied = CheckedStrCpyWide(
            sourceCopy.data(),
            filtered.data(),
            prefixLength);
    filtered.resize(copied);
    value.insert(0u, filtered);
}

void CFastStringInt::TruncBeforeIndex(unsigned long index) {
    if (index + 1u < value.size()) {
        value.erase(0u, index + 1u);
        return;
    }
    Clear();
}

void CFastStringInt::TruncAfterIndex(unsigned long newLength) {
    if (newLength < value.size()) {
        value.resize(newLength);
    }
}

int CFastStringInt::Compare(
        const SStringParamInt &other,
        unsigned long maxCount) const {
    if (maxCount == 0u) {
        return CompareWide(Data(), other.Data());
    }
    if (maxCount > Length() || maxCount > other.Length()) {
        return -1;
    }
    return CompareWidePrefix(Data(), other.Data(), maxCount);
}

int CFastStringInt::CompareNoCase(
        const SStringParamInt &other,
        unsigned long maxCount) const {
    if (maxCount == 0u) {
        return CompareWideNoCase(Data(), other.Data());
    }
    if (maxCount > Length() || maxCount > other.Length()) {
        return -1;
    }
    return CompareWidePrefixNoCase(Data(), other.Data(), maxCount);
}

int CFastStringInt::Compare(
        const SStringParam &other,
        unsigned long maxCount) const {
    CFastStringInt wideScratch;
    wideScratch.SetString(other);
    const SStringParamInt wideScratchParam = {
        wideScratch.Data(),
        wideScratch.Length(),
        0u,
    };
    return Compare(wideScratchParam, maxCount);
}

int CFastStringInt::CompareNoCase(
        const SStringParam &other,
        unsigned long maxCount) const {
    CFastStringInt wideScratch;
    wideScratch.SetString(other);
    const SStringParamInt wideScratchParam = {
        wideScratch.Data(),
        wideScratch.Length(),
        0u,
    };
    return CompareNoCase(wideScratchParam, maxCount);
}
