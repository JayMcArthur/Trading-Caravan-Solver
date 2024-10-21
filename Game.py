from Enums import TownOptions, PlayerAction, Items, ItemData, NPCData, NPCs, GameActions, MerchOptions, \
    merch_options_string, WitchOptions, witch_options_string
from dataclasses import replace as dc_replace
from player import Player
from copy import deepcopy


def apply_town(player: Player, town: TownOptions):
    match town:
        case TownOptions.Normalia:  # 00 - No Effect
            pass
        case TownOptions.Fang_Mo:  # 01 - Begin with 1 Pottery
            player = dc_replace(player, fang_mo_pot=1)
        case TownOptions.L_Exquise:  # 02 - Traders don't need Food
            npc_data = deepcopy(player.npc_shop)
            consumption = player.food_consumption - (npc_data[NPCs.Trader][NPCData.eat] * player.trader)
            npc_data[NPCs.Trader][NPCData.eat] = 0
            player = dc_replace(player, npc_shop=npc_data, food_consumption=consumption)
        case TownOptions.Mahaji:  # 03 - Spice sell +5
            item_data = deepcopy(player.item_shop)
            item_data[Items.Spice][ItemData.sell] += 5
            player = dc_replace(player, item_shop=item_data)
        case TownOptions.Tocaccialli:  # 04 - Max Day +1
            player = dc_replace(player, max_day=player.max_day + 1)
        case TownOptions.Bajaar:  # 05 - x2 Trader income
            npc_data = deepcopy(player.npc_shop)
            npc_data[NPCs.Trader][NPCData.effect] += 20
            player = dc_replace(player, npc_shop=npc_data)
        case TownOptions.Doba_Lao:  # 06 - Interest Rate +20%
            player = dc_replace(player, interest_rate=player.interest_rate + 0.2)
        case TownOptions.Huangoz:  # 07 - Marble Cost -20
            item_data = deepcopy(player.item_shop)
            item_data[Items.Marble][ItemData.buy] -= 20
            player = dc_replace(player, item_shop=item_data)
        case TownOptions.Port_Willam:  # 08 - Carry Weight +15
            player = dc_replace(player, backpack=player.backpack + 15)
        case TownOptions.Moonin:  # 09 - Call Merchant at anytime
            player = dc_replace(player, call_merchant=True)
        case TownOptions.Cornecopea:  # 10 - Food is free
            item_data = deepcopy(player.item_shop)
            item_data[Items.Food][ItemData.buy] = 0
            player = dc_replace(player, item_shop=item_data)
        case TownOptions.Eduming:  # 11 - Buying trader > Interest Rate +10%
            player = dc_replace(player, trader_interest=True)
        case TownOptions.Octoyashi:  # 12 - Begin with +3 Food
            player = dc_replace(player, food=player.food + 3)
        case TownOptions.Kifuai:  # 13 - Begin with +10 Gold
            player = dc_replace(player, gold=player.gold + 10)
        case TownOptions.Skjollird:  # 14 - Camel cost +5 and weight +10
            npc_data = deepcopy(player.npc_shop)
            npc_data[NPCs.Camel][NPCData.cost] += 5
            npc_data[NPCs.Camel][NPCData.effect] += 10
            player = dc_replace(player, npc_shop=npc_data)
        case TownOptions.Petrinov:  # 15 - Interest rate doesn't decrease
            player = dc_replace(player, set_interest=True)
        case TownOptions.Jilliqo:  # 16 - Daily Gold Income +10
            player = dc_replace(player, daily_income=player.daily_income + 10)
        case TownOptions.Gayawaku:  # 17 - Food is weightless
            item_data = deepcopy(player.item_shop)
            item_data[Items.Food][ItemData.weight] = 0
            player = dc_replace(player, item_shop=item_data)
        case TownOptions.Vilparino:  # 18 - Strange Merchant 20% Sale!
            player = dc_replace(player, merch_discount=True)
        case TownOptions.Sanctifan:  # 19 - One Free Collect Interest Action
            player = dc_replace(player, quick_interest=True)
        case TownOptions.Mehaz:  # 20 - Camels don't need food
            npc_data = deepcopy(player.npc_shop)
            consumption = player.food_consumption - (npc_data[NPCs.Camel][NPCData.eat] * player.camel)
            npc_data[NPCs.Camel][NPCData.eat] = 0
            player = dc_replace(player, npc_shop=npc_data, food_consumption=consumption)
        case TownOptions.Ipartus:  # 21 - Silk Cost -100
            item_data = deepcopy(player.item_shop)
            item_data[Items.Silk][ItemData.buy] -= 100
            player = dc_replace(player, item_shop=item_data)
    return player


