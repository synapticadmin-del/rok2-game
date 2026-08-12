-- تخطيط القلعة تجميلي لكنه مملوك للاعب ويزامن بين الأجهزة.
CREATE TABLE IF NOT EXISTS city_layouts (
  player_id TEXT PRIMARY KEY,
  layout_json TEXT NOT NULL,
  version INTEGER NOT NULL DEFAULT 1,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE INDEX IF NOT EXISTS idx_city_layouts_updated_at ON city_layouts(updated_at);
