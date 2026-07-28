import { DurableObject } from "cloudflare:workers";
import type { Env, Troops } from "../env";
import { getMap, type MapPass, type MapRegion } from "../lib/gameData";
import { newId, nowMs, dist } from "../lib/ids";
import { resolveCombat, totalTroops, troopPower } from "./sim/combat";
import { marchDurationMs, planMarch } from "./sim/pathfinding";
import { COMMANDER_CONSTANTS, xpForLevel, type CommanderInstance } from "./sim/commanders";
import { admitWounded, hospitalCapacity } from "./sim/hospital";

type CityEntity = {
  playerId: string;
  name: string;
  allianceId: string | null;
  x: number;
  y: number;
  hallLevel: number;
  regionId: string;
  ap: number;
  lastApMs: number;
};

type AllianceFlag = {
  id: string;
  allianceId: string;
  x: number;
  y: number;
  radius: number;
};

type PassEntity = {
  id: string;
  ownerAllianceId: string | null;
  captureProgress: number;
  state: "open" | "contested";
  level: number;
  from: string;
  to: string;
  x: number;
  y: number;
  unlockDay: number;
};

type MarchEntity = {
  id: string;
  ownerPlayerId: string;
  allianceId: string | null;
  fromX: number;
  fromY: number;
  toX: number;
  toY: number;
  startMs: number;
  etaMs: number;
  troops: Troops;
  state: "moving" | "arrived" | "returned" | "cancelled" | "gathering" | "returning";
  targetType: "pass" | "resource" | "barb" | "city" | "point" | "throne";
  targetId: string;
  payload?: any;
};

type NodeEntity = {
  id: string;
  kind: "food" | "wood" | "stone" | "gold" | "barb";
  level: number;
  x: number;
  y: number;
  remaining: number;
};

type ThroneEntity = {
  ownerAllianceId: string | null;
  captureProgress: number;
  state: "open" | "contested";
  x: number;
  y: number;
  unlockDay: number;
};

type QueueEntity = {
  id: string;
  playerId: string;
  type: "build" | "train" | "research" | "heal";
  data: any;
  startMs: number;
  etaMs: number;
  state: "running" | "completed" | "cancelled";
};

type Attach = {
  playerId: string;
  aoi?: { x: number; y: number; r: number };
};

export class KingdomShard extends DurableObject<Env> {
  private ready = false;
  private regions: MapRegion[] = [];
  private passDefs: MapPass[] = [];
  private mountainBelt = 20;
  private passWidth = 10;
  private seasonDay = 0;
  private cities = new Map<string, CityEntity>();
  private throne: ThroneEntity = { ownerAllianceId: null, captureProgress: 0, state: "open", x: 1200, y: 1200, unlockDay: 14 };
  private throneScores = new Map<string, number>();
  private passes = new Map<string, PassEntity>();
  private marches = new Map<string, MarchEntity>();
  private nodes = new Map<string, NodeEntity>();
  private flags = new Map<string, AllianceFlag>();
  private queues = new Map<string, QueueEntity>();
  private reports: any[] = [];

  constructor(ctx: DurableObjectState, env: Env) {
    super(ctx, env);
    this.ctx.blockConcurrencyWhile(async () => {
      this.migrate();
      try { this.ctx.storage.sql.exec("ALTER TABLE marches ADD COLUMN payload_json TEXT"); } catch {}
      this.loadMapDefs();
      this.loadState();
      if (this.passes.size === 0) this.seedWorld();
      this.ready = true;
    });
  }

