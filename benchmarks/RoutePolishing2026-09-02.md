# Exact-replay route polishing

The route polisher is a bounded local search around replay-verified routes. It
does not replace the general solver. It is intended to recover improvements
that a broad beam found structurally but whose exact food, cargo, crew order,
or encounter placement was not fully refined.

Every candidate passes `ValidatedRouteReplay` before it may enter the beam.
That boundary enforces start-of-day food, gold, cargo capacity, the cargo Buy
lock, NPC food requirements, Merchant/Witch timing, and completion on the
actual final player day. This avoids the permissive low-level `Game` methods
accepting an impossible route during a polishing experiment.

The deterministic neighborhood includes:

- exact Buy-coordinate changes at several step sizes;
- cargo transfers between item types;
- food transfers between Buy actions;
- Trader/Camel/Interest substitutions;
- adjacent and long action-order swaps; and
- Merchant and Witch option changes.

Run it through the native safety wrapper:

```bash
./scripts/run_safe.sh --town Normalia --polish-record \
  --polish-beam 256 --polish-rounds 20 --polish-evaluations 2000000
```

## Results

| Town | Starting best | Polished best | Candidates evaluated | Legal candidates |
| --- | ---: | ---: | ---: | ---: |
| Normalia | 37,527 | 37,527 | 1,797,262 | 95,220 |
| Sanctifan | 55,602 | **55,728** | 1,873,620 | 84,826 |

Normalia used the current record plus the documented Cleppsydra and Statuette
routes as independent seeds. No candidate improved 37,527. This is not a proof
of global optimality, but it is stronger evidence than another copy of the
same beam search: the replay polisher directly perturbed the record's exact
quantities and action structure. The record already calls Merchant on day 7,
takes the boosted Trader on that same day, buys the Camel on day 8, and calls
Witch on the earliest possible day 9.

Sanctifan's winning path keeps Magic Cleppsydra and Midas but redistributes the
early food and changes the first cargo portfolio. Its exact action log is:

```text
1: Trader
2: Buy [11, 0, 0, 0, 0, 0]
3: Trader
4: Camel
5: Buy [25, 0, 0, 0, 0, 0]
6: Trader
Merch - Magic Cleppsydra
7: Trader
8: Camel
Witch - Midas was a Trader
9: Buy [35, 0, 0, 0, 0, 0]
10: Camel
11: Camel
12: Buy [12, 8, 9, 0, 1, 2]
13: Sell
14: Buy [20, 0, 0, 0, 0, 9]
15: Sell
16: Quick Interest
16: Interest
```

The two day-16 actions are intentional: Sanctifan makes the first Interest a
same-day quick action, and the second Interest consumes the final day.
