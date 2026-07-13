#include "format/archive/mw_id_archive_codec.h"
#include <limits>
#include <string>
#include <utility>

namespace TmnfFormat {
namespace {

constexpr std::uint32_t NameKindMask = 0xc0000000u;
constexpr std::uint32_t LocalNameKind = 0x40000000u;
constexpr std::uint32_t TranslatedNameKind = 0x80000000u;
constexpr std::uint32_t InternalNameKind = 0xc0000000u;
constexpr std::uint32_t NameOrdinalMask = 0x3fffffffu;
constexpr std::uint32_t ArchiveNameReferenceMask = 0x0fffffffu;
constexpr std::uint32_t InvalidIdentifier =
        std::numeric_limits<std::uint32_t>::max();

}  // namespace

bool ArchiveIdentifierWord::IsNamed(void) const {
    return kind == ArchiveIdentifierKind::LocalName ||
           kind == ArchiveIdentifierKind::TranslatedName;
}

bool ArchiveIdentifierValue::IsNamed(void) const {
    return kind == ArchiveIdentifierKind::LocalName ||
           kind == ArchiveIdentifierKind::TranslatedName;
}

ArchiveIdentifierWord CMwIdArchiveCodec::ParseWord(
        std::uint32_t archiveWord) {
    if (archiveWord == InvalidIdentifier) {
        return {ArchiveIdentifierKind::Invalid, 0u};
    }
    switch (archiveWord & NameKindMask) {
    case LocalNameKind:
        return {ArchiveIdentifierKind::LocalName,
                archiveWord & ArchiveNameReferenceMask};
    case TranslatedNameKind:
        return {ArchiveIdentifierKind::TranslatedName,
                archiveWord & ArchiveNameReferenceMask};
    case InternalNameKind:
        return {ArchiveIdentifierKind::InternalName,
                archiveWord & NameOrdinalMask};
    default:
        return {ArchiveIdentifierKind::Number, archiveWord};
    }
}

bool CMwIdArchiveCodec::Materialize(
        const ArchiveIdentifierWord &word,
        std::string_view resolvedName,
        CMwId &idOut) {
    ArchiveIdentifierValue value;
    return Resolve(word, resolvedName, value) && Materialize(value, idOut);
}

bool CMwIdArchiveCodec::Resolve(
        const ArchiveIdentifierWord &word,
        std::string_view resolvedName,
        ArchiveIdentifierValue &valueOut) {
    ArchiveIdentifierValue value;
    value.kind = word.kind;
    if (word.IsNamed()) {
        if (resolvedName.empty()) {
            return false;
        }
        value.name.assign(resolvedName.data(), resolvedName.size());
    } else {
        value.number = word.payload;
    }
    valueOut = std::move(value);
    return true;
}

bool CMwIdArchiveCodec::Materialize(
        const ArchiveIdentifierValue &value,
        CMwId &idOut) {
    CMwId decoded;
    switch (value.kind) {
    case ArchiveIdentifierKind::Invalid:
        decoded.SetInvalid();
        break;
    case ArchiveIdentifierKind::Number:
        decoded.SetNumber(value.number);
        break;
    case ArchiveIdentifierKind::LocalName:
        if (value.name.empty()) {
            return false;
        }
        decoded.SetLocalName(value.name.c_str());
        if (decoded.IsInvalid()) {
            return false;
        }
        break;
    case ArchiveIdentifierKind::TranslatedName:
        if (value.name.empty()) {
            return false;
        }
        decoded.SetTranslatedName(value.name);
        if (decoded.IsInvalid()) {
            return false;
        }
        break;
    case ArchiveIdentifierKind::InternalName:
        decoded.SetInternalName(value.number);
        break;
    }
    idOut = std::move(decoded);
    return true;
}

bool CMwIdArchiveCodec::EncodeStandalone(
        const CMwId &id,
        std::uint32_t &archiveWordOut) {
    switch (id.Kind()) {
    case CMwId::IdentityKind::Invalid:
        archiveWordOut = InvalidIdentifier;
        return true;
    case CMwId::IdentityKind::Number: {
        const auto &number = std::get<CMwId::NumberIdentity>(id.identity_);
        if (number.number > NameOrdinalMask) {
            return false;
        }
        archiveWordOut = number.number;
        return true;
    }
    case CMwId::IdentityKind::InternalName: {
        const auto &internal =
                std::get<CMwId::InternalNameIdentity>(id.identity_);
        if (internal.ordinal >= NameOrdinalMask) {
            return false;
        }
        archiveWordOut = InternalNameKind | internal.ordinal;
        return true;
    }
    case CMwId::IdentityKind::LocalName:
    case CMwId::IdentityKind::TranslatedName:
        return false;
    }
    return false;
}

bool CMwIdArchiveCodec::EncodeNameReference(
        ArchiveIdentifierKind nameKind,
        std::uint32_t oneBasedReference,
        std::uint32_t &archiveWordOut) {
    if (oneBasedReference > ArchiveNameReferenceMask) {
        return false;
    }
    if (nameKind == ArchiveIdentifierKind::LocalName) {
        archiveWordOut = LocalNameKind | oneBasedReference;
        return true;
    }
    if (nameKind == ArchiveIdentifierKind::TranslatedName) {
        archiveWordOut = TranslatedNameKind | oneBasedReference;
        return true;
    }
    return false;
}

}  // namespace TmnfFormat
