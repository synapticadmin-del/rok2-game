# ROK2 API Reference

Base local: `http://127.0.0.1:8787`

Auth header: `Authorization: Bearer <token>`

## Health / Meta
- `GET /v1/health`
- `GET /v1/meta/map`
- `GET /v1/meta/civilizations`
- `GET /v1/meta/buildings`
- `GET /v1/meta/troops`
- `GET /v1/meta/commanders`
- `GET /v1/meta/techtree`
- `GET /v1/meta/all` — **بيانات التوازن الموحدة (P1-T6):** civilizations + buildings + troops + commanders + techTree + constants (productionBase, productionLevelMult, trainableUnits). يقرأها العميل مرة واحدة عند البدء بدل القيم الثابتة.

## Auth
- `POST /v1/auth/guest` `{ deviceId?, name? }` → `{ token, accountId, player? }`
- `GET /v1/me`

## City
- `POST /v1/city/init` `{ civ, name? }` → new token with playerId
- `GET /v1/city`
- `POST /v1/city/upgrade` `{ buildingId }`
- `POST /v1/city/train` `{ unit: infantry_t1|cavalry_t1|archer_t1, count }`
- `POST /v1/city/collect`

## Alliance
- `POST /v1/alliance/create` `{ name, tag }`
- `POST /v1/alliance/join` `{ allianceId }`
- `GET /v1/alliance/:id`

## World
- `GET /v1/world/snapshot`
- `GET /v1/world/ws` (WebSocket upgrade)
- `POST /v1/world/march` `{ targetType, targetId, troops, toX?, toY?, passId? }`
- `POST /v1/world/pass/attack` `{ passId, troops }` (requires alliance)

### WS client messages
```json
{"type":"hello","playerId":"..."}
{"type":"aoi_sub","x":600,"y":1000,"r":80}
{"type":"pass_attack","passId":"P_R2_R3","troops":{"infantry_t1":200}}
{"type":"march_create","targetType":"resource","targetId":"node_R2_0","troops":{"infantry_t1":50}}
{"type":"ping"}
```

## Admin (`x-admin-key`)
- `POST /v1/admin/tick`
- `POST /v1/admin/set-time` `{ day }`
- `POST /v1/admin/grant` `{ playerId, food?, wood?, stone?, gold?, troops? }`
