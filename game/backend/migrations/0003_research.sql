-- ROK2 D1 research schema (P2-T3)
CREATE TABLE IF NOT EXISTS player_research (
  player_id TEXT NOT NULL,
  tech_id TEXT NOT NULL,
  level INTEGER NOT NULL DEFAULT 0,
  updated_at INTEGER NOT NULL,
  PRIMARY KEY (player_id, tech_id),
  FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE INDEX IF NOT EXISTS idx_player_research_player ON player_research(player_id);
