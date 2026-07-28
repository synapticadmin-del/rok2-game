-- ROK2 D1 alliance schema (P2-T5): رتب الأعضاء + مساعدات helps + حملات rally

CREATE TABLE IF NOT EXISTS alliance_members (
  alliance_id TEXT NOT NULL,
  player_id TEXT NOT NULL,
  rank TEXT NOT NULL DEFAULT 'R1',
  joined_at INTEGER NOT NULL,
  PRIMARY KEY (alliance_id, player_id),
  FOREIGN KEY (alliance_id) REFERENCES alliances(id),
  FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE INDEX IF NOT EXISTS idx_alliance_members_player ON alliance_members(player_id);

-- طابور speedup لكل طابور لاعب عبر مساعدات الأعضاء
CREATE TABLE IF NOT EXISTS alliance_helps (
  queue_id TEXT NOT NULL,
  helper_player_id TEXT NOT NULL,
  created_at INTEGER NOT NULL,
  PRIMARY KEY (queue_id, helper_player_id)
);

-- حملة rally على ممر/عرش: قائد + مشاركون بقواتهم
CREATE TABLE IF NOT EXISTS rallies (
  id TEXT PRIMARY KEY,
  alliance_id TEXT NOT NULL,
  leader_player_id TEXT NOT NULL,
  target_type TEXT NOT NULL,
  target_id TEXT NOT NULL,
  status TEXT NOT NULL DEFAULT 'forming',
  start_ms INTEGER NOT NULL,
  launch_ms INTEGER NOT NULL,
  march_id TEXT,
  commander_id TEXT,
  commander_skills_json TEXT,
  created_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS rally_participants (
  rally_id TEXT NOT NULL,
  player_id TEXT NOT NULL,
  troops_json TEXT NOT NULL,
  joined_at INTEGER NOT NULL,
  PRIMARY KEY (rally_id, player_id),
  FOREIGN KEY (rally_id) REFERENCES rallies(id)
);

CREATE INDEX IF NOT EXISTS idx_rallies_alliance ON rallies(alliance_id, status);