def auto_game_loop(player: Player, event_list: list[PlayerAction]):
    for event in event_list:
        player = full_day(player, event)
    return player


def full_day(player: Player, event: PlayerAction):
    if not player.day_start_skip:
        player = start_of_day(player)
    else:
        player = dc_replace(player, day_start_skip=False)

    match event.action:
        case GameActions.a_buy:
            player = event_buy_items(player, *event.buy_data)
        case GameActions.a_sell:
            player = event_sell_items(player)
        case GameActions.a_npc:
            player = event_buy_npc(player, event.npc_data)
        case GameActions.a_interest:
            player = event_interest(player)
        case GameActions.a_merchant:
            player = event_merchant(player, event.merch_data)
        case GameActions.a_witch:
            player = event_witch(player, event.witch_data)
    return player


def start_of_day(player: Player, error: bool = True):
    new_food = player.food
    if player.day != 0:
        new_food -= player.food_consumption
        if new_food < 0 and error:
            raise Exception(f'Zero Food')
    return dc_replace(player, day=player.day + 1, food=new_food)


def end_of_day(player: Player, action):
    new_gold = player.gold + player.daily_income + player.trader * player.npc_shop[NPCs.Trader][NPCData.effect]
    if player.day != player.max_day:
        c_type = "Day End"
    else:
        c_type = "Final"
    player = dc_replace(player, gold=new_gold, actions=player.actions + [action],
                        item_shop_hashable=str(player.item_shop), npc_shop_hashable=str(player.npc_shop))
    return dc_replace(player, points=check__points(player, c_type))


# TODO - This needs to be double checked
def find_buy(player: Player, food_needed: int, only_food: bool = False):
    # Food, Spice, Pottery, Marble, Silk, Jewelry
    what_to_buy = [food_needed, 0, 0, 0, 0, 0]
    gold_left = player.gold - player.item_shop[Items.Food][ItemData.buy] * food_needed
    weight_left = check__weight_left(player) - player.item_shop[Items.Food][ItemData.weight] * food_needed

    if not only_food:
        items = [
            [0.0, Items.Food],
            [check__item_worth(player, Items.Spice), Items.Spice],
            [check__item_worth(player, Items.Pottery), Items.Pottery],
            [check__item_worth(player, Items.Marble), Items.Marble],
            [check__item_worth(player, Items.Silk), Items.Silk],
            [check__item_worth(player, Items.Jewelry), Items.Jewelry]
        ]
        items.sort(key=lambda x: x[0], reverse=True)
        for [value, item] in items:
            if item == Items.Food:
                continue
            while weight_left >= player.item_shop[item][ItemData.weight] and gold_left >= player.item_shop[item][
                ItemData.buy]:
                weight_left -= player.item_shop[item][ItemData.weight]
                gold_left -= player.item_shop[item][ItemData.buy]
                what_to_buy[item] += 1

    return event_buy_items(player, *what_to_buy)


def event_buy_items(player: Player, food, spice, pottery, marble, silk, jewelry):
    new_gold = player.gold - (
            food * player.item_shop[Items.Food][ItemData.buy] +
            spice * player.item_shop[Items.Spice][ItemData.buy] +
            pottery * player.item_shop[Items.Pottery][ItemData.buy] +
            marble * player.item_shop[Items.Marble][ItemData.buy] +
            silk * player.item_shop[Items.Silk][ItemData.buy] +
            jewelry * player.item_shop[Items.Jewelry][ItemData.buy])
    if new_gold < 0:
        raise Exception(f'Negative Gold')

    new_food = player.food + food
    new_spice = player.spice + spice
    new_pottery = player.pottery + pottery
    new_marble = player.marble + marble
    new_silk = player.silk + silk
    new_jewelry = player.jewelry + jewelry

    player = dc_replace(player, bought_last=True, gold=new_gold, food=new_food, spice=new_spice, pottery=new_pottery,
                        marble=new_marble, silk=new_silk, jewelry=new_jewelry)
    action = f"{player.day}: Buy [{food}, {spice}, {pottery}, {marble}, {silk}, {jewelry}]"
    return end_of_day(player, action)


