-- P8-T6: المهام اليومية والأسبوعية
-- player_quests: حالة المهام النشطة (5 يومية + 3 أسبوعية) مع تقدمها
CREATE TABLE IF NOT EXISTS player_quests (
  player_id TEXT NOT NULL,
  cycle TEXT NOT NULL, -- 'daily' أو 'weekly'
  slot INTEGER NOT NULL, -- 0..4 يومية / 0..2 أسبوعية
  cycle_day INTEGER NOT NULL, -- يوم/أسبوع التوزيع (للتحقق من الاستبدال)
  quest_id TEXT NOT NULL,
  type_id TEXT NOT NULL,
  goal INTEGER NOT NULL,
  points INTEGER NOT NULL,
  progress INTEGER NOT NULL DEFAULT 0,
  claimed INTEGER NOT NULL DEFAULT 0,
  completed INTEGER NOT NULL DEFAULT 0,
  updated_at INTEGER NOT NULL,
  PRIMARY KEY (player_id, cycle, slot)
);

-- player_quest_points: إجمالي النقاط المكتسبة لكل دورة (للحدود والمكافآت)
CREATE TABLE IF NOT EXISTS player_quest_points (
  player_id TEXT NOT NULL,
  cycle TEXT NOT NULL, -- 'daily' أو 'weekly'
  cycle_day INTEGER NOT NULL,
  points INTEGER NOT NULL DEFAULT 0,
  golden_key_granted INTEGER NOT NULL DEFAULT 0,
  updated_at INTEGER NOT NULL,
  PRIMARY KEY (player_id, cycle, cycle_day)
);

-- player_quest_rewards: سجل المكافآت المستردة (golden key / weekly chest)
CREATE TABLE IF NOT EXISTS player_quest_rewards (
  player_id TEXT NOT NULL,
  cycle TEXT NOT NULL,
  cycle_day INTEGER NOT NULL,
  reward_type TEXT NOT NULL, -- 'golden_key' أو 'weekly_chest'
  granted_at INTEGER NOT NULL,
  PRIMARY KEY (player_id, cycle, cycle_day, reward_type)
);
