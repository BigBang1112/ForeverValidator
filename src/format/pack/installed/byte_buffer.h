#pragma once

#include <cstddef>
#include <vector>

// Owned bytes at an archive boundary. This is intentionally a normal value
// type: readers receive explicit data/size views and never manage allocation.
class ByteBuffer {
public:
    bool Append(const void *bytes, std::size_t byteCount);
    bool Resize(std::size_t byteCount);
    unsigned char *Data();
    const unsigned char *Data() const;
    std::size_t Size() const;
    bool Empty() const;
    void Clear();

private:
    std::vector<unsigned char> bytes_;
};
