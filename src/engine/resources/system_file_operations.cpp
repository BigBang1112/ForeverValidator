#include "engine/resources/system_file_operations.h"

#include <atomic>

namespace tmnf::platform {
namespace {

std::atomic<SystemFileOperations *> ActiveOperations{nullptr};

}  // namespace

SystemFileOperations *GetSystemFileOperations() noexcept {
    return ActiveOperations.load(std::memory_order_acquire);
}

void SetSystemFileOperations(SystemFileOperations *operations) noexcept {
    ActiveOperations.store(operations, std::memory_order_release);
}

}  // namespace tmnf::platform
