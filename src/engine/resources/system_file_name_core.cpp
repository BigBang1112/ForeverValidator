#include "engine/resources/system_file_name.h"
#include <algorithm>
#include <cstring>
#include <string_view>

namespace {

using CodeUnit = CFastStringInt::CodeUnit;
using String = CFastStringInt::Storage;
using StringView = std::u16string_view;

CodeUnit ToLowerAscii(CodeUnit value) {
    return value >= u'A' && value <= u'Z'
            ? static_cast<CodeUnit>(value + (u'a' - u'A'))
            : value;
}

void SetPathPart(CFastStringInt &outPart, StringView part) {
    outPart.SetString({
        part.data(), static_cast<u32>(part.size()), 1u,
    });
}

bool SegmentEquals(const CFastStringInt &segment, StringView expected) {
    return segment.View() == expected;
}

std::size_t FirstEmbeddedNull(StringView text) {
    const std::size_t terminator = text.find(u'\0');
    return terminator == StringView::npos ? text.size() : terminator;
}

String NormalizedSeparators(StringView source) {
    String normalized(source);
    std::replace(normalized.begin(), normalized.end(), u'/', u'\\');
    return normalized;
}

}  // namespace

int CSystemFileName::IsExtension(
        const CFastStringInt &filename,
        const char *extension) {
    if (extension == nullptr) {
        return 0;
    }
    const StringView name = filename.View();
    const std::size_t extensionLength = std::strlen(extension);
    if (name.size() <= extensionLength) {
        return 0;
    }

    const std::size_t suffixOffset = name.size() - extensionLength;
    for (std::size_t index = 0u; index < extensionLength; ++index) {
        const CodeUnit nameCharacter = name[suffixOffset + index];
        const unsigned char extensionCharacter =
                static_cast<unsigned char>(extension[index]);
        if (nameCharacter > 0x7fu ||
            ToLowerAscii(nameCharacter) !=
                    ToLowerAscii(static_cast<CodeUnit>(extensionCharacter))) {
            return 0;
        }
    }
    return 1;
}

int CSystemFileName::IsExtensionGbx(const CFastStringInt &filename) {
    return IsExtension(filename, ".gbx");
}

int CSystemFileName::IsNormalized(const CFastStringInt &filename) {
    return filename.View().find(u'/') == StringView::npos;
}

int CSystemFileName::IsValidShortFileName(
        const CFastStringInt &filename) {
    for (CodeUnit character : filename.View()) {
        if (character == u'\0') {
            return 1;
        }
        switch (character) {
            case u'"':
            case u'*':
            case u'/':
            case u':':
            case u'<':
            case u'>':
            case u'?':
            case u'\\':
            case u'|':
                return 0;
            default:
                break;
        }
    }
    return 1;
}

int CSystemFileName::SplitFirstDirectory(
        const CFastStringInt &filename,
        CFastStringInt &outDirectory,
        unsigned long &inOutOffset) {
    const StringView path = filename.View();
    if (inOutOffset >= path.size()) {
        outDirectory.Clear();
        return 0;
    }

    const std::size_t start = inOutOffset;
    std::size_t search = start;
    if (start == 0u && path.size() >= 2u &&
        path[0] == u'\\' && path[1] == u'\\') {
        search = 2u;
    }

    const std::size_t slash = path.find(u'\\', search);
    if (slash == StringView::npos) {
        SetPathPart(outDirectory, path.substr(start));
        return 0;
    }

    SetPathPart(outDirectory, path.substr(start, slash - start + 1u));
    std::size_t next = slash + 1u;
    while (next < path.size() && path[next] == u'\\') {
        ++next;
    }
    inOutOffset = static_cast<unsigned long>(next);
    return 1;
}