  private migrate() {
    this.ctx.storage.sql.exec(`
      CREATE TABLE IF NOT EXISTS _sql_schema_migrations (
        id INTEGER PRIMARY KEY,
        applied_at TEXT NOT NULL DEFAULT (datetime('now'))
      );
    `);
    const ver = this.ctx.storage.sql
      .exec<{ version: number }>("SELECT COALESCE(MAX(id), 0) as version FROM _sql_schema_migrations")
      .one().version;

    if (ver < 1) {
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS world_meta (
          id INTEGER PRIMARY KEY CHECK (id = 1),
          season_day INTEGER NOT NULL,
          last_tick_ms INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS map_cities (
          player_id TEXT PRIMARY KEY,
          name TEXT NOT NULL,
          alliance_id TEXT,
          x REAL NOT NULL,
          y REAL NOT NULL,
          hall_level INTEGER NOT NULL,
          region_id TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS passes (
          pass_id TEXT PRIMARY KEY,
          owner_alliance_id TEXT,
          capture_progress REAL NOT NULL,
          state TEXT NOT NULL,
          level INTEGER NOT NULL,
          from_region TEXT NOT NULL,
          to_region TEXT NOT NULL,
          x REAL NOT NULL,
          y REAL NOT NULL,
          unlock_day INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS marches (
          id TEXT PRIMARY KEY,
          owner_player_id TEXT NOT NULL,
          alliance_id TEXT,
          from_x REAL NOT NULL,
          from_y REAL NOT NULL,
          to_x REAL NOT NULL,
          to_y REAL NOT NULL,
          start_ms INTEGER NOT NULL,
          eta_ms INTEGER NOT NULL,
          troops_json TEXT NOT NULL,
          state TEXT NOT NULL,
          target_type TEXT NOT NULL,
          target_id TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS throne (
          id INTEGER PRIMARY KEY CHECK (id = 1),
          owner_alliance_id TEXT,
          capture_progress REAL NOT NULL,
          state TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS throne_scores (
          alliance_id TEXT PRIMARY KEY,
          points INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS resource_nodes (
          id TEXT PRIMARY KEY,
          kind TEXT NOT NULL,
          level INTEGER NOT NULL,
          x REAL NOT NULL,
          y REAL NOT NULL,
          remaining REAL NOT NULL
        );
        CREATE TABLE IF NOT EXISTS battle_reports (
          id TEXT PRIMARY KEY,
          payload_json TEXT NOT NULL,
          created_at INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS flags (
          id TEXT PRIMARY KEY,
          alliance_id TEXT NOT NULL,
          x REAL NOT NULL,
          y REAL NOT NULL,
          radius REAL NOT NULL
        );
        CREATE TABLE IF NOT EXISTS queues (
          id TEXT PRIMARY KEY,
          player_id TEXT NOT NULL,
          type TEXT NOT NULL,
          data_json TEXT NOT NULL,
          start_ms INTEGER NOT NULL,
          eta_ms INTEGER NOT NULL,
          state TEXT NOT NULL
        );
        INSERT INTO _sql_schema_migrations (id) VALUES (1);
      `);
    }
  }

  private loadMapDefs() {
    const map = getMap();
    this.regions = map.regions;
    this.passDefs = map.passes;
    this.mountainBelt = map.constants.mountainBelt;
    this.passWidth = map.constants.passWidth;
  }

  private loadState() {
    const meta = this.ctx.storage.sql
      .exec<{ season_day: number; last_tick_ms: number }>("SELECT season_day, last_tick_ms FROM world_meta WHERE id = 1")
      .toArray()[0];
    if (meta) this.seasonDay = meta.season_day;

    for (const row of this.ctx.storage.sql
      .exec<any>("SELECT * FROM map_cities")
      .toArray()) {
      this.cities.set(row.player_id, {
        playerId: row.player_id,
        name: row.name,
        allianceId: row.alliance_id,
        x: row.x,
        y: row.y,
        hallLevel: row.hall_level,
        regionId: row.region_id,
        ap: 1000, // Migration stub
        lastApMs: Date.now(),
      });
    }

    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM flags").toArray()) {
      this.flags.set(row.id, {
        id: row.id,
        allianceId: row.alliance_id,
        x: row.x,
        y: row.y,
        radius: row.radius,
      });
    }

    const throneRow = this.ctx.storage.sql.exec<any>("SELECT * FROM throne WHERE id = 1").toArray()[0];
    if (throneRow) {
      this.throne.ownerAllianceId = throneRow.owner_alliance_id;
      this.throne.captureProgress = throneRow.capture_progress;
      this.throne.state = throneRow.state;
    }
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM throne_scores").toArray()) {
      this.throneScores.set(row.alliance_id, row.points);
    }
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM passes").toArray()) {
      this.passes.set(row.pass_id, {
        id: row.pass_id,
        ownerAllianceId: row.owner_alliance_id,
        captureProgress: row.capture_progress,
        state: row.state,
        level: row.level,
        from: row.from_region,
        to: row.to_region,
        x: row.x,
        y: row.y,
        unlockDay: row.unlock_day,
      });
    }

    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM marches").toArray()) {
      this.marches.set(row.id, {
        id: row.id,
        ownerPlayerId: row.owner_player_id,
        allianceId: row.alliance_id,
        fromX: row.from_x,
        fromY: row.from_y,
        toX: row.to_x,
        toY: row.to_y,
        startMs: row.start_ms,
        etaMs: row.eta_ms,
        troops: JSON.parse(row.troops_json),
        state: row.state,
        targetType: row.target_type,
        targetId: row.target_id,
        payload: row.payload_json ? JSON.parse(row.payload_json) : undefined,
      });
    }

    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM resource_nodes").toArray()) {
      this.nodes.set(row.id, {
        id: row.id,
        kind: row.kind,
        level: row.level,
        x: row.x,
        y: row.y,
        remaining: row.remaining,
      });
    }

    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM queues").toArray()) {
      this.queues.set(row.id, {
        id: row.id,
        playerId: row.player_id,
        type: row.type,
        data: JSON.parse(row.data_json),
        startMs: row.start_ms,
        etaMs: row.eta_ms,
        state: row.state,
      });
    }

    this.reports = this.ctx.storage.sql
      .exec<any>("SELECT payload_json FROM battle_reports ORDER BY created_at DESC LIMIT 50")
      .toArray()
      .map((r) => JSON.parse(r.payload_json));
  }

  private seedWorld() {
    const now = nowMs();
    this.ctx.storage.sql.exec(
      "INSERT OR REPLACE INTO world_meta (id, season_day, last_tick_ms) VALUES (1, ?, ?)",
      0,
      now,
    );
    this.seasonDay = 0;
    this.ctx.storage.sql.exec(
      "INSERT OR IGNORE INTO throne (id, owner_alliance_id, capture_progress, state) VALUES (1, NULL, 0, 'open')"
    );

    for (const p of this.passDefs) {
      const unlock = p.unlock_day ?? 0;
      const ent: PassEntity = {
        id: p.id,
        ownerAllianceId: null,
        captureProgress: 0,
        state: "open",
        level: p.level,
        from: p.from,
        to: p.to,
        x: p.center[0],
        y: p.center[1],
        unlockDay: unlock,
      };
      this.passes.set(p.id, ent);
      this.ctx.storage.sql.exec(
        `INSERT OR REPLACE INTO passes
         (pass_id, owner_alliance_id, capture_progress, state, level, from_region, to_region, x, y, unlock_day)
         VALUES (?, NULL, 0, 'open', ?, ?, ?, ?, ?, ?)`,
        p.id,
        p.level,
        p.from,
        p.to,
        p.center[0],
        p.center[1],
        unlock,
      );
    }

    // seed a few resource nodes near each zone1 spawn
    const kinds: Array<NodeEntity["kind"]> = ["food", "wood", "stone", "gold", "barb"];
    let i = 0;
    for (const r of this.regions.filter((z) => z.zone_id === 1)) {
      const anchor = r.spawn_anchor || [ (r.aabb[0]+r.aabb[2])/2, (r.aabb[1]+r.aabb[3])/2 ];
      for (let k = 0; k < 5; k++) {
        const kind = kinds[k % kinds.length];
        const id = `node_${r.id}_${k}`;
        const ent: NodeEntity = {
          id,
          kind,
          level: 1 + (k % 3),
          x: anchor[0] + (k - 2) * 15,
          y: anchor[1] + ((k % 2) * 12 - 6),
          remaining: kind === "barb" ? 100 : 5000,
        };
        this.nodes.set(id, ent);
        this.ctx.storage.sql.exec(
          `INSERT OR REPLACE INTO resource_nodes (id, kind, level, x, y, remaining) VALUES (?, ?, ?, ?, ?, ?)`,
          ent.id,
          ent.kind,
          ent.level,
          ent.x,
          ent.y,
          ent.remaining,
        );
        i++;
      }
    }
  }

  private persistCity(c: CityEntity) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO map_cities (player_id, name, alliance_id, x, y, hall_level, region_id)
       VALUES (?, ?, ?, ?, ?, ?, ?)`,
      c.playerId,
      c.name,
      c.allianceId,
      c.x,
      c.y,
      c.hallLevel,
      c.regionId,
    );
  }

  private persistPass(p: PassEntity) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO passes
       (pass_id, owner_alliance_id, capture_progress, state, level, from_region, to_region, x, y, unlock_day)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      p.id,
      p.ownerAllianceId,
      p.captureProgress,
      p.state,
      p.level,
      p.from,
      p.to,
      p.x,
      p.y,
      p.unlockDay,
    );
  }

  private persistMarch(m: MarchEntity) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO marches
       (id, owner_player_id, alliance_id, from_x, from_y, to_x, to_y, start_ms, eta_ms, troops_json, state, target_type, target_id, payload_json)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      m.id,
      m.ownerPlayerId,
      m.allianceId,
      m.fromX,
      m.fromY,
      m.toX,
      m.toY,
      m.startMs,
      m.etaMs,
      JSON.stringify(m.troops),
      m.state,
      m.targetType,
      m.targetId,
      m.payload ? JSON.stringify(m.payload) : null,
    );
  }

  private persistNode(n: NodeEntity) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO resource_nodes (id, kind, level, x, y, remaining) VALUES (?, ?, ?, ?, ?, ?)`,
      n.id,
      n.kind,
      n.level,
      n.x,
      n.y,
      n.remaining,
    );
  }

  private persistThrone() {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO throne (id, owner_alliance_id, capture_progress, state) VALUES (1, ?, ?, ?)`,
      this.throne.ownerAllianceId,
      this.throne.captureProgress,
      this.throne.state
    );
  }

  private persistThroneScore(allianceId: string, points: number) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO throne_scores (alliance_id, points) VALUES (?, ?)`,
      allianceId,
      points
    );
  }

  private persistQueue(q: QueueEntity) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO queues
       (id, player_id, type, data_json, start_ms, eta_ms, state)
       VALUES (?, ?, ?, ?, ?, ?, ?)`,
      q.id,
      q.playerId,
      q.type,
      JSON.stringify(q.data),
      q.startMs,
      q.etaMs,
      q.state,
    );
  }

  private saveReport(report: any) {
    this.reports.unshift(report);
    this.reports = this.reports.slice(0, 50);
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO battle_reports (id, payload_json, created_at) VALUES (?, ?, ?)`,
      report.id,
      JSON.stringify(report),
      report.createdAt,
    );
  }

  private snapshot() {
    return {
      seasonDay: this.seasonDay,
      cities: [...this.cities.values()],
      throne: this.throne,
      throneScores: [...this.throneScores.entries()],
      passes: [...this.passes.values()],
      marches: [...this.marches.values()].filter((m) => m.state === "moving"),
      nodes: [...this.nodes.values()],
      flags: [...this.flags.values()],
      reports: this.reports.slice(0, 10),
      map: {
        width: getMap().width,
        height: getMap().height,
        regions: this.regions.map((r) => ({ id: r.id, zone_id: r.zone_id, name: r.name, aabb: r.aabb })),
      },
    };
  }

  private broadcast(data: unknown) {
    const msg = JSON.stringify(data);
    for (const ws of this.ctx.getWebSockets()) {
      try {
        ws.send(msg);
      } catch {
        // ignore broken sockets
      }
    }
  }

  private ensureAlarm() {
    // schedule next tick if active work
    const hasWork =
      this.ctx.getWebSockets().length > 0 ||
      [...this.marches.values()].some((m) => m.state === "moving") ||
      [...this.passes.values()].some((p) => p.state === "contested");
    if (hasWork) {
      void this.ctx.storage.setAlarm(Date.now() + 1000);
    }
  }

  async alarm(): Promise<void> {
    await this.tick();
    this.ensureAlarm();
  }

  private async tick() {
    const now = nowMs();
    let changed = false;

    // Process Queues
    const completedQueues = [];
    for (const q of this.queues.values()) {
      if (q.state === "running" && now >= q.etaMs) {
        q.state = "completed";
        this.persistQueue(q);
        completedQueues.push(q);
        changed = true;
      }
    }
    for (const q of completedQueues) {
      if (q.type === "build") {
        await this.env.DB.prepare(`UPDATE buildings SET level=? WHERE player_id=? AND building_id=?`)
          .bind(q.data.level, q.playerId, q.data.buildingId)
          .run();
        if (q.data.buildingId === "city_hall") {
          const c = this.cities.get(q.playerId);
          if (c) {
            c.hallLevel = q.data.level;
            this.persistCity(c);
            this.broadcast({ type: "city_upsert", city: c });
          }
        }
      } else if (q.type === "train" || q.type === "heal") {
        for (const [u, count] of Object.entries(q.data.troops || {})) {
          await this.env.DB.prepare(
            `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)
             ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`
          ).bind(q.playerId, u, Number(count)).run();
        }
      }
    }

    for (const m of this.marches.values()) {
      if (m.state === "moving") {
        if (now >= m.etaMs) {
          await this.resolveMarchArrival(m, now);
          changed = true;
        }
      } else if (m.state === "gathering") {
        if (now >= m.etaMs) {
          const node = this.nodes.get(m.targetId);
          if (node) {
            const gathered = node.remaining;
            node.remaining = 0;
            this.persistNode(node);
            m.payload = { kind: node.kind, amount: gathered };
          }
          this.spawnReturnMarch(m, now);
          changed = true;
        }
      } else if (m.state === "returning") {
        if (now >= m.etaMs) {
          await this.resolveMarchReturn(m);
          changed = true;
        }
      }
    }

    // capture decay if contested idle? keep simple: no decay

    if (this.throne.ownerAllianceId && this.seasonDay >= this.throne.unlockDay) {
      const current = this.throneScores.get(this.throne.ownerAllianceId) || 0;
      this.throneScores.set(this.throne.ownerAllianceId, current + 1);
      this.persistThroneScore(this.throne.ownerAllianceId, current + 1);
      changed = true;
    }

    this.ctx.storage.sql.exec(
      "INSERT OR REPLACE INTO world_meta (id, season_day, last_tick_ms) VALUES (1, ?, ?)",
      this.seasonDay,
      now,
    );

    if (changed) {
      this.broadcast({ type: "snapshot", ...this.snapshot() });
    } else if (this.ctx.getWebSockets().length > 0) {
      // lightweight march progress updates
      const moving = [...this.marches.values()].filter((m) => m.state === "moving");
      if (moving.length) {
        this.broadcast({
          type: "march_update",
          marches: moving.map((m) => ({
            id: m.id,
            progress: Math.min(1, (now - m.startMs) / Math.max(1, m.etaMs - m.startMs)),
            etaMs: m.etaMs,
          })),
        });
      }
    }
  }

  private async resolveMarchArrival(m: MarchEntity, now: number) {
    if (m.targetType === "pass") {
      const pass = this.passes.get(m.targetId);
      if (!pass) {
        this.spawnReturnMarch(m, now);
        return;
      }
      if (pass.unlockDay > this.seasonDay) {
        this.spawnReturnMarch(m, now);
        return;
      }

      // NPC garrison scales with level
      const garrisonCount = 80 * pass.level;
      const defenderTroops: Troops = pass.ownerAllianceId
        ? { infantry_t1: garrisonCount, archer_t1: Math.floor(garrisonCount / 2) }
        : { infantry_t1: Math.floor(garrisonCount * 0.6) };

      // if same alliance already owns, reinforce capture to 100
      if (pass.ownerAllianceId && m.allianceId && pass.ownerAllianceId === m.allianceId) {
        pass.captureProgress = 100;
        pass.state = "open";
        this.persistPass(pass);
        this.spawnReturnMarch(m, now);
        this.broadcast({ type: "pass_owner_changed", pass });
        return;
      }

      // P2-T1: القائد المرافق للمسيرة يمنح باف هجوم من مهاراته
      const attackerCommander = await this.fetchMarchCommander(m.id);

      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: m.troops },
        { name: pass.ownerAllianceId || "neutral_guard", troops: defenderTroops },
        1,
        attackerCommander,
        undefined,
      );

      const report = {
        id: newId("br"),
        createdAt: now,
        kind: "pass_attack",
        passId: pass.id,
        attackerPlayerId: m.ownerPlayerId,
        attackerAllianceId: m.allianceId,
        result,
      };
      this.saveReport(report);
      await this.grantCommanderXp(m.id, totalTroops(result.defenderLosses));

      // P2-T2: الجرحى الخطيرون للمستشفى حسب السعة (الفائض يموت)
      const hospital = await this.admitToHospital(m.ownerPlayerId, result.attackerSplit.severely);
      report.hospital = hospital;
      // كل الخسائر (موتى + جرحى) خرجت من رصيد المسيرة
      await this.deductMarchLosses(m.ownerPlayerId, result.attackerLosses);

      if (result.winner === "attacker") {
        const gain = Math.min(100, 35 + Math.floor(troopPower(result.attackerRemaining) / 20));
        // if attacking enemy-owned, reset then gain
        if (pass.ownerAllianceId && pass.ownerAllianceId !== m.allianceId) {
          pass.captureProgress = gain;
        } else {
          pass.captureProgress = Math.min(100, pass.captureProgress + gain);
        }
        pass.state = "contested";
        if (pass.captureProgress >= 100) {
          pass.ownerAllianceId = m.allianceId;
          pass.captureProgress = 100;
          pass.state = "open";
        }
        this.persistPass(pass);
        this.broadcast({ type: "pass_owner_changed", pass, report });
      } else {
        this.broadcast({ type: "battle_report", report });
      }

      m.troops = result.attackerRemaining;
      this.spawnReturnMarch(m, now);
      return;
    }

    if (m.targetType === "throne") {
      if (this.throne.unlockDay > this.seasonDay) {
        this.spawnReturnMarch(m, now);
        return;
      }
      const garrisonCount = 2000;
      const defenderTroops: Troops = this.throne.ownerAllianceId
        ? { infantry_t1: garrisonCount, archer_t1: Math.floor(garrisonCount / 2) }
        : { infantry_t1: Math.floor(garrisonCount * 0.8) };

      if (this.throne.ownerAllianceId && m.allianceId && this.throne.ownerAllianceId === m.allianceId) {
        this.throne.captureProgress = 100;
        this.throne.state = "open";
        this.persistThrone();
        this.spawnReturnMarch(m, now);
        return;
      }

      const throneAttackerCommander = await this.fetchMarchCommander(m.id);

      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: m.troops },
        { name: this.throne.ownerAllianceId || "neutral_guard", troops: defenderTroops },
        3,
        throneAttackerCommander,
        undefined,
      );

      const report = {
        id: newId("br"),
        createdAt: now,
        kind: "throne_attack",
        attackerPlayerId: m.ownerPlayerId,
        attackerAllianceId: m.allianceId,
        result,
      };
      this.saveReport(report);
      await this.grantCommanderXp(m.id, totalTroops(result.defenderLosses));

      const throneHospital = await this.admitToHospital(m.ownerPlayerId, result.attackerSplit.severely);
      report.hospital = throneHospital;
      await this.deductMarchLosses(m.ownerPlayerId, result.attackerLosses);

      if (result.winner === "attacker") {
        const gain = Math.min(100, 35 + Math.floor(troopPower(result.attackerRemaining) / 20));
        if (this.throne.ownerAllianceId && this.throne.ownerAllianceId !== m.allianceId) {
          this.throne.captureProgress = gain;
        } else {
          this.throne.captureProgress = Math.min(100, this.throne.captureProgress + gain);
        }
        this.throne.state = "contested";
        if (this.throne.captureProgress >= 100) {
          this.throne.ownerAllianceId = m.allianceId;
          this.throne.captureProgress = 100;
          this.throne.state = "open";
        }
        this.persistThrone();
      }

      m.troops = result.attackerRemaining;
      this.spawnReturnMarch(m, now);
      return;
    }

    if (m.targetType === "resource" || m.targetType === "barb") {
      const node = this.nodes.get(m.targetId);
      if (node) {
        if (node.kind === "barb") {
          const def: Troops = { infantry_t1: 40 * node.level };
          const barbCommander = await this.fetchMarchCommander(m.id);
          const result = resolveCombat({ name: m.ownerPlayerId, troops: m.troops }, { name: "barb", troops: def }, 1, barbCommander, undefined);
          const report = {
            id: newId("br"),
            createdAt: now,
            kind: "barb",
            nodeId: node.id,
            attackerPlayerId: m.ownerPlayerId,
            result,
          };
          this.saveReport(report);
          await this.grantCommanderXp(m.id, totalTroops(result.defenderLosses));
          const barbHospital = await this.admitToHospital(m.ownerPlayerId, result.attackerSplit.severely);
          report.hospital = barbHospital;
          await this.deductMarchLosses(m.ownerPlayerId, result.attackerLosses);
          m.troops = result.attackerRemaining;
          if (result.winner === "attacker") {
            node.remaining = Math.max(0, node.remaining - 50);
            this.persistNode(node);
          }
          this.broadcast({ type: "battle_report", report });
          this.spawnReturnMarch(m, now);
        } else {
          m.state = "gathering";
          const rate = 0.5 * totalTroops(m.troops); // units/sec
          const durationSec = node.remaining / rate;
          m.etaMs = now + durationSec * 1000;
          this.persistMarch(m);
        }
      } else {
        this.spawnReturnMarch(m, now);
      }
      return;
    }

    this.spawnReturnMarch(m, now);
  }

  private spawnReturnMarch(m: MarchEntity, now: number) {
    const city = this.cities.get(m.ownerPlayerId);
    if (!city) return;
    m.state = "returning";
    m.fromX = m.toX;
    m.fromY = m.toY;
    m.toX = city.x;
    m.toY = city.y;
    m.startMs = now;
    m.etaMs = now + marchDurationMs(dist(m.fromX, m.fromY, m.toX, m.toY), 40);
    this.persistMarch(m);
    this.broadcast({ type: "march_returning", march: m });
  }

  private async resolveMarchReturn(m: MarchEntity) {
    m.state = "returned";
    this.persistMarch(m);

    // Transfer troops to home
    for (const [u, count] of Object.entries(m.troops)) {
      await this.env.DB.prepare(
        `UPDATE troops SET count=count-? WHERE player_id=? AND unit_id=? AND status='marching'`
      ).bind(Number(count), m.ownerPlayerId, u).run();
      
      await this.env.DB.prepare(
        `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)
         ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`
      ).bind(m.ownerPlayerId, u, Number(count)).run();
    }

    // Add gathered resources
    if (m.payload?.amount) {
      const kind = m.payload.kind;
      if (kind === "food" || kind === "wood" || kind === "stone" || kind === "gold") {
        await this.env.DB.prepare(
          `UPDATE cities SET ${kind}=${kind}+? WHERE player_id=?`
        ).bind(m.payload.amount, m.ownerPlayerId).run();
      }
    }

    // P2-T1: خبرة القائد بعد القتال (تُنظف مشاركة القائد مع المسيرة)
    try {
      const mc = await this.env.DB.prepare(
        "SELECT commander_id, skills_json FROM march_commanders WHERE march_id = ?",
      ).bind(m.id).first<{ commander_id: string; skills_json: string }>();
      if (mc) {
        // XP يُحسب في resolveMarchArrival عبر battle reports؛ هنا نكتفي بالتنظيف
        await this.env.DB.prepare("DELETE FROM march_commanders WHERE march_id = ?").bind(m.id).run();
      }
    } catch {
      // الجدول قد لا يكون مُرحّلاً بعد
    }
  }

  // -------- RPC-ish API via fetch --------
  async fetch(request: Request): Promise<Response> {
    const url = new URL(request.url);
    const path = url.pathname;

    if (request.headers.get("Upgrade") === "websocket") {
      return this.handleWs(request);
    }

    if (path.endsWith("/snapshot") && request.method === "GET") {
      return Response.json(this.snapshot());
    }

    if (path.endsWith("/leaderboard") && request.method === "GET") {
      const scores = [...this.throneScores.entries()].map(([allianceId, points]) => ({ allianceId, points }));
      scores.sort((a, b) => b.points - a.points);
      return Response.json({ scores });
    }

    if (path.endsWith("/upsert-city") && request.method === "POST") {
      const body = await request.json<any>();
      const c: CityEntity = {
        playerId: body.playerId,
        name: body.name,
        allianceId: body.allianceId ?? null,
        x: body.x,
        y: body.y,
        hallLevel: body.hallLevel ?? 1,
        regionId: body.regionId,
        ap: body.ap ?? 1000,
        lastApMs: body.lastApMs ?? Date.now(),
      };
      this.cities.set(c.playerId, c);
      this.persistCity(c);
      this.broadcast({ type: "city_upsert", city: c });
      this.ensureAlarm();
      return Response.json({ ok: true, city: c });
    }

    if (path.endsWith("/set-alliance") && request.method === "POST") {
      const body = await request.json<any>();
      const c = this.cities.get(body.playerId);
      if (!c) return Response.json({ error: "city_not_found" }, { status: 404 });
      c.allianceId = body.allianceId;
      this.persistCity(c);
      this.broadcast({ type: "city_upsert", city: c });
      return Response.json({ ok: true, city: c });
    }

    if (path.endsWith("/build-flag") && request.method === "POST") {
      const body = await request.json<any>();
      const flag: AllianceFlag = {
        id: newId("flg"),
        allianceId: body.allianceId,
        x: body.x,
        y: body.y,
        radius: body.radius ?? 15,
      };
      this.flags.set(flag.id, flag);
      this.ctx.storage.sql.exec(
        `INSERT INTO flags (id, alliance_id, x, y, radius) VALUES (?, ?, ?, ?, ?)`,
        flag.id, flag.allianceId, flag.x, flag.y, flag.radius
      );
      this.broadcast({ type: "flag_created", flag });
      return Response.json({ ok: true, flag });
    }

    if (path.endsWith("/march") && request.method === "POST") {
      const body = await request.json<any>();
      try {
        const march = this.createMarch(body);
        this.ensureAlarm();
        return Response.json({ ok: true, march });
      } catch (e: any) {
        return Response.json({ error: e.message || "march_failed" }, { status: 400 });
      }
    }

    if (path.endsWith("/pass-attack") && request.method === "POST") {
      const body = await request.json<any>();
      try {
        const march = this.createMarch({
          ...body,
          targetType: "pass",
          targetId: body.passId,
        });
        this.ensureAlarm();
        return Response.json({ ok: true, march });
      } catch (e: any) {
        return Response.json({ error: e.message || "attack_failed" }, { status: 400 });
      }
    }

    if (path.endsWith("/admin") && request.method === "POST") {
      const body = await request.json<any>();
      if (body.action === "tick") {
        // force-complete in-flight marches for deterministic tests/debug
        if (body.force) {
          const now = nowMs();
          for (const m of this.marches.values()) {
            if (m.state === "moving" || m.state === "returning" || m.state === "gathering") {
              m.etaMs = now;
            }
          }
          for (const q of this.queues.values()) {
            if (q.state === "running") {
              q.etaMs = now;
            }
          }
        }
        await this.tick();
        return Response.json({ ok: true, snapshot: this.snapshot() });
      }
      if (body.action === "set_day") {
        this.seasonDay = Number(body.day) || 0;
        this.ctx.storage.sql.exec(
          "INSERT OR REPLACE INTO world_meta (id, season_day, last_tick_ms) VALUES (1, ?, ?)",
          this.seasonDay,
          nowMs(),
        );
        this.broadcast({ type: "season_day", day: this.seasonDay });
        return Response.json({ ok: true, seasonDay: this.seasonDay });
      }
      return Response.json({ error: "unknown_action" }, { status: 400 });
    }

    if (path.endsWith("/queue/add") && request.method === "POST") {
      const body = await request.json<any>();
      const q: QueueEntity = {
        id: body.id || newId("q"),
        playerId: body.playerId,
        type: body.type,
        data: body.data,
        startMs: body.startMs,
        etaMs: body.etaMs,
        state: "running",
      };
      this.queues.set(q.id, q);
      this.persistQueue(q);
      this.ensureAlarm();
      return Response.json({ ok: true, queue: q });
    }

    if (path.endsWith("/queue/speedup") && request.method === "POST") {
      const body = await request.json<any>();
      const q = this.queues.get(body.queueId);
      if (!q || q.state !== "running") return Response.json({ error: "queue_not_found" }, { status: 404 });
      q.etaMs = Math.max(nowMs(), q.etaMs - (body.seconds * 1000));
      this.persistQueue(q);
      this.ensureAlarm();
      return Response.json({ ok: true, queue: q });
    }

    return Response.json({ error: "not_found", path }, { status: 404 });
  }

  private createMarch(body: any): MarchEntity {
    const playerId = body.playerId as string;
    const city = this.cities.get(playerId);
    if (!city) throw new Error("player_city_not_on_map");

    const troops = (body.troops || {}) as Troops;
    if (totalTroops(troops) <= 0) throw new Error("no_troops");

    let toX = Number(body.toX);
    let toY = Number(body.toY);
    let targetType = (body.targetType || "point") as MarchEntity["targetType"];
    let targetId = String(body.targetId || "point");

    if (targetType === "throne" || body.targetType === "throne") {
      targetType = "throne";
      targetId = "throne";
      toX = this.throne.x;
      toY = this.throne.y;
    }

    if (targetType === "pass" || body.passId) {
      targetType = "pass";
      targetId = String(body.passId || body.targetId);
      const pass = this.passes.get(targetId);
      if (!pass) throw new Error("pass_not_found");
      if (pass.unlockDay > this.seasonDay) throw new Error("pass_locked");
      toX = pass.x;
      toY = pass.y;
    } else if (targetType === "resource" || targetType === "barb") {
      const node = this.nodes.get(targetId);
      if (!node) throw new Error("node_not_found");
      toX = node.x;
      toY = node.y;
      if (node.kind === "barb") {
        targetType = "barb";
        // AP deduction
        const apCost = 40 + node.level * 10;
        const now = Date.now();
        const apRegen = Math.floor((now - city.lastApMs) / 1000) * 1; // 1 AP per second
        city.ap = Math.min(1000, city.ap + apRegen);
        city.lastApMs = now;
        if (city.ap < apCost) throw new Error("not_enough_ap");
        city.ap -= apCost;
        this.persistCity(city);
      }
      else targetType = "resource";
    } else if (targetType === "city") {
      const enemy = this.cities.get(targetId);
      if (!enemy) throw new Error("target_city_not_found");
      toX = enemy.x;
      toY = enemy.y;
    }

    if (!Number.isFinite(toX) || !Number.isFinite(toY)) throw new Error("bad_target_coords");

    const allianceId = city.allianceId;
    const canTraverse = (passId: string) => {
      // For attack march TO a pass, allow path to that pass without owning it.
      if (targetType === "pass" && passId === targetId) return true;
      const p = this.passes.get(passId);
      if (!p) return false;
      if (!p.ownerAllianceId) return false;
      return !!allianceId && p.ownerAllianceId === allianceId;
    };

    // Same-region resource/barb: skip strict pass ownership checks by short-circuit path if close and same region
    const sameRegionTarget =
      (targetType === "resource" || targetType === "barb") &&
      this.regionOf(city.x, city.y) === this.regionOf(toX, toY);

    let plan = planMarch(
      { x: city.x, y: city.y },
      { x: toX, y: toY },
      this.regions,
      this.passDefs,
      this.mountainBelt,
      this.passWidth,
      canTraverse,
    );

    if (!plan.ok && sameRegionTarget) {
      plan = { ok: true, distance: dist(city.x, city.y, toX, toY), crossedPasses: [] };
    }

    // attacking a pass or throne: allow even if path flagged, use euclidean distance
    if (!plan.ok && (targetType === "pass" || targetType === "throne")) {
      plan = { ok: true, distance: dist(city.x, city.y, toX, toY), crossedPasses: [targetId] };
    }

    if (!plan.ok) throw new Error(plan.reason || "illegal_path");

    const start = nowMs();
    const eta = start + marchDurationMs(plan.distance, 40);
    const march: MarchEntity = {
      id: newId("m"),
      ownerPlayerId: playerId,
      allianceId,
      fromX: city.x,
      fromY: city.y,
      toX,
      toY,
      startMs: start,
      etaMs: eta,
      troops,
      state: "moving",
      targetType,
      targetId,
    };
    this.marches.set(march.id, march);
    this.persistMarch(march);

    // P2-T1: تسجيل القائد المرافق للمسيرة (إن أُرسل وأكّده الـ router بعد التحقق من الملكية)
    if (body.primaryCommanderId) {
      try {
        await this.env.DB.prepare(
          `INSERT OR REPLACE INTO march_commanders (march_id, player_id, commander_id, skills_json, created_at)
           VALUES (?, ?, ?, ?, ?)`,
        )
          .bind(
            march.id,
            playerId,
            String(body.primaryCommanderId),
            JSON.stringify(body.commanderSkills || [1, 1, 1]),
            nowMs(),
          )
          .run();
      } catch {
        // الجدول قد لا يكون مُرحّلاً بعد — المسيرة تكمل بدون قائد
      }
    }

    this.broadcast({ type: "march_created", march });
    return march;
  }

  /** P2-T1: جلب القائد المرافق لمسيرة من D1 (إن وُجد) */
  private async fetchMarchCommander(marchId: string): Promise<CommanderInstance | undefined> {
    try {
      const row = await this.env.DB.prepare(
        "SELECT commander_id, skills_json FROM march_commanders WHERE march_id = ?",
      )
        .bind(marchId)
        .first<{ commander_id: string; skills_json: string }>();
      if (!row) return undefined;
      return { commanderId: row.commander_id, level: 1, skills: JSON.parse(row.skills_json || "[1,1,1]") };
    } catch {
      return undefined; // الجدول قد لا يكون مُرحّلاً بعد
    }
  }

  /** P2-T1: منح خبرة للقائد بعد قتال + رفع مستواه تلقائياً */
  private async grantCommanderXp(marchId: string, kills: number) {
    if (kills <= 0) return;
    try {
      const mc = await this.env.DB.prepare(
        "SELECT commander_id, player_id FROM march_commanders WHERE march_id = ?",
      ).bind(marchId).first<{ commander_id: string; player_id: string }>();
      if (!mc) return;
      const pc = await this.env.DB.prepare(
        "SELECT level, xp FROM player_commanders WHERE player_id = ? AND commander_id = ?",
      ).bind(mc.player_id, mc.commander_id).first<{ level: number; xp: number }>();
      if (!pc) return;
      const xpGain = kills * 2; // 2 خبرة لكل قتيل
      let level = pc.level;
      let xp = pc.xp + xpGain;
      while (level < COMMANDER_CONSTANTS.max_level && xp >= xpForLevel(level)) {
        xp -= xpForLevel(level);
        level++;
      }
      if (level >= COMMANDER_CONSTANTS.max_level) xp = 0;
      await this.env.DB.prepare(
        "UPDATE player_commanders SET level = ?, xp = ? WHERE player_id = ? AND commander_id = ?",
      ).bind(level, xp, mc.player_id, mc.commander_id).run();
    } catch {
      // الجدول قد لا يكون مُرحّلاً بعد
    }
  }

  /**
   * P2-T2: استقبال الجرحى الخطيرين في مستشفى اللاعب حسب السعة.
   * المقبولون يُسجلون status='severely_wounded'؛ الفائض يموت.
   * يعيد ملخصاً يُضمَّن في تقرير المعركة.
   */
  private async admitToHospital(
    playerId: string,
    severely: Troops,
  ): Promise<{ admitted: Troops; died: Troops; capacity: number }> {
    const zero: Troops = {};
    try {
      if (totalTroops(severely) <= 0) return { admitted: zero, died: zero, capacity: 0 };

      const bRows = await this.env.DB.prepare(
        "SELECT level FROM buildings WHERE player_id = ? AND building_id = 'hospital'",
      ).bind(playerId).first<{ level: number }>();
      const hospitalLevel = bRows?.level || 0;

      const wRows = await this.env.DB.prepare(
        "SELECT unit_id, count FROM troops WHERE player_id = ? AND status = 'severely_wounded'",
      ).bind(playerId).all<{ unit_id: string; count: number }>();
      const already: Troops = {};
      for (const r of wRows.results || []) already[r.unit_id] = r.count;

      const { admitted, died } = admitWounded(severely, already, hospitalLevel);

      for (const [u, c] of Object.entries(admitted)) {
        await this.env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'severely_wounded', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`,
        ).bind(playerId, u, Number(c)).run();
      }

      const capacity = hospitalCapacity(hospitalLevel);
      return { admitted, died, capacity };
    } catch {
      return { admitted: zero, died: zero, capacity: 0 };
    }
  }

  /** P2-T2: خصم خسائر المعركة من رصيد 'marching' (الناجون يُخصمون لاحقاً عند العودة للمدينة) */
  private async deductMarchLosses(playerId: string, losses: Troops) {
    for (const [u, c] of Object.entries(losses)) {
      if (Number(c) <= 0) continue;
      await this.env.DB.prepare(
        `UPDATE troops SET count = MAX(0, count - ?) WHERE player_id = ? AND unit_id = ? AND status = 'marching'`,
      ).bind(Number(c), playerId, u).run();
    }
  }

  private regionOf(x: number, y: number): string | null {
    for (const r of this.regions) {
      const [x0, y0, x1, y1] = r.aabb;
      if (x >= x0 && x <= x1 && y >= y0 && y <= y1) return r.id;
    }
    return null;
  }

  private handleWs(request: Request): Response {
    const pair = new WebSocketPair();
    const [client, server] = Object.values(pair);
    this.ctx.acceptWebSocket(server);
    server.serializeAttachment({ playerId: "", aoi: undefined } satisfies Attach);
    this.ensureAlarm();
    return new Response(null, { status: 101, webSocket: client });
  }

  async webSocketMessage(ws: WebSocket, message: string | ArrayBuffer) {
    const text = typeof message === "string" ? message : new TextDecoder().decode(message);
    let msg: any;
    try {
      msg = JSON.parse(text);
    } catch {
      ws.send(JSON.stringify({ type: "error", error: "bad_json" }));
      return;
    }

    const att = (ws.deserializeAttachment() || { playerId: "" }) as Attach;

    if (msg.type === "hello") {
      att.playerId = String(msg.playerId || "");
      ws.serializeAttachment(att);
      ws.send(JSON.stringify({ type: "hello_ok", playerId: att.playerId }));
      ws.send(JSON.stringify({ type: "snapshot", ...this.snapshot() }));
      return;
    }

    if (msg.type === "aoi_sub") {
      att.aoi = { x: Number(msg.x) || 0, y: Number(msg.y) || 0, r: Number(msg.r) || 100 };
      ws.serializeAttachment(att);
      ws.send(JSON.stringify({ type: "aoi_ok", aoi: att.aoi }));
      return;
    }

    if (msg.type === "march_create" || msg.type === "pass_attack") {
      try {
        const body = {
          playerId: att.playerId || msg.playerId,
          troops: msg.troops,
          targetType: msg.type === "pass_attack" ? "pass" : msg.targetType,
          targetId: msg.targetId,
          passId: msg.passId || msg.targetId,
          toX: msg.toX,
          toY: msg.toY,
          primaryCommanderId: msg.primaryCommanderId,
          commanderSkills: msg.commanderSkills,
        };
        const march = this.createMarch(body);
        this.ensureAlarm();
        ws.send(JSON.stringify({ type: "march_ok", march }));
      } catch (e: any) {
        ws.send(JSON.stringify({ type: "error", error: e.message || "march_failed" }));
      }
      return;
    }

    if (msg.type === "ping") {
      ws.send(JSON.stringify({ type: "pong", t: nowMs() }));
      return;
    }

    ws.send(JSON.stringify({ type: "error", error: "unknown_type" }));
  }

  async webSocketClose(ws: WebSocket, code: number, reason: string) {
    try {
      ws.close(code, reason);
    } catch {
      // ignore
    }
  }

  async webSocketError(ws: WebSocket, error: unknown) {
    try {
      ws.close(1011, "error");
    } catch {
      // ignore
    }
    console.error("ws error", error);
  }
}
