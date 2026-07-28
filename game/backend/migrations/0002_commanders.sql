-- ROK2 D1 commanders schema (P2-T1)
CREATE TABLE IF NOT EXISTS player_commanders (
  id TEXT PRIMARY KEY,
  player_id TEXT NOT NULL,
  commander_id TEXT NOT NULL,
  level INTEGER NOT NULL DEFAULT 1,
  xp INTEGER NOT NULL DEFAULT 0,
  tomes INTEGER NOT NULL DEFAULT 0,
  skills_json TEXT NOT NULL DEFAULT '[1,1,1]',
  created_at INTEGER NOT NULL,
  UNIQUE (player_id, commander_id),
  FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE TABLE IF NOT EXISTS march_commanders (
  march_id TEXT PRIMARY KEY,
  player_id TEXT NOT NULL,
  commander_id TEXT NOT NULL,
  skills_json TEXT NOT NULL DEFAULT '[1,1,1]',
  created_at INTEGER NOT NULL,
  FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE INDEX IF NOT EXISTS idx_player_commanders_player ON player_commanders(player_id);
