#include "SolverStateCodec.h"

#include <limits>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "DeferredGoldFunction.h"
#include "DeferredPurchaseModel.h"
#include "DeferredSearchState.h"
#include "DeferredSoldModel.h"
#include "GameSolver.h"

namespace {
constexpr uint32_t SNAPSHOT_MAGIC = 0x53544353U; // "SCTS" little endian.
constexpr uint32_t PROVENANCE_MAGIC = 0x56525053U; // "SPRV" little endian.

template <typename Value, bool IsEnum = std::is_enum_v<Value>>
struct RawInteger {
    using type = Value;
};

template <typename Value>
struct RawInteger<Value, true> {
    using type = std::underlying_type_t<Value>;
};

class Writer {
public:
    template <typename Value>
    void integer(Value value) {
        static_assert(std::is_integral_v<Value> || std::is_enum_v<Value>);
        using Raw = typename RawInteger<Value>::type;
        using Unsigned = std::make_unsigned_t<Raw>;
        Unsigned bits = static_cast<Unsigned>(static_cast<Raw>(value));
        for (size_t byte = 0; byte < sizeof(Unsigned); ++byte) {
            bytes.push_back(static_cast<uint8_t>(bits & 0xffU));
            bits >>= 8U;
        }
    }

    void count(const size_t value) {
        if (value > std::numeric_limits<uint32_t>::max()) {
            throw std::overflow_error("Persistent solver collection is too large");
        }
        integer(static_cast<uint32_t>(value));
    }

    std::vector<uint8_t> bytes;
};

class Reader {
public:
    explicit Reader(const std::vector<uint8_t>& source) : bytes(source) {}

    template <typename Value>
    Value integer() {
        static_assert(std::is_integral_v<Value> || std::is_enum_v<Value>);
        using Raw = typename RawInteger<Value>::type;
        using Unsigned = std::make_unsigned_t<Raw>;
        if (offset + sizeof(Unsigned) > bytes.size()) {
            throw std::runtime_error("Truncated persistent solver snapshot");
        }
        Unsigned bits = 0;
        for (size_t byte = 0; byte < sizeof(Unsigned); ++byte) {
            bits |= static_cast<Unsigned>(bytes[offset++]) << (8U * byte);
        }
        return static_cast<Value>(static_cast<Raw>(bits));
    }

    size_t count() {
        return integer<uint32_t>();
    }

    void require_end() const {
        if (offset != bytes.size()) {
            throw std::runtime_error("Persistent solver snapshot has trailing data");
        }
    }

private:
    const std::vector<uint8_t>& bytes;
    size_t offset = 0;
};

}

class SolverStateCodecAccess {
public:
    static void write_gold(Writer& out, const DeferredGoldFunction& gold);
    static DeferredGoldFunction read_gold(Reader& in);
    static void write_deferred(
        Writer& out, const SolverState& state, bool provenance
    );
    static void read_deferred_snapshot(Reader& in, SolverState& state);
    static void read_deferred_semantic(Reader& in, SolverState& state);
    static void write_provenance(Writer& out, const SolverState& state);
    static void apply_provenance(Reader& in, SolverState& state);
};

