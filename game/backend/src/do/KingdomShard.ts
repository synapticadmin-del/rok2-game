import { DurableObject } from "cloudflare:workers";
import type { Env, Troops } from "../env";
import { getMap, type MapPass, type MapRegion } from "../lib/gameData";
import { newId, nowMs, dist } from "../lib/ids";
import { resolveCombat, totalTroops, troopPower } from "./sim/combat";
import { marchDurationMs, planMarch } from "./sim/pathfinding";
import { COMMANDER_CONSTANTS, xpForLevel, type CommanderInstance } from "./sim/commanders";
import { admitWounded, hospitalCapacity } from "./sim/hospital";
import { researchBuff } from "./sim/research";
import {
  isRegionUnlocked,
  isThroneUnlocked,
  nodeLevelForRegion,
  nodeRichness,
  passUnlockDay,
  seasonDayAt,
  seasonSchedule,
  seasonUnlockState,
  throneUnlockDay,
  SEASON_SERVICE,
  zonesStatus,
  holdScorePerTick,
  coreGarrison,
  coreCaptureGain,
  firstCaptureBonus,
  coreContestActive,
  type CoreObjectiveKind,
} from "./sim/zones";
import {
  eventsStatus,
  activeEvents,
  eventBuff,
  barbExtraPerRegion,
  barbLevelBonus,
  EVENT_CONSTANTS,
} from "./sim/events";

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
  targetType: "pass" | "resource" | "barb" | "city" | "point" | "throne" | "core_objective";
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
  regionId?: string;
  zoneId?: number;
};

type ThroneEntity = {
  ownerAllianceId: string | null;
  captureProgress: number;
  state: "open" | "contested";
  x: number;
  y: number;
  unlockDay: number;
};