def event_sell_items(player: Player):
    new_gold = player.gold + (
            player.spice * player.item_shop[Items.Spice][ItemData.sell] +
            (player.pottery + player.fang_mo_pot) * player.item_shop[Items.Pottery][ItemData.sell] +
            player.marble * player.item_shop[Items.Marble][ItemData.sell] +
            player.silk * player.item_shop[Items.Silk][ItemData.sell] +
            player.jewelry * player.item_shop[Items.Jewelry][ItemData.sell] +
            player.hand_of_midas * 100)
    player = dc_replace(player, bought_last=False, gold=new_gold, spice=0, pottery=0, marble=0, silk=0, jewelry=0,
                        fang_mo_pot=0)
    action = f"{player.day}: Sell"
    return end_of_day(player, action)


def event_buy_npc(player: Player, selection):
    if selection == NPCs.Trader and player.gold >= player.npc_shop[NPCs.Trader][NPCData.cost]:
        new_gold = player.gold - player.npc_shop[NPCs.Trader][NPCData.cost]
        new_consumption = player.food_consumption + player.npc_shop[NPCs.Trader][NPCData.eat]
        new_interest = player.interest_rate + player.trader_interest * 0.1
        player = dc_replace(player, bought_last=False, gold=new_gold, trader=player.trader + 1,
                            interest_rate=new_interest, food_consumption=new_consumption)
        action = f"{player.day}: Trader"
    elif selection == NPCs.Camel and player.gold >= player.npc_shop[NPCs.Camel][NPCData.cost]:
        new_gold = player.gold - player.npc_shop[NPCs.Camel][NPCData.cost]
        new_consumption = player.food_consumption + player.npc_shop[NPCs.Camel][NPCData.eat]
        player = dc_replace(player, bought_last=False, gold=new_gold, camel=player.camel + 1,
                            food_consumption=new_consumption)
        action = f"{player.day}: Camel"
    else:
        return None
    return end_of_day(player, action)


def event_interest(player: Player):
    new_gold = round(player.interest_rate * player.gold + 0.25)
    new_interest = player.interest_rate
    if not (player.set_interest or player.quick_interest):
        new_interest = max(0.1, new_interest - 0.1)
    player = dc_replace(player, gold=new_gold, interest_rate=new_interest)

    if not player.quick_interest:
        player = dc_replace(player, bought_last=False)
        action = f"{player.day}: Interest"
        return end_of_day(player, action)
    # TODO this should update points
    return dc_replace(player, day_start_skip=True, quick_interest=False,
                      actions=player.actions + [f"{player.day}: Quick Interest"], )


