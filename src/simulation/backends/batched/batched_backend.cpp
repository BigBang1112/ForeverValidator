#include "simulation/backends/simulation_backend.h"

namespace forevervalidator::simulation {

void ExecuteBatched(
        std::size_t count,
        const std::function<void(std::size_t)> &operation) {
    for (std::size_t index = 0u; index < count; ++index) {
        operation(index);
    }
}

}  // namespace forevervalidator::simulation