// P3-T2: هدف احتلال في قلب Zone 3 (حصن خارجي أو مذبح جانبي) — يسجّل نقاط موسم
type CoreObjective = {
  id: string;
  kind: CoreObjectiveKind;
  ownerAllianceId: string | null;
  captureProgress: number;
  state: "open" | "contested";
  x: number;
  y: number;
  firstCapturedBy: string | null; // أول تحالف احتلّه في الموسم (مكافأة)
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
  // P3-T1: طابع بداية الموسم — خدمة فتح المناطق تحسب اليوم منه زمنياً
  private seasonStartMs = 0;
  private cities = new Map<string, CityEntity>();
  private throne: ThroneEntity = { ownerAllianceId: null, captureProgress: 0, state: "open", x: 1200, y: 1200, unlockDay: 14 };
  private throneScores = new Map<string, number>();
  // P3-T2: أهداف قلب Zone 3 (4 حصون خارجية + 4 مذابح جانبية) — تسجيل نقاط الموسم
  private coreObjectives = new Map<string, CoreObjective>();
  // P3-T3: الأحداث التي أُعلن بدؤها في هذا اليوم (لا يُعاد بث event_started لها)
  private eventsAnnouncedStarted = new Set<string>();
  private passes = new Map<string, PassEntity>();
  private marches = new Map<string, MarchEntity>();
  private nodes = new Map<string, NodeEntity>();
  private flags = new Map<string, AllianceFlag>();
  private queues = new Map<string, QueueEntity>();
  private reports: any[] = [];
  // P2-T4: المناطق التي بُثّ فتحها مسبقاً (لا يُعاد بث zone_unlocked لها)
  private zoneUnlockAnnounced = new Set<string>();

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
          last_tick_ms INTEGER NOT NULL,
          season_start_ms INTEGER NOT NULL DEFAULT 0
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

    if (ver < 2) {
      // P3-T1: خدمة فتح المناطق — طابع بداية الموسم لحساب اليوم زمنياً
      try {
        this.ctx.storage.sql.exec("ALTER TABLE world_meta ADD COLUMN season_start_ms INTEGER NOT NULL DEFAULT 0");
      } catch {
        // العمود موجود مسبقاً (قاعدة جديدة بُنيت مباشرة على ver>=2)
      }
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (2)");
    }

    if (ver < 3) {
      // P3-T2: أهداف قلب Zone 3 (حصون خارجية + مذابح جانبية) لتسجيل نقاط الموسم
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS core_objectives (
          id TEXT PRIMARY KEY,
          kind TEXT NOT NULL,
          owner_alliance_id TEXT,
          capture_progress REAL NOT NULL,
          state TEXT NOT NULL,
          x REAL NOT NULL,
          y REAL NOT NULL,
          first_captured_by TEXT
        );
      `);
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (3)");
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
      .exec<{ season_day: number; last_tick_ms: number; season_start_ms?: number }>("SELECT season_day, last_tick_ms, season_start_ms FROM world_meta WHERE id = 1")
      .toArray()[0];
    if (meta) {
      this.seasonDay = meta.season_day;
      // P3-T1: إن لم يُسجَّل season_start_ms بعد (قواعد قديمة)، نعتبر الموسم بدأ مع أول tick
      this.seasonStartMs = meta.season_start_ms && meta.season_start_ms > 0 ? meta.season_start_ms : meta.last_tick_ms;
    }

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
    // P3-T1: يوم فتح العرش يُشتق دائماً من zones.json (لا قيمة ثابتة)
    this.throne.unlockDay = throneUnlockDay();
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM throne_scores").toArray()) {
      this.throneScores.set(row.alliance_id, row.points);
    }
    // P3-T2: أهداف قلب Zone 3 المحفوظة
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM core_objectives").toArray()) {
      this.coreObjectives.set(row.id, {
        id: row.id,
        kind: row.kind,
        ownerAllianceId: row.owner_alliance_id,
        captureProgress: row.capture_progress,
        state: row.state,
        x: row.x,
        y: row.y,
        firstCapturedBy: row.first_captured_by,
      });
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
      "INSERT OR REPLACE INTO world_meta (id, season_day, last_tick_ms, season_start_ms) VALUES (1, ?, ?, ?)",
      0,
      now,
      now,
    );
    this.seasonDay = 0;
    this.seasonStartMs = now; // P3-T1: الموسم يبدأ لحظة بذر العالم — خدمة الفتح تحسب منها
    // P3-T1: يوم فتح العرش من core_objective.open_day في zones.json — لا قيمة ثابتة
    this.throne.unlockDay = throneUnlockDay();
    this.ctx.storage.sql.exec(
      "INSERT OR IGNORE INTO throne (id, owner_alliance_id, capture_progress, state) VALUES (1, NULL, 0, 'open')"
    );

    // P3-T2: بذر أهداف قلب Zone 3 (4 حصون خارجية + 4 مذابح جانبية) من map_spec
    this.seedCoreObjectives();

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

    // P2-T4: بذر عقد موارد حتمية في كل المناطق (Z1 + Z2 stubs).
    // Zone 1: مستويات ضمن resource_level_range [1,4]؛ Zone 2: [3,6] مع غنى مضاعف.
    // العقدة تقع في منطقة مقفلة زمنياً تبقى مرئية لكن لا يمكن استهدافها قبل الفتح.
    const kinds: Array<NodeEntity["kind"]> = ["food", "wood", "stone", "gold", "barb"];
    const NODES_PER_REGION = 5;
    const MIN_GAP = 8;
    const placed: Array<{ x: number; y: number }> = [];
    for (const r of this.regions) {
      if (r.zone_id > 2) continue; // Zone 3: لا عقد عشوائية (CORE أهداف خاصة)
      const [x0, y0, x1, y1] = r.aabb;
      const w = x1 - x0;
      const h = y1 - y0;
      const cx = x0 + w / 2;
      const cy = y0 + h / 2;
      const rx = w / 2 - MIN_GAP;
      const ry = h / 2 - MIN_GAP;
      for (let k = 0; k < NODES_PER_REGION; k++) {
        const kind = kinds[k % kinds.length];
        const id = `node_${r.id}_${k}`;
        const level = nodeLevelForRegion(r.id, r.zone_id, id);
        const gx = Math.floor(k / 2) - 0.5; // -0.5, 0.5, 0.5
        const gy = (k % 2) - 0.5;           // -0.5, 0.5 متناوبة
        const ent: NodeEntity = {
          id,
          kind,
          level,
          x: cx + gx * rx,
          y: cy + gy * ry,
          remaining: nodeRichness(kind, level, r.zone_id),
          regionId: r.id,
          zoneId: r.zone_id,
        };
        placed.push({ x: ent.x, y: ent.y });
        this.nodes.set(id, ent);
        this.persistNode(ent);
      }
    }
    void placed;

  // P3-T2: بذر أهداف قلب Zone 3 من map_spec.zone3_objectives (حصون + مذابح)
  private seedCoreObjectives() {
    const map = getMap() as any;
    const z3o = map.zone3 ?? map.zone3_objectives;
    if (!z3o) return;
    const spawn = (id: string, kind: CoreObjectiveKind, pos: [number, number]) => {
      if (this.coreObjectives.has(id)) return;
      const ent: CoreObjective = {
        id, kind,
        ownerAllianceId: null, captureProgress: 0, state: "open",
        x: pos[0], y: pos[1], firstCapturedBy: null,
      };
      this.coreObjectives.set(id, ent);
      this.persistCoreObjective(ent);
    };
    for (const f of z3o.outer_forts ?? []) spawn(f.id, "outer_fort", f.pos);
    for (const a of z3o.side_altars ?? []) spawn(a.id, "side_altar", a.pos);
  }

  private persistCoreObjective(o: CoreObjective) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO core_objectives
       (id, kind, owner_alliance_id, capture_progress, state, x, y, first_captured_by)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      o.id, o.kind, o.ownerAllianceId, o.captureProgress, o.state, o.x, o.y, o.firstCapturedBy,
    );
  }

  // P3-T3: تكثيف البرابرة أثناء حدث "غزو البرابرة" — يزرع معسكرات إضافية حتمياً
  // (id يعتمد على اليوم فلا يتكرر الزرع كل tick). يعيد true إن زرع شيئاً جديداً.
  private seedEventBarbarians(extraPerRegion: number): boolean {
    let spawned = false;
    const tickInDay = this.seasonStartMs > 0 ? Math.floor((nowMs() - this.seasonStartMs) / 1000) % 86_400 : 0;
    const lvlBonus = barbLevelBonus(this.seasonDay, tickInDay);
    const hpMult = eventBuff(this.seasonDay, tickInDay, "barb_hp_mult");
    for (const r of this.regions) {
      if (r.zone_id > 2) continue; // البرابرة في Zone 1/2 فقط
      if (!isRegionUnlocked(r.id, r.zone_id, this.seasonDay)) continue;
      const [x0, y0, x1, y1] = r.aabb;
      const w = x1 - x0, h = y1 - y0;
      const cx = x0 + w / 2, cy = y0 + h / 2;
      for (let k = 0; k < extraPerRegion; k++) {
        const id = `event_barb_${this.seasonDay}_${r.id}_${k}`;
        if (this.nodes.has(id)) continue;
        // موقع حتمي من id (زوايا مختلفة داخل المنطقة)
        let hash = 0;
        for (let i = 0; i < id.length; i++) hash = (hash * 31 + id.charCodeAt(i)) >>> 0;
        const fx = ((hash % 100) / 100 - 0.5) * (w / 2 - 10);
        const fy = (((hash >> 8) % 100) / 100 - 0.5) * (h / 2 - 10);
        const level = EVENT_CONSTANTS.barbBaseLevel + lvlBonus + (hash % 3);
        const ent: NodeEntity = {
          id, kind: "barb", level,
          x: cx + fx, y: cy + fy,
          remaining: Math.floor(nodeRichness("barb", level, r.zone_id) * hpMult),
          regionId: r.id, zoneId: r.zone_id,
        };
        this.nodes.set(id, ent);
        this.persistNode(ent);
        spawned = true;
      }
    }
    if (spawned) this.broadcast({ type: "barb_horde", seasonDay: this.seasonDay, extraPerRegion });
    return spawned;
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
      // P3-T2: أهداف قلب Zone 3 (حصون + مذابح) مع مالكيها — لتسجيل نقاط الموسم
      coreObjectives: [...this.coreObjectives.values()],
      passes: [...this.passes.values()],
      marches: [...this.marches.values()].filter((m) => m.state === "moving"),
      nodes: [...this.nodes.values()],
      flags: [...this.flags.values()],
      reports: this.reports.slice(0, 10),
      // P2-T5: الطوابير الجارية (لمساعدات التحالف — تقليل المدة عبر /v1/alliance/help)
      queues: [...this.queues.values()].filter((q) => q.state === "running"),
      map: {
        width: getMap().width,
        height: getMap().height,
        regions: this.regions.map((r) => ({ id: r.id, zone_id: r.zone_id, name: r.name, aabb: r.aabb })),
      },
      // P2-T4: حالة قفل/فتح كل منطقة + يوم الفتح (لرسم المؤقت في العميل)
      zones: zonesStatus(this.seasonDay, this.regions),
      // P3-T1: حالة الموسم الكاملة (ميزات الجدول + قفل العرش) — خدمة فتح المناطق
      season: seasonUnlockState(this.seasonDay),
      // P3-T3: حالة الأحداث اليومية/الأسبوعية (نشطة/مجدولة + مؤقت) — barbarians/resource_rush/war_fever
      events: eventsStatus(this.seasonDay, this.seasonStartMs > 0 ? Math.floor((nowMs() - this.seasonStartMs) / 1000) % 86_400 : 0),
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

    // P3-T1: خدمة فتح المناطق — تقدّم يوم الموسم زمنياً من season_start_ms.
    // لا يحدث شيء إن عُيّن اليوم يدوياً (set_day) لقيمة أعلى — نحتفظ بالأعلى.
    if (SEASON_SERVICE.autoAdvance && this.seasonStartMs > 0) {
      const computed = seasonDayAt(this.seasonStartMs, now);
      if (computed > this.seasonDay) {
        this.seasonDay = computed;
        this.broadcast({ type: "season_day", day: this.seasonDay });
        changed = true;
      }
    }

    // P3-T3: أحداث يومية/أسبوعية — حساب tick داخل اليوم + بث بدء/انتهاء + تكثيف البرابرة.
    // tickInDay حتمي من season_start_ms: عدد ثوانٍ داخل اليوم الحالي (tick = 1s).
    const tickInDay = this.seasonStartMs > 0
      ? Math.floor((now - this.seasonStartMs) / 1000) % 86_400
      : 0;
    const activeNow = new Set(activeEvents(this.seasonDay, tickInDay).map((e) => e.id));
    for (const ev of eventsStatus(this.seasonDay, tickInDay)) {
      const key = `${this.seasonDay}:${ev.id}`;
      if (ev.active && !this.eventsAnnouncedStarted.has(key)) {
        this.eventsAnnouncedStarted.add(key);
        this.broadcast({ type: "event_started", event: ev, seasonDay: this.seasonDay });
        changed = true;
      } else if (!ev.active && ev.scheduledToday && this.eventsAnnouncedStarted.has(key)) {
        this.eventsAnnouncedStarted.delete(key);
        this.broadcast({ type: "event_ended", eventId: ev.id, seasonDay: this.seasonDay });
        changed = true;
      }
    }
    // تكثيف البرابرة أثناء حدث البرابرة (حتمي: يُزرع مرة واحدة لكل منطقة/يوم)
    const extraBarbs = barbExtraPerRegion(this.seasonDay, tickInDay);
    if (extraBarbs > 0) changed = this.seedEventBarbarians(extraBarbs) || changed;

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
      } else if (q.type === "research") {
        // P2-T3: اكتمال البحث يكتب المستوى في D1
        try {
          await this.env.DB.prepare(
            `INSERT INTO player_research (player_id, tech_id, level, updated_at) VALUES (?, ?, ?, ?)
             ON CONFLICT(player_id, tech_id) DO UPDATE SET level=excluded.level, updated_at=excluded.updated_at`,
          ).bind(q.playerId, q.data.techId, q.data.level, now).run();
          this.broadcast({ type: "tech_researched", playerId: q.playerId, techId: q.data.techId, level: q.data.level });
        } catch {
          // الجدول قد لا يكون مُرحّلاً بعد
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
            // P3-T3: باف اندفاع الموارد — عقد أغنى أثناء الحدث
            const tickInDay3 = this.seasonStartMs > 0 ? Math.floor((now - this.seasonStartMs) / 1000) % 86_400 : 0;
            const richMult = eventBuff(this.seasonDay, tickInDay3, "resource_richness_mult");
            const gathered = Math.floor(node.remaining * richMult);
            node.remaining = 0;
            this.persistNode(node);
            m.payload = { kind: node.kind, amount: gathered };
            // نقاط الجمع أثناء اندفاع الموارد
            const gatherScore = eventBuff(this.seasonDay, tickInDay3, "gather_score", true);
            if (gatherScore > 0 && m.allianceId) {
              const pts = gatherScore * node.level;
              const cur = this.throneScores.get(m.allianceId) || 0;
              this.throneScores.set(m.allianceId, cur + pts);
              this.persistThroneScore(m.allianceId, cur + pts);
            }
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

    // P2-T4: بث فتح المناطق عند بلوغ يوم الفتح (مرة واحدة لكل منطقة)
    for (const r of this.regions) {
      if (this.zoneUnlockAnnounced.has(r.id)) continue;
      if (!isRegionUnlocked(r.id, r.zone_id, this.seasonDay)) continue;
      this.zoneUnlockAnnounced.add(r.id);
      this.broadcast({
        type: "zone_unlocked",
        zoneId: r.zone_id,
        regionId: r.id,
        seasonDay: this.seasonDay,
        unlockDay: this.seasonDay,
      });
      changed = true;
    }

    if (this.throne.ownerAllianceId && this.seasonDay >= this.throne.unlockDay) {
      const current = this.throneScores.get(this.throne.ownerAllianceId) || 0;
      const next = current + holdScorePerTick("throne");
      this.throneScores.set(this.throne.ownerAllianceId, next);
      this.persistThroneScore(this.throne.ownerAllianceId, next);
      changed = true;
    }

    // P3-T2: نقاط الاحتفاظ لأهداف قلب Zone 3 (حصون + مذابح) — كل tick عند نشاط المسابقة
    if (coreContestActive(this.seasonDay)) {
      for (const o of this.coreObjectives.values()) {
        if (!o.ownerAllianceId) continue;
        const cur = this.throneScores.get(o.ownerAllianceId) || 0;
        const next = cur + holdScorePerTick(o.kind);
        this.throneScores.set(o.ownerAllianceId, next);
        this.persistThroneScore(o.ownerAllianceId, next);
        changed = true;
      }
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
      const attackerResearchMod = await this.fetchResearchAttackMod(m.ownerPlayerId);

      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: m.troops },
        { name: pass.ownerAllianceId || "neutral_guard", troops: defenderTroops },
        1,
        attackerCommander,
        undefined,
        attackerResearchMod,
        0,
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
      const throneResearchMod = await this.fetchResearchAttackMod(m.ownerPlayerId);

      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: m.troops },
        { name: this.throne.ownerAllianceId || "neutral_guard", troops: defenderTroops },
        3,
        throneAttackerCommander,
        undefined,
        throneResearchMod,
        0,
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

    // P3-T2: احتلال هدف في قلب Zone 3 (حصن خارجي / مذبح جانبي) — يسجّل نقاط موسم
    if (m.targetType === "core_objective") {
      const obj = this.coreObjectives.get(m.targetId);
      if (!obj) {
        this.spawnReturnMarch(m, now);
        return;
      }
      // الأهداف لا تُفتح إلا مع فتح مسابقة القلب (يوم فتح العرش)
      if (!coreContestActive(this.seasonDay)) {
        this.spawnReturnMarch(m, now);
        return;
      }

      const garrisonCount = coreGarrison(obj.kind);
      const defenderTroops: Troops = obj.ownerAllianceId
        ? { infantry_t1: garrisonCount, archer_t1: Math.floor(garrisonCount / 2) }
        : { infantry_t1: Math.floor(garrisonCount * 0.7) };

      // تحالف يعزّز هدفه: اكتمال فوري
      if (obj.ownerAllianceId && m.allianceId && obj.ownerAllianceId === m.allianceId) {
        obj.captureProgress = 100;
        obj.state = "open";
        this.persistCoreObjective(obj);
        this.spawnReturnMarch(m, now);
        this.broadcast({ type: "core_objective_changed", objective: obj });
        return;
      }

      const coCommander = await this.fetchMarchCommander(m.id);
      const coResearchMod = await this.fetchResearchAttackMod(m.ownerPlayerId);
      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: m.troops },
        { name: obj.ownerAllianceId || "neutral_guard", troops: defenderTroops },
        2,
        coCommander,
        undefined,
        coResearchMod,
        0,
      );

      const report = {
        id: newId("br"),
        createdAt: now,
        kind: `core_${obj.kind}`,
        objectiveId: obj.id,
        attackerPlayerId: m.ownerPlayerId,
        attackerAllianceId: m.allianceId,
        result,
      };
      this.saveReport(report);
      await this.grantCommanderXp(m.id, totalTroops(result.defenderLosses));
      const coHospital = await this.admitToHospital(m.ownerPlayerId, result.attackerSplit.severely);
      report.hospital = coHospital;
      await this.deductMarchLosses(m.ownerPlayerId, result.attackerLosses);

      if (result.winner === "attacker") {
        const gain = coreCaptureGain(obj.kind, troopPower(result.attackerRemaining));
        if (obj.ownerAllianceId && obj.ownerAllianceId !== m.allianceId) {
          obj.captureProgress = gain; // احتلال عدو: يبدأ من جديد
        } else {
          obj.captureProgress = Math.min(100, obj.captureProgress + gain);
        }
        obj.state = "contested";
        if (obj.captureProgress >= 100) {
          // مكافأة أول احتلال في الموسم لهذا الهدف
          if (!obj.firstCapturedBy && m.allianceId) {
            obj.firstCapturedBy = m.allianceId;
            const bonus = firstCaptureBonus();
            const cur = this.throneScores.get(m.allianceId) || 0;
            this.throneScores.set(m.allianceId, cur + bonus);
            this.persistThroneScore(m.allianceId, cur + bonus);
            report.firstCaptureBonus = bonus;
          }
          obj.ownerAllianceId = m.allianceId;
          obj.captureProgress = 100;
          obj.state = "open";
        }
        this.persistCoreObjective(obj);
        this.broadcast({ type: "core_objective_changed", objective: obj, report });
      } else {
        this.broadcast({ type: "battle_report", report });
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
          const barbResearchMod = await this.fetchResearchAttackMod(m.ownerPlayerId);
          const result = resolveCombat({ name: m.ownerPlayerId, troops: m.troops }, { name: "barb", troops: def }, 1, barbCommander, undefined, barbResearchMod, 0);
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
            // P3-T3: نقاط قتل البرابرة أثناء حدث غزو البرابرة
            const tickInDay2 = this.seasonStartMs > 0 ? Math.floor((now - this.seasonStartMs) / 1000) % 86_400 : 0;
            const killScore = eventBuff(this.seasonDay, tickInDay2, "barb_kill_score", true);
            if (killScore > 0 && m.allianceId) {
              const pts = killScore * node.level;
              const cur = this.throneScores.get(m.allianceId) || 0;
              this.throneScores.set(m.allianceId, cur + pts);
              this.persistThroneScore(m.allianceId, cur + pts);
              report.barbKillScore = pts;
            }
            node.remaining = Math.max(0, node.remaining - 50);
            this.persistNode(node);
          }
          this.broadcast({ type: "battle_report", report });
          this.spawnReturnMarch(m, now);
        } else {
          m.state = "gathering";
          // P3-T3: باف اندفاع الموارد — جمع أسرع أثناء الحدث
          const tickInDay2 = this.seasonStartMs > 0 ? Math.floor((now - this.seasonStartMs) / 1000) % 86_400 : 0;
          const gatherMult = eventBuff(this.seasonDay, tickInDay2, "gather_rate_mult");
          const rate = 0.5 * totalTroops(m.troops) * gatherMult; // units/sec
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

    // P2-T4: حالة فتح/قفل المناطق مع يوم الفتح لكل منطقة
    // P3-T1: + حالة الموسم الكاملة (اليوم الحالي + العرش + ميزات الجدول)
    if (path.endsWith("/zones-status") && request.method === "GET") {
      return Response.json({
        seasonDay: this.seasonDay,
        season: seasonUnlockState(this.seasonDay),
        zones: zonesStatus(this.seasonDay, this.regions),
      });
    }

    // P3-T3: الأحداث النشطة والمجدولة اليوم (barbarians / resource_rush / war_fever)
    if (path.endsWith("/events") && request.method === "GET") {
      const tickInDay = this.seasonStartMs > 0 ? Math.floor((nowMs() - this.seasonStartMs) / 1000) % 86_400 : 0;
      return Response.json({
        seasonDay: this.seasonDay,
        tickInDay,
        events: eventsStatus(this.seasonDay, tickInDay),
        activeIds: activeEvents(this.seasonDay, tickInDay).map((e) => e.id),
      });
    }

    // P3-T1: جدول فتح الموسم الكامل (Zone unlock service) — مناطق + ممرات + عرش + ميزات
    if (path.endsWith("/season/schedule") && request.method === "GET") {
      const sched = seasonSchedule(
        this.regions,
        [...this.passes.values()].map((p) => ({ id: p.id, unlockDay: p.unlockDay })),
      );
      return Response.json({
        seasonDay: this.seasonDay,
        seasonStartMs: this.seasonStartMs,
        ...sched,
      });
    }

    // P3-T2: لوحة نقاط الموسم الكاملة — أهداف قلب Zone 3 + النقاط الحالية + المتصدر
    if (path.endsWith("/season/scoreboard") && request.method === "GET") {      const scores = [...this.throneScores.entries()]
        .map(([allianceId, points]) => ({ allianceId, points: Math.round(points * 100) / 100 }))
        .sort((a, b) => b.points - a.points);
      return Response.json({
        seasonDay: this.seasonDay,
        contestActive: coreContestActive(this.seasonDay),
        leader: scores[0]?.allianceId ?? null,
        scores,
        throne: {
          ownerAllianceId: this.throne.ownerAllianceId,
          captureProgress: this.throne.captureProgress,
          state: this.throne.state,
          unlockDay: this.throne.unlockDay,
        },
        coreObjectives: [...this.coreObjectives.values()].map((o) => ({
          id: o.id,
          kind: o.kind,
          ownerAllianceId: o.ownerAllianceId,
          captureProgress: o.captureProgress,
          state: o.state,
          firstCapturedBy: o.firstCapturedBy,
        })),
      });
    }

    // P2-T5: معالجة حملات rally المستحقة — يستدعيها الـ worker كل ثانية
    if (path.endsWith("/process-rallies") && request.method === "POST") {
      const now = nowMs();
      const launched: string[] = [];
      try {
        const due = await this.env.DB.prepare(
          "SELECT * FROM rallies WHERE status = 'forming' AND launch_ms <= ?",
        ).bind(now).all<any>();
        for (const r of due.results || []) {
          const parts = await this.env.DB.prepare(
            "SELECT player_id, troops_json FROM rally_participants WHERE rally_id = ?",
          ).bind(r.id).all<{ player_id: string; troops_json: string }>();
          const list = parts.results || [];
          if (list.length === 0) {
            await this.env.DB.prepare("UPDATE rallies SET status='cancelled' WHERE id=?").bind(r.id).run();
            continue;
          }
          // تجميع قوات كل المشاركين في مسيرة واحدة يقودها صاحب الـ rally
          const merged: Troops = {};
          for (const p of list) {
            const t = JSON.parse(p.troops_json || "{}") as Troops;
            for (const [u, c] of Object.entries(t)) merged[u] = (merged[u] || 0) + Number(c);
          }
          try {
            const march = await this.createMarch({
              playerId: r.leader_player_id,
              troops: merged,
              targetType: r.target_type,
              targetId: r.target_id,
              passId: r.target_id,
              primaryCommanderId: r.commander_id || undefined,
              commanderSkills: r.commander_skills_json ? JSON.parse(r.commander_skills_json) : undefined,
            });
            march.payload = { rallyId: r.id, participantIds: list.map((p) => p.player_id) };
            this.persistMarch(march);
            await this.env.DB.prepare(
              "UPDATE rallies SET status='launched', march_id=? WHERE id=?",
            ).bind(march.id, r.id).run();
            this.broadcast({
              type: "rally_launched",
              rallyId: r.id,
              allianceId: r.alliance_id,
              targetType: r.target_type,
              targetId: r.target_id,
              marchId: march.id,
              participants: list.map((p) => p.player_id),
            });
            launched.push(r.id);
          } catch (e: any) {
            await this.env.DB.prepare("UPDATE rallies SET status='failed' WHERE id=?").bind(r.id).run();
            // فشل المسار: إعادة قوات المشاركين إلى home
            for (const p of list) {
              const t = JSON.parse(p.troops_json || "{}") as Troops;
              for (const [u, c] of Object.entries(t)) {
                await this.env.DB.prepare(
                  `UPDATE troops SET count = MAX(0, count - ?) WHERE player_id = ? AND unit_id = ? AND status = 'marching'`,
                ).bind(Number(c), p.player_id, u).run();
                await this.env.DB.prepare(
                  `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)
                   ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`,
                ).bind(p.player_id, u, Number(c)).run();
              }
            }
          }
        }
      } catch {
        // الجداول قد لا تكون مُرحّلة بعد
      }
      this.ensureAlarm();
      return Response.json({ ok: true, launched });
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
        const march = await this.createMarch(body);
        this.ensureAlarm();
        return Response.json({ ok: true, march });
      } catch (e: any) {
        return Response.json({ error: e.message || "march_failed" }, { status: 400 });
      }
    }

    if (path.endsWith("/pass-attack") && request.method === "POST") {
      const body = await request.json<any>();
      try {
        const march = await this.createMarch({
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
          "INSERT OR REPLACE INTO world_meta (id, season_day, last_tick_ms, season_start_ms) VALUES (1, ?, ?, ?)",
          this.seasonDay,
          nowMs(),
          this.seasonStartMs,
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

  private async createMarch(body: any): Promise<MarchEntity> {
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

    // P3-T2: استهداف هدف في قلب Zone 3 (حصن خارجي / مذبح جانبي)
    if (targetType === "core_objective" || body.coreObjectiveId) {
      targetType = "core_objective";
      targetId = String(body.coreObjectiveId || body.targetId);
      const obj = this.coreObjectives.get(targetId);
      if (!obj) throw new Error("core_objective_not_found");
      if (!coreContestActive(this.seasonDay)) throw new Error("core_contest_locked");
      toX = obj.x;
      toY = obj.y;
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
      // P2-T4: عقدة في منطقة مقفلة زمنياً تُرفض (stub حتى يوم الفتح)
      if (node.regionId && node.zoneId != null && !isRegionUnlocked(node.regionId, node.zoneId, this.seasonDay)) {
        throw new Error("zone_locked");
      }
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

    // P2-T4: أي هدف (نقطة/مدينة/عرش) داخل منطقة مقفلة زمنياً يُرفض
    if (targetType === "point" || targetType === "city" || targetType === "throne") {
      const tRegion = this.regionOf(toX, toY);
      if (tRegion) {
        const tZone = this.regions.find((r) => r.id === tRegion)?.zone_id ?? 1;
        if (!isRegionUnlocked(tRegion, tZone, this.seasonDay)) throw new Error("zone_locked");
      }
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

    // attacking a pass, throne, or core objective: allow even if path flagged, use euclidean distance
    if (!plan.ok && (targetType === "pass" || targetType === "throne" || targetType === "core_objective")) {
      plan = { ok: true, distance: dist(city.x, city.y, toX, toY), crossedPasses: [targetId] };
    }

    if (!plan.ok) throw new Error(plan.reason || "illegal_path");

    // P2-T3: باف سرعة المسير من أبحاث العسكر (march_speed)
    const marchSpeedMod = 1 + (await this.fetchMarchSpeedMod(playerId));

    const start = nowMs();
    const eta = start + marchDurationMs(plan.distance, 40 * marchSpeedMod);
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

  /** P2-T3: باف هجوم القوات من أبحاث العسكر للاعب (troop_attack) */
  private async fetchResearchAttackMod(playerId: string): Promise<number> {
    try {
      const rows = await this.env.DB.prepare("SELECT tech_id, level FROM player_research WHERE player_id = ?")
        .bind(playerId).all<{ tech_id: string; level: number }>();
      const levels: Record<string, number> = {};
      for (const r of rows.results || []) levels[r.tech_id] = r.level;
      return researchBuff(levels, "troop_attack");
    } catch {
      return 0; // الجدول قد لا يكون مُرحّلاً بعد
    }
  }

  /** P2-T3: باف سرعة المسير من أبحاث العسكر (march_speed) */
  private async fetchMarchSpeedMod(playerId: string): Promise<number> {
    try {
      const rows = await this.env.DB.prepare("SELECT tech_id, level FROM player_research WHERE player_id = ?")
        .bind(playerId).all<{ tech_id: string; level: number }>();
      const levels: Record<string, number> = {};
      for (const r of rows.results || []) levels[r.tech_id] = r.level;
      return researchBuff(levels, "march_speed");
    } catch {
      return 0;
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
        const march = await this.createMarch(body);
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
