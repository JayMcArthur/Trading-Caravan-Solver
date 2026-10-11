#ifndef TRADING_CARAVAN_SOLVER_PROCESS_SAFETY_H
#define TRADING_CARAVAN_SOLVER_PROCESS_SAFETY_H

#include <cstdint>

// Installs process-local hard limits before any solver or test allocation.
// A lower limit inherited from an external wrapper is always preserved.
[[nodiscard]] bool install_process_safety_limits(
    uint64_t maximum_memory_mb,
    uint64_t maximum_cpu_seconds = 0
);

[[nodiscard]] uint64_t process_memory_limit_bytes();

#endif // TRADING_CARAVAN_SOLVER_PROCESS_SAFETY_H