namespace {

void write_buy(Writer& out, const buy_list& buy) {
    out.integer(buy.food);
    out.integer(buy.spice);
    out.integer(buy.pottery);
    out.integer(buy.marble);
    out.integer(buy.silk);
    out.integer(buy.jewelry);
}

buy_list read_buy(Reader& in) {
    // Function-argument evaluation order is not left-to-right in C++17. Read
    // sequentially so stream position cannot permute the six item quantities.
    const uint8_t food = in.integer<uint8_t>();
    const uint8_t spice = in.integer<uint8_t>();
    const uint8_t pottery = in.integer<uint8_t>();
    const uint8_t marble = in.integer<uint8_t>();
    const uint8_t silk = in.integer<uint8_t>();
    const uint8_t jewelry = in.integer<uint8_t>();
    return buy_list(food, spice, pottery, marble, silk, jewelry);
}

void write_action(Writer& out, const SolverAction& action) {
    out.integer(action.action);
    write_buy(out, action.buy);
    out.integer(action.option);
}

SolverAction read_action(Reader& in) {
    SolverAction action;
    action.action = in.integer<game_actions>();
    action.buy = read_buy(in);
    action.option = in.integer<uint8_t>();
    return action;
}

void write_actions(Writer& out, const std::vector<SolverAction>& actions) {
    out.count(actions.size());
    for (const SolverAction& action : actions) {
        write_action(out, action);
    }
}

std::vector<SolverAction> read_actions(Reader& in) {
    std::vector<SolverAction> actions(in.count());
    for (SolverAction& action : actions) {
        action = read_action(in);
    }
    return actions;
}

void write_player(Writer& out, const Player& player) {
    out.integer<int32_t>(player.gold);
    out.integer<int32_t>(player.food);
    out.integer<int32_t>(player.spice);
    out.integer<int32_t>(player.pottery);
    out.integer<int32_t>(player.marble);
    out.integer<int32_t>(player.silk);
    out.integer<int32_t>(player.jewelry);
    out.integer<int32_t>(player.trader);
    out.integer<int32_t>(player.camel);
    out.integer<int32_t>(player.food_consumption);
    out.integer<int32_t>(player.bonus_food_consumption);
    out.integer<int32_t>(player.backpack);
    out.integer<int32_t>(player.daily_income);
    out.integer<int32_t>(static_cast<int32_t>(std::llround(player.interest_rate * 100.0)));
    uint32_t flags = 0;
    flags |= static_cast<uint32_t>(player.set_interest) << 0U;
    flags |= static_cast<uint32_t>(player.quick_interest) << 1U;
    flags |= static_cast<uint32_t>(player.statue) << 2U;
    flags |= static_cast<uint32_t>(player.hand_of_midas) << 3U;
    flags |= static_cast<uint32_t>(player.merch_discount) << 4U;
    flags |= static_cast<uint32_t>(player.trader_interest) << 5U;
    flags |= static_cast<uint32_t>(player.traveler_mode) << 6U;
    flags |= static_cast<uint32_t>(player.merchant_happened) << 7U;
    flags |= static_cast<uint32_t>(player.call_merchant) << 8U;
    flags |= static_cast<uint32_t>(player.witch_happened) << 9U;
    flags |= static_cast<uint32_t>(player.disaster_happened) << 10U;
    flags |= static_cast<uint32_t>(player.day_start_skip) << 11U;
    flags |= static_cast<uint32_t>(player.bought_last) << 12U;
    flags |= static_cast<uint32_t>(player.cargo_buy_locked) << 13U;
    out.integer(flags);
    out.integer<int32_t>(player.day);
    out.integer<int32_t>(player.max_day);
    out.integer<int32_t>(player.merchant_day);
    out.integer<int32_t>(player.witch_day);
    out.integer<int32_t>(player.disaster_day);
    out.integer(player.scheduled_disaster);
    out.integer(player.player_world.town);
    out.integer(player.player_world.merch);
    out.integer(player.player_world.witch);
    out.integer(player.player_world.disaster);
    for (int item = i_Food; item <= i_Jewelry; ++item) {
        for (int datum = id_Buy; datum <= id_Weight; ++datum) {
            out.integer<int32_t>(player.item_shop[item][datum]);
        }
    }
    for (int npc = n_Trader; npc <= n_Camel; ++npc) {
        for (int datum = nd_Cost; datum <= nd_Eat; ++datum) {
            out.integer<int32_t>(player.npc_shop[npc][datum]);
        }
    }
}

Player read_player(Reader& in) {
    Player player;
    player.gold = in.integer<int32_t>();
    player.food = in.integer<int32_t>();
    player.spice = in.integer<int32_t>();
    player.pottery = in.integer<int32_t>();
    player.marble = in.integer<int32_t>();
    player.silk = in.integer<int32_t>();
    player.jewelry = in.integer<int32_t>();
    player.trader = in.integer<int32_t>();
    player.camel = in.integer<int32_t>();
    player.food_consumption = in.integer<int32_t>();
    player.bonus_food_consumption = in.integer<int32_t>();
    player.backpack = in.integer<int32_t>();
    player.daily_income = in.integer<int32_t>();
    player.interest_rate = static_cast<double>(in.integer<int32_t>()) / 100.0;
    const uint32_t flags = in.integer<uint32_t>();
    player.set_interest = (flags & (1U << 0U)) != 0;
    player.quick_interest = (flags & (1U << 1U)) != 0;
    player.statue = (flags & (1U << 2U)) != 0;
    player.hand_of_midas = (flags & (1U << 3U)) != 0;
    player.merch_discount = (flags & (1U << 4U)) != 0;
    player.trader_interest = (flags & (1U << 5U)) != 0;
    player.traveler_mode = (flags & (1U << 6U)) != 0;
    player.merchant_happened = (flags & (1U << 7U)) != 0;
    player.call_merchant = (flags & (1U << 8U)) != 0;
    player.witch_happened = (flags & (1U << 9U)) != 0;
    player.disaster_happened = (flags & (1U << 10U)) != 0;
    player.day_start_skip = (flags & (1U << 11U)) != 0;
    player.bought_last = (flags & (1U << 12U)) != 0;
    player.cargo_buy_locked = (flags & (1U << 13U)) != 0;
    player.day = in.integer<int32_t>();
    player.max_day = in.integer<int32_t>();
    player.merchant_day = in.integer<int32_t>();
    player.witch_day = in.integer<int32_t>();
    player.disaster_day = in.integer<int32_t>();
    player.scheduled_disaster = in.integer<disaster_options>();
    player.player_world.town = in.integer<uint8_t>();
    player.player_world.merch = in.integer<uint8_t>();
    player.player_world.witch = in.integer<uint8_t>();
    player.player_world.disaster = in.integer<uint8_t>();
    for (int item = i_Food; item <= i_Jewelry; ++item) {
        for (int datum = id_Buy; datum <= id_Weight; ++datum) {
            player.item_shop[item][datum] = in.integer<int32_t>();
        }
    }
    for (int npc = n_Trader; npc <= n_Camel; ++npc) {
        for (int datum = nd_Cost; datum <= nd_Eat; ++datum) {
            player.npc_shop[npc][datum] = in.integer<int32_t>();
        }
    }
    return player;
}

void write_state_base(Writer& out, const SolverState& state, const bool provenance) {
    out.integer(state.gold);
    out.integer(state.food);
    out.integer(state.spice);
    out.integer(state.pottery);
    out.integer(state.marble);
    out.integer(state.silk);
    out.integer(state.jewelry);
    out.integer(state.trader);
    out.integer(state.camel);
    out.integer(state.food_consumption);
    out.integer(state.bonus_food_consumption);
    out.integer(state.backpack);
    out.integer(state.daily_income);
    out.integer(state.day);
    out.integer(state.max_day);
    out.integer(state.interest_hundredths);
    out.integer(state.disaster_day);
    out.integer(state.scheduled_disaster);
    out.integer(state.merchant_day);
    out.integer(state.witch_day);
    out.integer(state.player_world.town);
    out.integer(state.player_world.merch);
    out.integer(state.player_world.witch);
    out.integer(state.player_world.disaster);
    out.integer(state.flags);
    for (const int16_t value : state.item_shop) out.integer(value);
    for (const int16_t value : state.npc_shop) out.integer(value);
    if (provenance) out.integer(state.trace_id);
}

SolverState read_state_base(Reader& in, const bool provenance) {
    SolverState state;
    state.gold = in.integer<int32_t>();
    state.food = in.integer<uint16_t>();
    state.spice = in.integer<uint16_t>();
    state.pottery = in.integer<uint16_t>();
    state.marble = in.integer<uint16_t>();
    state.silk = in.integer<uint16_t>();
    state.jewelry = in.integer<uint16_t>();
    state.trader = in.integer<int16_t>();
    state.camel = in.integer<int16_t>();
    state.food_consumption = in.integer<int16_t>();
    state.bonus_food_consumption = in.integer<int16_t>();
    state.backpack = in.integer<int16_t>();
    state.daily_income = in.integer<int16_t>();
    state.day = in.integer<uint8_t>();
    state.max_day = in.integer<uint8_t>();
    state.interest_hundredths = in.integer<int16_t>();
    state.disaster_day = in.integer<uint8_t>();
    state.scheduled_disaster = in.integer<uint8_t>();
    state.merchant_day = in.integer<uint8_t>();
    state.witch_day = in.integer<uint8_t>();
    state.player_world.town = in.integer<uint8_t>();
    state.player_world.merch = in.integer<uint8_t>();
    state.player_world.witch = in.integer<uint8_t>();
    state.player_world.disaster = in.integer<uint8_t>();
    state.flags = in.integer<uint16_t>();
    for (int16_t& value : state.item_shop) value = in.integer<int16_t>();
    for (int16_t& value : state.npc_shop) value = in.integer<int16_t>();
    state.trace_id = provenance ? in.integer<uint32_t>() : 0;
    return state;
}

}

