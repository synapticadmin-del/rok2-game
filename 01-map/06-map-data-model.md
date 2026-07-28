# نموذج بيانات الخريطة (Map Data Model)

## basemap
```json
{
  "map_id": "season_home_01",
  "width": 1200,
  "height": 1200,
  "tile_size": 1,
  "zones": [
    {"zone_id": 1, "name": "Home Ring", "region_ids": ["R1","R2","R3","R4","R5","R6","R7","R8"]},
    {"zone_id": 2, "name": "Expansion Ring", "region_ids": ["Z2N","Z2E","Z2S","Z2W"]},
    {"zone_id": 3, "name": "Finals Core", "region_ids": ["CORE"]}
  ]
}
```

## region
```json
{
  "region_id": "R1",
  "zone_id": 1,
  "name_key": "region.northern_marches",
  "polygon": [[x,y], ["..."]],
  "spawn_weights": 1.0,
  "resource_bias": {"food":1.0, "wood":1.2, "stone":0.9, "gold":0.8},
  "holy_site_ids": ["altar_r1"],
  "neighbor_pass_ids": ["P_R1_R2", "P_R1_R8", "P_R1_Z2"]
}
```

## terrain layer
- grid أو mesh مبسّط
- قيم: plain / forest_visual / mountain / water / decoration

## graph العملي للحرب
لا تعتمد فقط على البلاطات؛ ابنِ:
- `RegionGraph` (nodes=regions, edges=passes)
- `TerritoryGraph` (alliance flags connectivity)
- `NavGrid` (walkable for marches)

## runtime entities (DB/Redis)
| Entity | Storage |
|--------|---------|
| Static map (regions/passes/terrain) | Config + cache |
| Cities | SQL |
| Marches | Redis + SQL snapshot |
| Pass ownership | Redis hot + SQL |
| Holy site ownership/timers | Redis + SQL |
| Resource node remaining | Redis |
| Alliance territory | SQL + cache |

## خدمة السيرفر المقترحة
- `WorldService`: AOI, spawn, queries
- `MarchService`: movement, eta, recalls
- `SiegeService`: pass/holy/core combat hold logic
- `SeasonService`: unlock timers بين Zones

## فتح الـ Zones
```json
{
  "season_id": "S1",
  "unlocks": [
    {"system": "zone2_inner_passes", "at_day": 10},
    {"system": "zone2_all_passes", "at_day": 14},
    {"system": "zone3_outer", "at_day": 35},
    {"system": "zone3_core", "at_day": 40}
  ]
}
```
