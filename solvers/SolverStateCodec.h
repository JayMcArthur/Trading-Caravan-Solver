#ifndef TRADING_CARAVAN_SOLVER_SOLVERSTATECODEC_H
#define TRADING_CARAVAN_SOLVER_SOLVERSTATECODEC_H

#include <cstdint>
#include <vector>

struct SolverState;

// Versioned, architecture-independent encoding for persistent exact search.
// The semantic key deliberately omits trace and audit provenance. Persistent
// storage pairs it with a compact provenance delta rather than duplicating the
// complete state in a second snapshot.
class SolverStateCodec {
public:
    static constexpr uint32_t FORMAT_VERSION = 1;

    [[nodiscard]] static std::vector<uint8_t> semantic_key(
        const SolverState& state
    );
    [[nodiscard]] static std::vector<uint8_t> snapshot(
        const SolverState& state
    );
    [[nodiscard]] static std::vector<uint8_t> provenance(
        const SolverState& state
    );
    [[nodiscard]] static SolverState restore(
        const std::vector<uint8_t>& encoded
    );
    [[nodiscard]] static SolverState restore(
        const std::vector<uint8_t>& semantic_key,
        const std::vector<uint8_t>& provenance
    );
    [[nodiscard]] static uint64_t fast_hash(
        const std::vector<uint8_t>& semantic_key
    ) noexcept;
};

#endif // TRADING_CARAVAN_SOLVER_SOLVERSTATECODEC_H
