from consts.Enums import GameActions, PlayerAction, NPCs, MerchOptions, WitchOptions, TownOptions, town_options_string
from solvers.GameSolver import BruteForceSolver
from pathlib import Path



if __name__ == '__main__':
    que = []
    # Day 1
    que.append(PlayerAction(GameActions.a_npc, npc_data=NPCs.Trader))
    # Day 2
    que.append(PlayerAction(GameActions.a_buy, buy_data=[10, 0, 0, 0, 0, 0]))
    # Day 3
    que.append(PlayerAction(GameActions.a_npc, npc_data=NPCs.Trader))
    # Day 4
    que.append(PlayerAction(GameActions.a_npc, npc_data=NPCs.Trader))
    # Day 5
    que.append(PlayerAction(GameActions.a_merchant, merch_data=MerchOptions.Magic_Cleppsydra))
    que.append(PlayerAction(GameActions.a_buy, buy_data=[11, 0, 0, 0, 0, 0]))
    # Day 6
    que.append(PlayerAction(GameActions.a_npc, npc_data=NPCs.Camel))
    # Day 7
    que.append(PlayerAction(GameActions.a_buy, buy_data=[30, 0, 0, 0, 0, 0]))
    # Day 8
    que.append(PlayerAction(GameActions.a_npc, npc_data=NPCs.Trader))
    # Day 9
    que.append(PlayerAction(GameActions.a_witch, witch_data=WitchOptions.Midas_was_a_Trader))
    que.append(PlayerAction(GameActions.a_npc, npc_data=NPCs.Camel))
    # Day 10
    # que.append(PlayerAction(GameActions.a_npc, npc_data=NPCs.Camel))
    # Day 11
    # que.append(PlayerAction(GameActions.a_buy, buy_data=[18, 1, 12, 0, 1, 1]))
    # Day 12
    # que.append(PlayerAction(GameActions.a_sell))
    # Day 13
    # que.append(PlayerAction(GameActions.a_buy, buy_data=[27, 0, 1, 0, 0, 5]))
    # Day 14
    # que.append(PlayerAction(GameActions.a_sell))
    # Day 15
    # que.append(PlayerAction(GameActions.a_interest))
    # Day 16
    # Day 17
    # Day 18

    que = []
    limit = 100000
    town = TownOptions.Normalia
    merch = True
    witch = True

    solver = BruteForceSolver(limit,town, merch, witch, que=[])
    if len(que) > 0:
        solver.find_solve()
    else:
        Path(f'./hashes/{town_options_string[town]}_{merch}_{witch}').mkdir(parents=True, exist_ok=True)
        solver.find_solve(f'./hashes/{town_options_string[town]}_{merch}_{witch}')
        # Hash makes way to many files and takes to long to save that many
        # Also hash collision problems