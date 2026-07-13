#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "engine/core/engine_types.h"
struct CFastString;
struct CFastStringInt;

namespace TmnfFormat {
class CMwIdArchiveCodec;
}

class CMwId {
public:
    enum class IdentityKind : std::uint8_t {
        Invalid,
        Number,
        LocalName,
        TranslatedName,
        InternalName,
    };

    CMwId(void);
    CMwId(const CMwId &other);
    ~CMwId(void);

    CMwId &operator=(const CMwId &other) = default;

    bool operator==(const CMwId &other) const;
    bool operator!=(const CMwId &other) const;
    bool operator<(const CMwId &other) const;

    IdentityKind Kind(void) const;
    int IsInvalid(void) const;
    void SetInvalid(void);
    int HasNumberName(void) const;
    int IsLocalName(void) const;
    int IsTranslatedName(void) const;
    int IsInternalName(void) const;

    void SetNumber(u32 number);
    int TryGetNumber(u32 &numberOut) const;

    void SetLocalName(const char *name);
    void SetLocalName(const CFastStringInt &name);
    const char *GetString(void) const;
    void GetName(CFastString &outName) const;
    const CFastString GetName(void) const;
    void GetName(CFastStringInt &outName) const;
    static CMwId CreateFromLocalName(const char *name);

private:
    struct InvalidIdentity {};
    struct NumberIdentity {
        u32 number = 0u;
    };
    struct LocalNameIdentity {
        std::string name;
    };
    struct TranslatedNameIdentity {
        std::string name;
    };
    struct InternalNameIdentity {
        u32 ordinal = 0u;
    };

    using Identity = std::variant<
            InvalidIdentity,
            NumberIdentity,
            LocalNameIdentity,
            TranslatedNameIdentity,
            InternalNameIdentity>;

    Identity identity_;

    static bool IsNullOrUnassignedName(const char *name);
    void SetTranslatedName(std::string name);
    void SetInternalName(u32 ordinal);

    friend class TmnfFormat::CMwIdArchiveCodec;
};
