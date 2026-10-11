#include "FrontierAccumulator.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <utility>

#include "DeferredSearchState.h"
#include "RouteSurvivalAudit.h"

namespace {
bool concrete_dominates(const SolverState& lhs, const SolverState& rhs) {
    if (!(SolverContextKey::from_state(lhs) == SolverContextKey::from_state(rhs))) {
        return false;
    }
    const bool food_dominates = lhs.item_shop[i_Food * 3 + id_Weight] == 0
        ? lhs.food >= rhs.food
        : lhs.food == rhs.food;
    return lhs.gold >= rhs.gold &&
           food_dominates &&
           lhs.spice == rhs.spice &&
           lhs.pottery == rhs.pottery &&
           lhs.marble == rhs.marble &&
           lhs.silk == rhs.silk &&
           lhs.jewelry == rhs.jewelry;
}

int quantity_rank(const RouteAuditQuantityStatus status) noexcept {
    switch (status) {
        case RouteAuditQuantityStatus::Exact:
            return 3;
        case RouteAuditQuantityStatus::DominatingReplacement:
            return 2;
        case RouteAuditQuantityStatus::Missing:
            return 1;
        case RouteAuditQuantityStatus::NotApplicable:
            return 0;
    }
    return 0;
}

void merge_observation(
    SolverState& destination,
    const SolverState& source,
    const bool is_dominating_replacement
) {
    RouteAuditQuantityStatus source_quantity = source.route_audit_quantity;
    if (is_dominating_replacement &&
        source_quantity == RouteAuditQuantityStatus::Exact) {
        source_quantity = RouteAuditQuantityStatus::DominatingReplacement;
    }
    if (source.route_audit_progress > destination.route_audit_progress) {
        destination.route_audit_progress = source.route_audit_progress;
        destination.route_audit_quantity = source_quantity;
    } else if (source.route_audit_progress == destination.route_audit_progress &&
               quantity_rank(source_quantity) >
                   quantity_rank(destination.route_audit_quantity)) {
        destination.route_audit_quantity = source_quantity;
    }
}

struct DeferredBucketKey {
    SolverContextKey context;
    size_t partition_hash = 0;

    bool operator==(const DeferredBucketKey& other) const noexcept {
        return partition_hash == other.partition_hash && context == other.context;
    }
};

struct DeferredBucketKeyHash {
    size_t operator()(const DeferredBucketKey& key) const noexcept {
        size_t seed = SolverContextKeyHash{}(key.context);
        seed ^= key.partition_hash + 0x9e3779b97f4a7c15ULL +
                (seed << 6U) + (seed >> 2U);
        return seed;
    }
};

bool deferred_dominates(
    const SolverState& lhs,
    const SolverState& rhs,
    ExactFrontierReduction& result
) {
    ++result.deferred_dominance_checks;
    if (!lhs.deferred || !rhs.deferred ||
        lhs.spice != rhs.spice || lhs.pottery != rhs.pottery ||
        lhs.marble != rhs.marble || lhs.silk != rhs.silk ||
        lhs.jewelry != rhs.jewelry) {
        return false;
    }
    DeferredDominanceMetrics metrics;
    const bool dominated = lhs.deferred->dominates(*rhs.deferred, &metrics);
    result.deferred_gold_function_comparisons += metrics.gold_function_comparisons;
    result.deferred_exact_spend_evaluations += metrics.exact_spend_evaluations;
    result.deferred_origin_coverage_checks += metrics.origin_coverage_checks;
    if (dominated) {
        result.deferred_cross_shape_dominance_prunes +=
            metrics.successful_cross_shape_checks;
    }
    return dominated;
}

void note_deferred_prune(
    const SolverState& dominator,
    ExactFrontierReduction& result
) {
    ++result.dominance_prunes;
    if (dominator.deferred->has_open_purchase()) {
        ++result.deferred_purchase_dominance_prunes;
    } else {
        ++result.deferred_sold_dominance_prunes;
    }
}

void prefer_lower_trace(SolverState& retained, SolverState candidate) {
    merge_observation(retained, candidate, false);
    if (candidate.trace_id >= retained.trace_id) {
        return;
    }
    const int16_t progress = retained.route_audit_progress;
    const RouteAuditQuantityStatus quantity = retained.route_audit_quantity;
    retained = std::move(candidate);
    retained.route_audit_progress = progress;
    retained.route_audit_quantity = quantity;
}
}

