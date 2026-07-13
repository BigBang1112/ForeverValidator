#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "engine/core/mw_id.h"
namespace TmnfFormat {

enum class ArchiveIdentifierKind : std::uint8_t {
    Invalid,
    Number,
    LocalName,
    TranslatedName,
    InternalName,
};

struct ArchiveIdentifierWord {
    ArchiveIdentifierKind kind = ArchiveIdentifierKind::Invalid;
    std::uint32_t payload = 0u;

    bool IsNamed(void) const;
};

struct ArchiveIdentifierValue {
    ArchiveIdentifierKind kind = ArchiveIdentifierKind::Invalid;
    std::uint32_t number = 0u;
    std::string name;

    bool IsNamed(void) const;
};

class CMwIdArchiveCodec {
public:
    static ArchiveIdentifierWord ParseWord(std::uint32_t archiveWord);
    static bool Materialize(
            const ArchiveIdentifierWord &word,
            std::string_view resolvedName,
            CMwId &idOut);
    static bool Resolve(
            const ArchiveIdentifierWord &word,
            std::string_view resolvedName,
            ArchiveIdentifierValue &valueOut);
    static bool Materialize(
            const ArchiveIdentifierValue &value,
            CMwId &idOut);
    static bool EncodeStandalone(
            const CMwId &id,
            std::uint32_t &archiveWordOut);
    static bool EncodeNameReference(
            ArchiveIdentifierKind nameKind,
            std::uint32_t oneBasedReference,
            std::uint32_t &archiveWordOut);
};

}  // namespace TmnfFormat
