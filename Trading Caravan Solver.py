import os
clear = lambda: os.system('cls')

class Caravan:
    def __init__(self, name, owned, buy, sell, weight):
        self.name = name
        self.owned = owned
        self.buy = buy
        self.sell = sell
        self.weight = weight
        #self.want = 0

class TradingCaravan:
    def __init__(self):
        self.backpack = 0
        self.quickMoney = 0
        self.currentDay = 0
        self.maxDay = 15
        self.day = 0
        self.gold = 30
        self.saveGold = 1.50
        self.merchDay = 0
        self.witchDay = 0

        self.LExquise = False  # Trader don't need food
        self.Moonin = False    # Call Merchant anytime
        self.Eduming = False   # Buy Trader > +10% Saving
        self.Petrinov = False  # Saving Rate doesn't decrease
        self.Vilparino = False # Strange Merchant 20% Sale!
        self.Sanctifan = False # 1 free Save

        self.cornucopia = False  # +2 food every turn
        self.handOfMidas = False # 100g when you sell everything 

        self.camelization = False     # Camels cost 2 food
        self.oasisOfSanctifan = False # Crew don't eat

        self.AI = False


# Trade Items
food    = Caravan("Food",    5,   3,    2,  2) # *0.6667, W:  -0.5
spice   = Caravan("Spice",   0,   5,   10,  4) # *2.0000, W:   1.2
pottery = Caravan("Pottery", 0,  20,   33,  5) # *1.6500, W:   2.6
marble  = Caravan("Marble",  0, 110,  300, 20) # *2.7273, W:   9.5
silk    = Caravan("Silk",    0, 530, 1150,  8) # *2.1698, W:  77.5
jewelry = Caravan("Jewelry", 0, 900, 2500, 12) # *2.7778, W: 133.3
statue  = Caravan("Statue",  0,   0, 1500,  0) # Special item
# Crew
trader = Caravan("Trader", 1, 25, 20,  0)
camel  = Caravan("Camel",  1, 30,  0, 30)
# Game
G = TradingCaravan()


def gameMode():
    tempInput = -1
    while not(0 < tempInput and tempInput < 21):
        clear()
        print("Level Select")
        print(" 1 - Normalia   : No Effect")
        print(" 2 - Fang-Mo    : Begin with 1 pottery")
        print(" 3 - L'Exquise  : Traders don't need food")
        print(" 4 - Mahaji     : +5 Spice sell")
        print(" 5 - Tocaccialli: +1 Day")
        print(" 6 - Bajaar     : x2 Trader income")
        print(" 7 - Doba Lao   : +20% Saving Rate")
        print(" 8 - Huangoz    : -20 Marble buy")
        print(" 9 - Port-Willam: +15 Carry")
        print("10 - Moonin     : Call Merchant at anytime")
        print("11 - Cornecopea : Food is free")
        print("12 - Eduming    : Buying a trader > +10% Saving")
        print("13 - Octoyashi  : +3 Food")
        print("14 - Kifuai     : +10 Gold")
        print("15 - Skjollird  : Camel cost +10, weight +10")
        print("16 - Petrinov   : Saving rate doesn't decrease")
        print("17 - Jilliqo    : +10 Income")
        print("18 - Gayawaku   : Food is weightless")
        print("19 - Vilparino  : Strange Merchant 20% Sale!")
        print("20 - Sanctifan  : 1 free Save")
  
        try:
            tempInput = int(input("Selection: "))
        except ValueError:
            tempInput = -1

    if tempInput == 1:
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 2:
        pottery.owned += 1
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 3:
        G.LExquise = True
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 4:
        spice.sell += 5
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 5:
        G.maxDay += 1
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 6:
        trader.sell += 20
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 7:
        G.saveGold += .2
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 8:
        marble.buy -= 20
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 9:
        G.backpack += 15
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 10:
        G.Moonin = True
        G.merchDay = -1
        G.witchDay = 9
    elif tempInput == 11:
        food.buy -= 5
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 12:
        G.Eduming = True
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 13:
        food.owned += 3
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 14:
        G.gold += 10
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 15:
        camel.buy += 5
        camel.weight += 10
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 16:
        G.Petrinov = True
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 17:
        G.quickMoney += 10
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 18:
        food.weight -= 2
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 19:
        G.Vilparino = True
        G.merchDay = 6
        G.witchDay = 9
    elif tempInput == 20:
        G.Sanctifan = True
        G.merchDay = 6
        G.witchDay = 9

    print("")
    print("Have AI play?")
    tempInput = input("(y/n): ")
    if tempInput == "y":
        G.AI = True

