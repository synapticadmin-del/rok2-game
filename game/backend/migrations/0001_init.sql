-- ROK2 D1 initial schema
CREATE TABLE IF NOT EXISTS accounts (
  id TEXT PRIMARY KEY,
  device_id TEXT NOT NULL UNIQUE,
  created_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS players (
  id TEXT PRIMARY KEY,
  account_id TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL,
  civ TEXT,
  alliance_id TEXT,
  power INTEGER NOT NULL DEFAULT 0,
  region_id TEXT,
  x REAL,
  y REAL,
  created_at INTEGER NOT NULL,
  FOREIGN KEY (account_id) REFERENCES accounts(id)
);

CREATE TABLE IF NOT EXISTS cities (
  player_id TEXT PRIMARY KEY,
  hall_level INTEGER NOT NULL DEFAULT 1,
  food REAL NOT NULL DEFAULT 5000,
  wood REAL NOT NULL DEFAULT 5000,
  stone REAL NOT NULL DEFAULT 3000,
  gold REAL NOT NULL DEFAULT 2000,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE TABLE IF NOT EXISTS buildings (
  player_id TEXT NOT NULL,
  building_id TEXT NOT NULL,
  level INTEGER NOT NULL DEFAULT 1,
  PRIMARY KEY (player_id, building_id),
  FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE TABLE IF NOT EXISTS troops (
  player_id TEXT NOT NULL,
  unit_id TEXT NOT NULL,
  status TEXT NOT NULL DEFAULT 'home',
  count INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY (player_id, unit_id, status),
  FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE TABLE IF NOT EXISTS alliances (
  id TEXT PRIMARY KEY,
  name TEXT NOT NULL,
  tag TEXT NOT NULL UNIQUE,
  leader_player_id TEXT NOT NULL,
  created_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS sessions (
  token_hash TEXT PRIMARY KEY,
  account_id TEXT NOT NULL,
  player_id TEXT,
  expires_at INTEGER NOT NULL,
  FOREIGN KEY (account_id) REFERENCES accounts(id)
);

CREATE INDEX IF NOT EXISTS idx_players_alliance ON players(alliance_id);
CREATE INDEX IF NOT EXISTS idx_sessions_account ON sessions(account_id);
