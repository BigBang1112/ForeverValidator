#include "simulation/backends/simulation_backend.h"

namespace forevervalidator::simulation {

SimulationBackend ResolveLeafBackend(SimulationBackend backend) noexcept {
    // The optimized implementation deliberately forwards while its independent
    // physics kernels are developed behind this boundary.
    switch (backend) {
    case SimulationBackend::OptimizedCpu:
    case SimulationBackend::Batched:
        return SimulationBackend::Reference;
    case SimulationBackend::Reference:
        return SimulationBackend::Reference;
    }
    return SimulationBackend::Reference;
}

}  // namespace forevervalidator::simulation
