#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "engine/core/engine_types.h"
struct CFastStringInt;

struct SStringParam {
    SStringParam(void) = default;
    SStringParam(const char *text);
    SStringParam(const char *data, u32 length);
    explicit SStringParam(std::string_view text);

    std::string_view View(void) const;
    const char *Data(void) const;
    u32 Length(void) const;

private:
    std::string text_;
};

struct SStringParamInt {
    SStringParamInt(void) = default;
    SStringParamInt(const wchar_t *text);
    SStringParamInt(const CFastStringInt &text);
    SStringParamInt(const char16_t *data, u32 length, u32 sanitize = 0u);
    explicit SStringParamInt(std::u16string_view text);

    std::u16string_view View(void) const;
    const char16_t *Data(void) const;
    u32 Length(void) const;

private:
    std::u16string text_;
};

struct CFastString {
    CFastString();
    CFastString(const char *text);
    CFastString(const SStringParam &text);
    CFastString(const CFastString &other);
    CFastString(CFastString &&other) noexcept = default;
    CFastString &operator=(const CFastString &other) = default;
    CFastString &operator=(CFastString &&other) noexcept = default;
    ~CFastString();

    static const CFastString s_Null;

    u32 Length(void) const;
    const char *Data(void) const;
    char *MutableData(void);
    std::string_view View(void) const;
    const std::string &String(void) const;
    SStringParam Param(void) const;
    bool Empty(void) const;
    void Assign(std::string_view text);
    void Append(std::string_view suffix);
    void Prepend(std::string_view prefix);
    void SetString(unsigned long textLength, const char *text);
    void SetString(const SStringParam &source);
    void SetLength(unsigned long newLength);
    void Concat(const SStringParam &suffix);
    void Concat(char ch);
    void ConcatBefore(const SStringParam &prefix);
    int Compare(const SStringParam &other, unsigned long maxCount) const;
    int CompareNoCase(
            const SStringParam &other,
            unsigned long maxCount) const;

private:
    std::string value;
};

struct CFastStringInt {
    using CodeUnit = char16_t;
    using Storage = std::u16string;

    CFastStringInt(void);
    CFastStringInt(const SStringParam &source);
    CFastStringInt(const wchar_t *source);
    CFastStringInt(const CFastStringInt &source);
    CFastStringInt(CFastStringInt &&source) noexcept = default;
    ~CFastStringInt(void);
    CFastStringInt &operator=(const CFastStringInt &source) = default;
    CFastStringInt &operator=(CFastStringInt &&source) noexcept = default;
    u32 Length(void) const;
    const CodeUnit *Data(void) const;
    CodeUnit *MutableData(void);
    std::u16string_view View(void) const;
    const std::u16string &String(void) const;
    SStringParamInt Param(u32 sanitize = 0u) const;
    bool Empty(void) const;
    void Clear(void);
    void Assign(std::u16string_view text);
    void Assign(Storage text);
    void Append(std::u16string_view suffix);
    Storage Release(void);
    void SetLength(unsigned long newLength);
    void SetString(const SStringParamInt &source);
    void SetString(const SStringParam &source);
    void SetUtf8(const SStringParam &source);
    void GetAscii(CFastString &outAscii) const;
    void GetUtf8(CFastString &outUtf8, int withByteOrderMark) const;
    void GetUtf8OrAscii(CFastString &outText) const;
    void Concat(const SStringParamInt &suffix);
    void Concat(const SStringParam &suffix);
    void Concat(unsigned long ch);
    void ConcatBefore(const SStringParamInt &prefix);
    void TruncBeforeIndex(unsigned long index);
    void TruncAfterIndex(unsigned long newLength);
    int Compare(const SStringParamInt &other, unsigned long maxCount) const;
    int CompareNoCase(const SStringParamInt &other,
                      unsigned long maxCount) const;
    int Compare(const SStringParam &other, unsigned long maxCount) const;
    int CompareNoCase(const SStringParam &other,
                      unsigned long maxCount) const;

private:
    Storage value;
};

unsigned long ReadNextUtf8Char(const char *&text);
unsigned long ConcatUtf8Char(CFastString &text, unsigned long codePoint);