void CSystemFileName::SplitFirstDirectory(
        CFastStringInt &filename,
        CFastStringInt &outDirectory) {
    const StringView path = filename.View();
    std::size_t search = 0u;
    if (path.size() >= 2u && path[0] == u'\\' && path[1] == u'\\') {
        search = 2u;
    }

    const std::size_t slash = path.find(u'\\', search);
    if (slash == StringView::npos) {
        outDirectory.Clear();
        return;
    }

    std::size_t afterRepeated = slash + 1u;
    while (afterRepeated < path.size() && path[afterRepeated] == u'\\') {
        ++afterRepeated;
    }
    SetPathPart(outDirectory, path.substr(0u, afterRepeated));
    filename.Assign(path.substr(slash + 1u));
}

void CSystemFileName::ConcatDirectory(
        CFastStringInt &filename,
        const CFastStringInt &directory) {
    filename.Append(directory.View());
    if (directory.Empty() || directory.View().back() != u'\\') {
        filename.Concat('\\');
    }
}

void CSystemFileName::StripTrailingSlash(CFastStringInt &filename) {
    StringView path = filename.View();
    std::size_t length = path.size();
    while (length != 0u &&
           (path[length - 1u] == u'\\' || path[length - 1u] == u'/')) {
        --length;
    }
    filename.Assign(path.substr(0u, length));
}

int CSystemFileName::IsDirectoryName(const CFastStringInt &filename) {
    const StringView path = filename.View();
    return path.empty() ||
           (path.size() > 1u && path.back() == u'\\' &&
            IsNormalized(filename) != 0);
}

void CSystemFileName::SplitPath(
        const CFastStringInt &filename,
        CFastStringInt *outDrive,
        CFastStringInt *outDirectory,
        CFastStringInt *outBaseName,
        CFastStringInt *outExtensionLast,
        CFastStringInt *outExtensionComplete) {
    const String normalized = NormalizedSeparators(filename.View());
    const StringView path(normalized);

    std::size_t scan = 0u;
    const std::size_t colon = path.find(u':');
    if (colon != StringView::npos) {
        if (outDrive != nullptr) {
            SetPathPart(*outDrive, path.substr(0u, colon + 1u));
        }
        scan = colon + 1u;
    } else if (outDrive != nullptr) {
        outDrive->Clear();
    }

    std::size_t separator = path.rfind(u'\\');
    if (separator == StringView::npos || separator < scan) {
        separator = path.find(u':', scan);
    }

    std::size_t base = scan;
    if (separator != StringView::npos) {
        base = separator + 1u;
        if (outDirectory != nullptr) {
            SetPathPart(*outDirectory, path.substr(scan, base - scan));
        }
    } else if (outDirectory != nullptr) {
        outDirectory->Clear();
    }

    const std::size_t firstDot = path.find(u'.', base);
    const std::size_t lastDot = path.rfind(u'.');
    if (firstDot != StringView::npos) {
        if (outBaseName != nullptr) {
            SetPathPart(*outBaseName, path.substr(base, firstDot - base));
        }
        if (outExtensionComplete != nullptr) {
            SetPathPart(*outExtensionComplete, path.substr(firstDot));
        }
        if (outExtensionLast != nullptr) {
            SetPathPart(*outExtensionLast, path.substr(lastDot));
        }
        return;
    }

    if (outBaseName != nullptr) {
        SetPathPart(*outBaseName, path.substr(base));
    }
    if (outExtensionLast != nullptr) {
        outExtensionLast->Clear();
    }
    if (outExtensionComplete != nullptr) {
        outExtensionComplete->Clear();
    }
}

void CSystemFileName::ExtractExtLast(
        const CFastStringInt &filename,
        CFastStringInt &outExtension) {
    const StringView path = filename.View();
    const std::size_t lastDot = path.rfind(u'.');
    if (lastDot == StringView::npos ||
        path.find(u'\\', lastDot) != StringView::npos) {
        outExtension.Clear();
        return;
    }
    SetPathPart(outExtension, path.substr(lastDot));
}

