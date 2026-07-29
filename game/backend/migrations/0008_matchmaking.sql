-- P4-T6: matchmaking ممالك — عمود المملكة على اللاعب + سجل التعيينات
-- المملكة المعينة لكل لاعب عند إنشاء مدينته (kingdom-1 / kingdom-2 ...)
ALTER TABLE players ADD COLUMN kingdom_id TEXT;

-- سجل تعيينات matchmaking: لاعب ← مملكة + السبب (استراتيجية + نسبة الامتلاء لحظتها)
CREATE TABLE IF NOT EXISTS kingdom_assignments (
  player_id TEXT PRIMARY KEY,
  kingdom_id TEXT NOT NULL,
  strategy TEXT NOT NULL,
  fill_ratio REAL NOT NULL DEFAULT 0,
  reason TEXT,
  assigned_at INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_kingdom_assignments_kingdom ON kingdom_assignments(kingdom_id);