void SolverStateCodecAccess::write_gold(
    Writer& out, const DeferredGoldFunction& gold
) {
    out.integer<int32_t>(gold.zero_spend_gold);
    out.integer<int32_t>(gold.maximum_spend);
    out.count(gold.operations.size());
    for (const auto& operation : gold.operations) {
        out.integer(operation.type);
        out.integer<int32_t>(operation.value);
    }
}

DeferredGoldFunction SolverStateCodecAccess::read_gold(Reader& in) {
    const int zero = in.integer<int32_t>();
    const int maximum = in.integer<int32_t>();
    DeferredGoldFunction result(zero, maximum);
    const size_t count = in.count();
    result.operations.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        result.operations.push_back(DeferredGoldFunction::Operation{
            in.integer<DeferredGoldFunction::OperationType>(),
            in.integer<int32_t>(),
        });
    }
    return result;
}

void SolverStateCodecAccess::write_deferred(
    Writer& out, const SolverState& state, const bool provenance
) {
    if (!state.deferred) {
        out.integer<uint8_t>(0);
        return;
    }
    if (state.deferred->has_open_purchase()) {
        out.integer<uint8_t>(1);
        const DeferredPurchaseModel& purchase = state.deferred->purchase();
        if (provenance) {
            write_player(out, *purchase.buy_market);
        } else {
            // Match DeferredPurchaseModel::semantically_equal exactly. Only
            // the pinned Buy prices and weights distinguish the Buy market.
            for (int item = i_Food; item <= i_Jewelry; ++item) {
                out.integer<int32_t>(purchase.buy_market->item_shop[item][id_Buy]);
                out.integer<int32_t>(purchase.buy_market->item_shop[item][id_Weight]);
            }
        }
        out.integer<int32_t>(purchase.cumulative_food_used);
        out.integer<int32_t>(purchase.minimum_food_coverage);
        out.count(purchase.origin_rows().size());
        for (const auto& origin : purchase.origin_rows()) {
            SolverStateCodecAccess::write_gold(out, origin.gold);
            out.integer<int32_t>(origin.food_before_buy);
            out.integer<int32_t>(origin.free_weight_before_buy);
            if (provenance) out.integer(origin.parent_trace);
        }
        if (provenance) {
            out.count(purchase.seeds_before_compaction);
            write_actions(out, purchase.trace.actions_after_buy());
        }
        return;
    }

    out.integer<uint8_t>(2);
    const DeferredSoldModel& sold = state.deferred->sold();
    out.count(sold.rows->size());
    for (const auto& row : *sold.rows) {
        out.integer<int32_t>(row.food);
        out.integer<int32_t>(row.gold);
        if (provenance) {
            write_buy(out, row.concrete_buy);
            out.integer(row.parent_trace);
        }
    }
    if (provenance) {
        write_actions(out, sold.purchase_trace.actions_after_buy());
        write_actions(out, sold.actions_after_sell);
    }
}

