#ifndef FOREVERVALIDATOR_RESULT_H
#define FOREVERVALIDATOR_RESULT_H

#include <utility>
#include <variant>

namespace forevervalidator {

template<typename T, typename E>
class DiscriminatedResult {
public:
    static DiscriminatedResult Success(T value) {
        return DiscriminatedResult(std::in_place_index<0>, std::move(value));
    }
    static DiscriminatedResult Failure(E error) {
        return DiscriminatedResult(std::in_place_index<1>, std::move(error));
    }
    bool HasValue() const noexcept { return state_.index() == 0u; }
    bool Succeeded() const noexcept { return HasValue(); }
    explicit operator bool() const noexcept { return HasValue(); }
    T &Value() & { return std::get<0>(state_); }
    const T &Value() const & { return std::get<0>(state_); }
    T &&Value() && { return std::get<0>(std::move(state_)); }
    E &Error() & { return std::get<1>(state_); }
    const E &Error() const & { return std::get<1>(state_); }
    E &&Error() && { return std::get<1>(std::move(state_)); }
private:
    template<typename... Args>
    explicit DiscriminatedResult(std::in_place_index_t<0> index,
                                 Args &&...args)
        : state_(index, std::forward<Args>(args)...) {}
    template<typename... Args>
    explicit DiscriminatedResult(std::in_place_index_t<1> index,
                                 Args &&...args)
        : state_(index, std::forward<Args>(args)...) {}
    std::variant<T, E> state_;
};

}  // namespace forevervalidator

#endif
