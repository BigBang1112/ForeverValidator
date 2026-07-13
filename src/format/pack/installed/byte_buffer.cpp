#include "format/pack/installed/byte_buffer.h"
#include <new>

bool ByteBuffer::Append(const void *bytes, std::size_t byteCount) {
    if (byteCount == 0u) {
        return true;
    }
    if (bytes == nullptr || byteCount > bytes_.max_size() - bytes_.size()) {
        return false;
    }
    try {
        const auto *first = static_cast<const unsigned char *>(bytes);
        bytes_.insert(bytes_.end(), first, first + byteCount);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

bool ByteBuffer::Resize(std::size_t byteCount) {
    try {
        bytes_.resize(byteCount);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

unsigned char *ByteBuffer::Data() {
    return bytes_.empty() ? nullptr : bytes_.data();
}

const unsigned char *ByteBuffer::Data() const {
    return bytes_.empty() ? nullptr : bytes_.data();
}

std::size_t ByteBuffer::Size() const {
    return bytes_.size();
}

bool ByteBuffer::Empty() const {
    return bytes_.empty();
}

void ByteBuffer::Clear() {
    bytes_.clear();
}
