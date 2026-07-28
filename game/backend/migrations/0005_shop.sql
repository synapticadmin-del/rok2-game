-- P3-T4: متجر sandbox + speedups + VIP أساسي
-- مخزون اللاعب من speedups + نقاط/مستوى VIP + عمود gems في cities
CREATE TABLE IF NOT EXISTS player_inventory (
  player_id TEXT NOT NULL,
  item_id TEXT NOT NULL,
  count INTEGER NOT NULL DEFAULT 0,
  updated_at INTEGER NOT NULL,
  PRIMARY KEY (player_id, item_id)
);

CREATE TABLE IF NOT EXISTS player_vip (
  player_id TEXT PRIMARY KEY,
  points INTEGER NOT NULL DEFAULT 0,
  level INTEGER NOT NULL DEFAULT 0,
  last_daily_gems_day INTEGER NOT NULL DEFAULT -1,
  last_free_speedup_day INTEGER NOT NULL DEFAULT -1,
  updated_at INTEGER NOT NULL
);

-- gems (عملة sandbox) — تُضاف لجدول cities
ALTER TABLE cities ADD COLUMN gems REAL NOT NULL DEFAULT 1000;
