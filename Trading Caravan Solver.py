import os
import math
from copy import deepcopy
clear: () = lambda: os.system('cls')
from sympy import symbols, Eq, solve
from sympy.solvers.diophantine import diophantine

class Game:
    def __init__(self, with_merch=False, with_witch=False):
        self.day = 0
        self.max_day = 15
        self.interest_rate = 1.50
        self.set_interest = False
        self.free_interest = False
        self.daily_income = 0
        self.merch_on = with_merch
        self.merch_day = 6  # TODO Find out which random days it can it (5-7)?
        self.merch_sale = False
        self.witch_on = with_witch
        self.witch_day = 9  # TODO Find out which random days it can it (8-10)?
        self.free_food = 0
        self.hand_of_midas = False
        self.item_shop = {
            # Name:     Buy, Sell, Weight,  Sell_Value, Profit_per_weight
            'Food':    [  3,    2,  2],  # ....*0.6667,     -0.5
            'Spice':   [  5,   10,  4],  # ....*2.0000,      1.2
            'Pottery': [ 20,   33,  5],  # ....*1.6500,      2.6
            'Marble':  [110,  300, 20],  # ....*2.7273,      9.5
            'Silk':    [530, 1150,  8],  # ....*2.1698,     77.5
            'Jewelry': [900, 2500, 12],  # ....*2.7778,    133.3
        }
        self.npc = {
            # Name: Buy, Earn/Bag, Eat
            'Camel': [30, 30, 1],
            'Trader': [25, 20, 1]
        }
        self.trader_interest = False
        self.player = {
            'Food': 5,
            'Spice': 0,
            'Pottery': 0,
            'Marble': 0,
            'Silk': 0,
            'Jewelry': 0,
            'Statue': 0,
            'Trader': 1,
            'Camel': 1,
            'Backpack': 0,
            'Gold': 30
        }

        self.debug__merchant_event_worked = False
        self.debug__negative_gold = False
        self.debug__starved_to_death = False
        self.debug__planned_buy = 0

    def event_merchant(self, selection):
        if self.player['Gold'] >= 25 - (5 * self.merch_sale):
            if selection == 'Cornucopia':  # +2 food every turn
                self.player['Gold'] -= 25 - (5 * self.merch_sale)
                self.free_food += 2
                self.debug__merchant_event_worked = True
            elif selection == 'Route to Mahaji':  # Spice is worth $20 more
                self.player['Gold'] -= 25 - (5 * self.merch_sale)
                self.item_shop['Spice'][1] += 20
                self.debug__merchant_event_worked = True
            elif selection == 'Wooden Statuette':  # +1 Statue (aka +500 per Trader and Camel)
                self.player['Gold'] -= 25 - (5 * self.merch_sale)
                self.player['Statue'] += 1
                self.debug__merchant_event_worked = True

        if self.player['Gold'] >= 50 - (10 * self.merch_sale):
            if selection == 'Canvas Bag':  # +20 storage
                self.player['Gold'] -= 50 - (10 * self.merch_sale)
                self.player['Backpack'] += 20
                self.debug__merchant_event_worked = True

            elif selection == 'Leaders Necklace':  # +1 Trader -TODO: Why would you pay this much???
                self.player['Gold'] -= 50 - (10 * self.merch_sale)
                self.player['Trader'] += 1
                if self.trader_interest: self.interest_rate += 0.1
                self.debug__merchant_event_worked = True

            elif selection == 'Hand of Midas':  # 100g when you sell everything
                self.player['Gold'] -= 50 - (10 * self.merch_sale)
                self.hand_of_midas = True
                self.debug__merchant_event_worked = True

        if self.player['Gold'] >= 100 - (20 * self.merch_sale):
            if selection == 'Sturdy Saddle':  # Camels carry an extra 20
                self.player['Gold'] -= 100 - (20 * self.merch_sale)
                self.npc['Camel'][1] += 20
                self.debug__merchant_event_worked = True

            elif selection == 'Magic Cleppsydra':  # 1 more day to trade
                self.player['Gold'] -= 100 - (20 * self.merch_sale)
                self.max_day += 1
                self.debug__merchant_event_worked = True

            elif selection == 'Blue Treasure':  # An extra 20% from saving
                self.player['Gold'] -= 100 - (20 * self.merch_sale)
                self.interest_rate += 0.2
                self.debug__merchant_event_worked = True

        self.merch_day = 0
        self.debug__planned_buy = 0

    def event_witch(self, selection):
        if selection == "Present's Gift":  # +20 weight, -20% Saving
            self.player['Backpack'] += 20
            self.interest_rate = max(self.interest_rate - 0.2, 0)

        elif selection == "Vertue of":  # Saving doesn't decrease, +100 Jewelry cost
            self.free_interest = True
            self.item_shop['Jewelry'][0] += 100

        elif selection == "Midas was a trader":  # +130 Trader earn, -300 Marble sell
            self.npc['Trader'][1] += 130
            self.item_shop['Marble'][1] -= 300

        elif selection == "Camelization":  # +10 Camel weight, +1 Camel food cost
            self.npc['Camel'][1] += 10
            self.npc['Camel'][2] += 1

        elif selection == "Time is Money":  # +1 Day, - 500 Jewelry sell (95% sure this is never worth it)
            self.max_day += 1
            self.item_shop['Jewelry'][1] -= 500

        elif selection == "Animal Lover":  # +2 Camels, -1 Day
            self.player['Camel'] += 2
            self.max_day -= 1

        elif selection == "Oasis of Sanctifan":  # Crew don't eat, +200 Silk buy
            self.npc['Trader'][2] = 0
            self.npc['Camel'][2] = 0
            self.item_shop['Silk'][0] += 200

        elif selection == "The Stonecutter":  # -15 Marble weight, +60 Marble cost
            self.item_shop['Marble'][2] -= 15
            self.item_shop['Marble'][0] += 60

        self.debug__planned_buy = 0

    def event_buy_npc(self, selection):
        if selection == 'Trader' and self.player['Gold'] >= 25:
            self.player['Gold'] -= 25
            self.player['Trader'] += 1

        elif selection == 'Camel' and self.player['Gold'] >= 30:
            self.player['Gold'] -= 30
            self.player['Camel'] += 1

        self.debug__planned_buy = 0

    def event_buy_items(self, food, spice, pottery, marble, silk, jewelry, type_of_buy=0):
        self.player['Gold'] -= food * self.item_shop['Food'][0]
        self.player['Food'] += food
        self.player['Gold'] -= spice * self.item_shop['Spice'][0]
        self.player['Spice'] += spice
        self.player['Gold'] -= pottery * self.item_shop['Pottery'][0]
        self.player['Pottery'] += pottery
        self.player['Gold'] -= marble * self.item_shop['Marble'][0]
        self.player['Marble'] += marble
        self.player['Gold'] -= silk * self.item_shop['Silk'][0]
        self.player['Silk'] += silk
        self.player['Gold'] -= jewelry * self.item_shop['Jewelry'][0]
        self.player['Jewelry'] += jewelry

        if self.player['Gold'] < 0:
            self.debug__negative_gold = True
        self.debug__planned_buy = type_of_buy

    def event_sell_items(self):
        self.player['Gold'] += self.player['Spice'] * self.item_shop['Spice'][1]
        self.player['Spice'] = 0
        self.player['Gold'] += self.player['Pottery'] * self.item_shop['Pottery'][1]
        self.player['Pottery'] = 0
        self.player['Gold'] += self.player['Marble'] * self.item_shop['Marble'][1]
        self.player['Marble'] = 0
        self.player['Gold'] += self.player['Silk'] * self.item_shop['Silk'][1]
        self.player['Silk'] = 0
        self.player['Gold'] += self.player['Jewelry'] * self.item_shop['Jewelry'][1]
        self.player['Jewelry'] = 0
        self.debug__planned_buy = 0

    def event_interest(self):
        self.player['Gold'] = math.ceil(self.interest_rate * self.player['Gold'])
        if not self.set_interest:
            self.interest_rate -= 0.1
        self.debug__planned_buy = 0

    def start_of_day(self):
        if self.day != 0:
            self.player['Food'] -= self.check__food_cost()
            if self.player['Food'] < 0:
                self.debug__starved_to_death = True
        self.day += 1

    def end_of_day(self):
        self.player['Gold'] += self.daily_income + self.player['Trader'] * self.npc['Trader'][1]

    def check__food_cost(self):
        return 0 if self.day == self.max_day else self.player['Trader'] * self.npc['Trader'][2] + self.player['Camel'] * self.npc['Camel'][2]

    def check__has_items(self):
        return self.player['Spice'] + self.player['Pottery'] + self.player['Marble'] + self.player['Silk'] + self.player['Jewelry']

    def check__points(self):
        return self.player['Spice'] * self.item_shop['Spice'][0] + \
               self.player['Pottery'] * self.item_shop['Pottery'][0] + \
               self.player['Marble'] * self.item_shop['Marble'][0] + \
               self.player['Silk'] * self.item_shop['Silk'][0] + \
               self.player['Jewelry'] * self.item_shop['Jewelry'][0] + \
               self.player['Statue'] * 500 * (self.player['Trader'] + self.player['Camel']) + \
               self.player['Trader'] * 25 + \
               self.player['Camel'] * 30 + \
               self.player['Gold']

    def check__weight_left(self):
        return (self.player['Food'] * self.item_shop['Food'][2] +
               self.player['Spice'] * self.item_shop['Spice'][2] +
               self.player['Pottery'] * self.item_shop['Pottery'][2] +
               self.player['Marble'] * self.item_shop['Marble'][2] +
               self.player['Silk'] * self.item_shop['Silk'][2] +
               self.player['Jewelry'] * self.item_shop['Jewelry'][2] -
               self.player['Camel'] * self.npc['Camel'][1] -
               self.player['Backpack']) * -1

    def check__worth(self, item_to_check):
        return self.item_shop[item_to_check][2]/(self.item_shop[item_to_check][1] - self.item_shop[item_to_check][0])

    def game_loop(self): # TODO - Finish - This is for when you want to play
        self.start_of_day()
        if self.merch_on and self.day == self.merch_day:
            self.event_merchant()
        if self.witch_on and self.day == self.witch_day:
            self.event_witch()
        # Test for which thing you want to do then do it
        # check end

