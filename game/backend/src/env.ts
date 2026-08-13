export interface Env {
  DB: D1Database;
  KINGDOM_SHARD: DurableObjectNamespace;
  KINGDOM_ID: string;
  ADMIN_KEY: string;
  AUTH_SECRET: string;
}

export type Resources = {
  food: number;
  wood: number;
  stone: number;
  gold: number;
};

export type Troops = Record<string, number>;

export type AuthContext = {
  accountId: string;
  playerId: string | null;
  token: string;
};

export type PlayerRow = {
  id: string;
  account_id: string;
  name: string;
  civ: string | null;
  alliance_id: string | null;
  power: number;
  region_id: string | null;
  x: number | null;
  y: number | null;
  created_at: number;
};

export type CityRow = {
  player_id: string;
  hall_level: number;
  food: number;
  wood: number;
  stone: number;
  gold: number;
  gems: number;
  updated_at: number;
};

// P3-T4 / P9-T4: صف VIP للاعب (جدول player_vip)
export type VipRow = {
  player_id: string;
  points: number;
  level: number;
  last_daily_gems_day: number;
  last_free_speedup_day: number;
  // P9-T4: آخر يوم (UTC) مُنحت فيه نقاط VIP اليومية
  last_daily_points_day: number;
  // P9-T4: آخر يوم (UTC) نشط فيه اللاعب (لـ +20 اتصال يومي)
  last_login_day: number;
  updated_at: number;
};
