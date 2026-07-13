#pragma once

#include <memory>
#include <utility>
#include <variant>

// Stores one object relationship with an explicit lifetime policy. Borrowed
// entries never affect lifetime; owned entries transfer through Release().
template <typename T>
class OwnedOrBorrowed {
public:
    explicit OwnedOrBorrowed(T &borrowed)
            : storage_(Borrowed{&borrowed}) {}

    explicit OwnedOrBorrowed(std::unique_ptr<T> owned)
            : storage_(std::move(owned)) {}

    OwnedOrBorrowed(OwnedOrBorrowed &&) noexcept = default;
    OwnedOrBorrowed &operator=(OwnedOrBorrowed &&) noexcept = default;

    OwnedOrBorrowed(const OwnedOrBorrowed &) = delete;
    OwnedOrBorrowed &operator=(const OwnedOrBorrowed &) = delete;

    T *Get(void) const {
        if (const auto *borrowed = std::get_if<Borrowed>(&storage_)) {
            return borrowed->object;
        }
        return std::get<std::unique_ptr<T>>(storage_).get();
    }

    bool Owns(const T *object) const {
        const auto *owned = std::get_if<std::unique_ptr<T>>(&storage_);
        return owned != nullptr && owned->get() == object;
    }

    bool IsOwned(void) const {
        return std::holds_alternative<std::unique_ptr<T>>(storage_);
    }

    std::unique_ptr<T> Release(void) {
        auto *owned = std::get_if<std::unique_ptr<T>>(&storage_);
        return owned != nullptr ? std::move(*owned) : nullptr;
    }

private:
    struct Borrowed {
        T *object;
    };

    std::variant<Borrowed, std::unique_ptr<T>> storage_;
};
