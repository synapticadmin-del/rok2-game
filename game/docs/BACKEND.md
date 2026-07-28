# ROK2 Backend Architecture

## Components
1. **Worker router** (`src/http/router.ts`): auth, city economy, alliance, proxies world commands to DO.
2. **D1**: durable account/city data.
3. **KingdomShard DO**: one instance per kingdom (`kingdom-1`), holds hot map state + WS.

## Authority
- Resources, training, buildings: D1 + production formula on read/write.
- Map positions, marches, pass ownership, combat: Durable Object only.
- Clients never trusted for balances.

## Map
Loaded from `src/data/map_spec_coordinates.json` scaled by **0.5** → 1200×1200 prototype world.
- 8 Zone1 regions
- Pass capture requires alliance
- Season day gates unlock_day passes (admin can set day)

## Tick
DO alarm every 1s while there are connected sockets or moving marches/contested passes; otherwise idle/hibernate-friendly.
