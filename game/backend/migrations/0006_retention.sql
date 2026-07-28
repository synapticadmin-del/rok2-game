-- P3-T5: Soft launch — تتبع نشاط اللاعبين لقياس retention
-- يوم دخول واحد لكل لاعب (UTC day) + آخر ظهور على الحساب
CREATE TABLE IF NOT EXISTS player_activity (
  player_id TEXT NOT NULL,
  day INTEGER NOT NULL,
  first_seen_ms INTEGER NOT NULL,
  last_seen_ms INTEGER NOT NULL,
  PRIMARY KEY (player_id, day)
);

CREATE INDEX IF NOT EXISTS idx_player_activity_day ON player_activity(day);

ALTER TABLE accounts ADD COLUMN last_seen_at INTEGER NOT NULL DEFAULT 0;
