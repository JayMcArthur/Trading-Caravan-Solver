#include "ProcessSafety.h"

#include <algorithm>
#include <limits>

#ifdef __linux__
#include <sys/resource.h>
#endif

namespace {
#ifdef __linux__
bool lower_limit(const int resource, const rlim_t requested_soft, const rlim_t requested_hard) {
    rlimit current{};
    if (getrlimit(resource, &current) != 0) {
        return false;
    }

    const rlim_t hard = current.rlim_max == RLIM_INFINITY
        ? requested_hard
        : std::min(current.rlim_max, requested_hard);
    const rlim_t soft = current.rlim_cur == RLIM_INFINITY
        ? std::min(requested_soft, hard)
        : std::min({current.rlim_cur, requested_soft, hard});
    const rlimit desired{soft, hard};
    return setrlimit(resource, &desired) == 0;
}
#endif
}

bool install_process_safety_limits(
    const uint64_t maximum_memory_mb,
    const uint64_t maximum_cpu_seconds
) {
#ifdef __linux__
    if (maximum_memory_mb == 0 ||
        maximum_memory_mb > std::numeric_limits<uint64_t>::max() / (1024ULL * 1024ULL)) {
        return false;
    }

    const auto memory_bytes = static_cast<rlim_t>(maximum_memory_mb * 1024ULL * 1024ULL);
    if (!lower_limit(RLIMIT_AS, memory_bytes, memory_bytes)) {
        return false;
    }

    const rlimit no_core{0, 0};
    if (setrlimit(RLIMIT_CORE, &no_core) != 0) {
        return false;
    }

    if (maximum_cpu_seconds > 0) {
        const auto soft_seconds = static_cast<rlim_t>(maximum_cpu_seconds);
        const auto hard_seconds = static_cast<rlim_t>(maximum_cpu_seconds + 5);
        if (!lower_limit(RLIMIT_CPU, soft_seconds, hard_seconds)) {
            return false;
        }
    }
    return true;
#else
    (void)maximum_memory_mb;
    (void)maximum_cpu_seconds;
    return false;
#endif
}

uint64_t process_memory_limit_bytes() {
#ifdef __linux__
    rlimit current{};
    if (getrlimit(RLIMIT_AS, &current) != 0 || current.rlim_cur == RLIM_INFINITY) {
        return 0;
    }
    return static_cast<uint64_t>(current.rlim_cur);
#else
    return 0;
#endif
}
