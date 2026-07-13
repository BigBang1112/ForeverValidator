#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace TmnfFormat {

struct ClassicArchiveIdentifier {
    std::uint32_t encodedValue = 0u;
    std::string text;
};

class ClassicArchiveReader {
public:
    bool Open(const std::uint8_t *bytes,
              std::size_t byteCount,
              std::size_t maximumStoredIdentifiers);

    std::size_t Offset() const;
    std::size_t Remaining() const;
    bool Seek(std::size_t offset);
    bool Skip(std::size_t byteCount);
    bool PeekU32(std::uint32_t &value) const;
    bool ReadU8(std::uint8_t &value);
    bool ReadU32(std::uint32_t &value);
    bool ReadF32(float &value);
    bool ReadString(std::string &value);
    bool ReadIdentifier(ClassicArchiveIdentifier &identifier);

private:
    bool ReadEncodedIdentifier(std::uint32_t encodedValue,
                               ClassicArchiveIdentifier &identifier);

    const std::uint8_t *bytes_ = nullptr;
    std::size_t byteCount_ = 0u;
    std::size_t offset_ = 0u;
    std::size_t maximumStoredIdentifiers_ = 0u;
    std::uint32_t identifierEncoding_ = 0u;
    std::vector<std::string> storedIdentifiers_;
};

}  // namespace TmnfFormat
