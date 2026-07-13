#include "engine/core/mw_id.h"
#include <cstdio>
#include <cstring>
#include <utility>

#include "engine/core/fast_string.h"
namespace {

void SetText(CFastString &outName, const char *text) {
    outName.SetString(
            text != nullptr ? static_cast<u32>(std::strlen(text)) : 0u,
            text);
}

void SetFixedText(CFastString &outName, const char *text, u32 textLength) {
    outName.SetString(textLength, text);
}

int IdentitySortOrder(CMwId::IdentityKind kind) {
    switch (kind) {
    case CMwId::IdentityKind::Number:
        return 0;
    case CMwId::IdentityKind::LocalName:
        return 1;
    case CMwId::IdentityKind::TranslatedName:
        return 2;
    case CMwId::IdentityKind::InternalName:
        return 3;
    case CMwId::IdentityKind::Invalid:
    default:
        return 4;
    }
}

}  // namespace

CMwId::CMwId(void) : identity_(InvalidIdentity{}) {}

CMwId::CMwId(const CMwId &other) : identity_(other.identity_) {}

CMwId::~CMwId(void) = default;

bool CMwId::operator==(const CMwId &other) const {
    if (Kind() != other.Kind()) {
        return false;
    }
    switch (Kind()) {
    case IdentityKind::Invalid:
        return true;
    case IdentityKind::Number:
        return std::get<NumberIdentity>(identity_).number ==
               std::get<NumberIdentity>(other.identity_).number;
    case IdentityKind::LocalName:
        return std::get<LocalNameIdentity>(identity_).name ==
               std::get<LocalNameIdentity>(other.identity_).name;
    case IdentityKind::TranslatedName:
        return std::get<TranslatedNameIdentity>(identity_).name ==
               std::get<TranslatedNameIdentity>(other.identity_).name;
    case IdentityKind::InternalName:
        return std::get<InternalNameIdentity>(identity_).ordinal ==
               std::get<InternalNameIdentity>(other.identity_).ordinal;
    }
    return false;
}

bool CMwId::operator!=(const CMwId &other) const {
    return !(*this == other);
}

bool CMwId::operator<(const CMwId &other) const {
    const int thisOrder = IdentitySortOrder(Kind());
    const int otherOrder = IdentitySortOrder(other.Kind());
    if (thisOrder != otherOrder) {
        return thisOrder < otherOrder;
    }
    switch (Kind()) {
    case IdentityKind::Invalid:
        return false;
    case IdentityKind::Number:
        return std::get<NumberIdentity>(identity_).number <
               std::get<NumberIdentity>(other.identity_).number;
    case IdentityKind::LocalName:
        return std::get<LocalNameIdentity>(identity_).name <
               std::get<LocalNameIdentity>(other.identity_).name;
    case IdentityKind::TranslatedName:
        return std::get<TranslatedNameIdentity>(identity_).name <
               std::get<TranslatedNameIdentity>(other.identity_).name;
    case IdentityKind::InternalName:
        return std::get<InternalNameIdentity>(identity_).ordinal <
               std::get<InternalNameIdentity>(other.identity_).ordinal;
    }
    return false;
}

CMwId::IdentityKind CMwId::Kind(void) const {
    switch (identity_.index()) {
    case 1u:
        return IdentityKind::Number;
    case 2u:
        return IdentityKind::LocalName;
    case 3u:
        return IdentityKind::TranslatedName;
    case 4u:
        return IdentityKind::InternalName;
    case 0u:
    default:
        return IdentityKind::Invalid;
    }
}

int CMwId::IsInvalid(void) const {
    return Kind() == IdentityKind::Invalid;
}

void CMwId::SetInvalid(void) {
    identity_ = InvalidIdentity{};
}

int CMwId::HasNumberName(void) const {
    return Kind() == IdentityKind::Number;
}

int CMwId::IsLocalName(void) const {
    return Kind() == IdentityKind::LocalName;
}

int CMwId::IsTranslatedName(void) const {
    return Kind() == IdentityKind::TranslatedName;
}

int CMwId::IsInternalName(void) const {
    return Kind() == IdentityKind::InternalName;
}

void CMwId::SetNumber(u32 number) {
    identity_ = NumberIdentity{number};
}

int CMwId::TryGetNumber(u32 &numberOut) const {
    const auto *number = std::get_if<NumberIdentity>(&identity_);
    if (number == nullptr) {
        return 0;
    }
    numberOut = number->number;
    return 1;
}

bool CMwId::IsNullOrUnassignedName(const char *name) {
    return name == nullptr || name[0] == '\0' ||
           std::strcmp(name, "Unassigned") == 0;
}

void CMwId::SetLocalName(const char *name) {
    if (IsNullOrUnassignedName(name)) {
        SetInvalid();
        return;
    }
    identity_ = LocalNameIdentity{std::string(name)};
}

void CMwId::SetLocalName(const CFastStringInt &name) {
    CFastString utf8;
    name.GetUtf8OrAscii(utf8);
    SetLocalName(utf8.Data());
}

void CMwId::SetTranslatedName(std::string name) {
    if (IsNullOrUnassignedName(name.c_str())) {
        SetInvalid();
        return;
    }
    identity_ = TranslatedNameIdentity{std::move(name)};
}

void CMwId::SetInternalName(u32 ordinal) {
    identity_ = InternalNameIdentity{ordinal};
}

CMwId CMwId::CreateFromLocalName(const char *name) {
    CMwId id;
    id.SetLocalName(name);
    return id;
}

const char *CMwId::GetString(void) const {
    if (const auto *local = std::get_if<LocalNameIdentity>(&identity_)) {
        return local->name.c_str();
    }
    if (const auto *translated =
                std::get_if<TranslatedNameIdentity>(&identity_)) {
        return translated->name.c_str();
    }
    return nullptr;
}

void CMwId::GetName(CFastString &outName) const {
    if (const auto *number = std::get_if<NumberIdentity>(&identity_)) {
        char text[32];
        const int length = std::snprintf(
                text,
                sizeof(text),
                "Id%u",
                static_cast<unsigned int>(number->number));
        outName.SetString(length > 0 ? static_cast<u32>(length) : 0u, text);
        return;
    }
    if (IsInvalid() || IsInternalName()) {
        SetFixedText(outName, "Unassigned", 10u);
        return;
    }
    SetText(outName, GetString());
}

const CFastString CMwId::GetName(void) const {
    CFastString name;
    GetName(name);
    return name;
}

void CMwId::GetName(CFastStringInt &outName) const {
    CFastString utf8;
    GetName(utf8);
    outName.SetUtf8(utf8.Param());
}
