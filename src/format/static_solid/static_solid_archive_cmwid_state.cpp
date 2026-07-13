#include "format/static_solid/static_solid_archive_cmwid_state.h"
#include <algorithm>
#include <charconv>
#include <string>

#include "format/archive/mw_id_archive_codec.h"
#include "format/static_solid/static_solid_archive_byte_stream.h"
namespace {

bool DecodeDecimalIdentifier(const std::string &text, CMwId &idOut) {
    u32 number = 0u;
    const char *begin = text.data();
    const char *end = begin + text.size();
    const std::from_chars_result parsed =
            std::from_chars(begin, end, number, 10);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return false;
    }
    idOut.SetNumber(number);
    return true;
}

TmnfFormat::ArchiveIdentifierWord TextTaggedWord(u32 tag) {
    using TmnfFormat::ArchiveIdentifierKind;
    if (tag == 1u) {
        return {ArchiveIdentifierKind::LocalName, 0u};
    }
    if (tag == 2u) {
        return {ArchiveIdentifierKind::TranslatedName, 0u};
    }
    return {ArchiveIdentifierKind::Invalid, 0u};
}

}  // namespace

void CGameCtnReplayStaticSolidArchiveCMwIdState::ResetSharedNameCache(
        std::size_t capacity) {
    sharedNameCacheCapacity_ = capacity;
    sharedNameCache_.clear();
    sharedNameCache_.reserve(capacity);
}

void CGameCtnReplayStaticSolidArchiveCMwIdState::CaptureText(
        char *textOut,
        std::size_t textOutSize,
        const char *name) {
    if (textOut == nullptr || textOutSize == 0u) {
        return;
    }
    const std::string_view text = name != nullptr
            ? std::string_view(name)
            : std::string_view{};
    const std::size_t length = std::min(text.size(), textOutSize - 1u);
    std::copy_n(text.data(), length, textOut);
    textOut[length] = '\0';
}

void CGameCtnReplayStaticSolidArchiveCMwIdState::ClearLastText() {
    lastText_[0] = '\0';
    hasLastText_ = false;
}

void CGameCtnReplayStaticSolidArchiveCMwIdState::CaptureLastText(
        const char *name) {
    if (name == nullptr || name[0] == '\0') {
        return;
    }
    CaptureText(lastText_, sizeof(lastText_), name);
    hasLastText_ = lastText_[0] != '\0';
}

const char *CGameCtnReplayStaticSolidArchiveCMwIdState::LastTextOrNull()
        const {
    return hasLastText_ ? lastText_ : nullptr;
}

int CGameCtnReplayStaticSolidArchiveCMwIdState::ReadResolved(
        CGameCtnReplayStaticSolidArchiveByteStream &stream,
        CMwId *idOut,
        char *textOut,
        std::size_t textOutSize) {
    if (textOut != nullptr && textOutSize != 0u) {
        textOut[0] = '\0';
    }
    ClearLastText();

    u32 archiveWord = 0u;
    if (!stream.ReadU32(&archiveWord)) {
        return 0;
    }
    if (mode_ == EncodingMode::Unknown &&
        archiveWord >= 1u && archiveWord <= 3u) {
        mode_ = static_cast<EncodingMode>(archiveWord);
        if (!stream.ReadU32(&archiveWord)) {
            return 0;
        }
    }

    CMwId decoded;
    if (mode_ == EncodingMode::TextTagged) {
        if (archiveWord == 0u) {
            decoded.SetInvalid();
        } else {
            std::string name;
            if (!stream.ReadStringOwned(&name)) {
                return 0;
            }
            CaptureText(textOut, textOutSize, name.c_str());
            CaptureLastText(name.c_str());
            if (archiveWord == 3u) {
                if (!DecodeDecimalIdentifier(name, decoded)) {
                    return 0;
                }
            } else if (!TmnfFormat::CMwIdArchiveCodec::Materialize(
                               TextTaggedWord(archiveWord),
                               name,
                               decoded)) {
                return 0;
            }
        }
    } else {
        const TmnfFormat::ArchiveIdentifierWord parsed =
                TmnfFormat::CMwIdArchiveCodec::ParseWord(archiveWord);
        if (!parsed.IsNamed()) {
            if (!TmnfFormat::CMwIdArchiveCodec::Materialize(
                        parsed, {}, decoded)) {
                return 0;
            }
        } else if (mode_ == EncodingMode::SharedNames &&
                   parsed.payload != 0u) {
            if (parsed.payload > sharedNameCache_.size()) {
                return 0;
            }
            decoded = sharedNameCache_[parsed.payload - 1u];
            CaptureText(textOut, textOutSize, decoded.GetString());
            CaptureLastText(decoded.GetString());
        } else {
            if (mode_ == EncodingMode::Unknown && parsed.payload != 0u) {
                return 0;
            }
            std::string name;
            if (!stream.ReadStringOwned(&name) ||
                !TmnfFormat::CMwIdArchiveCodec::Materialize(
                        parsed, name, decoded)) {
                return 0;
            }
            if (mode_ == EncodingMode::SharedNames) {
                // Inline names remain valid after the shared-reference cache
                // reaches the archive's configured capacity.
                if (sharedNameCache_.size() < sharedNameCacheCapacity_) {
                    sharedNameCache_.push_back(decoded);
                }
            }
            CaptureText(textOut, textOutSize, name.c_str());
            CaptureLastText(name.c_str());
        }
    }

    if (idOut != nullptr) {
        *idOut = decoded;
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveCMwIdState::ReadSkip(
        CGameCtnReplayStaticSolidArchiveByteStream &stream) {
    return ReadResolved(stream, nullptr, nullptr, 0u);
}

int CGameCtnReplayStaticSolidArchiveCMwIdState::ReadText(
        CGameCtnReplayStaticSolidArchiveByteStream &stream,
        char *out,
        std::size_t outSize) {
    return ReadResolved(stream, nullptr, out, outSize);
}

int CGameCtnReplayStaticSolidArchiveCMwIdState::ReadId(
        CGameCtnReplayStaticSolidArchiveByteStream &stream,
        CMwId *idOut) {
    if (idOut == nullptr) {
        return 0;
    }
    return ReadResolved(stream, idOut, nullptr, 0u);
}
