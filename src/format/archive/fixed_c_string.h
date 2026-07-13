#ifndef TMNF_FIXED_C_STRING_H
#define TMNF_FIXED_C_STRING_H

#include <cstddef>
#include <string_view>

namespace TmnfFormat::FixedCString {

inline std::size_t Length(const char *text) {
    std::size_t length = 0u;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

inline bool Equals(const char *lhs, const char *rhs) {
    if (lhs == nullptr || rhs == nullptr) {
        return lhs == rhs;
    }
    for (std::size_t index = 0u;; ++index) {
        if (lhs[index] != rhs[index]) {
            return false;
        }
        if (lhs[index] == '\0') {
            return true;
        }
    }
}

inline bool CopyBytes(char *destination,
                      std::size_t capacity,
                      const char *source,
                      std::size_t byteCount) {
    if (destination == nullptr || capacity == 0u || source == nullptr ||
        byteCount >= capacity) {
        return false;
    }
    for (std::size_t index = 0u; index < byteCount; ++index) {
        destination[index] = source[index];
    }
    destination[byteCount] = '\0';
    return true;
}

inline bool Copy(char *destination,
                 std::size_t capacity,
                 const char *source) {
    return source != nullptr &&
           CopyBytes(destination, capacity, source, Length(source));
}

inline void CopyTruncated(char *destination,
                          std::size_t capacity,
                          const char *source) {
    if (destination == nullptr || capacity == 0u) {
        return;
    }
    destination[0] = '\0';
    if (source == nullptr) {
        return;
    }
    const std::size_t sourceLength = Length(source);
    const std::size_t copyLength =
            sourceLength < capacity ? sourceLength : capacity - 1u;
    for (std::size_t index = 0u; index < copyLength; ++index) {
        destination[index] = source[index];
    }
    destination[copyLength] = '\0';
}

inline void CopyTruncated(char *destination,
                          std::size_t capacity,
                          std::string_view source) {
    if (destination == nullptr || capacity == 0u) {
        return;
    }
    const std::size_t copyLength =
            source.size() < capacity ? source.size() : capacity - 1u;
    for (std::size_t index = 0u; index < copyLength; ++index) {
        destination[index] = source[index];
    }
    destination[copyLength] = '\0';
}

inline bool Append(char *destination,
                   std::size_t capacity,
                   const char *source) {
    if (destination == nullptr || capacity == 0u || source == nullptr) {
        return false;
    }
    const std::size_t destinationLength = Length(destination);
    const std::size_t sourceLength = Length(source);
    if (destinationLength + sourceLength >= capacity) {
        return false;
    }
    for (std::size_t index = 0u; index <= sourceLength; ++index) {
        destination[destinationLength + index] = source[index];
    }
    return true;
}

}  // namespace TmnfFormat::FixedCString

#endif