void CSystemFileName::StripLastExtension(
        const CFastStringInt &filename,
        CFastStringInt &outName) {
    outName = filename;
    const StringView path = filename.View();
    const std::size_t lastDot = path.rfind(u'.');
    if (lastDot != StringView::npos &&
        path.find(u'\\', lastDot) == StringView::npos) {
        outName.Assign(path.substr(0u, lastDot));
    }
}

void CSystemFileName::FixFileName(
        CFastStringInt &filename,
        unsigned long flags) {
    const bool forceTrailingSlash = (flags & 0x01u) != 0u;
    const bool collapseRepeatedSlashes = (flags & 0x02u) != 0u;
    const bool allowDrivePrefix = (flags & 0x04u) != 0u;
    const bool preserveLeadingDotDot = (flags & 0x08u) != 0u;
    const bool allowQuestionMark = (flags & 0x10u) != 0u;
    const bool keepUncRepeatedSlashes = (flags & 0x20u) != 0u;

    const StringView source = filename.View();
    if (source.empty()) {
        return;
    }

    String filtered;
    filtered.reserve(source.size());
    unsigned long prefixCharsRemaining = 2ul;
    bool atComponentStart = true;
    bool hasDotSegments = false;
    const std::size_t sourceLength = FirstEmbeddedNull(source);

    for (std::size_t index = 0u; index < sourceLength; ++index) {
        CodeUnit current = source[index] == u'/' ? u'\\' : source[index];
        const CodeUnit next = index + 1u < sourceLength
                ? (source[index + 1u] == u'/' ? u'\\' : source[index + 1u])
                : u'\0';
        bool skipCharacter = false;

        if (prefixCharsRemaining != 0u &&
            current != u'\\' && current != u':' && next != u':') {
            prefixCharsRemaining = 0u;
        }

        if (current < 0x20u || current == 0x7fu) {
            skipCharacter = true;
        } else {
            switch (current) {
                case u'"':
                case u'*':
                case u'<':
                case u'>':
                case u'|':
                    current = u'_';
                    break;
                case u'.':
                    if (atComponentStart && (next == u'.' || next == u'\\')) {
                        hasDotSegments = true;
                    }
                    break;
                case u':':
                    if (prefixCharsRemaining == 0u || !allowDrivePrefix) {
                        current = u'_';
                    }
                    break;
                case u'?':
                    if (!allowQuestionMark) {
                        current = u'_';
                    }
                    break;
                case u'\\':
                    if (prefixCharsRemaining != 0u && !allowDrivePrefix) {
                        skipCharacter = true;
                    } else if (collapseRepeatedSlashes ||
                               (forceTrailingSlash && next == u'\0')) {
                        if (next == u'\\' &&
                            (prefixCharsRemaining == 0u ||
                             !keepUncRepeatedSlashes)) {
                            skipCharacter = true;
                        }
                    } else {
                        current = u'_';
                    }
                    break;
                default:
                    break;
            }
        }

        if (current == u'\\' && next != u'\\') {
            atComponentStart = true;
        } else if (!skipCharacter) {
            atComponentStart = false;
        }

        if (prefixCharsRemaining != 0u) {
            --prefixCharsRemaining;
        }
        if (!skipCharacter) {
            filtered.push_back(current);
        }
    }

    if (filtered.size() >= 0x103u) {
        filtered.resize(258u);
    }
    filename.Assign(std::move(filtered));
    if (filename.Empty()) {
        return;
    }

    if (forceTrailingSlash && filename.View().back() != u'\\') {
        filename.Concat('\\');
    }
    if (!hasDotSegments) {
        return;
    }

    CFastStringInt remainingPath(filename);
    filename.Clear();
    CFastStringInt segment;
    SplitFirstDirectory(remainingPath, segment);
    int depth = 0;
    while (!segment.Empty()) {
        if (!SegmentEquals(segment, u".\\")) {
            if (SegmentEquals(segment, u"..\\")) {
                --depth;
                if (depth >= 0) {
                    SplitLastDirectory(filename, segment);
                } else if (preserveLeadingDotDot) {
                    ConcatDirectory(filename, segment);
                }
            } else {
                ++depth;
                ConcatDirectory(filename, segment);
            }
        }
        SplitFirstDirectory(remainingPath, segment);
    }
    filename.Append(remainingPath.View());
}