struct FrontierAccumulator::Impl {
    RouteSurvivalAudit* audit = nullptr;
    ExactFrontierReduction result;
    std::unordered_map<
        SolverContextKey,
        std::vector<SolverState>,
        SolverContextKeyHash
    > concrete;
    std::unordered_map<
        DeferredBucketKey,
        std::vector<SolverState>,
        DeferredBucketKeyHash
    > deferred;
    std::unordered_map<SolverContextKey, size_t, SolverContextKeyHash>
        deferred_context_seen;
    std::unordered_map<DeferredBucketKey, size_t, DeferredBucketKeyHash>
        deferred_partition_seen;
    bool finished = false;

    explicit Impl(RouteSurvivalAudit* route_audit) : audit(route_audit) {}

    [[nodiscard]] size_t size() const noexcept {
        size_t count = 0;
        for (const auto& [key, bucket] : concrete) {
            (void)key;
            count += bucket.size();
        }
        for (const auto& [key, bucket] : deferred) {
            (void)key;
            count += bucket.size();
        }
        return count;
    }

    [[nodiscard]] uint64_t memory_bytes() const {
        uint64_t bytes = sizeof(*this) +
            static_cast<uint64_t>(concrete.bucket_count()) * sizeof(void*) +
            static_cast<uint64_t>(deferred.bucket_count()) * sizeof(void*) +
            static_cast<uint64_t>(concrete.size()) *
                sizeof(decltype(concrete)::value_type) +
            static_cast<uint64_t>(deferred.size()) *
                sizeof(decltype(deferred)::value_type) +
            static_cast<uint64_t>(deferred_context_seen.bucket_count()) * sizeof(void*) +
            static_cast<uint64_t>(deferred_partition_seen.bucket_count()) * sizeof(void*) +
            static_cast<uint64_t>(deferred_context_seen.size()) *
                sizeof(decltype(deferred_context_seen)::value_type) +
            static_cast<uint64_t>(deferred_partition_seen.size()) *
                sizeof(decltype(deferred_partition_seen)::value_type);
        std::unordered_set<const DeferredSearchState*> counted_deferred;
        std::unordered_set<const void*> counted_shared;
        const auto add_bucket = [&](const std::vector<SolverState>& bucket) {
            bytes += static_cast<uint64_t>(bucket.capacity()) * sizeof(SolverState);
            for (const SolverState& state : bucket) {
                if (state.deferred && counted_deferred.insert(state.deferred.get()).second) {
                    bytes += state.deferred->memory_bytes(counted_shared);
                }
            }
        };
        for (const auto& [key, bucket] : concrete) {
            (void)key;
            add_bucket(bucket);
        }
        for (const auto& [key, bucket] : deferred) {
            (void)key;
            add_bucket(bucket);
        }
        return bytes;
    }

    void note(
        const RouteAuditReductionStage stage,
        const SolverState& state
    ) const {
        if (audit != nullptr) {
            audit->note_reduction(
                stage,
                state.route_audit_progress,
                state.route_audit_quantity
            );
        }
    }

    void insert_concrete(SolverState state) {
        std::vector<SolverState>& bucket =
            concrete[SolverContextKey::from_state(state)];
        const auto duplicate = std::find_if(
            bucket.begin(),
            bucket.end(),
            [&](const SolverState& kept) { return kept == state; }
        );
        if (duplicate != bucket.end()) {
            ++result.duplicate_prunes;
            prefer_lower_trace(*duplicate, std::move(state));
            note(RouteAuditReductionStage::AfterExact, *duplicate);
            return;
        }
        note(RouteAuditReductionStage::AfterExact, state);

        size_t index = 0;
        while (index < bucket.size()) {
            SolverState& kept = bucket[index];
            if (concrete_dominates(kept, state)) {
                ++result.dominance_prunes;
                merge_observation(kept, state, true);
                return;
            }
            if (concrete_dominates(state, kept)) {
                ++result.dominance_prunes;
                merge_observation(state, kept, true);
                bucket.erase(bucket.begin() + static_cast<std::ptrdiff_t>(index));
                continue;
            }
            ++index;
        }
        bucket.emplace_back(std::move(state));
    }

