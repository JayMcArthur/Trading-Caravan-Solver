# Merchant, Witch, and disaster code parity

This implementation follows the recovered Android `1.147.9` (`11479`) runtime
operations. The source audit is in the companion extraction project at
`../Random/CaravanTradingGame/EVENT_CODE.md`; this file records how those
operations map into the C++ solver.

## Numeric representation

The game uses whole-number state for gold, inventory, prices, weights, crew,
food consumption, and days. `Player` therefore keeps those values as `int`.
Saving rate is the exception: `Player::interest_rate` is a decimal `double`.
The compact solver state stores that rate as signed hundredths, so `1.5` is
stored as `150`, `-0.1` as `-10`, and Traveler's `+0.05` remains exact.

`Steal` is the one disaster that converts a fractional result back to a whole
number. `Game::event_disaster` calculates exactly three quarters of the gold
with integer arithmetic and rounds a half to the nearest even integer, matching
`Convert.ToInt32` (`2 * .75 = 1.5 -> 2`; `6 * .75 = 4.5 -> 4`).

## Encounter timing

| Encounter | Classical | Traveler |
| --- | --- | --- |
| Merchant | uniformly selected day 5-7 | uniformly selected day 7-9 |
| Witch | uniformly selected day 9-11 | uniformly selected day 11-13 |
| Disaster | none | uniformly selected day 15-17 |

Traveler selects one of the 12 disasters uniformly when the selected day is
reached. The shipped availability flags allow every disaster. The old safety
filter exists in code but has no active caller, so the solver does not apply it.

`Game::create_traveler_player` builds the recovered Traveler base state before
applying faction effects: 20 weight bonus, 10 gold, zero traders, zero camels,
and 20 maximum days. It requires three distinct preparation choices. This order
matches `GlobalVariables.Start`, which calls `PrepareUpdate()` and then
`FactionImpact()`. Passing the realized `merchant_day`, `witch_day`,
`disaster_day`, and disaster produces an exact fixed random schedule. Zero
Merchant/Witch days retain the older window-search behavior for Classical
solver experiments; they do not claim to represent a sampled game run.

| Preparation choice | Exact C++ mutation |
| --- | --- |
| Weight | `backpack += 10` |
| Gold | `gold += 10` |
| Pottery | pottery buy `-= 5` |
| Marble | marble weight `-= 5` |
| Jewelry | jewelry sell `+= 300` |
| Trader | trader cost `-= 10` |
| Camel | camel cost `= 0` |
| Traders income | trader income `+= 5` |
| Load capacity | camel load `+= 5` |
| Saving | `interest_rate += 0.05` |

Disasters are runtime mutations. They last for the rest of the current run but
are not saved across runs. In the C++ solver, set `Player::disaster_day` and
`Player::scheduled_disaster` to model one realized random scenario. The solver
applies that outcome automatically and exactly once. It deliberately does not
branch over the 12 outcomes and maximize them, because the player does not get
to choose the disaster. Analyze the full random space as 36 fixed scenarios
(three days times twelve outcomes), then aggregate those results separately.

On a fixed Merchant day the solver includes the game's Leave action. Leaving
sets `merchant_happened = true`, applies no artifact, and prevents the encounter
from incorrectly reappearing on a later day.

## Merchant

The item price is subtracted before its effect is applied.

| Item | C++ state mutation |
| --- | --- |
| Blue Treasure | `interest_rate += 0.20` |
| Route to Mahaji | `item_shop[Spice][Sell] += 20` |
| Cornucopia | `bonus_food_consumption = food_consumption <= 1 ? -1 : -2`, then recalculate total consumption |
| Canvas Bag | `backpack += 20` |
| Hand of Midas | `hand_of_midas = true`; each completed sell does `gold += 100` |
| Magic Clepsydra | `max_day += 1` |
| Wooden Statuette | `statue = true`; final bonus is `(camel + trader) * 500` |
| Sturdy Saddle | `npc_shop[Camel][Effect] += 20` |
| Leader's Necklace | `trader += 1`, then recalculate consumption |

Leader's Necklace is not a trader purchase and therefore does not trigger
Eduming's `interest_rate += 0.1` purchase bonus. A later ordinary trader
purchase still does.

## Witch

| Power | C++ state mutations |
| --- | --- |
| Present's Gift | `backpack += 20`; `interest_rate = max(0, interest_rate - 0.20)` |
| Vertue of Patience | `set_interest = true`; jewelry buy `+= 100` |
| Midas Was a Trader | trader income `+= 150`; marble sell `= 0` |
| Camelization | camel load `+= 10`; camel food `*= 2`; recalculate total |
| Time Is Money | `max_day += 1`; jewelry sell `-= 500` |
| Animal Lover | `camel += 2`; `max_day -= 1`; recalculate total |
| Oasis of Sanctifan | trader food `= 0`; camel food `= 0`; food bonus `= 0`; silk buy `+= 200`; recalculate total |
| The Stonecutter | marble weight `-= 15`; marble buy `+= 60` |

`set_interest` stops the ordinary saving action from reducing the rate. It does
not stop Savingcrisis, whose code writes the rate directly.

## Disasters

| Disaster | C++ state mutations |
| --- | --- |
| Steal | `gold = round_to_even(gold * 0.75)` |
| Brokeleg | `backpack -= 10` |
| Tradersleft | `trader = 0`, then recalculate consumption |
| Marblecrisis | marble sell `= 0` |
| Savingcrisis | `interest_rate -= 0.30` with no clamp |
| Silkpenury | silk buy `+= 200` |
| Seer | `max_day += 1`; `camel -= 1`, then recalculate consumption |
| Heavyfood | food weight `*= 2` |
| Jewelrycrisis | jewelry sell `-= 700` |
| Silkcrisis | silk sell `-= 350` |
| Foodcrisis | food buy `= 15` |
| Camelsick | camel food `*= 2`, then recalculate total consumption |

Seer's camel subtraction is deliberately signed and unclamped. At zero camels
it produces `-1`; buying a camel later performs the normal `camel += 1`, so the
count becomes zero. There is no hidden status flag that restores the camel.

## Interaction rules

- `=` overwrites the earlier value. Tradersleft removes every current trader,
  including Leader's Necklace's trader. Midas's income-per-trader bonus remains
  and affects traders recruited later.
- `+=` and `-=` stack in encounter order. Canvas Bag, Present's Gift, and
  Brokeleg produce `+20 +20 -10`. Time Is Money and Jewelrycrisis produce
  `-500 -700` to jewelry sell value.
- `*=` compounds. Camelization followed by Camelsick makes camel food four
  times its earlier value. Oasis followed by Camelsick remains zero.
- Cornucopia modifies a separate bonus-food component. Oasis explicitly assigns
  that component to zero, so recalculating crew consumption cannot accidentally
  bring the Cornucopia modifier back.
- Merchant payment happens first. A later Steal keeps 75% of the post-purchase
  balance.
- Animal Lover followed by Seer is net `camel += 1` and net zero maximum days.

The parity and interaction cases are executable assertions in `tests.cpp`.
