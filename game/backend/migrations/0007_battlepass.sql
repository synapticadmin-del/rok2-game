-- P4-T1: Battle Pass — تقدم اللاعب (XP + مستوى) + المكافآت المطالب بها + فتح المسار المدفوع
CREATE TABLE IF NOT EXISTS player_battlepass (
  player_id TEXT NOT NULL,
  season_id TEXT NOT NULL,
  xp INTEGER NOT NULL DEFAULT 0,
  level INTEGER NOT NULL DEFAULT 0,
  premium INTEGER NOT NULL DEFAULT 0,
  updated_at INTEGER NOT NULL,
  PRIMARY KEY (player_id, season_id)
);

-- مكافأة واحدة مطلوبة لكل (لاعب, مستوى, مسار): track = 'free' | 'premium'
CREATE TABLE IF NOT EXISTS battlepass_claims (
  player_id TEXT NOT NULL,
  season_id TEXT NOT NULL,
  level INTEGER NOT NULL,
  track TEXT NOT NULL,
  claimed_at INTEGER NOT NULL,
  PRIMARY KEY (player_id, season_id, level, track)
);

CREATE INDEX IF NOT EXISTS idx_bp_claims_player ON battlepass_claims(player_id, season_id);