    void insert_deferred(SolverState state) {
        DeferredBucketKey key{
            SolverContextKey::from_state(state),
            state.deferred->dominance_partition_hash(),
        };
        size_t& context_seen = deferred_context_seen[key.context];
        size_t& partition_seen = deferred_partition_seen[key];
        result.deferred_partition_rejections += context_seen - partition_seen;
        ++context_seen;
        ++partition_seen;
        std::vector<SolverState>& bucket = deferred[key];
        const auto duplicate = std::find_if(
            bucket.begin(),
            bucket.end(),
            [&](const SolverState& kept) { return kept == state; }
        );
        if (duplicate != bucket.end()) {
            ++result.duplicate_prunes;
            prefer_lower_trace(*duplicate, std::move(state));
            note(RouteAuditReductionStage::AfterExact, *duplicate);
            return;
        }
        note(RouteAuditReductionStage::AfterExact, state);

        size_t index = 0;
        while (index < bucket.size()) {
            SolverState& kept = bucket[index];
            if (deferred_dominates(kept, state, result)) {
                note_deferred_prune(kept, result);
                merge_observation(kept, state, true);
                return;
            }
            if (deferred_dominates(state, kept, result)) {
                note_deferred_prune(state, result);
                merge_observation(state, kept, true);
                bucket.erase(bucket.begin() + static_cast<std::ptrdiff_t>(index));
                continue;
            }
            ++index;
        }
        bucket.emplace_back(std::move(state));
    }
};

FrontierAccumulator::FrontierAccumulator(RouteSurvivalAudit* audit)
    : impl(std::make_unique<Impl>(audit)) {}

FrontierAccumulator::~FrontierAccumulator() = default;
FrontierAccumulator::FrontierAccumulator(FrontierAccumulator&&) noexcept = default;
FrontierAccumulator& FrontierAccumulator::operator=(FrontierAccumulator&&) noexcept = default;

void FrontierAccumulator::insert(SolverState state) {
    if (impl->finished) {
        throw std::logic_error("Cannot insert into a finished frontier accumulator");
    }
    impl->note(RouteAuditReductionStage::BeforeExact, state);
    if (state.deferred) {
        impl->insert_deferred(std::move(state));
    } else {
        impl->insert_concrete(std::move(state));
    }
}

size_t FrontierAccumulator::size() const noexcept {
    return impl->size();
}

bool FrontierAccumulator::empty() const noexcept {
    return size() == 0;
}

uint64_t FrontierAccumulator::memory_bytes() const {
    return impl->memory_bytes();
}

ExactFrontierReduction FrontierAccumulator::finish() {
    if (impl->finished) {
        throw std::logic_error("Frontier accumulator can only be finished once");
    }
    impl->finished = true;
    size_t survivor_count = 0;
    for (const auto& [key, bucket] : impl->concrete) {
        (void)key;
        survivor_count += bucket.size();
    }
    for (const auto& [key, bucket] : impl->deferred) {
        (void)key;
        survivor_count += bucket.size();
    }
    impl->result.states.reserve(survivor_count);
    for (auto& [key, bucket] : impl->concrete) {
        (void)key;
        for (SolverState& state : bucket) {
            impl->note(RouteAuditReductionStage::AfterDominance, state);
            impl->result.states.emplace_back(std::move(state));
        }
    }
    for (auto& [key, bucket] : impl->deferred) {
        (void)key;
        for (SolverState& state : bucket) {
            impl->note(RouteAuditReductionStage::AfterDominance, state);
            impl->result.states.emplace_back(std::move(state));
        }
    }
    return std::move(impl->result);
}