void SolverStateCodecAccess::read_deferred_snapshot(Reader& in, SolverState& state) {
    const uint8_t kind = in.integer<uint8_t>();
    if (kind == 0) return;
    if (kind == 1) {
        Player buy_market = read_player(in);
        const int cumulative_food_used = in.integer<int32_t>();
        const int minimum_food_coverage = in.integer<int32_t>();
        const size_t origin_count = in.count();
        std::vector<DeferredPurchaseModel::Origin> origins;
        origins.reserve(origin_count);
        for (size_t index = 0; index < origin_count; ++index) {
            origins.push_back(DeferredPurchaseModel::Origin{
                SolverStateCodecAccess::read_gold(in),
                in.integer<int32_t>(),
                in.integer<int32_t>(),
                in.integer<uint32_t>(),
            });
        }
        const size_t seeds = in.count();
        DeferredTraceRecipe trace;
        for (const SolverAction& action : read_actions(in)) trace.append(action);
        DeferredPurchaseModel purchase(
            std::make_shared<const Player>(std::move(buy_market)),
            state.to_player(), std::move(origins), seeds
        );
        purchase.cumulative_food_used = cumulative_food_used;
        purchase.minimum_food_coverage = minimum_food_coverage;
        purchase.trace = std::move(trace);
        state.deferred = std::make_shared<const DeferredSearchState>(std::move(purchase));
        return;
    }
    if (kind == 2) {
        const size_t row_count = in.count();
        std::vector<DeferredSoldModel::EnvelopeRow> rows;
        rows.reserve(row_count);
        for (size_t index = 0; index < row_count; ++index) {
            rows.push_back(DeferredSoldModel::EnvelopeRow{
                in.integer<int32_t>(), in.integer<int32_t>(), read_buy(in),
                in.integer<uint32_t>(),
            });
        }
        DeferredTraceRecipe purchase_trace;
        for (const SolverAction& action : read_actions(in)) purchase_trace.append(action);
        std::vector<SolverAction> after_sell = read_actions(in);
        DeferredSoldModel sold(state.to_player(), std::move(rows), std::move(purchase_trace));
        sold.actions_after_sell = std::move(after_sell);
        state.deferred = std::make_shared<const DeferredSearchState>(std::move(sold));
        return;
    }
    throw std::runtime_error("Unknown deferred state kind in persistent snapshot");
}