void CSystemFileName::FilterFileName(
        CFastStringInt &outFilename,
        const CFastStringInt &source,
        int allowBackslash,
        int allowDrivePrefix) {
    outFilename = source;
    unsigned long flags = 0ul;
    if (allowBackslash != 0) {
        flags |= 0x02u;
    }
    if (allowDrivePrefix != 0) {
        flags |= 0x04u;
    }
    FixFileName(outFilename, flags);
}

void CSystemFileName::Normalize(
        CFastStringInt &filename,
        int forceTrailingSlash) {
    FixFileName(filename, (forceTrailingSlash != 0 ? 1u : 0u) | 0x16u);
}

int CSystemFileName::GetRelativeName(
        const CFastStringInt &filename,
        const CFastStringInt &baseName,
        CFastStringInt &outRelativeName) {
    outRelativeName = filename;
    if (outRelativeName.CompareNoCase(baseName.Param(), baseName.Length()) != 0) {
        Normalize(outRelativeName, 0);
        return 0;
    }
    if (!baseName.Empty()) {
        outRelativeName.Assign(outRelativeName.View().substr(baseName.Length()));
    }
    Normalize(outRelativeName, 0);
    return 1;
}

void CSystemFileName::SplitLastDirectory(
        CFastStringInt &filename,
        CFastStringInt &outDirectory) {
    StringView path = filename.View();
    std::size_t end = path.size();
    while (end != 0u && path[end - 1u] == u'\\') {
        --end;
    }
    if (end == 0u) {
        outDirectory.Clear();
        filename.Clear();
        return;
    }

    const std::size_t slash = path.rfind(u'\\', end - 1u);
    if (slash == StringView::npos) {
        outDirectory.Assign(path.substr(0u, end));
        filename.Clear();
        return;
    }

    outDirectory.Assign(path.substr(slash + 1u, end - slash - 1u));
    filename.Assign(path.substr(0u, slash + 1u));
}

void CSystemFileName::ExtractShortBaseName(
        const CFastStringInt &filename,
        CFastStringInt &outBaseName) {
    SplitPath(filename, nullptr, nullptr, &outBaseName, nullptr, nullptr);
}

void CSystemFileName::ExtractShortName(
        const CFastStringInt &filename,
        CFastStringInt &outShortName) {
    CFastStringInt extension;
    SplitPath(filename, nullptr, nullptr, &outShortName, nullptr, &extension);
    outShortName.Append(extension.View());
}

void CSystemFileName::ExtractFullPathName(
        const CFastStringInt &filename,
        CFastStringInt &outFullPathName) {
    CFastStringInt directory;
    SplitPath(filename, &outFullPathName, &directory, nullptr, nullptr, nullptr);
    outFullPathName.Append(directory.View());
}

void CSystemFileName::ExtractExtComplete(
        const CFastStringInt &filename,
        CFastStringInt &outExtension) {
    SplitPath(filename, nullptr, nullptr, nullptr, nullptr, &outExtension);
}

void CSystemFileName::ExtractDriveDirAndBaseName(
        const CFastStringInt &filename,
        CFastStringInt &outName) {
    CFastStringInt drive;
    CFastStringInt directory;
    CFastStringInt baseName;
    SplitPath(filename, &drive, &directory, &baseName, nullptr, nullptr);

    outName.Clear();
    outName.Append(drive.View());
    outName.Append(directory.View());
    outName.Append(baseName.View());
}

void CSystemFileName::StripExtension(
        const CFastStringInt &filename,
        CFastStringInt &outName) {
    ExtractDriveDirAndBaseName(filename, outName);
}
