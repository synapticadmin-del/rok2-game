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
  updated_at: number;
};