def event_merchant(player: Player, selection: MerchOptions):
    match selection:
        case MerchOptions.Cornucopia:  # +2 food every turn
            if player.gold >= 25 - (5 * player.merch_discount):
                new_gold = player.gold - (25 - (5 * player.merch_discount))
                new_consumption = max(player.food_consumption - 2, 0)
                return dc_replace(player, merchant_happened=True, gold=new_gold, food_consumption=new_consumption,
                                  day_start_skip=True,
                                  actions=player.actions + [f'Merch - {merch_options_string[selection]}'])
        case MerchOptions.Route_to_Mahaji:  # Spice is worth $20 more
            if player.gold >= 25 - (5 * player.merch_discount):
                new_gold = player.gold - (25 - (5 * player.merch_discount))
                item_data = deepcopy(player.item_shop)
                item_data[Items.Spice][ItemData.sell] += 20
                return dc_replace(player, merchant_happened=True, gold=new_gold, item_shop=item_data,
                                  day_start_skip=True,
                                  actions=player.actions + [f'Merch - {merch_options_string[selection]}'])
        case MerchOptions.Wooden_Statuette:  # +1 Statue (aka +500 per Trader and Camel)
            if player.gold >= 25 - (5 * player.merch_discount):
                new_gold = player.gold - (25 - (5 * player.merch_discount))
                return dc_replace(player, merchant_happened=True, gold=new_gold, statue=True, day_start_skip=True,
                                  actions=player.actions + [f'Merch - {merch_options_string[selection]}'])
        case MerchOptions.Canvas_Bag:  # +20 storage
            if player.gold >= 50 - (10 * player.merch_discount):
                new_gold = player.gold - (50 - (10 * player.merch_discount))
                return dc_replace(player, merchant_happened=True, gold=new_gold, backpack=player.backpack + 20,
                                  day_start_skip=True,
                                  actions=player.actions + [f'Merch - {merch_options_string[selection]}'])
        case MerchOptions.Leaders_Necklace:  # +1 Trader
            if player.gold >= 50 - (10 * player.merch_discount):
                new_gold = player.gold - (50 - (10 * player.merch_discount))
                new_interest = player.interest_rate + player.trader_interest * 0.1
                return dc_replace(player, merchant_happened=True, gold=new_gold, trader=player.trader + 1,
                                  interest_rate=new_interest, day_start_skip=True,
                                  actions=player.actions + [f'Merch - {merch_options_string[selection]}'])
        case MerchOptions.Hand_of_Midas:  # 100g when you sell everything
            if player.gold >= 50 - (10 * player.merch_discount):
                new_gold = player.gold - (50 - (10 * player.merch_discount))
                return dc_replace(player, merchant_happened=True, gold=new_gold, hand_of_midas=True,
                                  day_start_skip=True,
                                  actions=player.actions + [f'Merch - {merch_options_string[selection]}'])
        case MerchOptions.Sturdy_Saddle:  # Camels carry an extra 20
            if player.gold >= 100 - (20 * player.merch_discount):
                new_gold = player.gold - (100 - (20 * player.merch_discount))
                npc_data = deepcopy(player.npc_shop)
                npc_data[NPCs.Camel][NPCData.effect] += 20
                return dc_replace(player, merchant_happened=True, gold=new_gold, npc_shop=npc_data, day_start_skip=True,
                                  actions=player.actions + [f'Merch - {merch_options_string[selection]}'])
        case MerchOptions.Magic_Cleppsydra:  # 1 more day to trade
            if player.gold >= 100 - (20 * player.merch_discount):
                new_gold = player.gold - (100 - (20 * player.merch_discount))
                return dc_replace(player, merchant_happened=True, gold=new_gold, max_day=player.max_day + 1,
                                  day_start_skip=True,
                                  actions=player.actions + [f'Merch - {merch_options_string[selection]}'])
        case MerchOptions.Blue_Treasure:  # An extra 20% from saving
            if player.gold >= 100 - (20 * player.merch_discount):
                new_gold = player.gold - (100 - (20 * player.merch_discount))
                return dc_replace(player, merchant_happened=True, gold=new_gold,
                                  interest_rate=player.interest_rate + 0.2, day_start_skip=True,
                                  actions=player.actions + [f'Merch - {merch_options_string[selection]}'])