void SolverStateCodecAccess::read_deferred_semantic(Reader& in, SolverState& state) {
    const uint8_t kind = in.integer<uint8_t>();
    if (kind == 0) return;
    if (kind == 1) {
        Player buy_market = state.to_player();
        for (int item = i_Food; item <= i_Jewelry; ++item) {
            buy_market.item_shop[item][id_Buy] = in.integer<int32_t>();
            buy_market.item_shop[item][id_Weight] = in.integer<int32_t>();
        }
        const int cumulative_food_used = in.integer<int32_t>();
        const int minimum_food_coverage = in.integer<int32_t>();
        const size_t origin_count = in.count();
        std::vector<DeferredPurchaseModel::Origin> origins;
        origins.reserve(origin_count);
        for (size_t index = 0; index < origin_count; ++index) {
            origins.push_back(DeferredPurchaseModel::Origin{
                SolverStateCodecAccess::read_gold(in),
                in.integer<int32_t>(),
                in.integer<int32_t>(),
                0,
            });
        }
        DeferredPurchaseModel purchase(
            std::make_shared<const Player>(std::move(buy_market)),
            state.to_player(), std::move(origins), origin_count
        );
        purchase.cumulative_food_used = cumulative_food_used;
        purchase.minimum_food_coverage = minimum_food_coverage;
        state.deferred = std::make_shared<const DeferredSearchState>(std::move(purchase));
        return;
    }
    if (kind == 2) {
        const size_t row_count = in.count();
        std::vector<DeferredSoldModel::EnvelopeRow> rows;
        rows.reserve(row_count);
        for (size_t index = 0; index < row_count; ++index) {
            rows.push_back(DeferredSoldModel::EnvelopeRow{
                in.integer<int32_t>(), in.integer<int32_t>(), buy_list(), 0,
            });
        }
        DeferredSoldModel sold(state.to_player(), std::move(rows), DeferredTraceRecipe{});
        state.deferred = std::make_shared<const DeferredSearchState>(std::move(sold));
        return;
    }
    throw std::runtime_error("Unknown deferred state kind in persistent semantic key");
}

void SolverStateCodecAccess::write_provenance(Writer& out, const SolverState& state) {
    out.integer(state.trace_id);
    if (!state.deferred) {
        out.integer<uint8_t>(0);
        return;
    }
    if (state.deferred->has_open_purchase()) {
        out.integer<uint8_t>(1);
        const DeferredPurchaseModel& purchase = state.deferred->purchase();
        out.count(purchase.origin_rows().size());
        for (const auto& origin : purchase.origin_rows()) {
            out.integer(origin.parent_trace);
        }
        out.count(purchase.seeds_before_compaction);
        write_actions(out, purchase.trace.actions_after_buy());
        return;
    }

    out.integer<uint8_t>(2);
    const DeferredSoldModel& sold = state.deferred->sold();
    out.count(sold.rows->size());
    for (const auto& row : *sold.rows) {
        write_buy(out, row.concrete_buy);
        out.integer(row.parent_trace);
    }
    write_actions(out, sold.purchase_trace.actions_after_buy());
    write_actions(out, sold.actions_after_sell);
}