def traderEvent():
    if G.Vilparino: cost = [20, 40, 80, 0]
    else: cost = [25, 50, 100, 0]
    tempInput = -1
    while not(0 < tempInput and tempInput < 11) or cost[round(tempInput/3-0.5)] > G.gold:
        clear()
        print("A Strange Merchant has appeared")
        print("Current Gold: " + str(G.gold))
        if G.gold < cost[0]:
            print("You can't afford anything!")
            input("-Leave-")
            tempInput = 0
            break
        print("1 - Cornucopia      : " + str("% 3s" % cost[0]) + "g, +2 food every turn")
        print("2 - Route to Mahaji : " + str("% 3s" % cost[0]) + "g, Spice is worth $20 more")
        print("3 - Wooden Satuette : " + str("% 3s" % cost[0]) + "g, +1500 points at end")
        print("4 - Canvas Bag      : " + str("% 3s" % cost[1]) + "g, +20 storage")
        print("5 - Leaders Necklace: " + str("% 3s" % cost[1]) + "g, +1 Trader")
        print("6 - Hand of Midas   : " + str("% 3s" % cost[1]) + "g, 100g when you sell everything")
        print("7 - Sturdy Saddle   : " + str("% 3s" % cost[2]) + "g, Camels carry an extra 20")
        print("8 - Magic Cleppsydra: " + str("% 3s" % cost[2]) + "g, 1 more day to trade")
        print("9 - Blue Treasure   : " + str("% 3s" % cost[2]) + "g, An extra 20% from saving")
        print("10 - Leave and buy nothing")

        try: tempInput = int(input("Selection: "))
        except ValueError: tempInput = -1

    if tempInput == 1:
        G.gold -= cost[0]
        G.cornucopia = True
    elif tempInput == 2:
        G.gold -= cost[0]
        spice.sell += 20
    elif tempInput == 3:
        G.gold -= cost[0]
        statue.owned += 1
    elif tempInput == 4:
        G.gold -= cost[1]
        G.backpack += 20
    elif tempInput == 5:
        G.gold -= cost[1]
        trader.owned += 1
    elif tempInput == 6:
        G.gold -= cost[1]
        G.handOfMidas = True
    elif tempInput == 7:
        G.gold -= cost[2]
        camel.weight += 20
    elif tempInput == 8:
        G.gold -= cost[2]
        G.maxDay += 1
    elif tempInput == 9:
        G.gold -= cost[2]
        G.saveGold += 0.2
    if tempInput != 10: G.Moonin = False
    #input("New Gold: " + str(G.gold) + " ")

