from dataclasses import dataclass, field


@dataclass(slots=True, frozen=True)
class Player:
    #  Inventory
    gold: int = field(default=30, compare=False, hash=True)
    food: int = field(default=5, compare=False, hash=True)
    spice: int = field(default=0, compare=False, hash=True)
    pottery: int = field(default=0, compare=False, hash=True)
    marble: int = field(default=0, compare=False, hash=True)
    silk: int = field(default=0, compare=False, hash=True)
    jewelry: int = field(default=0, compare=False, hash=True)
    fang_mo_pot: int = field(default=0, compare=False, hash=True)
    # Crew
    trader: int = field(default=1, compare=False, hash=True)
    camel: int = field(default=1, compare=False, hash=True)
    food_consumption: int = field(default=2, compare=False, hash=True)
    # Stores
    item_shop: list[list[int]] = field(default_factory=lambda: [
        # [Buy, Sell, Weight] # Sell Value, Profit per weight
        [3, 2, 2],            # *0.6667,     -0.5
        [5, 10, 4],           # *2.0000,      1.25
        [20, 33, 5],          # *1.6500,      2.6
        [110, 300, 20],       # *2.7273,      9.5
        [530, 1150, 8],       # *2.1698,     77.5
        [900, 2500, 12]       # *2.7778,    133.3
    ], compare=False, repr=False, hash=False)
    item_shop_hashable: str = field(default='[]', compare=False, hash=True)
    npc_shop: list[list[int]] = field(default_factory=lambda: [
        # [Buy, Earn/Bag, Eat]
        [25, 20, 1],
        [30, 30, 1]
    ], compare=False, repr=False, hash=False)
    npc_shop_hashable: str = field(default='[]', compare=False, hash=True)
    # Effects
    backpack: int = field(default=0, compare=False, hash=True)
    daily_income: int = field(default=0, compare=False, hash=True)
    interest_rate: float = field(default=1.5, compare=False, hash=True)
    set_interest: bool = field(default=False, compare=False, hash=True)
    quick_interest: bool = field(default=False, compare=False, hash=True)
    statue: bool = field(default=False, compare=False, hash=True)
    hand_of_midas: bool = field(default=False, compare=False, hash=True)
    merch_discount: bool = field(default=False, compare=False, hash=True)
    trader_interest: bool = field(default=False, compare=False, hash=True)
    #  Day Info
    day: int = field(default=0, compare=False, repr=False, hash=True)
    max_day: int = field(default=15, compare=False, hash=True)
    merchant_happened: bool = field(default=False, compare=False, repr=False, hash=True)
    call_merchant: bool = field(default=False, compare=False, hash=True)
    witch_happened: bool = field(default=False, compare=False, repr=False, hash=True)
    day_start_skip: bool = field(default=False, hash=False, compare=False, repr=False)
    bought_last: bool = field(default=False, compare=False, hash=False, repr=False)
    actions: list[str] = field(default_factory=list, compare=False, hash=False, repr=False)
    points: int = field(default=0, compare=False, hash=False, repr=False)