def event_witch(player: Player, selection: WitchOptions):
    match selection:
        case WitchOptions.Presents_Gift:  # +20 weight, -20% Saving
            new_interest = max(player.interest_rate - 0.2, 0.1)
            return dc_replace(player, witch_happened=True, backpack=player.backpack + 20, interest_rate=new_interest,
                              day_start_skip=True,
                              actions=player.actions + [f'Witch - {witch_options_string[selection]}'])
        case WitchOptions.Vertue_of_Patience:  # Saving doesn't decrease, +100 Jewelry cost
            item_data = deepcopy(player.item_shop)
            item_data[Items.Jewelry][ItemData.buy] += 100
            return dc_replace(player, witch_happened=True, set_interest=True, item_shop=item_data, day_start_skip=True,
                              actions=player.actions + [f'Witch - {witch_options_string[selection]}'])
        case WitchOptions.Midas_was_a_Trader:  # +150 Trader earn, -300 Marble sell
            npc_data = deepcopy(player.npc_shop)
            npc_data[NPCs.Trader][NPCData.effect] += 150
            item_data = deepcopy(player.item_shop)
            item_data[Items.Marble][ItemData.sell] = 0
            return dc_replace(player, witch_happened=True, npc_shop=npc_data, item_shop=item_data, day_start_skip=True,
                              actions=player.actions + [f'Witch - {witch_options_string[selection]}'])
        case WitchOptions.Camelization:  # +10 Camel weight, +1 Camel food cost
            npc_data = deepcopy(player.npc_shop)
            npc_data[NPCs.Camel][NPCData.effect] += 10
            new_consumption = player.food_consumption
            new_consumption += npc_data[NPCs.Camel][NPCData.eat] * player.camel
            npc_data[NPCs.Camel][NPCData.eat] *= 2
            return dc_replace(player, witch_happened=True, npc_shop=npc_data, day_start_skip=True,
                              food_consumption=new_consumption,
                              actions=player.actions + [f'Witch - {witch_options_string[selection]}'])
        case WitchOptions.Time_is_Money:  # +1 Day, - 500 Jewelry sell
            item_data = deepcopy(player.item_shop)
            item_data[Items.Jewelry][ItemData.sell] -= 500
            return dc_replace(player, witch_happened=True, item_shop=item_data, max_day=player.max_day + 1,
                              day_start_skip=True,
                              actions=player.actions + [f'Witch - {witch_options_string[selection]}'])
        case WitchOptions.Animal_Lover:  # +2 Camels, -1 Day
            # todo these need to eat?
            return dc_replace(player, witch_happened=True, max_day=player.max_day - 1, camel=player.camel + 2,
                              day_start_skip=True,
                              actions=player.actions + [f'Witch - {witch_options_string[selection]}'])
        case WitchOptions.Oasis_of_Sanctifan:  # Crew don't eat, +200 Silk buy
            npc_data = deepcopy(player.npc_shop)
            npc_data[NPCs.Trader][NPCData.eat] = 0
            npc_data[NPCs.Camel][NPCData.eat] = 0
            item_data = deepcopy(player.item_shop)
            item_data[Items.Silk][ItemData.buy] += 200
            return dc_replace(player, witch_happened=True, npc_shop=npc_data, item_shop=item_data, food_consumption=0,
                              day_start_skip=True,
                              actions=player.actions + [f'Witch - {witch_options_string[selection]}'])
        case WitchOptions.The_Stonecutter:  # -15 Marble weight, +60 Marble cost
            item_data = deepcopy(player.item_shop)
            item_data[Items.Marble][ItemData.weight] -= 15
            item_data[Items.Marble][ItemData.buy] += 60
            return dc_replace(player, witch_happened=True, item_shop=item_data, day_start_skip=True,
                              actions=player.actions + [f'Witch - {witch_options_string[selection]}'])


def check__has_items(player: Player):
    return player.spice + player.pottery + player.marble + player.silk + player.jewelry + player.fang_mo_pot


def check__points(player: Player, c_type: str):
    # TODO this needs to be better
    # Camels are not worth enough so it discards them
    # Statue is worth too much
    value = (player.spice * player.item_shop[Items.Spice][ItemData.buy] +
             (player.pottery + player.fang_mo_pot) * player.item_shop[Items.Pottery][ItemData.buy] +
             player.marble * player.item_shop[Items.Marble][ItemData.buy] +
             player.silk * player.item_shop[Items.Silk][ItemData.buy] +
             player.jewelry * player.item_shop[Items.Jewelry][ItemData.buy] +
             player.trader * player.npc_shop[NPCs.Trader][NPCData.cost] +
             player.camel * player.npc_shop[NPCs.Camel][NPCData.cost] +
             player.statue * (500 if c_type == "Final" else 25) * (player.trader + player.camel) +
             player.gold)
    return value


def check__weight_left(player: Player):
    return (player.food * -player.item_shop[Items.Food][ItemData.weight] +
            player.spice * -player.item_shop[Items.Spice][ItemData.weight] +
            player.pottery * -player.item_shop[Items.Pottery][ItemData.weight] +
            player.marble * -player.item_shop[Items.Marble][ItemData.weight] +
            player.silk * -player.item_shop[Items.Silk][ItemData.weight] +
            player.jewelry * -player.item_shop[Items.Jewelry][ItemData.weight] +
            player.camel * player.npc_shop[NPCs.Camel][NPCData.effect] +
            player.backpack)


def check__item_worth(player: Player, item_to_check: Items):
    weight = max(player.item_shop[item_to_check][ItemData.weight], 1)
    sell = player.item_shop[item_to_check][ItemData.sell]
    buy = player.item_shop[item_to_check][ItemData.buy]
    return (sell - buy) / weight
