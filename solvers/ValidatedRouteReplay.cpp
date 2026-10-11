#include "ValidatedRouteReplay.h"

#include "EncounterTiming.h"
#include "../game/Game.h"

namespace {
RouteReplayResult failure(
    const Player& player,
    const size_t action_index,
    const std::string& reason
) {
    RouteReplayResult result;
    result.failed_action = action_index;
    result.reason = reason;
    result.final_player = player;
    return result;
}

int purchase_cost(const Player& player, const buy_list& buy) {
    return buy.food * player.item_shop[i_Food][id_Buy] +
           buy.spice * player.item_shop[i_Spice][id_Buy] +
           buy.pottery * player.item_shop[i_Pottery][id_Buy] +
           buy.marble * player.item_shop[i_Marble][id_Buy] +
           buy.silk * player.item_shop[i_Silk][id_Buy] +
           buy.jewelry * player.item_shop[i_Jewelry][id_Buy];
}

int purchase_weight(const Player& player, const buy_list& buy) {
    return buy.food * player.item_shop[i_Food][id_Weight] +
           buy.spice * player.item_shop[i_Spice][id_Weight] +
           buy.pottery * player.item_shop[i_Pottery][id_Weight] +
           buy.marble * player.item_shop[i_Marble][id_Weight] +
           buy.silk * player.item_shop[i_Silk][id_Weight] +
           buy.jewelry * player.item_shop[i_Jewelry][id_Weight];
}

}

RouteReplayResult ValidatedRouteReplay::run(
    const town_options town,
    const std::vector<ga_data>& route
) {
    Player player;
    Game::apply_town(player, town);
    std::vector<std::string> action_log;

    for (size_t index = 0; index < route.size(); ++index) {
        const ga_data& action = route[index];
        const bool resumed_same_day = player.day_start_skip;
        if (!resumed_same_day) {
            if (player.day >= player.max_day) {
                return failure(player, index, "route continues after the final player day");
            }
            Game::start_of_day(player);
            if (player.food < 0) {
                return failure(player, index, "food became negative at the start of the day");
            }
        } else {
            player.day_start_skip = false;
        }

        const EncounterOpportunity merchant_timing = EncounterTiming::merchant(player);
        const bool merchant_due = !resumed_same_day &&
            !player.merchant_happened && merchant_timing.due;
        const EncounterOpportunity witch_timing = EncounterTiming::witch(player);
        const bool witch_due = !resumed_same_day &&
            !player.witch_happened && witch_timing.due;
        const bool disaster_due = !resumed_same_day && !player.disaster_happened &&
            player.scheduled_disaster != d_No_Disaster &&
            player.day == player.disaster_day;

        if (merchant_due && !merchant_timing.may_defer && action.action != _a_merchant) {
            return failure(player, index, "the final Merchant opportunity must be resolved first");
        }
        if (witch_due && !witch_timing.may_defer && action.action != _a_witch) {
            return failure(player, index, "the final Witch opportunity must be resolved first");
        }
        if (disaster_due && action.action != _a_disaster) {
            return failure(player, index, "the scheduled disaster must be resolved first");
        }

        switch (action.action) {
            case _a_merchant: {
                if (!merchant_due) {
                    return failure(player, index, "Merchant is not available on this day");
                }
                if (action.merch_data == m_No_Merchant && player.merchant_day == 0) {
                    return failure(player, index, "an open-timing Merchant cannot be declined as an action");
                }
                Player affordability = player;
                if (action.merch_data != m_No_Merchant &&
                    !Game::check__afford_merchant(affordability, action.merch_data)) {
                    return failure(player, index, "Merchant option is unaffordable");
                }
                if (!Game::event_merchant(player, action.merch_data)) {
                    return failure(player, index, "Merchant option failed to apply");
                }
                break;
            }
            case _a_witch:
                if (!witch_due) {
                    return failure(player, index, "Witch is not available on this day");
                }
                Game::event_witch(player, action.witch_data);
                break;
            case _a_disaster:
                if (!disaster_due || action.disaster_data != player.scheduled_disaster) {
                    return failure(player, index, "disaster does not match the scheduled event");
                }
                Game::event_disaster(player, action.disaster_data);
                break;
            case _a_buy:
            case _a_wait: {
                if (player.day >= player.max_day || player.bought_last ||
                    !Game::check_can_buy_items(player)) {
                    return failure(player, index, "Buy is not legal in the current state");
                }
                const buy_list buy = action.action == _a_wait ? buy_list() : action.buy_data;
                if (purchase_cost(player, buy) > player.gold) {
                    return failure(player, index, "Buy is unaffordable");
                }
                if (purchase_weight(player, buy) > Game::check_weight_left(player)) {
                    return failure(player, index, "Buy exceeds cargo capacity");
                }
                if (action.action == _a_wait) {
                    Game::event_wait(player);
                } else {
                    Game::event_buy_items(player, buy);
                }
                break;
            }
            case _a_sell:
                if (!(player.day == player.max_day || player.food >= player.food_consumption)) {
                    return failure(player, index, "Sell leaves too little food for the next day");
                }
                if (!Game::check_has_items(player)) {
                    return failure(player, index, "Sell requires cargo");
                }
                Game::event_sell_items(player);
                break;
            case _a_npc: {
                if (!(player.day == player.max_day || player.food >= player.food_consumption)) {
                    return failure(player, index, "crew purchase leaves too little food for the next day");
                }
                if (player.day >= player.max_day) {
                    return failure(player, index, "crew cannot be hired on the final day");
                }
                if (action.npc_data != n_Trader && action.npc_data != n_Camel) {
                    return failure(player, index, "unknown crew type");
                }
                const int npc = static_cast<int>(action.npc_data);
                if (player.gold < player.npc_shop[npc][nd_Cost]) {
                    return failure(player, index, "crew purchase is unaffordable");
                }
                if (player.food < player.food_consumption + player.npc_shop[npc][nd_Eat]) {
                    return failure(player, index, "crew purchase cannot feed the enlarged caravan");
                }
                Game::event_buy_npc(player, action.npc_data);
                break;
            }
            case _a_interest:
                if (!(player.day == player.max_day || player.food >= player.food_consumption)) {
                    return failure(player, index, "Interest leaves too little food for the next day");
                }
                Game::event_interest(player);
                break;
        }
        action_log.push_back(player.last_action);
    }

    if (player.day != player.max_day) {
        return failure(player, route.size(), "route does not consume every player day");
    }
    if (player.day_start_skip) {
        return failure(player, route.size(), "route ends after a same-day event without a player action");
    }

    RouteReplayResult result;
    result.legal = true;
    result.score = Game::check_points(player, "Final");
    result.failed_action = route.size();
    result.final_player = player;
    result.action_log = std::move(action_log);
    return result;
}