# TOWNS
def Normalia(game):  # No Effect
    return game

def Fang_Mo(game):  # Begin with 1 pottery
    game.player['Pottery'] += 1
    return game

def L_Exquise(game): # Traders don't need food
    game.npc['Trader'][2] = 0
    return game

def Mahaji(game):  # +5 Spice sell
    game.item_shop['Spice'][1] += 5
    return game

def Tocaccialli(game):  # +1 Day
    game.max_day += 1
    return game

def Bajaar(game):  # x2 Trader income
    game.npc['Trader'][1] += 20
    return game

def Doba_Lao(game):  # +20% Saving Rate
    game.interest_rate += 0.2
    return game

def Huangoz(game):  # -20 Marble buy
    game.item_shop['Marble'][0] -= 20
    return game

def Port_Willam(game):  # +15 Carry
    game.player['Backpack'] += 15
    return game

def Moonin(game):  # Call Merchant at anytime
    game.merch_day = -1
    return game

def Cornecopea(game):  # Food is free
    game.item_shop['Food'][0] = 0
    return game

def Eduming(game):  # Buying a trader > +10% Saving
    game.trader_interest = True
    return game

def Octoyashi(game):  # +3 Food
    game.player['Food'] += 3
    return game

def Kifuai(game):  # +10 Gold
    game.player['Gold'] += 10
    return game