void SolverStateCodecAccess::apply_provenance(Reader& in, SolverState& state) {
    state.trace_id = in.integer<uint32_t>();
    const uint8_t kind = in.integer<uint8_t>();
    if (kind == 0) {
        if (state.deferred) {
            throw std::runtime_error("Persistent provenance kind does not match semantic state");
        }
        return;
    }
    if (!state.deferred ||
        (kind == 1) != state.deferred->has_open_purchase() ||
        (kind == 2) != state.deferred->has_sold_ledger()) {
        throw std::runtime_error("Persistent provenance kind does not match semantic state");
    }
    if (kind == 1) {
        DeferredPurchaseModel purchase = state.deferred->purchase();
        const size_t origin_count = in.count();
        if (origin_count != purchase.origin_rows().size()) {
            throw std::runtime_error("Persistent purchase provenance row count mismatch");
        }
        for (auto& origin : purchase.mutable_origins()) {
            origin.parent_trace = in.integer<uint32_t>();
        }
        purchase.seeds_before_compaction = in.count();
        for (const SolverAction& action : read_actions(in)) purchase.trace.append(action);
        state.deferred = std::make_shared<const DeferredSearchState>(std::move(purchase));
        return;
    }
    if (kind == 2) {
        DeferredSoldModel sold = state.deferred->sold();
        const size_t row_count = in.count();
        if (row_count != sold.rows->size()) {
            throw std::runtime_error("Persistent sold provenance row count mismatch");
        }
        auto& rows = sold.mutable_rows();
        for (auto& row : rows) {
            row.concrete_buy = read_buy(in);
            row.parent_trace = in.integer<uint32_t>();
        }
        for (const SolverAction& action : read_actions(in)) sold.purchase_trace.append(action);
        sold.actions_after_sell = read_actions(in);
        state.deferred = std::make_shared<const DeferredSearchState>(std::move(sold));
        return;
    }
    throw std::runtime_error("Unknown deferred state kind in persistent provenance");
}

namespace {

std::vector<uint8_t> encode(const SolverState& state, const bool provenance) {
    Writer out;
    out.integer(SNAPSHOT_MAGIC);
    out.integer(SolverStateCodec::FORMAT_VERSION);
    out.integer<uint8_t>(provenance ? 1 : 0);
    write_state_base(out, state, provenance);
    SolverStateCodecAccess::write_deferred(out, state, provenance);
    return std::move(out.bytes);
}
}

std::vector<uint8_t> SolverStateCodec::semantic_key(const SolverState& state) {
    return encode(state, false);
}

std::vector<uint8_t> SolverStateCodec::snapshot(const SolverState& state) {
    return encode(state, true);
}

std::vector<uint8_t> SolverStateCodec::provenance(const SolverState& state) {
    Writer out;
    out.integer(PROVENANCE_MAGIC);
    out.integer(FORMAT_VERSION);
    SolverStateCodecAccess::write_provenance(out, state);
    return std::move(out.bytes);
}

SolverState SolverStateCodec::restore(const std::vector<uint8_t>& encoded) {
    Reader in(encoded);
    if (in.integer<uint32_t>() != SNAPSHOT_MAGIC ||
        in.integer<uint32_t>() != FORMAT_VERSION ||
        in.integer<uint8_t>() != 1) {
        throw std::runtime_error("Unsupported persistent solver snapshot format");
    }
    SolverState state = read_state_base(in, true);
    SolverStateCodecAccess::read_deferred_snapshot(in, state);
    in.require_end();
    return state;
}

SolverState SolverStateCodec::restore(
    const std::vector<uint8_t>& semantic_key_value,
    const std::vector<uint8_t>& provenance_value
) {
    Reader semantic(semantic_key_value);
    if (semantic.integer<uint32_t>() != SNAPSHOT_MAGIC ||
        semantic.integer<uint32_t>() != FORMAT_VERSION ||
        semantic.integer<uint8_t>() != 0) {
        throw std::runtime_error("Unsupported persistent solver semantic-key format");
    }
    SolverState state = read_state_base(semantic, false);
    SolverStateCodecAccess::read_deferred_semantic(semantic, state);
    semantic.require_end();

    Reader provenance_reader(provenance_value);
    if (provenance_reader.integer<uint32_t>() != PROVENANCE_MAGIC ||
        provenance_reader.integer<uint32_t>() != FORMAT_VERSION) {
        throw std::runtime_error("Unsupported persistent solver provenance format");
    }
    SolverStateCodecAccess::apply_provenance(provenance_reader, state);
    provenance_reader.require_end();
    return state;
}

uint64_t SolverStateCodec::fast_hash(
    const std::vector<uint8_t>& semantic_key_value
) noexcept {
    uint64_t hash = 1469598103934665603ULL;
    for (const uint8_t byte : semantic_key_value) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}
