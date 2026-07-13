#pragma once

#include <utility>
#include <vector>

// A typed sequence with ordinary vector ownership and stable engine-facing
// method names.
template <typename T>
class CFastBuffer {
public:
    using value_type = T;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    CFastBuffer() = default;
    CFastBuffer(const CFastBuffer &) = default;
    CFastBuffer(CFastBuffer &&) noexcept = default;
    CFastBuffer &operator=(const CFastBuffer &) = default;
    CFastBuffer &operator=(CFastBuffer &&) noexcept = default;
    ~CFastBuffer() = default;

    bool empty() const { return elements_.empty(); }
    unsigned long size() const {
        return static_cast<unsigned long>(elements_.size());
    }
    unsigned long GetCount() const { return size(); }
    void clear() { elements_.clear(); }
    void reserve(unsigned long count) { elements_.reserve(count); }
    void push_back(const T &value) { elements_.push_back(value); }
    void push_back(T &&value) { elements_.push_back(std::move(value)); }

    template <typename... Args>
    T &emplace_back(Args &&...args) {
        return elements_.emplace_back(std::forward<Args>(args)...);
    }

    T &front() { return elements_.front(); }
    const T &front() const { return elements_.front(); }
    T &back() { return elements_.back(); }
    const T &back() const { return elements_.back(); }
    T &operator[](unsigned long index) { return elements_[index]; }
    const T &operator[](unsigned long index) const { return elements_[index]; }

    iterator begin() { return elements_.begin(); }
    iterator end() { return elements_.end(); }
    const_iterator begin() const { return elements_.begin(); }
    const_iterator end() const { return elements_.end(); }

    std::vector<T> &Values() { return elements_; }
    const std::vector<T> &Values() const { return elements_; }

private:
    std::vector<T> elements_;
};