def Skjollird(game):  # Camel cost +10, weight +10
    game.npc['Camel'][0] += 10
    game.npc['Camel'][1] += 10
    return game

def Petrinov(game):  # Saving rate doesn't decrease
    game.set_interest = True
    return game

def Jilliqo(game):  # +10 Income
    game.daily_income += 10
    return game

def Gayawaku(game):  # Food is weightless
    game.item_shop['Food'][2] = 0
    return game

def Vilparino(game):  # Strange Merchant 20% Sale!
    game.merch_sale = True
    return game

def Sanctifan(game):  # 1 free Save (Collect Interest)
    game.free_interest = True
    return game

def Mehaz(game):  # Camels don't need food
    game.npc['Camel'][2] = 0
    return game

def Ipartus(game):  # Buying price of silk decreased by 100
    game.item_shop['Silk'][0] -= 100
    return game


merchant_event_list = ['Cornucopia', 'Route to Mahaji', 'Wooden Statuette', 'Canvas Bag', 'Leaders Necklace', 'Hand of Midas', 'Sturdy Saddle', 'Magic Cleppsydra', 'Blue Treasure']
witch_event_list = ["Present's Gift", 'Vertue of', 'Midas was a Trader', 'Camelization', 'Time is Money', 'Animal Lover', 'Oasis of Sanctifan', 'The Stonecutter']