def witchEvent():
    tempInput = -1
    while not(0 < tempInput and tempInput < 9):
        clear()
        print("A Old Witch comes to you")
        print("1 - Present's Gift    : +20 weight, -20% Saving")
        print("2 - Vertue of         : Saving doesn't decrease, +100 Jewelry cost")
        print("3 - Midas was a trader: +130 Trader sell, -300 Marble sell")
        print("4 - Camelization      : +10 Camel weight, +1 Camel food cost")
        print("5 - Time is Money     : +1 Day, - 500 Jewelry sell")
        print("6 - Animal Lover      : +2 Camels, -1 Day")
        print("7 - Oasis of Sanctifan: Crew don't eat, +200 Silk buy")
        print("8 - The Stonecutter   : -15 Marble weight, +60 Marble cost")

        try: tempInput = int(input("Selection: "))
        except ValueError: tempInput = -1

    if tempInput == 1:
        G.backpack += 20
        G.saveGold -= 0.2
    elif tempInput == 2:
        G.Petrinov = True
        jewelry.buy += 100
    elif tempInput == 3:
        trader.sell += 130
        marble.sell -= 300
    elif tempInput == 4:
        camel.weight += 10
        G.camelization = True
    elif tempInput == 5:
        G.maxDay += 1
        jewelry.sell -= 500
    elif tempInput == 6:
        camel.owned += 2
        G.maxDay -= 1
    elif tempInput == 7:
        G.oasisOfSanctifan = True
        silk.buy += 200
    elif tempInput == 8:
        marble.weight -= 15
        marble.buy += 60
        
def getFoodCost():
    totalCrew = 0
    if not(G.LExquise or G.oasisOfSanctifan): totalCrew += trader.owned
    if not(G.oasisOfSanctifan): totalCrew += camel.owned + camel.owned * G.camelization
    if G.cornucopia: totalCrew = max(totalCrew - 2, 0)
    return totalCrew

def getWeight():
    totalweight = 0
    for i in [food, spice, pottery, marble, silk, jewelry, statue]:
        totalweight += i.owned * i.weight
    return totalweight

def startTurn():
    if G.day == 0:
        G.day += 1
        return
    
    # Calculate Food
    food.owned -= getFoodCost()
    if food.owned < 0:
        print("You lose")
        while True:
            input("Please restart")

    # Move Day
    G.day += 1
    if G.Moonin: traderEvent()
    if G.day == G.merchDay: traderEvent()
    if G.day == G.witchDay: witchEvent()

def midTurn():
    while True:
        clear()
        print("Day " + str(G.day) + "/" + str(G.maxDay))
        print("Gold: " + str(G.gold) + " < (+" + str(trader.owned * trader.sell + G.quickMoney) + ")")
        print("Food: " + str(food.owned) + " < (-" + str(getFoodCost()) + ")")
        print("Weight: " + str(getWeight()) + "/" + str(G.backpack + camel.owned * camel.weight))
        print("Traders: " + str(trader.owned) + ", Camels: " + str(camel.owned))
        print("What do you want to do today?")
        print("1 - Buy Items   : ")
        print("2 - Sell Items  : ")
        print("3 - Manage Crew : ")
        print("4 - Invest Money: x" + str(round(G.saveGold*10)/10))
        tempInput = input("Choice: ")
        print("")
        if tempInput == "1":
            print("Gold: " + str(G.gold))
            print("Name   : Own, $Buy, Weight")
            for i in [food, spice, pottery, marble, silk, jewelry]:
                print(i.name + (" "*(7-len(i.name))) + ": " + str( "% 2s" % i.owned) + ", $" + str( "% 4s" % i.buy) + ", " + str("% 3s" % i.weight))
            input("test ")
        elif tempInput == "2":
            #handOfMidas
            break
        elif tempInput == "3":
            #G.Eduming
            break
        elif tempInput == "4":
            print("You will make " + str(round(G.gold * G.saveGold - G.gold)))
            tempInput = input("(y/n): ")
            if tempInput == "y":
                if G.Sanctifan: 
                    G.gold = round(G.gold*G.saveGold)
                    input("You used your free Save")
                    G.Sanctifan = False
                G.gold = round(G.gold*G.saveGold)
                if not(G.Petrinov): G.saveGold = max(G.saveGold - 0.1, 0.1)
                break

def endTurn():
    # Calcualte Gold
    G.gold += trader.owned * trader.sell + G.quickMoney
    if G.day == G.maxDay:
        print(" TODO printScore()")


input("Touch to Start ")
gameMode()
while True:
    startTurn()
    midTurn()
    endTurn()

# 10% in min savings
# Add admin video hacks aka food doesn't kill you