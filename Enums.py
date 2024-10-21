from enum import IntEnum


class Items(IntEnum):
    Food = 0
    Spice = 1
    Pottery = 2
    Marble = 3
    Silk = 4
    Jewelry = 5


class ItemData(IntEnum):
    buy = 0
    sell = 1
    weight = 2


class NPCs(IntEnum):
    Trader = 0
    Camel = 1


class NPCData(IntEnum):
    cost = 0
    effect = 1
    eat = 2


class MerchOptions(IntEnum):
    Cornucopia = 0        # Food consumption -2
    Route_to_Mahaji = 1   # Spice sell +20
    Wooden_Statuette = 2  # Gain a Statue (+500 points per Trader and Camel)
    Canvas_Bag = 3        # Carry Weight +20
    Leaders_Necklace = 4  # Gain one Trader
    Hand_of_Midas = 5     # When you sell everything -> Gold +100
    Sturdy_Saddle = 6     # Camels weight +20
    Magic_Cleppsydra = 7  # Max Day +1
    Blue_Treasure = 8     # Interest Rate +20%


all_merch_options = [
    MerchOptions.Cornucopia,
    MerchOptions.Route_to_Mahaji,
    MerchOptions.Wooden_Statuette,
    MerchOptions.Canvas_Bag,
    MerchOptions.Leaders_Necklace,
    MerchOptions.Hand_of_Midas,
    MerchOptions.Sturdy_Saddle,
    MerchOptions.Magic_Cleppsydra,
    MerchOptions.Blue_Treasure
]

merch_options_string = [
    "Cornucopia",
    "Route to Mahaji",
    "Wooden Statuette",
    "Canvas Bag",
    "Leaders Necklace",
    "Hand of Midas",
    "Sturdy Saddle",
    "Magic Cleppsydra",
    "Blue Treasure"
]


class WitchOptions(IntEnum):
    Presents_Gift = 0       # Carry Weight +20        -- Interest Rate -20%
    Vertue_of_Patience = 1  # Interest Rate is stable -- Jewelry cost +100
    Midas_was_a_Trader = 2  # Trader income +150      -- Marble sell *0
    Camelization = 3        # Camel weight +10        -- Camel eat 2x
    Time_is_Money = 4       # Max Day +1              -- Jewelry sell -500
    Animal_Lover = 5        # Gain two Camels         -- Max Day -1
    Oasis_of_Sanctifan = 6  # Crew don't eat          -- Silk buy +200
    The_Stonecutter = 7     # Marble weight -15       -- Marble cost +60


all_witch_options = [
    WitchOptions.Presents_Gift,
    WitchOptions.Vertue_of_Patience,
    WitchOptions.Midas_was_a_Trader,
    WitchOptions.Camelization,
    WitchOptions.Time_is_Money,
    WitchOptions.Animal_Lover,
    WitchOptions.Oasis_of_Sanctifan,
    WitchOptions.The_Stonecutter
]

witch_options_string = [
    "Present's Gift",
    "Vertue of Patience",
    "Midas was a Trader",
    "Camelization",
    "Time is Money",
    "Animal Lover",
    "Oasis of Sanctifan",
    "The Stonecutter"
]


class TownOptions(IntEnum):
    Normalia = 0     # 00 - No Effect
    Fang_Mo = 1      # 01 - Begin with 1 Pottery
    L_Exquise = 2    # 02 - Traders don't need Food
    Mahaji = 3       # 03 - Spice sell +5
    Tocaccialli = 4  # 04 - Max Day +1
    Bajaar = 5       # 05 - x2 Trader income
    Doba_Lao = 6     # 06 - Interest Rate +20%
    Huangoz = 7      # 07 - Marble Cost -20
    Port_Willam = 8  # 08 - Carry Weight +15
    Moonin = 9       # 09 - Call Merchant at anytime
    Cornecopea = 10  # 10 - Food is free
    Eduming = 11     # 11 - Buying trader -> Interest Rate +10%
    Octoyashi = 12   # 12 - Begin with +3 Food
    Kifuai = 13      # 13 - Begin with +10 Gold
    Skjollird = 14   # 14 - Camel cost +5 and weight +10
    Petrinov = 15    # 15 - Interest rate doesn't decrease
    Jilliqo = 16     # 16 - Daily Gold Income +10
    Gayawaku = 17    # 17 - Food is weightless
    Vilparino = 18   # 18 - Strange Merchant 20% Sale!
    Sanctifan = 19   # 19 - One Free Collect Interest Action
    Mehaz = 20       # 20 - Camels don't need food
    Ipartus = 21     # 21 - Silk Cost -100


town_options_string = [
    "Normalia",
    "Fang-Mo",
    "L'Exquise",
    "Mahaji",
    "Tocaccialli",
    "Bajaar",
    "Doba Lao",
    "Huangoz",
    "Port-Willam",
    "Moonin",
    "Cornecopea",
    "Eduming",
    "Octoyashi",
    "Kifuai",
    "Skjollird",
    "Petrinov",
    "Jilliqo",
    "Gayawaku",
    "Vilparino",
    "Sanctifan",
    "Mehaz",
    "Ipartus"
]


class GameActions(IntEnum):
    a_buy = 0
    a_sell = 1
    a_npc = 2
    a_interest = 3
    a_merchant = 4
    a_witch = 5


class PlayerAction:
    __slots__ = ["action", "buy_data", "npc_data", "merch_data", "witch_data"]

    def __init__(self, action: GameActions, buy_data: list[int] = None, npc_data: NPCs = 0, merch_data: MerchOptions = 0, witch_data: WitchOptions = 0):
        self.action = action
        self.buy_data = buy_data
        self.npc_data = npc_data
        self.merch_data = merch_data
        self.witch_data = witch_data