# 01  03  05  07  09  11  13[B, S, I]
#   02  04  06  08  10  12  14[ I, I]




class AI:
    def __init__(self):
        self.game_array = [[Normalia(Game()), []]] # Wrap the game in what town you want to use
        self.finished_array = []
        self.what_to_buy = []

    def find_buy(self, game, type_of_buy):
        # Food, Spice, Pottery, Marble, Silk, Jewelry
        self.what_to_buy = [0, 0, 0, 0, 0, 0]
        gold = game.player['Gold']
        weight_left = game.check__weight_left()

        if type_of_buy == 1:  # Buy, Sell
            food_to_buy = max(2 * (game.player['Trader'] * game.npc['Trader'][2] + game.player['Camel'] * game.npc['Camel'][2]) - game.player['Food'], 0) / (2 if (game.day==game.max_day - 1) else 1)
            self.what_to_buy.append(1)
        if type_of_buy == 2:  # Buy, Crew (Plan for food)
            food_to_buy = 2 * max((game.player['Trader'] * game.npc['Trader'][2] + game.player['Camel'] * game.npc['Camel'][2] + 1) - game.player['Food'], 0)
            self.what_to_buy.append(2)

        while food_to_buy > 0 and weight_left >= game.item_shop['Food'][2] and gold >= game.item_shop['Food'][0]:
            self.what_to_buy[0] += 1
            weight_left -= game.item_shop['Food'][2]
            gold -= game.item_shop['Food'][0]
            food_to_buy -= 1

        if type_of_buy == 1:
            '''
            a, b, c, d, e = symbols('a b c d e', integer=True)
            eq1 = diophantine(weight_left - (
                    a * game.item_shop['Jewelry'][2] + b * game.item_shop['Silk'][2] + c * game.item_shop['Marble'][
                2] + d * game.item_shop['Pottery'][2] + e * game.item_shop['Spice'][2]))
            eq2 = diophantine(gold - (
                    a * game.item_shop['Jewelry'][0] + b * game.item_shop['Silk'][0] + c * game.item_shop['Marble'][
                0] + d * game.item_shop['Pottery'][0] + e * game.item_shop['Spice'][0]))
            print(eq1)
            print(eq2)
            answers = solve(diophantine(eq1, eq2), (a, b, c, d, e))
            self.what_to_buy[1] = math.floor(answers[e])
            self.what_to_buy[2] = math.floor(answers[d])
            self.what_to_buy[3] = math.floor(answers[c])
            self.what_to_buy[4] = math.floor(answers[b])
            self.what_to_buy[5] = math.floor(answers[a])
            '''
            items = [['Jewelry', 5], ['Silk', 4], ['Marble', 3], ['Pottery', 2], ['Spice', 1]]
            items.sort(key=lambda x: game.check__worth(x[0]), reverse=False)
            for i, pos in items:
                while weight_left >= game.item_shop[i][2] and gold >= game.item_shop[i][0]:
                    weight_left -= game.item_shop[i][2]
                    gold -= game.item_shop[i][0]
                    self.what_to_buy[pos] += 1

        return self.what_to_buy

    def run(self):
        while len(self.game_array) > 0:

            # Copy and delete what we are working with
            current = self.game_array[-1][0]
            path = self.game_array[-1][1]
            del self.game_array[-1]

            if current.debug__starved_to_death:
                # print('Starved to death: ' + path[-1])
                continue

            if current.debug__negative_gold:
                print(current)
                raise ValueError('Buy or Sell error')

            # Make all options
            current.start_of_day()
            # TODO - Logic for witch and merchant will just append all and continue also Debug merch logic

            buy_type_one = deepcopy(current)
            buy_type_two = deepcopy(current)
            buy_type_one.event_buy_items(*self.find_buy(buy_type_one, 1))
            buy_type_two.event_buy_items(*self.find_buy(buy_type_two, 2))
            buy_type_one.end_of_day()
            buy_type_two.end_of_day()

            if current.player['Food'] - current.check__food_cost() < 0:
                if buy_type_one.day < buy_type_one.max_day:
                    self.game_array.append([buy_type_one, path + ['Buy - One']])
                else:
                    self.finished_array.append([buy_type_one, path + ['Buy - One']])
                if buy_type_two.day < buy_type_two.max_day - 1:
                    self.game_array.append([buy_type_two, path + ['Buy - Two']])
                else:
                    self.finished_array.append([buy_type_two, path + ['Buy - Two']])
            else:
                if current.check__has_items():
                    sell = deepcopy(current)
                    sell.event_sell_items()
                    sell.end_of_day()
                    if sell.day < sell.max_day:
                        self.game_array.append([sell, path + ['Sell']])
                    else:
                        self.finished_array.append([sell, path + ['Sell']])

                else:
                    if buy_type_one.day < buy_type_one.max_day:
                        self.game_array.append([buy_type_one, path + ['Buy - One']])
                    else:
                        self.finished_array.append([buy_type_one, path + ['Buy - One']])
                    if buy_type_two.day < buy_type_two.max_day - 1:
                        self.game_array.append([buy_type_two, path + ['Buy - Two']])
                    else:
                        self.finished_array.append([buy_type_two, path + ['Buy - Two']])

                interest = deepcopy(current)
                interest.event_interest()
                interest.end_of_day()
                if interest.day < interest.max_day:
                    self.game_array.append([interest, path + ['Interest']])
                else:
                    self.finished_array.append([interest, path + ['Interest']])

                if current.player['Gold'] >= 25 and (current.debug__planned_buy == 2 or current.debug__planned_buy == 0):
                    buy_merchant = deepcopy(current)
                    buy_merchant.event_buy_npc('Trader')
                    buy_merchant.end_of_day()
                    if buy_merchant.day < buy_merchant.max_day:
                        self.game_array.append([buy_merchant, path + ['Buy - Trader']])
                    else:
                        self.finished_array.append([buy_merchant, path + ['Buy - Trader']])

                if current.player['Gold'] >= 30 and (current.debug__planned_buy == 2 or current.debug__planned_buy == 0):
                    buy_camel = deepcopy(current)
                    buy_camel.event_buy_npc('Camel')
                    buy_camel.end_of_day()
                    if buy_camel.day < buy_camel.max_day:
                        self.game_array.append([buy_camel, path + ['Buy - Camel']])
                    else:
                        self.finished_array.append([buy_camel, path + ['Buy - Camel']])
        self.finished_array.sort(key=lambda x: x[0].check__points(), reverse=True)
        for i in range(10):
            print('Place: ' + str(i + 1))
            print(self.finished_array[i][0].player)
            print(self.finished_array[i][0].check__points())
            print(self.finished_array[i][1])
            print("")
        input()


solver = AI()
solver.run()