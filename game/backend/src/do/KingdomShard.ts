import { DurableObject } from "cloudflare:workers";
import type { Env, Troops } from "../env";
import { getMap, getChatConfig, getAllianceStructures, type MapPass, type MapRegion } from "../lib/gameData";
import { newId, nowMs, dist } from "../lib/ids";
import opsData from "../data/ops.json";

const OPS_CONSTANTS = {
  enabled: (opsData as any).constants.enabled as boolean,
  commandErrorWindowMs: (opsData as any).constants.command_error_window_ms as number,
  tickStaleThresholdMs: (opsData as any).constants.tick_stale_threshold_ms as number,
  queueStuckThreshold: (opsData as any).constants.queue_stuck_threshold as number,
  commandAlertThreshold: (opsData as any).constants.command_alert_threshold as number,
  errorLogLimit: (opsData as any).constants.error_log_limit as number,
};

// P9-T1: نافذة تبرع واحدة بالملّي ثانية (من alliance_tech.json)
const ALLIANCE_TECH_WINDOW_MS = ALLIANCE_TECH_CFG.window_seconds * 1000;
const TICK_STALE_THRESHOLD_MS = OPS_CONSTANTS.tickStaleThresholdMs;
const COMMAND_OPS_WINDOW_MS = OPS_CONSTANTS.commandErrorWindowMs;
const QUEUE_STUCK_THRESHOLD = OPS_CONSTANTS.queueStuckThreshold;
const COMMAND_ALERT_THRESHOLD = OPS_CONSTANTS.commandAlertThreshold;
import { assertAdminKey } from "../lib/secrets";
import { resolveCombat, totalTroops, troopPower, scaleTroops, type CombatResult } from "./sim/combat";
import { marchDurationMs, planMarch } from "./sim/pathfinding";
import { COMMANDER_CONSTANTS, xpForLevel, type CommanderInstance } from "./sim/commanders";
import { talentAttackMod } from "./sim/talents";
import { equipmentAttackMod, type EquipmentState } from "./sim/equipment";
import { admitWounded, hospitalCapacity } from "./sim/hospital";
import {
  HOLY_SITES,
  holdDurationMs,
  holdForKingMs,
  siteCaptureGain,
  siteGuardTroops,
  templeGuardTroops,
  templeUnlocked,
  templeWoundedDeadShare,
} from "./sim/holy_sites";
import {
  AllianceTechService,
  ALLIANCE_TECH_CFG,
  ALLIANCE_TECH_RESEARCH_CFG,
  type TechProgress,
  type DonationWindow,
} from "./sim/alliance_tech";
import { rankHas } from "./sim/alliance";
import {
  type AllianceShopState,
  applyHelpCredit,
  applyGiftClaimCredit,
  itemCatalog,
  itemById,
  titleById,
  validatePurchase,
  purchase as purchaseShopItem,
  validateTitleGrant,
  grantTitle as grantAllianceTitle,
  revokeTitle,
  revokeTitlesForPlayer,
  titleBuffsForPlayer,
  titleDefinitions,
  allianceShopStateInitial,
  dailyCap,
  balanceCap,
} from "./sim/alliance_shop";
import {
  flagRadius,
  outpostRadius,
  gatherBonus,
  gatherMultiplier,
  patrolReduction,
  patrolMod,
  insideTerritory,
  marchCrossesTerritory,
  canBuildOutpost,
  validPosition,
  seedCenters,
  respawnDueCenters,
  lockCenter,
  centerGatherAmount,
  centerResource,
  TERRITORY_CFG,
  type CenterEntity,
} from "./sim/territory";
import {
  buildDailyQuests,
  buildWeeklyQuests,
  questDay,
  questWeek,
  questDescription,
  QUESTS,
} from "./sim/daily_quests";
import {
  regenAp,
  apCap,
  AP_INTERVAL_MS,
  apCost,
  warFrenzyDurationMs,
  shieldOptions,
  canActivateShield,
  relocationCooldownMs,
  relocationCosts,
  startingAp,
} from "./sim/action_points";

/** سقف صلب لأي عملية تسريع واحدة (30 يوماً). حاجز أخير ضد قيمة شاذة
 *  تتسرّب من مسار أعلى — لا يغيّر السلوك الشرعي لأن أطول عنصر تسريع
 *  في المتجر أقصر من ذلك بكثير. */
const MAX_SPEEDUP_SECONDS = 30 * 24 * 60 * 60;
import { researchBuff } from "./sim/research";
// P9-T4: دوال VIP النقية (15 مستوى + بافات + متجر) — تُقرأ من data/shop.json
import { vipTierForPoints, vipTiers } from "./sim/shop";
// P9-T5: منطق Trading Post النقي (سعر ديناميكي + رسوم + حدود) — يُقرأ من data/trading.json
import {
  tradingConstants,
  tradingResources,
  isValidTradingResource,
  resourceBasePrice,
  initialPriceFor,
  offerCostForBuyer,
  sellerNet,
  adaptPrice,
  validateOffer,
  validateClaim,
  settleTrade,
  rateBounds,
  type TradingOffer,
} from "./sim/trading";
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
import {
  AntiCheatRateLimiter,
  checkMarchPayload,
  ANTICHEAT_CONSTANTS,
} from "./sim/anticheat";

type CityEntity = {
  playerId: string;
  name: string;
  allianceId: string | null;
  // P9-T2: اسم التحالف ورتبة اللاعب فيه (يمررها الراوتر مع set-alliance) — لصلاحيات بناء قلاع outpost
  allianceName?: string;
  rank?: string;
  // P9-T3: لقب التحالف الممنوح للاعب (عبر متجر التحالف) — إن وجد
  titleId?: string;
  civ: string;
  x: number;
  y: number;
  hallLevel: number;
  regionId: string;
  ap: number;
  lastApMs: number;
  shieldUntilMs: number | null;
  warFrenzyUntilMs: number | null;
  lastRelocationMs: number | null;
};

// P6-T10: سجل سلطوي مختصر للوقائع التي تصنع «حكاية المملكة».
// لا يحتوي السجل على تقارير قتال خاصة أو موارد؛ فقط معالم الموسم العامة.
type SeasonStoryEvent = {
  id: string;
  kind: "region_unlocked" | "first_pass_capture" | "pass_conquered" | "throne_captured" | "season_champion" | "holy_site_captured" | "temple_captured" | "king_crowned";
  seasonDay: number;
  createdAt: number;
  subjectId: string;
  allianceId: string | null;
  previousAllianceId?: string | null;
  score?: number;
};

type AllianceFlag = {
  id: string;
  allianceId: string;
  x: number;
  y: number;
  radius: number;
};

// P8-T4: الموقع المقدس (Sanctum/Altar/Shrine) ودورة Lost Temple.
// النوع الواحد لا يتراكب في الباف — أعلى باف مملوك فقط يسري.
type HolySiteEntity = {
  id: string;
  kind: string; // sanctum | altar | shrine | temple
  ownerAllianceId: string | null;
  captureProgress: number;
  state: "open" | "contested" | "captured";
  x: number;
  y: number;
  heldSinceMs: number | null; // متى أصبح مملوكاً -- انقضاؤه (4h) يحرر الموقع
};
// P8-T4: حالة التتويج — من يحتفظ المعبد 8 ساعات متواصلة يصبح ملك المملكة.
type KingEntity = {
  allianceId: string;
  crownedAtMs: number;
  expiresAtMs: number | null; // يفقد اللقب عند فقدان المعبد
};

/** منشأة تحالف ثابتة على الخريطة. القيم التشغيلية تأتي من alliance_structures.json
 * ولا يقبل الخادم radius أو نطاق حماية من العميل. */
type AllianceStructure = {
  id: string;
  kind: string;
  allianceId: string;
  x: number;
  y: number;
  radius: number;
  protectionRadius: number;
  marchDamageReduction: number;
  mapMarker: string;
  createdBy: string;
  createdAt: number;
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
  targetType: "pass" | "resource" | "barb" | "city" | "point" | "throne" | "core_objective" | "holy_site" | "center";
  targetId: string;
  payload?: any;
};

// P5-T5: كشافة ضباب الحرب — مسيرة خفيفة بدون قوات تكشف المنطقة عند الوصول (الكشف نفسه محلي في العميل)
type ScoutEntity = {
  id: string;
  ownerPlayerId: string;
  fromX: number;
  fromY: number;
  toX: number;
  toY: number;
  startMs: number;
  etaMs: number;
  state: "moving" | "arrived";
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

// P2-T2: ملخص دخول المستشفى المرفق بتقارير القتال
type HospitalSummary = { admitted: Troops; died: Troops; capacity: number };

// P9-T1: حالة تكنولوجيا التحالف — progress لكل تقنية + سجل تبرعات اللاعب ونوافذها.
type AllianceTechState = {
  /** تقدم التقنيات لكل تحالف: allianceId → {techId: TechProgress} */
  allianceTech: Map<string, Record<string, TechProgress>>;
  /** سجل تبرعات كل لاعب: playerId → DonationWindow[] (نافذة 30 دقيقة بسقف 20) */
  donationWindows: Map<string, DonationWindow[]>;
};

// P9-T2: قلعة outpost للتحالف — تنشر نطاقًا إقليمياً (outpostRadius) حولها:
// باف جمع داخل النطاق + تخفيض أضرار البرابرة للممرات العابرة.
type AllianceOutpostEntity = {
  id: string;
  allianceId: string;
  x: number;
  y: number;
  radius: number;
  createdBy: string;
  createdAt: number;
};

// P6-T6: رسالة دردشة حية (قناة المملكة أو التحالف)
type ChatMessage = {
  id: string;
  channel: "kingdom" | "alliance";
  // تُلتقط عضوية التحالف وقت الإرسال كي لا تُسرَّب الرسالة إذا غادر المرسل لاحقاً.
  allianceId: string | null;
  playerId: string;
  playerName: string;
  civ: string;
  text: string;
  timestampMs: number;
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

  // P7-T15: مؤشرات التشغيل — أخطاء الأوامر حسب الرمز مع نافذة ساعة منزلقة.
  private commandErrorCounts = new Map<string, { n: number; firstMs: number; lastMs: number }>();
  private commandTotal = 0;
  private lastTickMs = 0;
  // P3-T1: طابع بداية الموسم — خدمة فتح المناطق تحسب اليوم منه زمنياً
  private seasonStartMs = 0;
  private cities = new Map<string, CityEntity>();
  private throne: ThroneEntity = { ownerAllianceId: null, captureProgress: 0, state: "open", x: 1200, y: 1200, unlockDay: 14 };
  private throneScores = new Map<string, number>();
  // P3-T2: أهداف قلب Zone 3 (4 حصون خارجية + 4 مذابح جانبية) — تسجيل نقاط الموسم
  private coreObjectives = new Map<string, CoreObjective>();
  // P8-T4: المواقع المقدسة (12 موقعًا + المعبد) — بافات لا تتراكب + دورة الملك
  private holySites = new Map<string, HolySiteEntity>();
  private king: KingEntity | null = null;
  // P3-T3: الأحداث التي أُعلن بدؤها في هذا اليوم (لا يُعاد بث event_started لها)
  private eventsAnnouncedStarted = new Set<string>();
  private passes = new Map<string, PassEntity>();
  private marches = new Map<string, MarchEntity>();
  private nodes = new Map<string, NodeEntity>();
  private flags = new Map<string, AllianceFlag>();
  // منشآت التحالف المرئية: حصون ومنجنيقات وأبراج مراقبة.
  private allianceStructures = new Map<string, AllianceStructure>();
  // P9-T2: أراضي التحالف — قلاع outpost المنشأة + مراكز الموارد على الخريطة
  private allianceOutposts = new Map<string, AllianceOutpostEntity>();
  private resourceCenters = new Map<string, CenterEntity>();
  // P9-T1: تكنولوجيا التحالف — تقدم تقنيات كل تحالف + نوافذ تبرع كل عضو
  private allianceTech: AllianceTechState = {
    allianceTech: new Map(),
    donationWindows: new Map(),
  };
  // P9-T3: متجر التحالف والألقاب — رصيد تحالف + مشتريات + ألقاب ممنوحة لكل تحالف
  private allianceShop = new Map<string, AllianceShopState>();
  // P5-T5: الكشافة النشطة على الخريطة
  private scouts = new Map<string, ScoutEntity>();
  private queues = new Map<string, QueueEntity>();
  private reports: any[] = [];
  // P6-T10: آخر معالم الموسم العامة؛ تُحمّل وتُبث ضمن لقطة العالم فقط.
  private seasonStory: SeasonStoryEvent[] = [];
  // P2-T4: المناطق التي بُثّ فتحها مسبقاً (لا يُعاد بث zone_unlocked لها)
  private zoneUnlockAnnounced = new Set<string>();
  // P4-T5: anti-cheat — rate limiter في الذاكرة لكل لاعب × نوع فعل (حدود من data/anticheat.json)
  private antiCheat = new AntiCheatRateLimiter();
  // P4-T5: سجل مخالفات حديث (آخر violation_log_limit مخالفة) للفحص الإداري
  private antiCheatViolations: Array<{ playerId: string; action: string; reason: string; at: number }> = [];
  // P6-T6: سجل الدردشة الحية (حلقة مُغلقة، آخر 100 رسالة من data/chat.json)
  private chatHistory: ChatMessage[] = [];
  // P6-T6: مُحدّد سرعة بسيط لكل لاعب — [عدد الرسائل، بداية النافذة]
  private chatRateLimit = new Map<string, { count: number; windowStart: number }>();
  // P9-T4: مستوى VIP لكل لاعب — حدّثه الراوتر (سلطوي، من D1) عبر header x-rok2-vip-level
  private playerVipLevels = new Map<string, number>();
  // P9-T5: Trading Post — أسعار السوق الحالية للموارد (قراءة من trading_prices)
  private tradingPrices = new Map<string, { price: number; day: number; demand: number; supply: number; updatedMs: number }>();

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

  /** المصدر الوحيد لتعريف جداول عالم المملكة الأساسية؛ تعيد الهجرة 4 استعماله للشاردات القديمة. */
  private ensureCoreWorldTables() {
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
          civ TEXT NOT NULL DEFAULT '',
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
        CREATE TABLE IF NOT EXISTS alliance_outposts (
          id TEXT PRIMARY KEY,
          alliance_id TEXT NOT NULL,
          x REAL NOT NULL,
          y REAL NOT NULL,
          created_by TEXT NOT NULL,
          created_at INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS alliance_centers (
          id TEXT PRIMARY KEY,
          kind TEXT NOT NULL,
          x REAL NOT NULL,
          y REAL NOT NULL,
          radius REAL NOT NULL,
          locked_alliance_id TEXT,
          locked_until_ms INTEGER,
          reserve REAL NOT NULL,
          spawned_season_day INTEGER NOT NULL DEFAULT 1
        );

    `);
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
      this.ensureCoreWorldTables();
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (1)");
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

    if (ver < 4) {
      // شاردات قديمة بُذرت قبل اكتمال كتلة ver 1 (جداول مفقودة مثل flags/throne).
      // CREATE TABLE IF NOT EXISTS يجعل الخطوة آمنة أيضاً للقواعد الجديدة والسليمة.
      this.ensureCoreWorldTables();

      try {
        this.ctx.storage.sql.exec("ALTER TABLE marches ADD COLUMN payload_json TEXT");
      } catch {
        // العمود موجود مسبقاً
      }
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (4)");
    }

    if (ver < 5) {
      // P5-T5: كشافة ضباب الحرب
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS scouts (
          id TEXT PRIMARY KEY,
          owner_player_id TEXT NOT NULL,
          from_x REAL NOT NULL,
          from_y REAL NOT NULL,
          to_x REAL NOT NULL,
          to_y REAL NOT NULL,
          start_ms INTEGER NOT NULL,
          eta_ms INTEGER NOT NULL,
          state TEXT NOT NULL
        );
      `);
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (5)");
    }

    if (ver < 6) {
      // P6-T6: دردشة حية — قناتا المملكة والتحالف
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS chat_messages (
          id TEXT PRIMARY KEY,
          channel TEXT NOT NULL,
          player_id TEXT NOT NULL,
          text TEXT NOT NULL,
          created_at INTEGER NOT NULL
        );
      `);
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (6)");
    }

    if (ver < 7) {
      // منشآت التحالف: لا تُخزّن قيم النطاق من العميل؛ تُشتق من الكتالوج عند البناء.
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS alliance_structures (
          id TEXT PRIMARY KEY,
          kind TEXT NOT NULL,
          alliance_id TEXT NOT NULL,
          x REAL NOT NULL,
          y REAL NOT NULL,
          radius REAL NOT NULL,
          protection_radius REAL NOT NULL,
          march_damage_reduction REAL NOT NULL,
          map_marker TEXT NOT NULL,
          created_by TEXT NOT NULL,
          created_at INTEGER NOT NULL
        );
      `);
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (7)");
    }

    if (ver < 8) {
      // P6-T10: خط زمني عام للموسم. لا يسجل تفاصيل المعارك الخاصة.
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS season_story_events (
          id TEXT PRIMARY KEY,
          kind TEXT NOT NULL,
          season_day INTEGER NOT NULL,
          created_at INTEGER NOT NULL,
          subject_id TEXT NOT NULL,
          alliance_id TEXT,
          previous_alliance_id TEXT,
          score INTEGER
        );
        CREATE INDEX IF NOT EXISTS idx_season_story_events_created_at
          ON season_story_events(created_at DESC);
      `);
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (8)");
    }

    if (ver < 9) {
      // قناة التحالف لا تُستنتج من المرسل عند القراءة؛ تحفظ هوية التحالف وقت النشر.
      this.ctx.storage.sql.exec("ALTER TABLE chat_messages ADD COLUMN alliance_id TEXT");
            this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (9)");
    }
    if (ver < 10) {
      // P8-T4: المواقع المقدسة (Sanctum/Altar/Shrine + Lost Temple) ودورة الملك.
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS holy_sites (
          id TEXT PRIMARY KEY,
          kind TEXT NOT NULL,
          owner_alliance_id TEXT,
          capture_progress REAL NOT NULL DEFAULT 0,
          state TEXT NOT NULL DEFAULT 'open',
          x REAL NOT NULL,
          y REAL NOT NULL,
          held_since_ms INTEGER
        );
      `);
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (10)");
    }
    if (ver < 11) {
      // P8-T5: نقاط العمل (AP) والدروع (Peace Shield) والتهجير (Relocation) — أعمدة مدينية.
      this.ctx.storage.sql.exec(
        "ALTER TABLE map_cities ADD COLUMN ap INTEGER NOT NULL DEFAULT 1000",
      );
      this.ctx.storage.sql.exec(
        "ALTER TABLE map_cities ADD COLUMN last_ap_ms INTEGER NOT NULL DEFAULT 0",
      );
      this.ctx.storage.sql.exec("ALTER TABLE map_cities ADD COLUMN shield_until_ms INTEGER");
      this.ctx.storage.sql.exec("ALTER TABLE map_cities ADD COLUMN war_frenzy_until_ms INTEGER");
      this.ctx.storage.sql.exec("ALTER TABLE map_cities ADD COLUMN last_relocation_ms INTEGER");
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (11)");
    }
    if (ver < 12) {
      // P9-T1: تكنولوجيا التحالف — تقدم التقنيات لكل تحالف + نوافذ تبرع الأعضاء.
      this.ctx.storage.sql.exec(
        `CREATE TABLE IF NOT EXISTS alliance_tech (
          alliance_id TEXT NOT NULL,
          tech_id TEXT NOT NULL,
          points INTEGER NOT NULL DEFAULT 0,
          level INTEGER NOT NULL DEFAULT 0,
          research_started_at_ms INTEGER,
          PRIMARY KEY (alliance_id, tech_id)
        )`,
      );
      this.ctx.storage.sql.exec(
        `CREATE TABLE IF NOT EXISTS donation_windows (
          player_id TEXT NOT NULL,
          window_start_ms INTEGER NOT NULL,
          count INTEGER NOT NULL DEFAULT 0,
          PRIMARY KEY (player_id, window_start_ms)
        )`,
      );
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (12)");
    }
    if (ver < 13) {
      // P9-T2: أراضي التحالف ومراكز الموارد — قلاع outpost المنشأة + مراكز موارد غير قابلة للهجوم.
      this.ensureCoreWorldTables();
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (13)");
    }
    if (ver < 14) {
      // P9-T3: متجر التحالف والألقاب — رصيد تحالف + مشتريات + ألقاب (JSON مرمّز لكل تحالف)
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS alliance_shop (
          alliance_id TEXT NOT NULL,
          balance INTEGER NOT NULL DEFAULT 0,
          daily_earned INTEGER NOT NULL DEFAULT 0,
          daily_earned_day INTEGER NOT NULL DEFAULT 0,
          items_json TEXT NOT NULL DEFAULT '{}',
          titles_json TEXT NOT NULL DEFAULT '{}',
          PRIMARY KEY (alliance_id)
        )
      `);
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (14)");
    }
    if (ver < 15) {
      // P9-T5: Trading Post — عروض السوق المفتوحة + أسعار ديناميكية حسب العرض والطلب.
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS trading_offers (
          id TEXT NOT NULL,
          seller_id TEXT NOT NULL,
          sell_resource TEXT NOT NULL,
          buy_resource TEXT NOT NULL,
          amount INTEGER NOT NULL,
          rate REAL NOT NULL,
          created_ms INTEGER NOT NULL,
          remaining INTEGER NOT NULL,
          PRIMARY KEY (id)
        )
      `);
      this.ctx.storage.sql.exec(`CREATE INDEX IF NOT EXISTS idx_trading_sell ON trading_offers (sell_resource)`);
      this.ctx.storage.sql.exec(`CREATE INDEX IF NOT EXISTS idx_trading_buy ON trading_offers (buy_resource)`);
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS trading_prices (
          resource TEXT NOT NULL,
          price REAL NOT NULL,
          day INTEGER NOT NULL DEFAULT 0,
          demand_trades INTEGER NOT NULL DEFAULT 0,
          supply_offers INTEGER NOT NULL DEFAULT 0,
          updated_ms INTEGER NOT NULL,
          PRIMARY KEY (resource)
        )
      `);
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO _sql_schema_migrations (id) VALUES (15)");
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
        civ: row.civ || "",
        x: row.x,
        y: row.y,
        hallLevel: row.hall_level,
        regionId: row.region_id,
                ap: typeof row.ap === "number" ? row.ap : 1000,
        lastApMs: typeof row.last_ap_ms === "number" && row.last_ap_ms > 0 ? row.last_ap_ms : Date.now(),
        shieldUntilMs: typeof row.shield_until_ms === "number" ? row.shield_until_ms : null,
        warFrenzyUntilMs: typeof row.war_frenzy_until_ms === "number" ? row.war_frenzy_until_ms : null,
        lastRelocationMs: typeof row.last_relocation_ms === "number" ? row.last_relocation_ms : null,
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

    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM alliance_structures").toArray()) {
      this.allianceStructures.set(row.id, {
        id: row.id,
        kind: row.kind,
        allianceId: row.alliance_id,
        x: row.x,
        y: row.y,
        radius: row.radius,
        protectionRadius: row.protection_radius,
        marchDamageReduction: row.march_damage_reduction,
        mapMarker: row.map_marker,
        createdBy: row.created_by,
        createdAt: row.created_at,
      });
    }

    // P9-T2: قلاع outpost المنشأة + مراكز الموارد — تُحمّل مع بقية حالات العالم
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM alliance_outposts").toArray()) {
      this.allianceOutposts.set(row.id, {
        id: row.id,
        allianceId: row.alliance_id,
        x: row.x,
        y: row.y,
        radius: Number(row.radius) || outpostRadius(),
        createdBy: row.created_by,
        createdAt: row.created_at,
      });
    }
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM alliance_centers").toArray()) {
      this.resourceCenters.set(row.id, {
        id: row.id,
        kind: row.kind,
        x: row.x,
        y: row.y,
        radius: row.radius,
        lockedAllianceId: row.locked_alliance_id || null,
        lockedUntilMs: typeof row.locked_until_ms === "number" ? row.locked_until_ms : null,
        reserve: row.reserve,
        spawnedSeasonDay: row.spawned_season_day ?? 1,
      });
    }
    if (this.resourceCenters.size === 0) this.seedAllianceCenters();

    // P5-T5: الكشافة التي لم تصل بعد فقط — الواصلة حُذفت عند بث scout_arrived
    for (const row of this.ctx.storage.sql
      .exec<any>("SELECT * FROM scouts WHERE state = 'moving'")
      .toArray()) {
      this.scouts.set(row.id, {
        id: row.id,
        ownerPlayerId: row.owner_player_id,
        fromX: row.from_x,
        fromY: row.from_y,
        toX: row.to_x,
        toY: row.to_y,
        startMs: row.start_ms,
        etaMs: row.eta_ms,
        state: row.state,
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
    // P8-T4: المواقع المقدسة ودورة الملك — تُحمّل من الحفظ السلطوي
    this.loadHolySites();
    // P9-T1: تكنولوجيا التحالف — التقدم والنوافذ يُحمّلان من الحفظ السلطوي
    this.loadAllianceTech();
    this.loadAllianceShop();
    // P9-T5: أسعار الموارد والعروض النشطة في سوق المملكة
    this.loadTradingState();
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

    // P6-T6: تحميل سجل الدردشة من SQLite (آخر 100 رسالة)
    const chatCfg = getChatConfig();
    const maxHistory = chatCfg.limits?.maxHistoryPerChannel ?? 100;
    this.chatHistory = this.ctx.storage.sql
      .exec<any>("SELECT * FROM chat_messages ORDER BY created_at DESC LIMIT ?", maxHistory)
      .toArray()
      .reverse()
      .map((r) => ({
        id: r.id,
        channel: r.channel,
        allianceId: r.alliance_id || null,
        playerId: r.player_id,
        playerName: "", // يُملأ عند البث
        civ: "",
        text: r.text,
        timestampMs: r.created_at,
      }));

    this.reports = this.ctx.storage.sql
      .exec<any>("SELECT payload_json FROM battle_reports ORDER BY created_at DESC LIMIT 50")
      .toArray()
      .map((r) => JSON.parse(r.payload_json));

    this.seasonStory = this.ctx.storage.sql
      .exec<any>("SELECT * FROM season_story_events ORDER BY created_at ASC LIMIT 120")
      .toArray()
      .map((r) => ({
        id: r.id,
        kind: r.kind,
        seasonDay: r.season_day,
        createdAt: r.created_at,
        subjectId: r.subject_id,
        allianceId: r.alliance_id,
        previousAllianceId: r.previous_alliance_id,
        score: r.score ?? undefined,
      }));
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
    // P8-T4: بذر المواقع المقدسة (12 موقعًا + المعبد المفقود) من holy_sites.json
    this.seedHolySites();
    // P9-T2: بذر مراكز الموارد الحتمية (14 مركزًا لكل موسم) من alliance_territory.json
    this.seedAllianceCenters();

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
  }

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
  // P8-T4: المواقع المقدسة — حفظ/بذر/تحميل
  private seedHolySites() {
    for (const site of HOLY_SITES.sites ?? []) {
      const id = site.id;
      if (this.holySites.has(id)) continue;
      const ent: HolySiteEntity = {
        id,
        kind: site.kind,
        ownerAllianceId: null,
        captureProgress: 0,
        state: "open",
        x: site.pos[0],
        y: site.pos[1],
        heldSinceMs: null,
      };
      this.holySites.set(id, ent);
      this.persistHolySite(ent);
    }
    // المعبد المفقود في قلب Zone 3
    const temple = HOLY_SITES.temple;
    if (temple && !this.holySites.has(temple.id)) {
      const ent: HolySiteEntity = {
        id: temple.id,
        kind: "temple",
        ownerAllianceId: null,
        captureProgress: 0,
        state: "open",
        x: temple.pos[0],
        y: temple.pos[1],
        heldSinceMs: null,
      };
      this.holySites.set(temple.id, ent);
      this.persistHolySite(ent);
    }
  }
  private persistHolySite(s: HolySiteEntity) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO holy_sites
       (id, kind, owner_alliance_id, capture_progress, state, x, y, held_since_ms)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      s.id, s.kind, s.ownerAllianceId, s.captureProgress, s.state, s.x, s.y, s.heldSinceMs,
    );
  }
  private loadHolySites() {
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM holy_sites").toArray()) {
      this.holySites.set(row.id, {
        id: row.id,
        kind: row.kind,
        ownerAllianceId: row.owner_alliance_id,
        captureProgress: row.capture_progress,
        state: row.state,
        x: row.x,
        y: row.y,
        heldSinceMs: row.held_since_ms ?? null,
      });
    }
    // إعادة اشتقاق الملك من حالة المعبد: مملوك منذ 8 ساعات متواصلة = ملك المملكة
    this.king = null;
    const temple = this.holySites.get(HOLY_SITES.temple.id);
    if (temple?.ownerAllianceId && temple.heldSinceMs != null && nowMs() - temple.heldSinceMs >= holdForKingMs()) {
      this.king = { allianceId: temple.ownerAllianceId, crownedAtMs: temple.heldSinceMs, expiresAtMs: null };
    }
  }

  // P9-T3: تحميل متجر التحالف والألقاب من الحفظ السلطوي
  private loadAllianceShop() {
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM alliance_shop").toArray()) {
      let items: Record<string, number> = {};
      let titles: Record<string, string> = {};
      try {
        items = JSON.parse(row.items_json || "{}");
        titles = JSON.parse(row.titles_json || "{}");
      } catch {
        items = {};
        titles = {};
      }
      this.allianceShop.set(row.alliance_id, {
        balance: row.balance ?? 0,
        dailyEarned: row.daily_earned ?? 0,
        dailyEarnedDay: row.daily_earned_day ?? 0,
        items,
        titles,
      });
    }
  }
  // P9-T5: تحميل حالة Trading Post من الحفظ السلطوي — أسعار الموارد + العروض النشطة.
  private loadTradingState() {
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM trading_prices").toArray()) {
      this.tradingPrices.set(row.resource, {
        price: Number(row.price),
        day: Number(row.day) || 0,
        demand: Number(row.demand_trades) || 0,
        supply: Number(row.supply_offers) || 0,
        updatedMs: Number(row.updated_ms) || 0,
      });
    }
  }

  // P9-T5: سعر مورد حالي — مهيأ من JSON عند أول طلب إن لم يُسجّل بعد
  private tradingPriceFor(resource: string, nowMs: number): number {
    const cur = this.tradingPrices.get(resource);
    if (cur) return cur.price;
    const price = initialPriceFor(resource);
    this.tradingPrices.set(resource, { price, day: 0, demand: 0, supply: 0, updatedMs: nowMs });
    this.ctx.storage.sql.exec(
      `INSERT OR IGNORE INTO trading_prices (resource, price, day, demand_trades, supply_offers, updated_ms) VALUES (?, ?, 0, 0, 0, ?)`,
      [resource, price, nowMs],
    );
    return price;
  }

  // P9-T5: حركة سعر مورد بعد صفقة (طلب ↑) أو عرض جديد دون إتمام (عرض ↑)
  // P9-T5: السعر المعروض = base التراكمية (base + Δd×step − Δs×step/2) مقربة لخطوة السعر.
  // نحسبه من العدادات التراكمية المخزنة (demand/supply) لا من السعر المقرب السابق —
  // لأن إعادة التقريب من قيمة مقربة ستفقد الخطوات الصغيرة (0.02 < 0.05) ولا يتراكم السعر.
  private bumpTradingPrice(resource: string, demandDelta: number, supplyDelta: number, nowMs: number) {
    if (demandDelta === 0 && supplyDelta === 0) return;
    const cur = this.tradingPrices.get(resource) ?? { price: initialPriceFor(resource), day: 0, demand: 0, supply: 0, updatedMs: nowMs };
    const price = adaptPrice(resourceBasePrice(resource), cur.demand + demandDelta, cur.supply + supplyDelta);
    this.tradingPrices.set(resource, {
      price,
      day: cur.day,
      demand: Math.max(0, cur.demand + demandDelta),
      supply: Math.max(0, cur.supply + supplyDelta),
      updatedMs: nowMs,
    });
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO trading_prices (resource, price, day, demand_trades, supply_offers, updated_ms) VALUES (?, ?, ?, ?, ?, ?)`,
      [resource, price, this.tradingPrices.get(resource)!.day, this.tradingPrices.get(resource)!.demand, this.tradingPrices.get(resource)!.supply, nowMs],
    );
  }

  // P9-T5: العروض النشطة للاعب (عدد) — لحد سقف العروض
  private activeOffersForPlayer(playerId: string, nowMs: number): TradingOffer[] {
    const ttlSec = Number(tradingConstants().offer_ttl_sec);
    const out: TradingOffer[] = [];
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM trading_offers WHERE seller_id = ? AND remaining > 0", [playerId]).toArray()) {
      if (row.created_ms + ttlSec * 1000 > nowMs) out.push(row);
    }
    return out;
  }

  private persistAllianceShop(allianceId: string, state: AllianceShopState) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO alliance_shop
       (alliance_id, balance, daily_earned, daily_earned_day, items_json, titles_json)
       VALUES (?, ?, ?, ?, ?, ?)`,
      allianceId,
      state.balance,
      state.dailyEarned,
      state.dailyEarnedDay,
      JSON.stringify(state.items),
      JSON.stringify(state.titles),
    );
  }
  private getOrInitShop(allianceId: string): AllianceShopState {
    let state = this.allianceShop.get(allianceId);
    if (!state) {
      state = allianceShopStateInitial();
      this.allianceShop.set(allianceId, state);
    }
    return state;
  }
  // P9-T3: بافات ألقاب اللاعب داخل تحالفه — تُطبَّق على مسيراته/مدنه
  allianceTitleBuffs(playerId: string, allianceId: string | null | undefined): Record<string, number> {
    if (!allianceId) return {};
    const state = this.allianceShop.get(allianceId);
    if (!state) return {};
    return titleBuffsForPlayer(state, playerId);
  }
  // P9-T1: تحميل تقدم تقنيات التحالف ونوافذ تبرع الأعضاء من الحفظ السلطوي
  private loadAllianceTech() {
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM alliance_tech").toArray()) {
      const allianceId = row.alliance_id;
      let state = this.allianceTech.allianceTech.get(allianceId);
      if (!state) {
        state = {};
        this.allianceTech.allianceTech.set(allianceId, state);
      }
      state[row.tech_id] = {
        points: row.points ?? 0,
        level: row.level ?? 0,
        researchStartedAtMs: row.research_started_at_ms ?? null,
      };
    }
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM donation_windows").toArray()) {
      const windows = this.allianceTech.donationWindows.get(row.player_id) || [];
      windows.push({ windowStartMs: row.window_start_ms, count: row.count ?? 0 });
      this.allianceTech.donationWindows.set(row.player_id, windows);
    }
  }

  // P9-T3: كسب رصيد تحالف من مساعدة عضو — persist تلقائي
  earnAllianceHelpCredit(allianceId: string): { earned: number; balance: number } {
    const state = this.getOrInitShop(allianceId);
    const { state: next, earned } = applyHelpCredit(state, this.seasonDay, nowMs());
    if (earned <= 0) return { earned: 0, balance: next.balance };
    this.allianceShop.set(allianceId, next);
    this.persistAllianceShop(allianceId, next);
    return { earned, balance: next.balance };
  }
  // P9-T3: كسب رصيد تحالف من مطالبة هدية
  earnAllianceGiftClaimCredit(allianceId: string): { earned: number; balance: number } {
    const state = this.getOrInitShop(allianceId);
    const { state: next, earned } = applyGiftClaimCredit(state, this.seasonDay);
    if (earned <= 0) return { earned: 0, balance: next.balance };
    this.allianceShop.set(allianceId, next);
    this.persistAllianceShop(allianceId, next);
    return { earned, balance: next.balance };
  }
  // P9-T1: حفظ تقدم تقنية تحالف — INSERT OR REPLACE عبر المفتاح المزدوج
  private persistAllianceTech(allianceId: string, techId: string, progress: TechProgress) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO alliance_tech
       (alliance_id, tech_id, points, level, research_started_at_ms)
       VALUES (?, ?, ?, ?, ?)`,
      allianceId,
      techId,
      progress.points,
      progress.level,
      progress.researchStartedAtMs,
    );
  }

  // P9-T1: حفظ نافذة تبرع لاعب واحدة — وينظّف النوافذ القديمة أولاً
  private persistDonationWindows(playerId: string, windows: DonationWindow[]) {
    this.ctx.storage.sql.exec(`DELETE FROM donation_windows WHERE player_id = ?`, playerId);
    const windowMs = ALLIANCE_TECH_WINDOW_MS;
    const alive = windows.filter((w) => nowMs() - w.windowStartMs < windowMs);
    for (const w of alive) {
      this.ctx.storage.sql.exec(
        `INSERT INTO donation_windows (player_id, window_start_ms, count) VALUES (?, ?, ?)`,
        playerId,
        w.windowStartMs,
        w.count,
      );
    }
  }

  // P5-T5: إنشاء كشافة من مدينة اللاعب إلى الهدف — أسرع من المسير العادي (ضعفا السرعة)
  private createScout(body: any): ScoutEntity {
    const ownerPlayerId = String(body?.ownerPlayerId || "");
    if (!ownerPlayerId) throw new Error("owner_player_required");
    const map = getMap();
    const toX = Number(body.toX);
    const toY = Number(body.toY);
    if (!Number.isFinite(toX) || !Number.isFinite(toY) || toX < 0 || toX > map.width || toY < 0 || toY > map.height) {
      throw new Error("bad_scout_coords");
    }
    // نقطة الانطلاق: مدينة اللاعب إن وُجدت وإلا الإحداثيات الممررة من الـ router
    const city = this.cities.get(ownerPlayerId);
    const fromX = city?.x ?? (Number(body.fromX) || 0);
    const fromY = city?.y ?? (Number(body.fromY) || 0);
    const now = nowMs();
    const ent: ScoutEntity = {
      id: newId("sct"),
      ownerPlayerId,
      fromX,
      fromY,
      toX,
      toY,
      startMs: now,
      etaMs: now + marchDurationMs(dist(fromX, fromY, toX, toY), 40),
      state: "moving",
    };
    this.scouts.set(ent.id, ent);
    this.persistScout(ent);
    this.broadcast({ type: "scout_created", scout: ent });
    return ent;
  }

  private persistScout(s: ScoutEntity) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO scouts
       (id, owner_player_id, from_x, from_y, to_x, to_y, start_ms, eta_ms, state)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      s.id, s.ownerPlayerId, s.fromX, s.fromY, s.toX, s.toY, s.startMs, s.etaMs, s.state,
    );
  }

  // P3-T3: تكثيف البرابرة أثناء حدث "غزو البرابرة" — يزرع معسكرات إضافية حتمياً
  // (id يعتمد على اليوم فلا يتكرر الزرع كل tick). يعيد true إن زرع شيئاً جديداً.
  private seedEventBarbarians(extraPerRegion: number, tickInDay: number): boolean {
    let spawned = false;
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
      `INSERT OR REPLACE INTO map_cities
         (player_id, name, alliance_id, civ, x, y, hall_level, region_id,
          ap, last_ap_ms, shield_until_ms, war_frenzy_until_ms, last_relocation_ms)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      c.playerId,
      c.name,
      c.allianceId,
      c.civ,
      c.x,
      c.y,
      c.hallLevel,
      c.regionId,
      c.ap,
      c.lastApMs,
      c.shieldUntilMs ?? null,
      c.warFrenzyUntilMs ?? null,
      c.lastRelocationMs ?? null,
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

  // ══════════ P8-T6: المهام اليومية/الأسبوعية (الكل في D1) ══════════

  /** حالة مهام اللاعب الحالية: توزيع حتمي لليوم/الأسبوع + تقدم حي من الطوابير الجارية. */
  private async questsStateFor(playerId: string): Promise<any> {
    const day = questDay(nowMs());
    const week = questWeek(nowMs());
    const [dailyRows, weeklyRows, pointsRows, rewardsRows] = await Promise.all([
      this.env.DB.prepare("SELECT * FROM player_quests WHERE player_id = ? AND cycle = 'daily'").bind(playerId).all<any>(),
      this.env.DB.prepare("SELECT * FROM player_quests WHERE player_id = ? AND cycle = 'weekly'").bind(playerId).all<any>(),
      this.env.DB.prepare("SELECT * FROM player_quest_points WHERE player_id = ?").bind(playerId).all<any>(),
      this.env.DB.prepare("SELECT * FROM player_quest_rewards WHERE player_id = ?").bind(playerId).all<any>(),
    ]);
    const dayRow = pointsRows.results?.find?.((r: any) => r.cycle === "daily" && r.cycle_day === day) || null;
    const weekRow = pointsRows.results?.find?.((r: any) => r.cycle === "weekly" && r.cycle_day === week) || null;
    const goldenKeyGranted = Boolean(dayRow?.golden_key_granted);
    const weeklyChestGranted = Boolean((rewardsRows.results || []).find((r: any) => r.reward_type === "weekly_chest" && r.cycle_day === week));
    return {
      day,
      week,
      daily: (dailyRows.results || []).map((r: any) => ({ id: r.quest_id, typeId: r.type_id, goal: r.goal, points: r.points, progress: r.progress, claimed: Boolean(r.claimed), description: questDescription(r.type_id, r.goal) })),
      weekly: (weeklyRows.results || []).map((r: any) => ({ id: r.quest_id, typeId: r.type_id, goal: r.goal, points: r.points, progress: r.progress, claimed: Boolean(r.claimed), description: questDescription(r.type_id, r.goal) })),
      dailyPoints: dayRow?.points || 0,
      weeklyPoints: weekRow?.points || 0,
      goldenKeyGranted,
      weeklyChestGranted,
      goldenKeyEligible: (dayRow?.points || 0) >= QUESTS.rewards.golden_key_cost_points,
      weeklyChestEligible: (weekRow?.points || 0) >= QUESTS.rewards.weekly_chest_cost_points,
    };
  }

  /** استدعاء shard داخلي: يوزع المهام إن لزم ثم يضيف تقدمًا من مصدر حدث. */
  private async recordQuestProgress(playerId: string, source: string, amount: number): Promise<{ dailyPoints: number; weeklyPoints: number; completedQuestIds: string[] }> {
    const day = questDay(nowMs());
    const week = questWeek(nowMs());
    const now = nowMs();
    await this.ensureQuestsDistributed(playerId, day, week, now);
    const [dailyRows, weeklyRows, pointsRows] = await Promise.all([
      this.env.DB.prepare("SELECT * FROM player_quests WHERE player_id = ? AND cycle = 'daily' AND cycle_day = ?").bind(playerId, day).all<any>(),
      this.env.DB.prepare("SELECT * FROM player_quests WHERE player_id = ? AND cycle = 'weekly' AND cycle_day = ?").bind(playerId, week).all<any>(),
      this.env.DB.prepare("SELECT * FROM player_quest_points WHERE player_id = ?").bind(playerId).all<any>(),
    ]);
    const completedIds: string[] = [];
    for (const r of dailyRows.results || []) {
      const def = QUESTS.types[r.type_id];
      if (!def || !def.progress_sources.includes(source)) continue;
      if (r.completed) continue;
      const next = Math.min(r.goal, Number(r.progress) + amount);
      const points = r.points;
      await this.env.DB.prepare("UPDATE player_quests SET progress=?, completed=?, updated_at=? WHERE player_id=? AND cycle='daily' AND slot=?")
        .bind(next, next >= r.goal ? 1 : 0, now, playerId, r.slot).run();
      if (next >= r.goal) {
        completedIds.push(r.quest_id);
        await this.addQuestPoints(playerId, "daily", day, points);
      }
    }
    for (const r of weeklyRows.results || []) {
      const def = QUESTS.types[r.type_id];
      if (!def || !def.progress_sources.includes(source)) continue;
      if (r.completed) continue;
      const next = Math.min(r.goal, Number(r.progress) + amount);
      const points = r.points;
      await this.env.DB.prepare("UPDATE player_quests SET progress=?, completed=?, updated_at=? WHERE player_id=? AND cycle='weekly' AND slot=?")
        .bind(next, next >= r.goal ? 1 : 0, now, playerId, r.slot).run();
      if (next >= r.goal) {
        completedIds.push(r.quest_id);
        await this.addQuestPoints(playerId, "weekly", week, points);
      }
    }
    const dayPoints = pointsRows.results?.find?.((r: any) => r.cycle === "daily" && r.cycle_day === day)?.points || 0;
    const weekPoints = pointsRows.results?.find?.((r: any) => r.cycle === "weekly" && r.cycle_day === week)?.points || 0;
    return { dailyPoints: dayPoints, weeklyPoints: weekPoints, completedQuestIds: completedIds };
  }

  /** إضافة نقاط دورة (y capped بالحد من JSON). */
  private async addQuestPoints(playerId: string, cycle: "daily" | "weekly", cycleDay: number, points: number) {
    const cap = cycle === "daily" ? QUESTS.constants.daily_points_limit : QUESTS.constants.weekly_points_limit;
    const row = await this.env.DB.prepare("SELECT points FROM player_quest_points WHERE player_id = ? AND cycle = ? AND cycle_day = ?")
      .bind(playerId, cycle, cycleDay).first<{ points: number | null }>();
    const next = Math.min(cap, (row?.points || 0) + points);
    await this.env.DB.prepare("INSERT INTO player_quest_points (player_id, cycle, cycle_day, points, golden_key_granted, updated_at) VALUES (?, ?, ?, ?, 0, ?) ON CONFLICT(player_id, cycle, cycle_day) DO UPDATE SET points=excluded.points, updated_at=excluded.updated_at")
      .bind(playerId, cycle, cycleDay, next, nowMs()).run();
  }

  /** توزيع/استبدال المهام الحتمية عند عدم وجودها أو انتهاء اليوم/الأسبوع. */
  private async ensureQuestsDistributed(playerId: string, day: number, week: number, now: number) {
    const dailyRows = await this.env.DB.prepare("SELECT cycle_day FROM player_quests WHERE player_id = ? AND cycle = 'daily' LIMIT 1").bind(playerId).first<{ cycle_day: number | null }>();
    if (!dailyRows || dailyRows.cycle_day !== day) {
      const quests = buildDailyQuests(playerId, day);
      await this.env.DB.prepare("DELETE FROM player_quests WHERE player_id = ? AND cycle = 'daily'").bind(playerId).run();
      for (let i = 0; i < quests.length; i++) {
        const q = quests[i];
        const questId = `daily_${day}_${i}`;
        await this.env.DB.prepare("INSERT INTO player_quests (player_id, cycle, slot, cycle_day, quest_id, type_id, goal, points, progress, claimed, completed, updated_at) VALUES (?, 'daily', ?, ?, ?, ?, ?, ?, 0, 0, 0, ?)")
          .bind(playerId, i, day, questId, q.typeId, q.goal, q.points, now).run();
      }
    }
    const weeklyRows = await this.env.DB.prepare("SELECT cycle_day FROM player_quests WHERE player_id = ? AND cycle = 'weekly' LIMIT 1").bind(playerId).first<{ cycle_day: number | null }>();
    if (!weeklyRows || weeklyRows.cycle_day !== week) {
      const quests = buildWeeklyQuests(playerId, week);
      await this.env.DB.prepare("DELETE FROM player_quests WHERE player_id = ? AND cycle = 'weekly'").bind(playerId).run();
      for (let i = 0; i < quests.length; i++) {
        const q = quests[i];
        const questId = `weekly_${week}_${i}`;
        await this.env.DB.prepare("INSERT INTO player_quests (player_id, cycle, slot, cycle_day, quest_id, type_id, goal, points, progress, claimed, completed, updated_at) VALUES (?, 'weekly', ?, ?, ?, ?, ?, ?, 0, 0, 0, ?)")
          .bind(playerId, i, week, questId, q.typeId, q.goal, q.points, now).run();
      }
    }
  }

  // ══════════ نهاية P8-T6 ══════════

  // ══════════ P9-T1: تكنولوجيا التحالف ══════════

  /** حالة تقنيات التحالف للّبث: تحالف اللاعب المعني إن حُدّد، وإلا كل التحالفات ذات التقدم. */
  private allianceTechStateFor(playerAllianceId: string | null | undefined) {
    const entries = playerAllianceId
      ? [[playerAllianceId, this.allianceTech.allianceTech.get(playerAllianceId)]]
      : [...this.allianceTech.allianceTech.entries()];
    return entries
      .filter(([, state]) => state && Object.keys(state).length > 0)
      .map(([allianceId, state]) => ({ allianceId, techs: Object.entries(state || {}) }));
  }
  /** حالة متجر التحالف والألقاب للّبث: تحالف اللاعب المعني إن حُدّد، وإلا كل التحالفات ذات نشاط (رصيد>0 أو ألقاب). */
  private allianceShopStateFor(playerAllianceId: string | null | undefined) {
    const entries: Array<[string, AllianceShopState]> = playerAllianceId
      ? [[playerAllianceId, this.allianceShop.get(playerAllianceId) || allianceShopStateInitial()]]
      : Array.from(this.allianceShop.entries());
    return entries
      .filter(([, state]) => state && (state.balance > 0 || Object.keys(state.titles).length > 0 || Object.keys(state.items).length > 0))
      .map(([allianceId, state]) => ({ allianceId, balance: state.balance, items: state.items, titles: state.titles }));
  }

  /** بث تقدم تقنية تغيرت — للتحالف المعني فقط. */
  private broadcastAllianceTechChange(allianceId: string, techId: string, progress: TechProgress) {
    const tech = AllianceTechService.techById(techId);
    this.broadcast({
      type: "alliance_tech_updated",
      allianceId,
      techId,
      techName: tech?.name ?? techId,
      points: progress.points,
      level: progress.level,
      buffs: AllianceTechService.computeBuffs(this.allianceTech.allianceTech.get(allianceId) ?? {}),
    });
  }

  /** نقاط باف البحث الحالية لتحالف عضو (للتطبيق على المساعدة/التدريب/القتال). */
  allianceTechBuffs(allianceId: string | null | undefined): Record<string, number> {
    if (!allianceId) return {};
    return AllianceTechService.computeBuffs(this.allianceTech.allianceTech.get(allianceId) ?? {});
  }

  /** بافات القتال (هجوم/دفاع/HP) لتحالف مسيرة — تُضاف إلى تعديلات القائد/الأبحاث/المواهب. */
  private marchAllianceTechAttackMod(marchAllianceId: string | null | undefined): number {
    const buffs = this.allianceTechBuffs(marchAllianceId);
    // الهجوم والدفاع والـHP تُجمع في aMult/dMult — سقف مجموع 0.5 كحد أمان ضد تراكم غير متوقع
    return Math.min(0.5, (buffs["alliance_attack_bonus"] || 0) + (buffs["alliance_defense_bonus"] || 0) + (buffs["alliance_hp_bonus"] || 0));
  }

  /** باف الحصار/الممرات لتحالف مسيرة — يضاف إلى attackerTalentAttackMod. */
  private marchAllianceTechSiegeMod(marchAllianceId: string | null | undefined): number {
    const buffs = this.allianceTechBuffs(marchAllianceId);
    return Math.min(0.5, (buffs["siege_damage_bonus"] || 0) + (buffs["pass_attack_bonus"] || 0));
  }
  /**
   * P9-T3: بافات ألقاب التحالف — تُطبَّق فقط على مسيرة اللاعب نفسه حين يكون حامل لقب نشط داخل تحالفها.
   * الألقاب سلطوية: تُقارن by playerId، لا يُخضع أحد لقوات حامل اللقب.
   * سقف 0.25 لكل باف (هجوم/دفاع/HP/جمع/حصار) كحد أمان.
   */
  private marchTitleMod(marchOwnerPlayerId: string, marchAllianceId: string | null | undefined, kind: "attack" | "defense" | "hp" | "siege" | "gather"): number {
    if (!marchAllianceId) return 0;
    const buffs = this.allianceTitleBuffs(marchOwnerPlayerId, marchAllianceId);
    let v = 0;
    if (kind === "attack") v = (buffs["attack_mod"] || 0) + (buffs["siege_mod"] || 0);
    if (kind === "defense") v = (buffs["defense_mod"] || 0);
    if (kind === "hp") v = (buffs["hp_mod"] || 0);
    if (kind === "siege") v = (buffs["siege_mod"] || 0);
    if (kind === "gather") v = (buffs["gather_mod"] || 0);
    return Math.min(0.25, v);
  }

  /** P9-T1: تبرع عضو بنقطة تقنية — البحث النشط للتحالف هو المستفيد الوحيد. */
  private donateAllianceTech(playerId: string, allianceId: string, techId: string): { ok: true; level: number; points: number } | { error: string; status: number } {
    const city = this.cities.get(playerId);
    if (!city || city.allianceId !== allianceId) return { error: "not_your_alliance", status: 403 };
    const state = this.allianceTech.allianceTech.get(allianceId) ?? {};
    // البحث النشط للتحالف: التقنية التي فيها progress مع البحث الجاري (أي تقنية بدأها ضباط)
    const activeTechId = Object.entries(state).find(([, p]) => p.researchStartedAtMs != null)?.[0];
    if (!activeTechId) return { error: "no_active_research", status: 400 };
    const tech = AllianceTechService.techById(activeTechId);
    if (!tech) return { error: "unknown_tech", status: 400 };
    if (techId !== activeTechId) return { error: "donate_goes_to_active_research", status: 400 };
    if (state[activeTechId].level >= tech.levels)
      return { error: "tech_max_level", status: 400 };
    const windows = this.allianceTech.donationWindows.get(playerId) || [];
    const now = nowMs();
    if (!AllianceTechService.canDonate(now, windows)) return { error: "donation_limit_reached", status: 429 };
    const nextWindows = AllianceTechService.recordDonation(now, windows);
    this.allianceTech.donationWindows.set(playerId, nextWindows);
    this.persistDonationWindows(playerId, nextWindows);
    // نقاط البحث تتراكم؛ البحث النشط يبدأ فقط بعد بلوغ عتبة المستوى التالي (نقاط كافية)
    const progress = AllianceTechService.applyPoints(state[activeTechId] ?? { points: 0, level: 0, researchStartedAtMs: now }, tech, ALLIANCE_TECH_CFG.points_per_donation);
    state[activeTechId] = progress;
    this.allianceTech.allianceTech.set(allianceId, state);
    this.persistAllianceTech(allianceId, activeTechId, progress);
    this.broadcastAllianceTechChange(allianceId, activeTechId, progress);
    return { ok: true, level: progress.level, points: progress.points };
  }

  /** P9-T1: ضابط (R3+) يبدأ بحثًا جماعيًا — بحث نشط واحد لكل تحالف. */
  private startAllianceResearch(
    playerId: string,
    allianceId: string,
    rank: string,
    techId: string,
  ): { ok: true; techId: string } | { error: string; status: number } {
    const city = this.cities.get(playerId);
    if (!city || city.allianceId !== allianceId) return { error: "not_your_alliance", status: 403 };
    if (!AllianceTechService.canStartResearch(rank)) return { error: "rank_insufficient", status: 403 };
    const tech = AllianceTechService.techById(techId);
    if (!tech) return { error: "unknown_tech", status: 400 };
    const state = this.allianceTech.allianceTech.get(allianceId) ?? {};
    // البحث النشط الوحيد؛ يمكن التبديل فقط عندما يكتمل البحث الحالي (نقاط العتبة الأخيرة)
    const activeEntry = Object.entries(state).find(([, p]) => p.researchStartedAtMs != null);
    if (activeEntry) {
      const [activeId, activeProgress] = activeEntry;
      const activeTech = AllianceTechService.techById(activeId);
      if (!activeTech || activeProgress.level < activeTech.levels)
        return { error: "active_research_exists", status: 409 };
      // البحث المكتمل يُغلق قبل بدء التقنية الجديدة
      activeEntry[1].researchStartedAtMs = null;
    }
    const progress = state[techId] ?? { points: 0, level: 0, researchStartedAtMs: null };
    if (progress.level >= tech.levels) return { error: "tech_max_level", status: 400 };
    progress.researchStartedAtMs = nowMs();
    state[techId] = progress;
    this.allianceTech.allianceTech.set(allianceId, state);
    this.persistAllianceTech(allianceId, techId, progress);
    this.broadcast({ type: "alliance_research_started", allianceId, techId, techName: tech.name, startedBy: playerId });
    return { ok: true, techId };
  }

  // ══════════ نهاية P9-T1 ══════════

  /** تُظهر التقرير للمهاجم أو المدافع أو المشارك، ولتحالف أي طرف ذي صلة فقط. */
  private reportVisibleTo(report: any, playerId: string, allianceId: string | null | undefined) {
    if (!playerId) return false;
    if (report.attackerPlayerId === playerId || report.defenderPlayerId === playerId) return true;
    if (allianceId && (report.attackerAllianceId === allianceId || report.defenderAllianceId === allianceId)) return true;
    const rally = report.rally;
    if (!rally) return false;
    if (Array.isArray(rally.participants) && rally.participants.some((p: any) => p.playerId === playerId)) return true;
    return Boolean(allianceId && rally.allianceId && rally.allianceId === allianceId);
  }

  private visibleReportsFor(playerId: string, allianceId: string | null | undefined) {
    return this.reports.filter((report) => this.reportVisibleTo(report, playerId, allianceId)).slice(0, 30);
  }

  /** يبث التقرير فقط للاتصالات التي يحق لها قراءته؛ لا يُستخدم البث العام للتقارير. */
  private broadcastReport(report: any) {
    for (const ws of this.ctx.getWebSockets()) {
      try {
        const attachment = (ws.deserializeAttachment() || { playerId: "" }) as Attach;
        const city = this.cities.get(attachment.playerId);
        if (this.reportVisibleTo(report, attachment.playerId, city?.allianceId)) {
          ws.send(JSON.stringify({ type: "battle_report", report }));
        }
      } catch {
        // تجاهل اتصال مغلق أو مرفق غير صالح.
      }
    }
  }

  /** توزيع حتمي لمجموع قوات على مساهمي الرالي، بالتناسب ثم بأكبر كسر متبقٍ. */
  private distributeRallyTroops(
    contributions: Array<{ playerId: string; troops: Troops }>,
    total: Troops,
  ): Record<string, Troops> {
    const distributed: Record<string, Troops> = {};
    for (const contribution of contributions) distributed[contribution.playerId] = {};
    for (const [unitId, rawTotal] of Object.entries(total || {})) {
      const eligible = contributions
        .filter((contribution) => Number(contribution.troops?.[unitId] || 0) > 0)
        .sort((a, b) => a.playerId.localeCompare(b.playerId));
      const capacity = eligible.reduce((sum, contribution) => sum + Number(contribution.troops[unitId] || 0), 0);
      let amount = Math.min(Math.max(0, Math.floor(Number(rawTotal) || 0)), capacity);
      if (amount <= 0 || capacity <= 0) continue;
      const shares = eligible.map((contribution) => {
        const raw = (amount * Number(contribution.troops[unitId] || 0)) / capacity;
        return { contribution, count: Math.floor(raw), fraction: raw - Math.floor(raw) };
      });
      let assigned = shares.reduce((sum, share) => sum + share.count, 0);
      shares.sort((a, b) => (b.fraction - a.fraction) || a.contribution.playerId.localeCompare(b.contribution.playerId));
      for (let i = 0; assigned < amount && shares.length > 0; i++, assigned++) shares[i % shares.length].count++;
      for (const share of shares) {
        if (share.count > 0) distributed[share.contribution.playerId][unitId] = share.count;
      }
    }
    return distributed;
  }

  /** يسوي أثر قتال رالي على أرصدة كل عضو ويعيد سياق التقرير التفصيلي. */
  private async settleRallyCombat(march: MarchEntity, result: CombatResult) {
    const source = Array.isArray(march.payload?.rallyParticipants) ? march.payload.rallyParticipants : [];
    const contributions = source
      .map((entry: any) => ({ playerId: String(entry.playerId || ""), troops: (entry.committed || entry.troops || {}) as Troops }))
      .filter((entry: { playerId: string }) => Boolean(entry.playerId));
    if (!march.payload?.rallyId || contributions.length === 0) return null;

    const losses = this.distributeRallyTroops(contributions, result.attackerLosses);
    const dead = this.distributeRallyTroops(contributions, result.attackerSplit.dead);
    const severely = this.distributeRallyTroops(contributions, result.attackerSplit.severely);
    const slightly = this.distributeRallyTroops(contributions, result.attackerSplit.slightly);
    // عمليات المشاركين مستقلة بعد تثبيت توزيع الخسائر. Promise.all يحافظ على ترتيب
    // contributions في مصفوفة الناتج، لذلك يبقى التقرير حتمياً مع تقليل دورات D1 المتسلسلة.
    const participants = await Promise.all(contributions.map(async (contribution: { playerId: string; troops: Troops }) => {
      const playerLosses = losses[contribution.playerId] || {};
      const remaining: Troops = {};
      for (const [unitId, committed] of Object.entries(contribution.troops)) {
        const survivorCount = Math.max(0, Number(committed) - Number(playerLosses[unitId] || 0));
        if (survivorCount > 0) remaining[unitId] = survivorCount;
      }
      const [hospital] = await Promise.all([
        this.admitToHospital(contribution.playerId, severely[contribution.playerId] || {}),
        this.deductMarchLosses(contribution.playerId, playerLosses),
      ]);
      return {
        playerId: contribution.playerId,
        committed: contribution.troops,
        remaining,
        losses: playerLosses,
        dead: dead[contribution.playerId] || {},
        severely: severely[contribution.playerId] || {},
        slightly: slightly[contribution.playerId] || {},
        hospital,
      };
    }));

    march.payload = { ...march.payload, rallyParticipants: participants };
    return {
      rallyId: String(march.payload.rallyId),
      allianceId: march.allianceId || null,
      leaderPlayerId: march.ownerPlayerId,
      participants,
    };
  }

  private async settleAttackerCombat(march: MarchEntity, result: CombatResult) {
    const rally = await this.settleRallyCombat(march, result);
    if (rally) return { rally };
    const [hospital] = await Promise.all([
      this.admitToHospital(march.ownerPlayerId, result.attackerSplit.severely),
      this.deductMarchLosses(march.ownerPlayerId, result.attackerLosses),
    ]);
    // P8-T5: حمى الحرب — المدينة المدافعة التي تعرضت لهجوم تصبح محمية 1h من هجمات أخرى
    if (march.targetType === "city" && march.targetId) {
      const defender = this.cities.get(march.targetId);
      if (defender && march.ownerPlayerId !== march.targetId) {
        defender.warFrenzyUntilMs = nowMs() + warFrenzyDurationMs();
        this.persistCity(defender);
      }
    }
    // P8-T6: تقدم مهمة الانتصارات اليومي/الأسبوعية (نصر في هجوم على مدينة)
    if (march.targetType === "city" && result.winner === "attacker") {
      try { void this.recordQuestProgress(march.ownerPlayerId, "battle_win", 1); } catch {}
    }
    return { hospital };
  }

  /** P8-T5: خصم نقاط عمل من مدينة قبل إطلاق مسيرة — يجرّد التجديد الحالي ثم يرفض إن لم تكفِ. */
  private deductApFromCity(city: CityEntity, action: "barb_battle" | "holy_site_battle" | "city_attack") {
    const cost = apCost(action);
    if (cost <= 0) return;
    const { ap, lastRegenMs } = regenAp(city.ap, city.lastApMs, nowMs());
    city.ap = ap;
    city.lastApMs = lastRegenMs;
    if (city.ap < cost) throw new Error("not_enough_ap");
    city.ap -= cost;
    this.persistCity(city);
  }
  /** P8-T4: جرحى المعبد الخطيرون — 50% منهم يموتون مباشرة (فوق قاعدة المستشفى) */
  private applyTempleSevereDeath(result: CombatResult) {
    const share = templeWoundedDeadShare();
    for (const [u, c] of Object.entries(result.attackerSplit.severely)) {
      const dead = Math.floor(Number(c) * share);
      result.attackerSplit.severely[u] = Number(c) - dead;
      result.attackerSplit.dead[u] = Math.max(0, Number(result.attackerSplit.dead[u] || 0) + dead);
    }
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

  /** كل القلاع التي تنشر نطاقًا إقليمياً: الأعلام + قلاع outpost. */
  private castleList(): Array<{ id: string; allianceId: string | null; x: number; y: number; radius?: number; kind: "flag" | "outpost" }> {
    return [
      ...[...this.flags.values()].map((f) => ({ id: f.id, allianceId: f.allianceId, x: f.x, y: f.y, radius: f.radius, kind: "flag" as const })),
      ...[...this.allianceOutposts.values()].map((o) => ({ id: o.id, allianceId: o.allianceId, x: o.x, y: o.y, radius: o.radius, kind: "outpost" as const })),
    ];
  }

  private seedAllianceCenters() {
    // P9-T2: بذر حتمي لكل موسم — نفس المواضع لكل المواسم (بذر موثوق من JSON).
    for (const c of seedCenters(this.seasonDay || 1)) {
      this.resourceCenters.set(c.id, c);
      this.persistCenter(c);
    }
  }

  private persistAllianceOutpost(outpost: AllianceOutpostEntity) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO alliance_outposts (id, alliance_id, x, y, radius, created_by, created_at)
       VALUES (?, ?, ?, ?, ?, ?, ?)`,
      outpost.id,
      outpost.allianceId,
      outpost.x,
      outpost.y,
      outpost.radius,
      outpost.createdBy,
      outpost.createdAt,
    );
  }

  private persistCenter(center: CenterEntity) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO alliance_centers
       (id, kind, x, y, radius, locked_alliance_id, locked_until_ms, reserve, spawned_season_day)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      center.id,
      center.kind,
      center.x,
      center.y,
      center.radius,
      center.lockedAllianceId,
      center.lockedUntilMs,
      center.reserve,
      center.spawnedSeasonDay,
    );
  }

  private persistAllianceStructure(structure: AllianceStructure) {
    this.ctx.storage.sql.exec(
      `INSERT OR REPLACE INTO alliance_structures
       (id, kind, alliance_id, x, y, radius, protection_radius, march_damage_reduction, map_marker, created_by, created_at)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      structure.id,
      structure.kind,
      structure.allianceId,
      structure.x,
      structure.y,
      structure.radius,
      structure.protectionRadius,
      structure.marchDamageReduction,
      structure.mapMarker,
      structure.createdBy,
      structure.createdAt,
    );
  }

  private snapshot(playerId?: string) {
    const playerAllianceId = playerId ? this.cities.get(playerId)?.allianceId : null;
    return {
      seasonDay: this.seasonDay,
      cities: [...this.cities.values()],
      throne: this.throne,
      throneScores: [...this.throneScores.entries()],
      // P3-T2: أهداف قلب Zone 3 (حصون + مذابح) مع مالكيها — لتسجيل نقاط الموسم
      coreObjectives: [...this.coreObjectives.values()],
      // P8-T4: المواقع المقدسة (مع بافات النوع الواحد التي لا تتراكب) + الملك الحالي
      holySites: [...this.holySites.values()],
      king: this.king,
      passes: [...this.passes.values()],
      marches: [...this.marches.values()].filter((m) => m.state === "moving"),
      nodes: [...this.nodes.values()],
      flags: [...this.flags.values()],
      // الكتالوج ومثيلاته يُبثان مع اللقطة ليعرض العميل العلامة ودائرة النطاق من البيانات السلطوية.
      allianceStructures: [...this.allianceStructures.values()],
      allianceStructureCatalog: getAllianceStructures(),
      // P9-T2: أراضي التحالف — القلاع + مراكز الموارد + إعدادات النطاقات للعميل.
      allianceOutposts: [...this.allianceOutposts.values()],
      resourceCenters: [...this.resourceCenters.values()],
      territoryCfg: {
        flagRadius: flagRadius(),
        outpostRadius: outpostRadius(),
        gatherBonus: gatherBonus(),
        gatherMultiplier: gatherMultiplier(),
        patrolReduction: patrolReduction(),
      },
      // P9-T1: حالة تكنولوجيا التحالف — التقدم والمستويات للتحالف المعني فقط إن حُدّدت لاعب، وإلا لكل التحالفات
      allianceTechState: this.allianceTechStateFor(playerAllianceId),
      // P9-T3: متجر التحالف والألقاب — الرصيد والمشتريات والألقاب للتحالف المعني فقط إن حُدّدت لاعب، وإلا لكل التحالفات
      allianceShopState: this.allianceShopStateFor(playerAllianceId),
      // P5-T5: الكشافة المتحركة (يكملها العميل محلياً لرسم مسارها)
      scouts: [...this.scouts.values()].filter((s) => s.state === "moving"),
      // التقارير خاصة؛ لا تدخل في اللقطات العامة أو إرسال الحالة لكل العالم.
      reports: playerId ? this.visibleReportsFor(playerId, playerAllianceId).slice(0, 10) : [],
      // P2-T5: الطوابير الجارية (لمساعدات التحالف — تقليل المدة عبر /v1/alliance/help)
      queues: [...this.queues.values()].filter((q) => q.state === "running"),
      // P6-T6: سجل الدردشة الحية (آخر 100 رسالة — playerName/civ يُملأ من cities)
      chatHistory: this.chatHistory.map((m) => {
        const city = this.cities.get(m.playerId);
        return { ...m, playerName: city?.name ?? m.playerId, civ: city ? (this.cities.get(m.playerId) as any)?.civ ?? "" : "" };
      }),
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
      // P6-T10: خط زمني عام فقط، منفصل عن تقارير القتال الخاصة.
      seasonStory: this.seasonStory,
    };
  }

  /**
   * تحديث دوري جزئي بعد تغيّر المحاكاة. يترك البيانات الثابتة والخاصة
   * (المدن، التقارير، سجل الدردشة، الخريطة والكتالوج) للّقطة الأولى/REST.
   */
  private worldDelta(playerId?: string) {
    return {
      seasonDay: this.seasonDay,
      passes: [...this.passes.values()],
      marches: [...this.marches.values()].filter((m) => m.state === "moving"),
      nodes: [...this.nodes.values()],
      allianceStructures: [...this.allianceStructures.values()],
      scouts: [...this.scouts.values()].filter((s) => s.state === "moving"),
      queues: [...this.queues.values()].filter((q) => q.state === "running"),
      // P9-T2: أراضي التحالف — deltas للعالم الحي
      allianceOutposts: [...this.allianceOutposts.values()],
      resourceCenters: [...this.resourceCenters.values()],
      // P8-T4: المواقع المقدسة (حالة المالك/الحيازة + الملك) — deltas للعالم الحي
      holySites: [...this.holySites.values()],
      king: this.king,
      // P9-T1: تقدم تقنيات التحالف — deltas للعالم الحي
      allianceTechState: this.allianceTechStateFor(playerId ? this.cities.get(playerId)?.allianceId ?? null : null),
      // P9-T3: متجر التحالف والألقاب — deltas للعالم الحي
      allianceShopState: this.allianceShopStateFor(playerId ? this.cities.get(playerId)?.allianceId ?? null : null),
      zones: zonesStatus(this.seasonDay, this.regions),
      seasonStory: this.seasonStory,
    };
  }

  private recordSeasonStory(event: Omit<SeasonStoryEvent, "id" | "createdAt" | "seasonDay">, now = nowMs()) {
    const id = `season_story:${event.kind}:${event.subjectId}`;
    if (this.seasonStory.some((existing) => existing.id === id)) return false;
    const entry: SeasonStoryEvent = { id, createdAt: now, seasonDay: this.seasonDay, ...event };
    this.seasonStory.push(entry);
    this.seasonStory = this.seasonStory.slice(-120);
    this.ctx.storage.sql.exec(
      `INSERT OR IGNORE INTO season_story_events
       (id, kind, season_day, created_at, subject_id, alliance_id, previous_alliance_id, score)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      entry.id,
      entry.kind,
      entry.seasonDay,
      entry.createdAt,
      entry.subjectId,
      entry.allianceId,
      entry.previousAllianceId ?? null,
      entry.score ?? null,
    );
    this.broadcast({ type: "season_story_event", event: entry });
    return true;
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

  /** ترسل رسائل التحالف إلى أعضاء التحالف المُلتقط وقت النشر فقط. */
  private broadcastChat(chatMsg: ChatMessage) {
    const msg = JSON.stringify({ type: "chat_message", message: chatMsg });
    for (const ws of this.ctx.getWebSockets()) {
      const att = ws.deserializeAttachment() as { playerId?: string } | null;
      const recipientAllianceId = att?.playerId ? this.cities.get(att.playerId)?.allianceId : null;
      if (chatMsg.channel === "alliance" && (!chatMsg.allianceId || recipientAllianceId !== chatMsg.allianceId)) {
        continue;
      }
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
    if (extraBarbs > 0) changed = this.seedEventBarbarians(extraBarbs, tickInDay) || changed;

    // P5-T5: وصول الكشافة — بث scout_arrived ليكشف العميل ضباب الحرب حول الهدف
    for (const s of [...this.scouts.values()]) {
      if (s.state === "moving" && now >= s.etaMs) {
        s.state = "arrived";
        this.broadcast({ type: "scout_arrived", scoutId: s.id, toX: s.toX, toY: s.toY });
        this.scouts.delete(s.id);
        this.ctx.storage.sql.exec("DELETE FROM scouts WHERE id = ?", s.id);
        changed = true;
      }
    }

    // P8-T4: المواقع المقدسة — انتهاء الحيازة (4h) يحرر الموقع + دورة المعبد/الملك (8h)
    const hold = holdDurationMs();
    const kingHold = holdForKingMs();
    for (const site of [...this.holySites.values()]) {
      if (site.heldSinceMs == null || site.ownerAllianceId == null) continue;
      // انتهاء الحيازة يحرر الموقع (لا ينطبق على المعبد — مملوكيته دائمة ما دام مضمونًا)
      if (site.kind !== "temple" && now - site.heldSinceMs >= hold) {
        site.ownerAllianceId = null;
        site.captureProgress = 0;
        site.state = "open";
        site.heldSinceMs = null;
        this.persistHolySite(site);
        this.broadcast({ type: "holy_site_changed", site });
        changed = true;
      }
    }
    // P8-T4: من يحتفظ المعبد 8 ساعات متواصلة يتوَّج ملك المملكة
    const temple = this.holySites.get(HOLY_SITES.temple.id);
    if (temple?.ownerAllianceId && temple.heldSinceMs != null) {
      if (!this.king && now - temple.heldSinceMs >= kingHold) {
        this.king = { allianceId: temple.ownerAllianceId, crownedAtMs: temple.heldSinceMs, expiresAtMs: null };
        this.recordSeasonStory({
          kind: "king_crowned",
          subjectId: temple.ownerAllianceId,
          allianceId: temple.ownerAllianceId,
        }, now);
        changed = true;
      } else if (this.king && this.king.allianceId !== temple.ownerAllianceId) {
        // المعبد انتقل لتحالف آخر — يفقد الملك السابق لقبه
        this.king = null;
        changed = true;
      }
    } else if (this.king) {
      // المعبد بلا مالك — يُزال اللقب
      this.king = null;
      changed = true;
    }

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
        // P8-T6: تقدم مهمة التدريب اليومي/الأسبوعي (مجموع الجنود المدربين في الطابور)
        const trainedCount = Object.values(q.data.troops || {}).reduce((s: number, c) => s + Number(c), 0);
        if (trainedCount > 0) {
          try { void this.recordQuestProgress(q.playerId, "train", trainedCount); } catch {}
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
        // P8-T6: تقدم مهمة البحث اليومي/الأسبوعي
        try { void this.recordQuestProgress(q.playerId, "research_start", 1); } catch {}
      } else if (q.type === "build") {
        // P8-T6: تقدم مهمة تطوير المباني اليومي/الأسبوعي
        try { void this.recordQuestProgress(q.playerId, "build_upgrade", 1); } catch {}
      }
    }

    for (const m of this.marches.values()) {
      if (m.state === "moving") {
        if (now >= m.etaMs) {
          await this.resolveMarchArrival(m, now, tickInDay);
          changed = true;
        }
      } else if (m.state === "gathering") {
        if (now >= m.etaMs) {
          // P9-T2: جمع من مركز مورد — داخل النطاق يحصل على باف +25% ويُقفل المركز عليه
          const center = m.targetType === "center" ? this.resourceCenters.get(m.targetId) : undefined;
          if (center) {
            const troopsCount = totalTroops(m.troops);
            let gathered = centerGatherAmount(center, troopsCount).amount;
            if (insideTerritory(center.x, center.y, this.castleList(), m.allianceId || null)) {
              gathered = Math.floor(gathered * gatherMultiplier());
            }
            // P9-T3: باف جمع لحامل لقب التحالف (سلطوي — playerId فقط)
            const gatherTitleMod = this.marchTitleMod(m.ownerPlayerId, m.allianceId, "gather");
            if (gatherTitleMod > 0) {
              gathered = Math.floor(gathered * (1 + gatherTitleMod));
            }
            // P9-T4: باف جمع VIP (حتى VIP 15: +30%) — من ذاكرة الشارد المحدّثة من الراوتر
            const centerVipMod = this.vipGatherMod(m.ownerPlayerId);
            if (centerVipMod > 0) gathered = Math.floor(gathered * (1 + centerVipMod));
            center.reserve = Math.max(0, center.reserve - gathered);
            if (m.allianceId && !center.lockedAllianceId) {
              const locked = lockCenter(center, m.allianceId, now);
              this.resourceCenters.set(locked.id, locked);
              this.persistCenter(locked);
            }
            this.resourceCenters.set(center.id, center);
            this.persistCenter(center);
            m.payload = { kind: centerResource(center.kind), amount: gathered, centerId: center.id };
            try { void this.recordQuestProgress(m.ownerPlayerId, "gather", gathered); } catch {}
          } else {
            const node = this.nodes.get(m.targetId);
            if (node) {
              // P3-T3: باف اندفاع الموارد — عقد أغنى أثناء الحدث
              const richMult = eventBuff(this.seasonDay, tickInDay, "resource_richness_mult");
              let gathered = Math.floor(node.remaining * richMult);
              // P9-T2: باف جمع +25% داخل الأرض الإقليمية للتحالف
              if (m.allianceId && insideTerritory(node.x, node.y, this.castleList(), m.allianceId)) {
                gathered = Math.floor(gathered * gatherMultiplier());
              }
              // P9-T3: باف جمع لحامل لقب التحالف
              const nodeGatherTitleMod = this.marchTitleMod(m.ownerPlayerId, m.allianceId, "gather");
              if (nodeGatherTitleMod > 0) {
                gathered = Math.floor(gathered * (1 + nodeGatherTitleMod));
              }
              // P9-T4: باف جمع VIP (حتى VIP 15: +30%) — من ذاكرة الشارد المحدّثة من الراوتر
              const nodeVipMod = this.vipGatherMod(m.ownerPlayerId);
              if (nodeVipMod > 0) gathered = Math.floor(gathered * (1 + nodeVipMod));
              node.remaining = 0;
              this.persistNode(node);
              m.payload = { kind: node.kind, amount: gathered };
              // P8-T6: تقدم مهمة الجمع اليومي/الأسبوعي
              try { void this.recordQuestProgress(m.ownerPlayerId, "gather", gathered); } catch {}
              // نقاط الجمع أثناء اندفاع الموارد
              const gatherScore = eventBuff(this.seasonDay, tickInDay, "gather_score", true);
              if (gatherScore > 0 && m.allianceId) {
                const pts = gatherScore * node.level;
                const cur = this.throneScores.get(m.allianceId) || 0;
                this.throneScores.set(m.allianceId, cur + pts);
                this.persistThroneScore(m.allianceId, cur + pts);
              }
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

    // P8-T5: تجديد نقاط العمل (AP) لكل مدينة كل 45s + انتهاء الدرع وحمى الحرب
    for (const city of this.cities.values()) {
      const { ap, lastRegenMs } = regenAp(city.ap, city.lastApMs, now);
      if (ap !== city.ap) {
        city.ap = ap;
        city.lastApMs = lastRegenMs;
        this.persistCity(city);
        this.broadcast({ type: "city_upsert", city });
        changed = true;
      }
      if (city.shieldUntilMs != null && now >= city.shieldUntilMs) {
        city.shieldUntilMs = null;
        this.persistCity(city);
        this.broadcast({ type: "city_upsert", city });
        changed = true;
      }
      if (city.warFrenzyUntilMs != null && now >= city.warFrenzyUntilMs) {
        city.warFrenzyUntilMs = null;
        this.persistCity(city);
        changed = true;
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
        this.recordSeasonStory({
          kind: "region_unlocked",
          subjectId: r.id,
          allianceId: null,
        }, now);

      changed = true;
    }

    // P9-T2: إعادة تعبئة مراكز الموارد المستنفدة بعد انتهاء فترة respawn
    const refilled = respawnDueCenters([...this.resourceCenters.values()], this.seasonDay, now);
    for (const c of refilled) {
      this.resourceCenters.set(c.id, c);
      this.persistCenter(c);
    }
    if (refilled.length > 0) changed = true;

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

    // P6-T10: تتويج واحد، حتمي، بعد انتهاء حساب نقاط اليوم الأخير.
    if (this.seasonDay >= SEASON_SERVICE.maxDay && this.throneScores.size > 0) {
      const [winnerAllianceId, winningScore] = [...this.throneScores.entries()]
        .sort(([leftId, leftScore], [rightId, rightScore]) => rightScore - leftScore || leftId.localeCompare(rightId))[0];
      if (this.recordSeasonStory({
        kind: "season_champion",
        subjectId: `season:${SEASON_SERVICE.maxDay}`,
        allianceId: winnerAllianceId,
        score: winningScore,
      }, now)) {
        changed = true;
      }
    }

    this.ctx.storage.sql.exec(
      `INSERT INTO world_meta (id, season_day, last_tick_ms, season_start_ms)
       VALUES (1, ?, ?, ?)
       ON CONFLICT(id) DO UPDATE SET
         season_day=excluded.season_day,
         last_tick_ms=excluded.last_tick_ms`,
      this.seasonDay,
      now,
      this.seasonStartMs,
    );

    // P7-T15: تسجيل زمن آخر tick لمؤشرات التشغيل (نافذة ساعة منزلقة تُحسب عند الطلب).
    this.lastTickMs = now;

    if (changed) {
      // لا تعاد اللقطة الكاملة مع كل tick: الاتصال الأول فقط يستلم snapshot.
      this.broadcast({ type: "world_delta", ...this.worldDelta() });
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

  private async resolveMarchArrival(m: MarchEntity, now: number, tickInDay: number) {
    // حدث وصول منفصل عن نتيجة القتال: يصل للعميل قبل أن تتحول المسيرة إلى قتال أو عودة.
    this.broadcast({ type: "march_arrived", march: m });
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

      // تحفظ الملكية السابقة لقصة الموسم فقط؛ لا تؤثر في حساب الحامية أو نتيجة القتال.
      const previousPassOwnerAllianceId = pass.ownerAllianceId;
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
      // P8-T1: باف troop_attack من مواهب القائد المرافق للمسيرة
      const attackerTalentAttackMod = talentAttackMod(attackerCommander?.talentAllocations);
      // P8-T2: باف troop_attack من معدات القائد (قطع مجهزة × جودة × set bonus 2/4/6)
      const attackerEquipmentMod = equipmentAttackMod(attackerCommander?.equipmentState);
      // P9-T1: باف تقنيات التحالف (هجوم/دفاع/HP + حصار/ممرات) — بحث جماعي نشط
      const allianceTechMod = this.marchAllianceTechAttackMod(m.allianceId) + this.marchAllianceTechSiegeMod(m.allianceId);
      // P9-T1: باف تقنيات التحالف يُطبَّق على قوات المهاجم قبل الحساب (تضخيم القوة)
      // P9-T2: الممر داخل أرض التحالف المدافع عنها يخفَّف على قوات المهاجم (دورية حامية)
      const passPatrolMod = patrolMod(
        !!m.allianceId && !!pass.ownerAllianceId && pass.ownerAllianceId !== m.allianceId
          && marchCrossesTerritory(m.fromX, m.fromY, m.toX, m.toY, this.castleList(), pass.ownerAllianceId),
      );
      // P9-T3: بافات ألقاب التحالف للمسير صاحب اللقب (سلطوي — playerId فقط)
      const passTitleMod = this.marchTitleMod(m.ownerPlayerId, m.allianceId, "attack");
      const passAugmentedTroops = scaleTroops(m.troops, (1 + allianceTechMod + passTitleMod) * passPatrolMod);

      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: passAugmentedTroops },
        { name: pass.ownerAllianceId || "neutral_guard", troops: defenderTroops },
        1,
        attackerCommander,
        undefined,
        attackerResearchMod,
        0,
        attackerTalentAttackMod,
        0,
        attackerEquipmentMod,
        0,
        this.cities.get(m.ownerPlayerId)?.civ || undefined,
      );

      const report: {
        id: string;
        createdAt: number;
        kind: string;
        passId: string;
        attackerPlayerId: string;
        attackerAllianceId: string | null;
        defenderAllianceId: string | null;
        result: CombatResult;
        hospital?: HospitalSummary;
        rally?: any;
        rewards?: Array<{ kind: string; amount: number }>;
      } = {
        id: newId("br"),
        createdAt: now,
        kind: "pass_attack",
        passId: pass.id,
        attackerPlayerId: m.ownerPlayerId,
        attackerAllianceId: m.allianceId,
        defenderAllianceId: previousPassOwnerAllianceId,
        result,
      };
      await this.grantCommanderXp(m.id, totalTroops(result.defenderLosses));
      const settlement = await this.settleAttackerCombat(m, result);
      if ("rally" in settlement) report.rally = settlement.rally;
      else report.hospital = settlement.hospital;

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
        this.broadcast({ type: "pass_owner_changed", pass });
        if (m.allianceId && pass.ownerAllianceId === m.allianceId && previousPassOwnerAllianceId !== m.allianceId) {
          this.recordSeasonStory({
            kind: previousPassOwnerAllianceId ? "pass_conquered" : "first_pass_capture",
            subjectId: previousPassOwnerAllianceId
              ? `${pass.id}:${previousPassOwnerAllianceId}:${m.allianceId}`
              : pass.id,
            allianceId: m.allianceId,
            previousAllianceId: previousPassOwnerAllianceId,
          }, now);
        }
      }
      this.saveReport(report);
      this.broadcastReport(report);

      m.troops = result.attackerRemaining;
      this.spawnReturnMarch(m, now);
      return;
    }

    if (m.targetType === "throne") {
      if (this.throne.unlockDay > this.seasonDay) {
        this.spawnReturnMarch(m, now);
        return;
      }
      // لقصة الموسم فقط: تؤخذ الملكية السابقة قبل تسوية القتال السلطوية.
      const previousThroneOwnerAllianceId = this.throne.ownerAllianceId;
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
      // P8-T1: باف troop_attack من مواهب القائد المرافق للمسيرة
      const throneTalentAttackMod = talentAttackMod(throneAttackerCommander?.talentAllocations);
      // P8-T2: باف troop_attack من معدات القائد
      const throneEquipmentMod = equipmentAttackMod(throneAttackerCommander?.equipmentState);
      // P9-T1: باف تقنيات التحالف (هجوم/دفاع/HP + حصار/ممرات) — بحث جماعي نشط
      const throneAllianceTechMod = this.marchAllianceTechAttackMod(m.allianceId) + this.marchAllianceTechSiegeMod(m.allianceId);
      // P9-T1: باف تقنيات التحالف يُطبَّق على قوات المهاجم قبل الحساب (تضخيم القوة)
      // P9-T2: العرش داخل أرض التحالف المدافع يخفَّف على قوات المهاجم (دورية حامية)
      const thronePatrolMod = patrolMod(
        !!m.allianceId && !!this.throne.ownerAllianceId && this.throne.ownerAllianceId !== m.allianceId
          && marchCrossesTerritory(m.fromX, m.fromY, m.toX, m.toY, this.castleList(), this.throne.ownerAllianceId),
      );
      // P9-T3: بافات ألقاب التحالف للمسير صاحب اللقب
      const throneTitleMod = this.marchTitleMod(m.ownerPlayerId, m.allianceId, "attack");
      const throneAugmentedTroops = scaleTroops(m.troops, (1 + throneAllianceTechMod + throneTitleMod) * thronePatrolMod);

      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: throneAugmentedTroops },
        { name: this.throne.ownerAllianceId || "neutral_guard", troops: defenderTroops },
        3,
        throneAttackerCommander,
        undefined,
        throneResearchMod,
        0,
        throneTalentAttackMod,
        0,
        throneEquipmentMod,
        0,
        this.cities.get(m.ownerPlayerId)?.civ || undefined,
      );

      const report: {
        id: string;
        createdAt: number;
        kind: string;
        attackerPlayerId: string;
        attackerAllianceId: string | null;
        defenderAllianceId: string | null;
        result: CombatResult;
        hospital?: HospitalSummary;
        rally?: any;
        rewards?: Array<{ kind: string; amount: number }>;
      } = {
        id: newId("br"),
        createdAt: now,
        kind: "throne_attack",
        attackerPlayerId: m.ownerPlayerId,
        attackerAllianceId: m.allianceId,
        defenderAllianceId: previousThroneOwnerAllianceId,
        result,
      };
      await this.grantCommanderXp(m.id, totalTroops(result.defenderLosses));
      const settlement = await this.settleAttackerCombat(m, result);
      if ("rally" in settlement) report.rally = settlement.rally;
      else report.hospital = settlement.hospital;

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
        if (m.allianceId && this.throne.ownerAllianceId === m.allianceId && previousThroneOwnerAllianceId !== m.allianceId) {
          this.recordSeasonStory({
            kind: "throne_captured",
            subjectId: previousThroneOwnerAllianceId
              ? `throne:${previousThroneOwnerAllianceId}:${m.allianceId}`
              : "throne",
            allianceId: m.allianceId,
            previousAllianceId: previousThroneOwnerAllianceId,
          }, now);
        }
      }
      this.saveReport(report);
      this.broadcastReport(report);

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

      const previousObjectiveOwnerAllianceId = obj.ownerAllianceId;
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
      // P8-T1: باف troop_attack من مواهب القائد المرافق للمسيرة
      const coTalentAttackMod = talentAttackMod(coCommander?.talentAllocations);
      // P8-T2: باف troop_attack من معدات القائد
      const coEquipmentMod = equipmentAttackMod(coCommander?.equipmentState);
      // P9-T1: باف تقنيات التحالف (هجوم/دفاع/HP + حصار/ممرات) — بحث جماعي نشط
      const coAllianceTechMod = this.marchAllianceTechAttackMod(m.allianceId) + this.marchAllianceTechSiegeMod(m.allianceId);
      // P9-T1: باف تقنيات التحالف يُطبَّق على قوات المهاجم قبل الحساب (تضخيم القوة)
      // P9-T2: الهدف داخل أرض التحالف المدافع يخفَّف على قوات المهاجم (دورية حامية)
      const coPatrolMod = patrolMod(
        !!m.allianceId && !!obj.ownerAllianceId && obj.ownerAllianceId !== m.allianceId
          && marchCrossesTerritory(m.fromX, m.fromY, m.toX, m.toY, this.castleList(), obj.ownerAllianceId),
      );
      // P9-T3: بافات ألقاب التحالف للمسير صاحب اللقب
      const coTitleMod = this.marchTitleMod(m.ownerPlayerId, m.allianceId, "attack");
      const coAugmentedTroops = scaleTroops(m.troops, (1 + coAllianceTechMod + coTitleMod) * coPatrolMod);
      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: coAugmentedTroops },
        { name: obj.ownerAllianceId || "neutral_guard", troops: defenderTroops },
        2,
        coCommander,
        undefined,
        coResearchMod,
        0,
        coTalentAttackMod,
        0,
        coEquipmentMod,
        0,
        this.cities.get(m.ownerPlayerId)?.civ || undefined,
      );

      const report: {
        id: string;
        createdAt: number;
        kind: string;
        objectiveId: string;
        attackerPlayerId: string;
        attackerAllianceId: string | null;
        defenderAllianceId: string | null;
        result: CombatResult;
        hospital?: HospitalSummary;
        rally?: any;
        firstCaptureBonus?: number;
        rewards?: Array<{ kind: string; amount: number }>;
      } = {
        id: newId("br"),
        createdAt: now,
        kind: `core_${obj.kind}`,
        objectiveId: obj.id,
        attackerPlayerId: m.ownerPlayerId,
        attackerAllianceId: m.allianceId,
        defenderAllianceId: previousObjectiveOwnerAllianceId,
        result,
      };
      await this.grantCommanderXp(m.id, totalTroops(result.defenderLosses));
      const settlement = await this.settleAttackerCombat(m, result);
      if ("rally" in settlement) report.rally = settlement.rally;
      else report.hospital = settlement.hospital;

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
            report.rewards = [{ kind: "season_points", amount: bonus }];
          }
          obj.ownerAllianceId = m.allianceId;
          obj.captureProgress = 100;
          obj.state = "open";
        }
        this.persistCoreObjective(obj);
        this.broadcast({ type: "core_objective_changed", objective: obj });
      }
      this.saveReport(report);
      this.broadcastReport(report);

      m.troops = result.attackerRemaining;
      this.spawnReturnMarch(m, now);
      return;
    }

    // P8-T4: احتلال موقع مقدس (Sanctum/Altar/Shrine + المعبد المفقود)
    if (m.targetType === "holy_site") {
      const site = this.holySites.get(m.targetId);
      if (!site) {
        this.spawnReturnMarch(m, now);
        return;
      }
      if (site.kind === "temple" && !templeUnlocked(this.seasonDay)) {
        this.spawnReturnMarch(m, now);
        return;
      }
      const previousOwner = site.ownerAllianceId;
      const guard = site.kind === "temple" ? templeGuardTroops() : siteGuardTroops(site.kind).troops;
      const defenderTroops: Troops = guard;
      // تحالف يعزّز موقعه: اكتمال فوري
      if (site.ownerAllianceId && m.allianceId && site.ownerAllianceId === m.allianceId) {
        site.captureProgress = 100;
        site.state = "open";
        this.persistHolySite(site);
        this.spawnReturnMarch(m, now);
        this.broadcast({ type: "holy_site_changed", site });
        return;
      }
      const hsCommander = await this.fetchMarchCommander(m.id);
      const hsResearchMod = await this.fetchResearchAttackMod(m.ownerPlayerId);
      const hsTalentAttackMod = talentAttackMod(hsCommander?.talentAllocations);
      const hsEquipmentMod = equipmentAttackMod(hsCommander?.equipmentState);
      // P9-T1: باف تقنيات التحالف (هجوم/دفاع/HP + حصار/ممرات) — بحث جماعي نشط
      const hsAllianceTechMod = this.marchAllianceTechAttackMod(m.allianceId) + this.marchAllianceTechSiegeMod(m.allianceId);
      // P9-T1: باف تقنيات التحالف يُطبَّق على قوات المهاجم قبل الحساب (تضخيم القوة)
      // P9-T2: الموقع المقدس داخل أرض التحالف المدافع يخفَّف على قوات المهاجم (دورية حامية)
      const hsPatrolMod = patrolMod(
        !!m.allianceId && !!site.ownerAllianceId && site.ownerAllianceId !== m.allianceId
          && marchCrossesTerritory(m.fromX, m.fromY, m.toX, m.toY, this.castleList(), site.ownerAllianceId),
      );
      // P9-T3: بافات ألقاب التحالف للمسير صاحب اللقب
      const hsTitleMod = this.marchTitleMod(m.ownerPlayerId, m.allianceId, "attack");
      const hsAugmentedTroops = scaleTroops(m.troops, (1 + hsAllianceTechMod + hsTitleMod) * hsPatrolMod);
      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: hsAugmentedTroops },
        { name: site.ownerAllianceId || "holy_guard", troops: defenderTroops },
        site.kind === "temple" ? 3 : 2,
        hsCommander,
        undefined,
        hsResearchMod,
        0,
        hsTalentAttackMod,
        0,
        hsEquipmentMod,
        0,
        this.cities.get(m.ownerPlayerId)?.civ || undefined,
      );
      const hsReport: any = {
        id: newId("br"),
        createdAt: now,
        kind: `holy_${site.kind}`,
        siteId: site.id,
        attackerPlayerId: m.ownerPlayerId,
        attackerAllianceId: m.allianceId,
        defenderAllianceId: previousOwner,
        result,
      };
      await this.grantCommanderXp(m.id, totalTroops(result.defenderLosses));
      // P8-T4: جرحى المعبد الخطيرون — 50% منهم يموتون فوق قاعدة المستشفى
      if (site.kind === "temple") {
        this.applyTempleSevereDeath(result);
      }
      const settlement = await this.settleAttackerCombat(m, result);
      if ("rally" in settlement) hsReport.rally = settlement.rally;
      else hsReport.hospital = settlement.hospital;
      if (result.winner === "attacker") {
        const gain = siteCaptureGain(troopPower(result.attackerRemaining));
        if (site.ownerAllianceId && site.ownerAllianceId !== m.allianceId) {
          site.captureProgress = gain; // احتلال عدو: يبدأ من جديد
        } else {
          site.captureProgress = Math.min(100, site.captureProgress + gain);
        }
        site.state = "contested";
        if (site.captureProgress >= 100) {
          site.ownerAllianceId = m.allianceId;
          site.captureProgress = 100;
          site.state = "captured";
          site.heldSinceMs = now;
          const storyKind = site.kind === "temple" ? "temple_captured" : "holy_site_captured";
          this.recordSeasonStory({
            kind: storyKind,
            subjectId: `${site.id}:${previousOwner || "neutral"}:${m.allianceId || "unclaimed"}`,
            allianceId: m.allianceId,
            previousAllianceId: previousOwner,
          }, now);
        }
        this.persistHolySite(site);
        this.broadcast({ type: "holy_site_changed", site });
      }
      this.saveReport(hsReport);
      this.broadcastReport(hsReport);
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
          // P8-T1: باف troop_attack من مواهب القائد المرافق للمسيرة
          const barbTalentAttackMod = talentAttackMod(barbCommander?.talentAllocations);
          // P8-T2: باف troop_attack من معدات القائد
          const barbEquipmentMod = equipmentAttackMod(barbCommander?.equipmentState);
          // P9-T1: باف تقنيات التحالف (هجوم/دفاع/HP) — بحث جماعي نشط
          const barbAllianceTechMod = this.marchAllianceTechAttackMod(m.allianceId);
          // P9-T2: مستعمرات البرابرة داخل أرض التحالف تخفَّف على قوات المهاجم (دورية حامية)
          const barbPatrolMod = patrolMod(marchCrossesTerritory(m.fromX, m.fromY, m.toX, m.toY, this.castleList(), m.allianceId));
          const barbAugmentedTroops = scaleTroops(m.troops, (1 + barbAllianceTechMod) * barbPatrolMod);
          const result = resolveCombat({ name: m.ownerPlayerId, troops: barbAugmentedTroops }, { name: "barb", troops: def }, 1, barbCommander, undefined, barbResearchMod, 0, barbTalentAttackMod, 0, barbEquipmentMod, 0, this.cities.get(m.ownerPlayerId)?.civ || undefined);
          const report: {
            id: string;
            createdAt: number;
            kind: string;
            nodeId: string;
            attackerPlayerId: string;
            result: CombatResult;
        hospital?: HospitalSummary;
        rally?: any;
        barbKillScore?: number;
        rewards?: Array<{ kind: string; amount: number }>;
      } = {
            id: newId("br"),
            createdAt: now,
            kind: "barb",
            nodeId: node.id,
            attackerPlayerId: m.ownerPlayerId,
            result,
          };
          await this.grantCommanderXp(m.id, totalTroops(result.defenderLosses));
          const settlement = await this.settleAttackerCombat(m, result);
          if ("rally" in settlement) report.rally = settlement.rally;
          else report.hospital = settlement.hospital;
          m.troops = result.attackerRemaining;
          if (result.winner === "attacker") {
            // P3-T3: نقاط قتل البرابرة أثناء حدث غزو البرابرة
            const killScore = eventBuff(this.seasonDay, tickInDay, "barb_kill_score", true);
            if (killScore > 0 && m.allianceId) {
              const pts = killScore * node.level;
              const cur = this.throneScores.get(m.allianceId) || 0;
              this.throneScores.set(m.allianceId, cur + pts);
              this.persistThroneScore(m.allianceId, cur + pts);
              report.barbKillScore = pts;
              report.rewards = [{ kind: "barbarian_event_points", amount: pts }];
            }
            node.remaining = Math.max(0, node.remaining - 50);
            this.persistNode(node);
            // P8-T6: تقدم مهمة قتل البرابرة اليومي/الأسبوعي (مجموع وحدات العدو المقتولة)
            try { void this.recordQuestProgress(m.ownerPlayerId, "barb_kill", totalTroops(result.defenderLosses)); } catch {}
          }
          this.saveReport(report);
          this.broadcastReport(report);
          this.spawnReturnMarch(m, now);
        } else {
          m.state = "gathering";
          // P3-T3: باف اندفاع الموارد — جمع أسرع أثناء الحدث
          const gatherMult = eventBuff(this.seasonDay, tickInDay, "gather_rate_mult");
          // P9-T2: باف جمع +25% داخل الأرض الإقليمية للتحالف
          const territoryMult = m.allianceId && insideTerritory(node.x, node.y, this.castleList(), m.allianceId) ? gatherMultiplier() : 1;
          const rate = 0.5 * totalTroops(m.troops) * gatherMult * territoryMult; // units/sec
          const durationSec = node.remaining / rate;
          m.etaMs = now + durationSec * 1000;
          this.persistMarch(m);
        }
      } else {
        this.spawnReturnMarch(m, now);
      }
      return;
    }

    // P9-T2: مراكز الموارد — حصينة لا تُهاجَم؛ تحالف عضويته صحيح فقط يجمع منها.
    if (m.targetType === "center") {
      const center = this.resourceCenters.get(m.targetId);
      if (!center) {
        this.spawnReturnMarch(m, now);
        return;
      }
      // المركز المقفول لتحالف آخر لا يجمع منه المهاجم: يعاد فورًا.
      if (center.lockedAllianceId && center.lockedAllianceId !== m.allianceId) {
        this.spawnReturnMarch(m, now);
        return;
      }
      m.state = "gathering";
      const troopsCount = totalTroops(m.troops);
      const perTick = Math.max(1, Math.floor(troopsCount / 100));
      // P9-T3: باف جمع لحامل لقب التحالف يُضاف إلى باف +25% الأرضي
      const gatherRateTitleMod = this.marchTitleMod(m.ownerPlayerId, m.allianceId, "gather");
      const ratePerSec = perTick * gatherMultiplier() * (1 + gatherRateTitleMod); // باف +25% دائم داخل النطاق (المركز داخل نطاق أرض التحالف)
      const durationSec = Math.max(60, Math.min(center.reserve, troopsCount * 6) / ratePerSec);
      m.etaMs = now + durationSec * 1000;
      this.persistMarch(m);
      this.broadcast({ type: "march_moving", march: m });
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

    // مسيرة الرالي تعيد الناجين إلى أصحاب المساهمات، لا إلى القائد وحده.
    const rallyParticipants = Array.isArray(m.payload?.rallyParticipants) ? m.payload.rallyParticipants : [];
    const returnEntries = rallyParticipants.length > 0
      ? rallyParticipants.map((entry: any) => ({
          playerId: String(entry.playerId || ""),
          troops: (entry.remaining || entry.committed || entry.troops || {}) as Troops,
        }))
      : [{ playerId: m.ownerPlayerId, troops: m.troops }];
    for (const entry of returnEntries) {
      if (!entry.playerId) continue;
      for (const [u, count] of Object.entries(entry.troops)) {
        if (Number(count) <= 0) continue;
        await this.env.DB.prepare(
          `UPDATE troops SET count=MAX(0, count-?) WHERE player_id=? AND unit_id=? AND status='marching'`,
        ).bind(Number(count), entry.playerId, u).run();
        await this.env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`,
        ).bind(entry.playerId, u, Number(count)).run();
      }
    }

    // Add gathered resources (يشمل موارد المراكز — payload من جمع المراكز يحمل {kind, amount, centerId})
    if (m.payload?.amount) {
      const kind = m.payload.kind;
      if (kind === "food" || kind === "wood" || kind === "stone" || kind === "gold") {
        await this.env.DB.prepare(
          `UPDATE cities SET ${kind}=${kind}+? WHERE player_id=?`
        ).bind(m.payload.amount, m.ownerPlayerId).run();
        this.broadcast({ type: "resources_gathered", playerId: m.ownerPlayerId, kind, amount: m.payload.amount });
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
    // P9-T4: مزامنة مستوى VIP السلطوي من الراوتر (D1) عبر header موثوق داخليًا —
    // header عديم القيمة إذا لم يحمله الراوتر الموقّع؛ لا يُقبل من مصادر خارجية
    // إلا مع x-rok2-player موقّع (حارس requireAuthenticatedPlayer أدناه).
    this.syncVipLevel(request);
    if (request.headers.get("Upgrade") === "websocket") {
      return this.handleWs(request);
    }

    // P8-T5: قراءة حالة AP/الدرع/حمى الحرب/آخر تهجير لمدينة واحدة (سريعة، لا تُدخل في اللقطات)
    if (path.endsWith("/ap-state") && request.method === "GET") {
      const q = new URL(request.url).searchParams;
      const pid = q.get("playerId") || "";
      const c = this.cities.get(pid);
      if (!c) return Response.json({ ok: false, error: "city_not_on_map" }, { status: 404 });
      // P8-T5: تجديد لحظي قبل القراءة حتى لا يُعرض رصيد قديم
      const { ap, lastRegenMs } = regenAp(c.ap, c.lastApMs, nowMs());
      return Response.json({
        ok: true,
        ap,
        lastApMs: lastRegenMs,
        apCap: apCap(),
        regenIntervalMs: AP_INTERVAL_MS,
        shieldUntilMs: c.shieldUntilMs ?? null,
        warFrenzyUntilMs: c.warFrenzyUntilMs ?? null,
        lastRelocationMs: c.lastRelocationMs ?? null,
      });
    }

    // P8-T6: حالة المهام اليومية/الأسبوعية للاعب — توزيع حتمي + تقدم حي من الطوابير
    if (path.endsWith("/quests/state") && request.method === "GET") {
      const pid = url.searchParams.get("playerId") || "";
      const state = await this.questsStateFor(pid);
      return Response.json(state);
    }

    // P8-T6: تسجيل تقدم مهمة من shard نفسه (train/battle_win/barb_kill/gather)
    if (path.endsWith("/quests/progress") && request.method === "POST") {
      try {
        const body = await request.json<any>();
        const pid = String(body.playerId || "");
        const source = String(body.source || "");
        const amount = Number(body.amount) || 0;
        if (!pid || !source || amount <= 0) return Response.json({ ok: false, error: "bad_quest_progress_args" }, { status: 400 });
        const result = await this.recordQuestProgress(pid, source, amount);
        return Response.json({ ok: true, ...result });
      } catch {
        return Response.json({ ok: false, error: "quest_progress_failed" }, { status: 500 });
      }
    }

    if (path.endsWith("/snapshot") && request.method === "GET") {
      const playerId = request.headers.get("x-rok2-player") || "";
      return Response.json(this.snapshot(playerId));
    }

    // التقارير تصل من router مصادقاً عليه؛ لا نثق في هوية جسم طلب من عميل مباشر.
    if (path.endsWith("/reports") && request.method === "GET") {
      const playerId = request.headers.get("x-rok2-player") || "";
      const allianceId = request.headers.get("x-rok2-alliance") || this.cities.get(playerId)?.allianceId || null;
      if (!playerId) return Response.json({ error: "auth_required" }, { status: 401 });
      return Response.json({ reports: this.visibleReportsFor(playerId, allianceId) });
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
            march.payload = {
              rallyId: r.id,
              participantIds: list.map((p) => p.player_id),
              // لقطة مساهمات سلطوية تُستخدم لاحقاً لتوزيع الخسائر والناجين وتقرير الرالي.
              rallyParticipants: list.map((p) => ({
                playerId: p.player_id,
                committed: JSON.parse(p.troops_json || "{}") as Troops,
              })),
            };
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
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const c: CityEntity = {
        playerId: body.playerId,
        name: body.name,
        allianceId: body.allianceId ?? null,
        civ: body.civ || "",
        x: body.x,
        y: body.y,
        hallLevel: body.hallLevel ?? 1,
        regionId: body.regionId,
        ap: body.ap ?? 1000,
        lastApMs: body.lastApMs ?? Date.now(),
        shieldUntilMs: body.shieldUntilMs ?? null,
        warFrenzyUntilMs: body.warFrenzyUntilMs ?? null,
        lastRelocationMs: body.lastRelocationMs ?? null,
      };
      this.cities.set(c.playerId, c);
      this.persistCity(c);
      this.broadcast({ type: "city_upsert", city: c });
      this.ensureAlarm();
      return Response.json({ ok: true, city: c });
    }

    // P8-T5: نقل مدينة داخل الشارد — تحديث الإحداثيات والمنطقة وآخر تهجير
    if (path.endsWith("/relocate") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const c = this.cities.get(body.playerId);
      if (!c) return Response.json({ ok: false, error: "city_not_on_map" }, { status: 404 });
      const nowRel = nowMs();
      // حماية حمى الحرب: لا يمكن التهجير الموجه خلال ساعة من آخر هجوم متلقَّى
      const relocationKind = body.kind || "random";
      if (relocationKind === "targeted" && c.warFrenzyUntilMs != null && c.warFrenzyUntilMs > nowRel) {
        return Response.json({ ok: false, error: "relocation_war_frenzy" }, { status: 400 });
      }
      c.x = Number(body.x);
      c.y = Number(body.y);
      c.regionId = body.regionId ?? this.regionOf(c.x, c.y);
      c.lastRelocationMs = body.lastRelocationMs ?? nowRel;
      this.persistCity(c);
      this.broadcast({ type: "city_upsert", city: c });
      return Response.json({ ok: true, city: c });
    }

    // P8-T5: تنشيط درع الحماية من داخل الشارد (تُستدعى من الـ router بعد خصم الجواهر في D1)
    if (path.endsWith("/activate-shield") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const c = this.cities.get(body.playerId);
      if (!c) return Response.json({ ok: false, error: "city_not_on_map" }, { status: 404 });
      const check = canActivateShield(c.warFrenzyUntilMs ?? null, c.shieldUntilMs ?? null, nowMs());
      if (!check.ok) return Response.json({ ok: false, error: `shield_${check.reason ?? "not_allowed"}` }, { status: 400 });
      c.shieldUntilMs = (c.shieldUntilMs ?? nowMs()) + body.durationMs;
      this.persistCity(c);
      this.broadcast({ type: "city_upsert", city: c });
      return Response.json({ ok: true, shieldUntilMs: c.shieldUntilMs });
    }

    if (path.endsWith("/set-alliance") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const c = this.cities.get(body.playerId);
      if (!c) { this.recordCommandError("city_not_found"); return Response.json({ error: "city_not_found" }, { status: 404 }); }
      c.allianceId = body.allianceId;
      // P9-T2: اسم التحالف ورتبة اللاعب فيه — يحددهما الراوتر بعد التحقق من SQL
      if (body.allianceName != null) c.allianceName = String(body.allianceName);
      if (body.rank != null) c.rank = String(body.rank);
      if (body.allianceId == null) {
        c.allianceName = undefined;
        c.rank = undefined;
        // P9-T3: عند مغادرة التحالف يُفقد اللقب الممنوح (يُسحب من قائمة ألقاب التحالف إن كان حاملًا له)
        if (c.titleId) {
          const prevAllianceId = String(body.previousAllianceId || "");
          if (prevAllianceId) {
            const shopState = this.allianceShop.get(prevAllianceId);
            if (shopState && shopState.titles[c.titleId] === body.playerId) {
              const nextShop = revokeTitle(shopState, c.titleId);
              this.allianceShop.set(prevAllianceId, nextShop);
              this.persistAllianceShop(prevAllianceId, nextShop);
              this.broadcast({ type: "alliance_title_revoked", allianceId: prevAllianceId, titleId: c.titleId, holder: body.playerId });
            }
          }
          c.titleId = undefined;
        }
      }
      this.persistCity(c);
      this.broadcast({ type: "city_upsert", city: c });
      return Response.json({ ok: true, city: c });
    }

    if (path.endsWith("/build-flag") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const builder = this.cities.get(body.playerId);
      if (!builder || !body.allianceId || builder.allianceId !== body.allianceId) {
        this.recordCommandError("not_your_alliance"); return Response.json({ error: "not_your_alliance" }, { status: 403 });
      }
      // تحقق من الإحداثيات قبل الحفظ — أعلام خارج الخريطة/NaN تفسد الرسم والمنطق
      const map = getMap();
      const x = Number(body.x);
      const y = Number(body.y);
      if (!Number.isFinite(x) || !Number.isFinite(y) || x < 0 || x > map.width || y < 0 || y > map.height) {
        this.recordCommandError("bad_flag_coords"); return Response.json({ error: "bad_flag_coords" }, { status: 400 });
      }
      const flag: AllianceFlag = {
        id: newId("flg"),
        allianceId: body.allianceId,
        x,
        y,
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

    if (path.endsWith("/build-alliance-structure") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.createdBy);
      if (identityError) return identityError;
      const builder = this.cities.get(body.createdBy);
      if (!builder || !body.allianceId || builder.allianceId !== body.allianceId) {
        this.recordCommandError("not_your_alliance"); return Response.json({ error: "not_your_alliance" }, { status: 403 });
      }
      const map = getMap();
      const x = Number(body.x);
      const y = Number(body.y);
      const allianceId = String(body.allianceId || "");
      const kind = String(body.kind || "");
      if (!allianceId || !body.createdBy) { this.recordCommandError("alliance_and_builder_required"); return Response.json({ error: "alliance_and_builder_required" }, { status: 400 }); }
      if (!Number.isFinite(x) || !Number.isFinite(y) || x < 0 || x > map.width || y < 0 || y > map.height) {
        this.recordCommandError("bad_structure_coords"); return Response.json({ error: "bad_structure_coords" }, { status: 400 });
      }

      const catalog = getAllianceStructures() as any;
      const spec = (catalog.structures || []).find((candidate: any) => candidate.id === kind);
      if (!spec) { this.recordCommandError("unknown_structure_kind"); return Response.json({ error: "unknown_structure_kind" }, { status: 400 }); }

      const allianceStructures = [...this.allianceStructures.values()].filter((s) => s.allianceId === allianceId);
      if (allianceStructures.length >= Number(catalog.placement?.max_structures_per_alliance || 0)) {
        this.recordCommandError("alliance_structure_cap_reached"); return Response.json({ error: "alliance_structure_cap_reached" }, { status: 400 });
      }
      if (allianceStructures.filter((s) => s.kind === kind).length >= Number(spec.max_per_alliance || 0)) {
        this.recordCommandError("structure_kind_cap_reached"); return Response.json({ error: "structure_kind_cap_reached" }, { status: 400 });
      }
      const spacing = Number(catalog.placement?.minimum_spacing || 0);
      if (this.allianceStructures.size && [...this.allianceStructures.values()].some((s) => dist(x, y, s.x, s.y) < spacing)) {
        this.recordCommandError("structure_too_close"); return Response.json({ error: "structure_too_close" }, { status: 400 });
      }
      if (catalog.placement?.requires_alliance_territory) {
        const insideTerritory = [...this.flags.values()].some((flag) =>
          flag.allianceId === allianceId && dist(x, y, flag.x, flag.y) <= flag.radius,
        );
        if (!insideTerritory) { this.recordCommandError("structure_requires_alliance_territory"); return Response.json({ error: "structure_requires_alliance_territory" }, { status: 400 }); }
      }

      const structure: AllianceStructure = {
        id: newId("ast"),
        kind: spec.id,
        allianceId,
        x,
        y,
        radius: Number(spec.radius || 0),
        protectionRadius: Number(spec.protection_radius || 0),
        marchDamageReduction: Number(spec.march_damage_reduction || 0),
        mapMarker: String(spec.map_marker || spec.id),
        createdBy: String(body.createdBy),
        createdAt: nowMs(),
      };
      this.allianceStructures.set(structure.id, structure);
      this.persistAllianceStructure(structure);
      this.broadcast({ type: "alliance_structure_created", structure });
      return Response.json({ ok: true, structure });
    }

    if (path.endsWith("/march") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      try {
        const march = await this.createMarch(body);
        this.ensureAlarm();
        return Response.json({ ok: true, march });
      } catch (e: any) {
        return Response.json({ error: e.message || "march_failed" }, { status: 400 });
      }
    }

    if (path.endsWith("/redirect-march") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      try {
        const march = await this.redirectMarch(body);
        return Response.json({ ok: true, march });
      } catch (e: any) {
        return Response.json({ error: e.message || "redirect_failed" }, { status: 400 });
      }
    }

    if (path.endsWith("/pass-attack") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
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

    // P5-T5: إرسال كشافة — تصل بعد زمن مسير قصير وتكشف المنطقة لدى العميل
    if (path.endsWith("/scout") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.ownerPlayerId);
      if (identityError) return identityError;
      try {
        const scout = this.createScout(body);
        this.ensureAlarm();
        return Response.json({ ok: true, scout });
      } catch (e: any) {
        return Response.json({ error: e.message || "scout_failed" }, { status: 400 });
      }
    }

    if (path.endsWith("/admin") && request.method === "POST") {
      try {
        assertAdminKey(request, this.env);
      } catch {
        return Response.json({ error: "admin_unauthorized" }, { status: 403 });
      }
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
          for (const s of this.scouts.values()) {
            if (s.state === "moving") {
              s.etaMs = now;
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

    if (path.endsWith("/queue/list") && request.method === "GET") {
      const playerId = new URL(request.url).searchParams.get("playerId") || "";
      const identityError = this.requireAuthenticatedPlayer(request, playerId);
      if (identityError) return identityError;
      const queues = [...this.queues.values()]
        .filter((queue) => queue.playerId === playerId && queue.state === "running")
        .sort((a, b) => a.etaMs - b.etaMs);
      return Response.json({ ok: true, queues });
    }

    if (path.endsWith("/queue/add") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      // قناة واحدة نشطة لكل نوع داخل المدينة: البناء والتدريب والشفاء والبحث
      // تعمل بالتوازي، لكن لا يُسمح بتكرار نوع واحد قبل اكتماله.
      // P9-T4: VIP 6+ يمتلك طابور بناء ثانٍ دائم (الراوتر مرّر المستوى عبر header موثوق).
      const queueType = String(body.type || "");
      const independentQueueTypes = new Set(["build", "train", "heal", "research"]);
      if (independentQueueTypes.has(queueType)) {
        const existingQueue = [...this.queues.values()].find(
          (queue) => queue.playerId === body.playerId && queue.type === queueType && queue.state === "running",
        );
        if (existingQueue) {
          const vipLv = this.vipLevelFor(body.playerId);
          const extraQueue = queueType === "build" && vipTierForPoints(0).extra_build_queue === false;
          // ملاحظة: فحص extra_build_queue يعتمد على مستوى الذاكرة المحدّث من الراوتر؛
          // إن كان VIP>=6 فالمستوى المحفوظ في playerVipLevels يتجاوز عتبة الطابور الثاني.
          const canHaveExtraBuildQueue = queueType === "build" && vipLv >= vipTiers()[6].level && vipTiers()[6].extra_build_queue;
          const runningQueuesOfType = [...this.queues.values()].filter(
            (queue) => queue.playerId === body.playerId && queue.type === queueType && queue.state === "running",
          ).length;
          if (!canHaveExtraBuildQueue || runningQueuesOfType >= (canHaveExtraBuildQueue ? 2 : 1)) {
            return Response.json({ error: `${queueType}_queue_busy`, queueId: existingQueue.id, type: queueType }, { status: 409 });
          }
        }
      }
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
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const q = this.queues.get(body.queueId);
      if (!q || q.state !== "running") { this.recordCommandError("queue_not_found"); return Response.json({ error: "queue_not_found" }, { status: 404 }); }

      // ملكية الطابور: بدون هذا يستطيع أي لاعب تسريع طابور لاعب آخر
      // بمجرد تخمين/معرفة المعرّف.
      if (!body.playerId || q.playerId !== body.playerId) {
        this.recordCommandError("not_your_queue"); return Response.json({ error: "not_your_queue" }, { status: 403 });
      }

      // الثواني تأتي من مصدر موثوق (عنصر في الحقيبة أو مزية VIP) لكن نتحقق
      // هنا أيضاً — هذه آخر نقطة قبل تعديل الحالة.
      const seconds = Number(body.seconds);
      if (!Number.isFinite(seconds) || seconds <= 0) {
        this.recordCommandError("invalid_seconds"); return Response.json({ error: "invalid_seconds" }, { status: 400 });
      }
      const cappedSeconds = Math.min(seconds, MAX_SPEEDUP_SECONDS);

      q.etaMs = Math.max(nowMs(), q.etaMs - cappedSeconds * 1000);
      this.persistQueue(q);
      this.ensureAlarm();
      return Response.json({ ok: true, queue: q });
    }

    // P4-T5: فحص إداري — آخر مخالفات anti-cheat المسجلة في هذه الـ shard
    if (path.endsWith("/anticheat/violations") && request.method === "GET") {
      return Response.json({
        ok: true,
        enabled: ANTICHEAT_CONSTANTS.enabled,
        violations: this.antiCheatViolations.slice().reverse(),
        trackedBuckets: this.antiCheat.size(),
      });
    }

    // P7-T15: مؤشرات التشغيل — قراءة داخلية فقط (الراوتر العام لا يعرّضها إلا تحت /v1/admin/ops مع requireAdmin).
    if (path === "/ops" && request.method === "GET") {
      const snap = this.opsSnapshot();
      return Response.json({ ok: true, enabled: OPS_CONSTANTS.enabled, ...snap, violations: this.antiCheatViolations.slice(-10).reverse() });
    }
    // P9-T1: تبرع عضو بنقطة تقنية — البحث النشط للتحالف هو المستفيد الوحيد
    if (path.endsWith("/alliance-tech-donate") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const playerId = String(body.playerId || "");
      const city = this.cities.get(playerId);
      const allianceId = city?.allianceId || String(body.allianceId || "");
      if (!city || !allianceId || city.allianceId !== allianceId) {
        this.recordCommandError("not_your_alliance"); return Response.json({ error: "not_your_alliance" }, { status: 403 });
      }
      const techId = String(body.techId || "");
      const result = this.donateAllianceTech(playerId, allianceId, techId);
      if ("error" in result) { this.recordCommandError(result.error); return Response.json({ error: result.error }, { status: result.status }); }
      return Response.json({ ok: true, techId, level: result.level, points: result.points });
    }
    // P9-T1: ضابط (R3+) يبدأ بحثًا جماعيًا — بحث نشط واحد لكل تحالف
    if (path.endsWith("/alliance-tech-start") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const playerId = String(body.playerId || "");
      const city = this.cities.get(playerId);
      const allianceId = city?.allianceId || String(body.allianceId || "");
      if (!city || !allianceId || city.allianceId !== allianceId) {
        this.recordCommandError("not_your_alliance"); return Response.json({ error: "not_your_alliance" }, { status: 403 });
      }
      const result = this.startAllianceResearch(playerId, allianceId, String(body.rank || "R1"), String(body.techId || ""));
      if ("error" in result) { this.recordCommandError(result.error); return Response.json({ error: result.error }, { status: result.status }); }
      return Response.json({ ok: true, techId: result.techId });
    }
    // P9-T1: حالة تقنيات التحالف (للبث — تحالف اللاعب إن حُدّدت، وإلا العامة)
    if (path.endsWith("/alliance-tech-state") && request.method === "GET") {
      const playerAllianceId = (() => {
        const headerId = request.headers.get("x-rok2-player") || "";
        return this.cities.get(headerId)?.allianceId ?? null;
      })();
      const techDefs = AllianceTechService.techs().map((tech) => ({
        id: tech.id,
        category: tech.category,
        name: tech.name,
        levels: tech.levels,
        levelRequired: tech.level_required,
        effect: tech.effect,
      }));
      return Response.json({
        ok: true,
        techs: techDefs,
        allianceTechState: this.allianceTechStateFor(playerAllianceId),
        donationQuota: playerAllianceId ? undefined : undefined,
      });
    }

    // P9-T2: حالة الأراضي — القلاع (أعلام + قلاع outpost) + مراكز الموارد + إعدادات النطاقات
    if (path.endsWith("/territory-state") && request.method === "GET") {
      return Response.json({
        ok: true,
        castles: this.castleList(),
        outposts: [...this.allianceOutposts.values()],
        resourceCenters: [...this.resourceCenters.values()],
        territoryCfg: {
          flagRadius: flagRadius(),
          outpostRadius: outpostRadius(),
          gatherBonus: gatherBonus(),
          gatherMultiplier: gatherMultiplier(),
          patrolReduction: patrolReduction(),
        },
      });
    }

    // P9-T2: قائد (R4+) يبني قلعة outpost — تنشر نطاق أرض التحالف حولها
    if (path.endsWith("/build-outpost") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const playerId = String(body.playerId || "");
      const city = this.cities.get(playerId);
      const allianceId = city?.allianceId || String(body.allianceId || "");
      if (!city || !allianceId || city.allianceId !== allianceId) {
        this.recordCommandError("not_your_alliance"); return Response.json({ error: "not_your_alliance" }, { status: 403 });
      }
      const rank = String(body.rank || city.rank || "R1");
      if (!rankHas(rank, "territory")) {
        this.recordCommandError("territory_permission_required"); return Response.json({ error: "territory_permission_required" }, { status: 403 });
      }
      const rl = this.antiCheat.check(`alliance:${allianceId}`, "alliance_outpost", nowMs());
      if (!rl.allowed) {
        this.logAntiCheatViolation(playerId, "alliance_outpost", rl.reason);
                return Response.json({ error: `rate_limited_${rl.reason}` }, { status: 429 });
      }
      const x = Number(body.x);
      const y = Number(body.y);
      const outpostMap = getMap();
      if (!Number.isFinite(x) || !Number.isFinite(y) || x < 0 || x > outpostMap.width || y < 0 || y > outpostMap.height) {
        this.recordCommandError("bad_outpost_coords"); return Response.json({ error: "bad_outpost_coords" }, { status: 400 });
      }
      // شروط التحالف: عدد القلاع + عدد الأعضاء + قوة إجمالية + مستوى قاعة المدينة (كلها من JSON + D1)
      const allianceOutposts = [...this.allianceOutposts.values()].filter((o) => o.allianceId === allianceId).length;
      let playerCount = 0;
      let hallLevels = 0;
      let totalPower = 0;
      if (this.env?.DB) {
        try {
          const nRow = await this.env.DB.prepare("SELECT COUNT(*) AS n FROM players WHERE alliance_id = ?").bind(allianceId).first<{ n: number }>();
          playerCount = Number(nRow?.n || 0);
        } catch {}
        try {
          const hlRow = await this.env.DB.prepare("SELECT MAX(b.level) AS hl FROM buildings b JOIN players p ON p.player_id = b.player_id WHERE p.alliance_id = ? AND b.building_id = 'hall'").bind(allianceId).first<{ hl: number | null }>();
          hallLevels = Number(hlRow?.hl || 0);
        } catch {}
        try {
          const pRow = await this.env.DB.prepare("SELECT SUM(power) AS p FROM players WHERE alliance_id = ?").bind(allianceId).first<{ p: number | null }>();
          totalPower = Number(pRow?.p || 0);
        } catch {}
      }
      if (!canBuildOutpost(allianceOutposts, playerCount, totalPower, hallLevels)) {
        this.recordCommandError("outpost_requirements_not_met");
        return Response.json({ error: "outpost_requirements_not_met", reason: "alliance needs more members/power/higher hall level or fewer outposts" }, { status: 400 });
      }
      if (!validPosition(x, y, this.castleList(), [...this.resourceCenters.values()])) {
        this.recordCommandError("outpost_bad_position"); return Response.json({ error: "outpost_bad_position" }, { status: 400 });
      }
      const outpost: AllianceOutpostEntity = {
        id: newId("out"),
        allianceId,
        x,
        y,
        radius: outpostRadius(),
        createdBy: playerId,
        createdAt: nowMs(),
      };
      this.allianceOutposts.set(outpost.id, outpost);
      this.persistAllianceOutpost(outpost);
      this.broadcast({ type: "alliance_outpost_built", outpost });
      return Response.json({ ok: true, outpost });
    }
    // P9-T3: متجر التحالف والألقاب — حالة الرصيد والمشتريات والألقاب (تحالف اللاعب المعني إن حُدّدت، وإلا العامة)
    if (path.endsWith("/alliance-shop-state") && request.method === "GET") {
      const playerAllianceId = (() => {
        const headerId = request.headers.get("x-rok2-player") || "";
        return this.cities.get(headerId)?.allianceId ?? null;
      })();
      const catalog = itemCatalog().map((it) => ({
        id: it.id,
        name: it.name,
        category: it.category,
        price: it.price,
        maxPerAlliance: it.max_per_alliance,
        description: it.description,
      }));
      const titleDefs = titleDefinitions().map((t) => ({
        id: t.id,
        name: t.name,
        icon: t.icon,
        buffs: t.buffs,
        description: t.description,
      }));
      return Response.json({
        ok: true,
        catalog,
        titleDefinitions: titleDefs,
        allianceShopState: this.allianceShopStateFor(playerAllianceId),
      });
    }
    // P9-T3: كسب رصيد تحالف من مساعدة — يستدعيه الراوتر بعد نجاح تسجيل المساعدة
    if (path.endsWith("/alliance-shop-earn-help") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const allianceId = String(body.allianceId || "");
      if (!allianceId) return Response.json({ error: "no_alliance" }, { status: 400 });
      const result = this.earnAllianceHelpCredit(allianceId);
      return Response.json({ ok: true, earned: result.earned, balance: result.balance });
    }
    // P9-T3: كسب رصيد تحالف من مطالبة هدية
    if (path.endsWith("/alliance-shop-earn-gift") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const allianceId = String(body.allianceId || "");
      if (!allianceId) return Response.json({ error: "no_alliance" }, { status: 400 });
      const result = this.earnAllianceGiftClaimCredit(allianceId);
      return Response.json({ ok: true, earned: result.earned, balance: result.balance });
    }
    // P9-T3: شراء عنصر من متجر التحالف برصيد التحالف — يتحقق الراوتر من التحالف قبله
    if (path.endsWith("/alliance-shop-purchase") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const allianceId = String(body.allianceId || "");
      if (!allianceId) return Response.json({ error: "no_alliance" }, { status: 400 });
      const state = this.getOrInitShop(allianceId);
      const v = validatePurchase(state, String(body.itemId || ""));
      if (!v.ok) return Response.json({ error: v.reason }, { status: 400 });
      const r = purchaseShopItem(state, String(body.itemId || ""), nowMs());
      if ("ok" in r && !r.ok) return Response.json({ error: r.reason }, { status: 400 });
      const { state: next, item } = r as { state: AllianceShopState; item: { id: string; name: string; price: number; grant: { type: string; amount: number } } };
      this.allianceShop.set(allianceId, next);
      this.persistAllianceShop(allianceId, next);
      this.broadcast({ type: "alliance_shop_purchased", allianceId, itemId: item.id, boughtBy: String(body.playerId || "") });
      return Response.json({ ok: true, item: { id: item.id, grant: item.grant }, balance: next.balance });
    }
    // P9-T3: منح لقب تحالف مخصص — القائد (R5) يمنح/يغيّر حامل لقب؛ التحقق من القيادة في الراوتر
    if (path.endsWith("/alliance-shop-grant-title") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const allianceId = String(body.allianceId || "");
      if (!allianceId) return Response.json({ error: "no_alliance" }, { status: 400 });
      const titleId = String(body.titleId || "");
      const targetPlayerId = String(body.targetPlayerId || "");
      if (!targetPlayerId) return Response.json({ error: "missing_target_player" }, { status: 400 });
      // حامل اللقب يجب أن يكون عضوًا فعليًا في التحالف
      const targetCity = this.cities.get(targetPlayerId);
      if (!targetCity || targetCity.allianceId !== allianceId) {
        this.recordCommandError("target_not_in_alliance"); return Response.json({ error: "target_not_in_alliance" }, { status: 400 });
      }
      const state = this.getOrInitShop(allianceId);
      // القائد قد يغيّر حامل لقب موجود — لا يُعدّ خطأ، بل إعادة تعيين (سقف الألقاب لا يتغيّر)
      if (state.titles[titleId] && state.titles[titleId] !== targetPlayerId) {
        this.broadcast({ type: "alliance_title_revoked", allianceId, titleId, holder: state.titles[titleId] });
      }
      const grantResult = grantAllianceTitle(state, titleId, targetPlayerId);
      if (!("state" in grantResult)) return Response.json({ error: grantResult.reason }, { status: 400 });
      const { state: next, title } = grantResult;
      this.allianceShop.set(allianceId, next);
      this.persistAllianceShop(allianceId, next);
      // P9-T3: تحديث بطاقة المدينة (titleId) ليصل للعميل مع snapshot
      targetCity.titleId = title.id;
      this.persistCity(targetCity);
      this.broadcast({ type: "alliance_title_granted", allianceId, titleId, holder: targetPlayerId, buffs: title.buffs });
      return Response.json({ ok: true, title: { id: title.id, name: title.name, buffs: title.buffs }, balance: next.balance });
    }
    // P9-T5: Trading Post — قائمة عروض السوق المفتوحة + أسعار الموارد الديناميكية
    if (path.endsWith("/trading-list") && request.method === "GET") {
      const now = nowMs();
      const c = tradingConstants();
      const ttlMs = Number(c.offer_ttl_sec) * 1000;
      // تنظيف العروض المنتهية (تُرجَّع كمياتها لصاحبها لاحقًا عند الطلب — هنا نحدّث الحالة فقط)
      for (const row of this.ctx.storage.sql.exec<any>("SELECT id, seller_id, sell_resource, buy_resource, amount, rate, created_ms, remaining FROM trading_offers").toArray()) {
        if (row.remaining <= 0 || row.created_ms + ttlMs <= now) {
          this.ctx.storage.sql.exec("DELETE FROM trading_offers WHERE id = ?", [row.id]);
        }
      }
      const offers: TradingOffer[] = [];
      for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM trading_offers WHERE remaining > 0 ORDER BY rate DESC, created_ms ASC").toArray()) {
        if (row.created_ms + ttlMs > now) offers.push(row);
      }
      const prices: Record<string, { price: number; day: number; demand: number; supply: number }> = {};
      for (const res of tradingResources()) {
        const p = this.tradingPriceFor(res, now);
        const s = this.tradingPrices.get(res)!;
        prices[res] = { price: p, day: s.day, demand: s.demand, supply: s.supply };
      }
      return Response.json({ ok: true, offers, prices, now });
    }
    // P9-T5: فتح عرض بيع جديد — البائع يستبسل sellResource ويعرض rate صرفًا مقابل buyResource
    if (path.endsWith("/trading-offer") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const playerId = String(body.playerId || "");
      const city = this.cities.get(playerId);
      if (!city) { this.recordCommandError("unknown_player"); return Response.json({ error: "unknown_player" }, { status: 404 }); }
      const sellResource = String(body.sellResource || "");
      const buyResource = String(body.buyResource || "");
      const amount = Number(body.amount);
      const rate = Number(body.rate);
      // فحص موارد البائع: الموارد في جدول D1 cities — الشارد لا يملك رصيدًا؛ يُفحص في الراوتر (refreshCity)
      // قبل استدعاء هذا المسار، فيجب أن يكون الراوتر قد تحقّق من توفر amount من sellResource.
      const hallLevel = city.hallLevel || 1;
      const minHall = Number(tradingConstants().min_trade_hall_level);
      if (hallLevel < minHall) return Response.json({ error: "hall_locked", required: minHall, hall: hallLevel }, { status: 403 });
      const validation = validateOffer({ sellResource, buyResource, amount, rate, activeOfferCount: this.activeOffersForPlayer(playerId, nowMs()).length });
      if (!validation.ok) { this.recordCommandError(validation.error); return Response.json({ error: validation.error }, { status: 400 }); }
      const id = newId("trd");
      const now = nowMs();
      this.ctx.storage.sql.exec(
        `INSERT INTO trading_offers (id, seller_id, sell_resource, buy_resource, amount, rate, created_ms, remaining) VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
        id, playerId, sellResource, buyResource, amount, rate, now, amount,
      );
      // P9-T5: العرض الجديد يرفع جانب العرض فيسبب ضغطًا هبوطيًا طفيفًا على سعر المورد المطلوب بالبيع
      this.bumpTradingPrice(sellResource, 0, 1, now);
      this.broadcast({ type: "trading_offer_opened", playerId, sellResource, buyResource, amount, rate, offerId: id });
      return Response.json({ ok: true, offerId: id, remaining: amount, rate });
    }
    // P9-T5: إتمام صفقة شراء — المشتري يدفع buyResource ويستلم sellResource ناقص الرسوم
    if (path.endsWith("/trading-claim") && request.method === "POST") {
      const body = await request.json<any>();
      const identityError = this.requireAuthenticatedPlayer(request, body.playerId);
      if (identityError) return identityError;
      const playerId = String(body.playerId || "");
      if (playerId === String(body.sellerId || "")) return Response.json({ error: "self_trade" }, { status: 400 });
      const offerId = String(body.offerId || "");
      const claimAmount = Number(body.amount);
      const rate = Number(body.rate);
      const now = nowMs();
      const row = this.ctx.storage.sql.exec<any>("SELECT * FROM trading_offers WHERE id = ?", [offerId]).toArray()[0];
      if (!row) { this.recordCommandError("offer_not_found"); return Response.json({ error: "offer_not_found" }, { status: 404 }); }
      // تحويل حقول SQL (snake_case) إلى نوع TradingOffer النقي (camelCase)
      const offer: TradingOffer = {
        id: String(row.id),
        sellerId: String(row.seller_id),
        sellResource: String(row.sell_resource),
        buyResource: String(row.buy_resource),
        amount: Number(row.amount),
        rate: Number(row.rate),
        created_ms: Number(row.created_ms),
        remaining: Number(row.remaining ?? row.amount),
      };
      const check = validateClaim({ claimAmount, offerAmount: offer.remaining ?? 0, claimRate: rate, offerRate: offer.rate, createdMs: offer.created_ms, ttlSec: Number(tradingConstants().offer_ttl_sec), nowMs: now, claimedToday: Number(body.claimedToday || 0) });
      if (!check.ok) { this.recordCommandError(check.error); return Response.json({ error: check.error }, { status: 400 }); }
      const amount = check.amount;
      // تسوية مالية نقية
      const settlement = settleTrade(offer, amount);
      // تحديث العرض + البائع
      const remaining = (offer.remaining ?? 0) - amount;
      if (remaining <= 0) {
        this.ctx.storage.sql.exec("DELETE FROM trading_offers WHERE id = ?", [offerId]);
      } else {
        this.ctx.storage.sql.exec("UPDATE trading_offers SET remaining = remaining - ? WHERE id = ?", [amount, offerId]);
      }
      // تحريك السعر: الطلب على sellResource (مورد المشتري) يرتفع، والعرض الزائد ينخفض
      this.bumpTradingPrice(offer.sellResource, 1, -1, now);
      this.broadcast({ type: "trading_claim_done", buyerId: playerId, sellerId: offer.sellerId, sellResource: offer.sellResource, buyResource: offer.buyResource, amount, netGainsBuy: settlement.sellerGainsBuy, fee: settlement.feeCharged });
      return Response.json({
        ok: true,
        amount,
        buyResource: offer.buyResource,
        sellResource: offer.sellResource,
        paysBuy: settlement.buyerPaysBuy,
        receivesSell: settlement.buyerReceivesSell,
        sellerLosesSell: settlement.sellerLosesSell,
        sellerGainsBuy: settlement.sellerGainsBuy,
        fee: settlement.feeCharged,
        offerDone: remaining <= 0,
      });
    }
    // P9-T5: أسعار السوق الحالية فقط (قراءة خفيفة — لا تنظيف)
    if (path.endsWith("/trading-prices") && request.method === "GET") {
      const now = nowMs();
      const prices: Record<string, number> = {};
      for (const res of tradingResources()) prices[res] = this.tradingPriceFor(res, now);
      return Response.json({ ok: true, prices, now });
    }
    return Response.json({ error: "not_found", path }, { status: 404 });
  }

  /** آخر خط دفاع لمسارات اللاعب: الرأس يحدده الراوتر بعد `requirePlayer`، ولا تثق الشارد بجسم الطلب. */
  private requireAuthenticatedPlayer(request: Request, claimedPlayerId: unknown): Response | null {
    const playerId = typeof claimedPlayerId === "string" ? claimedPlayerId : "";
    const authenticatedPlayerId = request.headers.get("x-rok2-player") || "";
    if (!playerId || !authenticatedPlayerId || authenticatedPlayerId !== playerId) {
      this.recordCommandError("player_identity_mismatch"); return Response.json({ error: "player_identity_mismatch" }, { status: 403 });
    }
    return null;
  }

  /** P9-T4: تسجيل مستوى VIP سلطوي من الراوتر — فقط إذا كان الطلب موقّعًا من عامل داخلي (x-rok2-player مطابق). */
  private syncVipLevel(request: Request): void {
    const authenticatedPlayerId = request.headers.get("x-rok2-player") || "";
    const vipHeader = request.headers.get("x-rok2-vip-level");
    if (!authenticatedPlayerId || vipHeader === null) return;
    const raw = Number(vipHeader);
    if (!Number.isFinite(raw) || raw < 0 || raw > 15) return;
    // لا يقبل قيمة أكبر من أعلى مستوى في shop.json (دفاع ضد تعديل headers من مصادر غير الراوتر)
    const maxLevel = vipTiers().length - 1;
    const level = Math.min(raw, maxLevel);
    this.playerVipLevels.set(authenticatedPlayerId, level);
  }

  /** P9-T4: مستوى VIP للطلب من ذاكرة الشارد (محدّثة من الراوتر) */
  private vipLevelFor(playerId: string): number {
    return this.playerVipLevels.get(playerId) || 0;
  }

  /** P9-T4: مضاعف جمع VIP للاعب (0 بدون مزايا حتى VIP 15: 1.30) — من مستويات الذاكرة. */
  private vipGatherMod(playerId: string): number {
    const tier = vipTierForPoints(0); // placeholder argument not used for lookup
    void tier;
    const lv = this.vipLevelFor(playerId);
    // البحث عن مستوى النقاط المطابق ضمن المستويات المتاحة (المستوى lv = نقاط threshold >= نقاط lv)
    const tiers = vipTiers();
    const match = [...tiers].reverse().find((t) => lv >= t.level) || null;
    if (!match || match.gather_mult <= 1) return 0;
    return match.gather_mult - 1;
  }

  /** P4-T5: تسجيل مخالفة anti-cheat (آخر violation_log_limit) للفحص الإداري. */
  private logAntiCheatViolation(playerId: string, action: string, reason: string): void {
    this.antiCheatViolations.push({ playerId, action, reason, at: nowMs() });
    if (this.antiCheatViolations.length > ANTICHEAT_CONSTANTS.violationLogLimit) {
      this.antiCheatViolations.splice(0, this.antiCheatViolations.length - ANTICHEAT_CONSTANTS.violationLogLimit);
    }
  }

  // P7-T15: تسجيل فشل أمر من خادم مصادق عليه — نافذة ساعة منزلقة حسب رمز الخطأ.
  private recordCommandError(code: string): void {
    if (!OPS_CONSTANTS.enabled) return;
    const now = nowMs();
    this.commandTotal += 1;
    const entry = this.commandErrorCounts.get(code);
    const windowStart = now - COMMAND_OPS_WINDOW_MS;
    if (!entry || entry.lastMs < windowStart) {
      this.commandErrorCounts.set(code, { n: 1, firstMs: now, lastMs: now });
    } else {
      entry.n += 1;
      entry.lastMs = now;
    }
    // حد سجل الأخطاء في الذاكرة مثل سجل anti-cheat.
    if (this.commandErrorCounts.size > OPS_CONSTANTS.errorLogLimit) {
      const oldest = [...this.commandErrorCounts.entries()].sort((a, b) => a[1].lastMs - b[1].lastMs)[0];
      if (oldest) this.commandErrorCounts.delete(oldest[0]);
    }
  }

  // P7-T15: لقطه مؤشرات التشغيل — أخطاء نافذة الساعة + آخر tick + عمق الطوابير + الانتهاكات.
  private opsSnapshot(): {
    seasonDay: number;
    seasonStartMs: number;
    lastTickMs: number;
    tickStaleMs: number;
    commandErrors: Array<{ code: string; n: number; firstMs: number; lastMs: number }>;
    commandErrorWindowMs: number;
    queuesTotal: number;
    queuesByKind: Record<string, number>;
    marchesActive: number;
    violationsTotal: number;
    alerts: string[];
  } {
    const now = nowMs();
    const windowStart = now - COMMAND_OPS_WINDOW_MS;
    const commandErrors: Array<{ code: string; n: number; firstMs: number; lastMs: number }> = [];
    for (const [code, entry] of this.commandErrorCounts) {
      if (entry.lastMs >= windowStart) commandErrors.push({ code, ...entry });
    }
    commandErrors.sort((a, b) => b.n - a.n);
    const queuesTotal = [...this.queues.values()].filter((q) => q.state === "running").length;
    const queuesByKind: Record<string, number> = {};
    for (const q of this.queues.values()) {
      if (q.state === "running") queuesByKind[q.type] = (queuesByKind[q.type] || 0) + 1;
    }
    const marchesActive = [...this.marches.values()].filter((m) => m.state === "moving" || m.state === "returning").length;
    const alerts: string[] = [];
    if (this.lastTickMs > 0 && now - this.lastTickMs > TICK_STALE_THRESHOLD_MS) {
      alerts.push("tick_stale");
    }
    if (queuesTotal > QUEUE_STUCK_THRESHOLD) {
      alerts.push("queue_pressure");
    }
    const topErrors = commandErrors.filter((e) => e.n >= COMMAND_ALERT_THRESHOLD).map((e) => `command_error_${e.code}`);
    alerts.push(...topErrors);
    return {
      seasonDay: this.seasonDay,
      seasonStartMs: this.seasonStartMs,
      lastTickMs: this.lastTickMs,
      tickStaleMs: this.lastTickMs > 0 ? now - this.lastTickMs : -1,
      commandErrors: commandErrors.slice(0, 10),
      commandErrorWindowMs: COMMAND_OPS_WINDOW_MS,
      queuesTotal,
      queuesByKind,
      marchesActive,
      violationsTotal: this.antiCheatViolations.length,
      alerts,
    };
  }

  private async createMarch(body: any): Promise<MarchEntity> {
    const playerId = body.playerId as string;
    const city = this.cities.get(playerId);
    if (!city) throw new Error("player_city_not_on_map");

    const troops = (body.troops || {}) as Troops;
    if (totalTroops(troops) <= 0) throw new Error("no_troops");

    // P4-T5: anti-cheat — فحص الشذوذ (أعداد ضخمة/سقف مسيرات نشطة) ثم حد المعدل لكل لاعب.
    const activeForPlayer = [...this.marches.values()].filter(
      (m) => m.ownerPlayerId === playerId && (m.state === "moving" || m.state === "gathering"),
    ).length;
    // سعة المسيرات جزء من تقدّم القلعة وليست قيمة يحددها العميل.
    // كل خمس مستويات لقاعة المدينة تضيف مسيرة، حتى سقف خمس مسيرات حية.
    const hallLevel = Math.max(1, Math.trunc(Number(city.hallLevel) || 1));
    const marchCapacity = Math.min(5, 1 + Math.floor((hallLevel - 1) / 5));
    if (activeForPlayer >= marchCapacity) {
      throw new Error("march_capacity_reached");
    }
    const anomaly = checkMarchPayload(troops, activeForPlayer);
    if (anomaly) {
      this.logAntiCheatViolation(playerId, "march", anomaly);
      throw new Error(`anticheat_${anomaly}`);
    }
    const rlAction = body.targetType === "pass" || body.passId ? "pass_attack" : "march";
    const rl = this.antiCheat.check(playerId, rlAction, nowMs());
    if (!rl.allowed) {
      this.logAntiCheatViolation(playerId, rlAction, rl.reason);
      throw new Error(`rate_limited_${rl.reason}`);
    }

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
    } else if (targetType === "holy_site" || body.holySiteId) {
      // P8-T4: استهداف موقع مقدس (Sanctum/Altar/Shrine/المعبد المفقود)
      targetType = "holy_site";
      targetId = String(body.holySiteId || body.targetId);
      const site = this.holySites.get(targetId);
      if (!site) throw new Error("holy_site_not_found");
      if (site.kind === "temple" && !templeUnlocked(this.seasonDay)) throw new Error("temple_locked");
      toX = site.x;
      toY = site.y;
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
    // P9-T2: استهداف مركز مورد — لا AP، داخل الأرض الإقليمية للتحالف فقط
    if (targetType === "center" || body.centerId) {
      targetType = "center";
      targetId = String(body.centerId || body.targetId);
      const center = this.resourceCenters.get(targetId);
      if (!center) throw new Error("center_not_found");
      if (!insideTerritory(center.x, center.y, this.castleList(), city.allianceId)) {
        throw new Error("center_outside_territory");
      }
      if (center.lockedAllianceId && center.lockedAllianceId !== city.allianceId) {
        throw new Error("center_locked");
      }
      toX = center.x;
      toY = center.y;
    }
    // P8-T4: المعبد والمواقع المقدسة داخل منطقة مقفلة زمنياً تُرفض
    if (targetType === "holy_site") {
      const tRegion = this.regionOf(toX, toY);
      if (tRegion) {
        const tZone = this.regions.find((r) => r.id === tRegion)?.zone_id ?? 1;
        if (!isRegionUnlocked(tRegion, tZone, this.seasonDay)) throw new Error("zone_locked");
      }
      if (targetId === HOLY_SITES.temple.id && !coreContestActive(this.seasonDay)) throw new Error("temple_locked");
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
    if (!plan.ok && (targetType === "pass" || targetType === "throne" || targetType === "core_objective" || targetType === "holy_site")) {
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

  /**
   * تغيير وجهة مسيرة حية من موضعها المستنتج في وقت الخادم.
   * لا تُقبل إحداثيات بداية أو زمن من العميل، ولا تُخصم القوات مرة ثانية.
   */
  private async redirectMarch(body: any): Promise<MarchEntity> {
    const playerId = String(body.playerId || "");
    const marchId = String(body.marchId || "");
    const march = this.marches.get(marchId);
    if (!march) throw new Error("march_not_found");
    if (march.ownerPlayerId !== playerId) throw new Error("not_your_march");
    if (march.state !== "moving") throw new Error("march_not_moving");
    // الرالي الموحد يحمل مساهمات عدة لاعبين؛ لا يحق لقائده تغيير وجهته بعد الإطلاق.
    if (march.payload?.rallyId || Array.isArray(march.payload?.rallyParticipants)) {
      throw new Error("rally_march_cannot_redirect");
    }

    const city = this.cities.get(playerId);
    if (!city) throw new Error("player_city_not_on_map");
    const now = nowMs();
    // لا يسبق التحويل معالجة الوصول: إن انتهى وقت المسيرة يحسم الشارد الوصول/القتال أولاً.
    if (now >= march.etaMs) {
      const tickInDay = this.seasonStartMs > 0 ? Math.floor((now - this.seasonStartMs) / 1000) % 86_400 : 0;
      await this.resolveMarchArrival(march, now, tickInDay);
      throw new Error("march_already_arrived");
    }
    const elapsedRatio = Math.max(0, Math.min(1, (now - march.startMs) / Math.max(1, march.etaMs - march.startMs)));
    const fromX = march.fromX + (march.toX - march.fromX) * elapsedRatio;
    const fromY = march.fromY + (march.toY - march.fromY) * elapsedRatio;

    let toX = Number(body.toX);
    let toY = Number(body.toY);
    let targetType = (body.targetType || "point") as MarchEntity["targetType"];
    let targetId = String(body.targetId || "point");

    if (targetType === "throne") {
      targetId = "throne";
      toX = this.throne.x;
      toY = this.throne.y;
    }
    if (targetType === "core_objective" || body.coreObjectiveId) {
      targetType = "core_objective";
      targetId = String(body.coreObjectiveId || body.targetId);
      const objective = this.coreObjectives.get(targetId);
      if (!objective) throw new Error("core_objective_not_found");
      if (!coreContestActive(this.seasonDay)) throw new Error("core_contest_locked");
      toX = objective.x;
      toY = objective.y;
    }
    if (targetType === "pass" || body.passId) {
      targetType = "pass";
      targetId = String(body.passId || body.targetId);
      const pass = this.passes.get(targetId);
      if (!pass) throw new Error("pass_not_found");
      if (pass.unlockDay > this.seasonDay) throw new Error("pass_locked");
      toX = pass.x;
      toY = pass.y;
    } else if (targetType === "holy_site" || body.holySiteId) {
      // P8-T4: إعادة توجيه مسيرة نحو موقع مقدس
      targetType = "holy_site";
      targetId = String(body.holySiteId || body.targetId);
      const site = this.holySites.get(targetId);
      if (!site) throw new Error("holy_site_not_found");
      if (site.kind === "temple" && !templeUnlocked(this.seasonDay)) throw new Error("temple_locked");
      toX = site.x;
      toY = site.y;
    } else if (targetType === "resource" || targetType === "barb") {
      const node = this.nodes.get(targetId);
      if (!node) throw new Error("node_not_found");
      if (node.regionId && node.zoneId != null && !isRegionUnlocked(node.regionId, node.zoneId, this.seasonDay)) {
        throw new Error("zone_locked");
      }

      if (node.kind === "barb") {
        targetType = "barb";
      }
      else targetType = "resource";
    } else if (targetType === "center" || body.centerId) {
      // P9-T2: إعادة توجيه مسيرة نحو مركز مورد — داخل أرض التحالف فقط
      targetType = "center";
      targetId = String(body.centerId || body.targetId);
      const center = this.resourceCenters.get(targetId);
      if (!center) throw new Error("center_not_found");
      if (!insideTerritory(center.x, center.y, this.castleList(), march.allianceId)) {
        throw new Error("center_outside_territory");
      }
      if (center.lockedAllianceId && center.lockedAllianceId !== march.allianceId) {
        throw new Error("center_locked");
      }
      toX = center.x;
      toY = center.y;
    } else if (targetType === "city") {
      const targetCity = this.cities.get(targetId);
      if (!targetCity) throw new Error("target_city_not_found");
      // P8-T5: مدينة محمية بدرع لا يمكن إعادة التوجيه نحوها
      if (targetCity.shieldUntilMs != null && targetCity.shieldUntilMs > now) {
        throw new Error("target_city_shielded");
      }
      toX = targetCity.x;
      toY = targetCity.y;
    }
    // P8-T5: خصم AP لإعادة التوجيه نحو هدف يكلف نقاط عمل
    const targetCityForRedirect = this.cities.get(playerId);
    if (targetType === "barb" && targetCityForRedirect) this.deductApFromCity(targetCityForRedirect, "barb_battle");
    if (targetType === "holy_site" && targetCityForRedirect) this.deductApFromCity(targetCityForRedirect, "holy_site_battle");
    if (targetType === "city" && targetCityForRedirect) this.deductApFromCity(targetCityForRedirect, "city_attack");

    if (!Number.isFinite(toX) || !Number.isFinite(toY)) throw new Error("bad_target_coords");
    if (targetType === march.targetType && targetId === march.targetId && Math.abs(toX - march.toX) < 0.01 && Math.abs(toY - march.toY) < 0.01) {
      throw new Error("redirect_same_target");
    }
    if (targetType === "point" || targetType === "city" || targetType === "throne") {
      const targetRegion = this.regionOf(toX, toY);
      if (targetRegion) {
        const zone = this.regions.find((region) => region.id === targetRegion)?.zone_id ?? 1;
        if (!isRegionUnlocked(targetRegion, zone, this.seasonDay)) throw new Error("zone_locked");
      }
    }

    const canTraverse = (passId: string) => {
      if (targetType === "pass" && passId === targetId) return true;
      const pass = this.passes.get(passId);
      return !!pass?.ownerAllianceId && !!march.allianceId && pass.ownerAllianceId === march.allianceId;
    };
    const sameRegionTarget = (targetType === "resource" || targetType === "barb" || targetType === "center") && this.regionOf(fromX, fromY) === this.regionOf(toX, toY);
    let plan = planMarch({ x: fromX, y: fromY }, { x: toX, y: toY }, this.regions, this.passDefs, this.mountainBelt, this.passWidth, canTraverse);
    if (!plan.ok && sameRegionTarget) plan = { ok: true, distance: dist(fromX, fromY, toX, toY), crossedPasses: [] };
    if (!plan.ok && (targetType === "pass" || targetType === "throne" || targetType === "core_objective" || targetType === "holy_site" || targetType === "center")) {
      plan = { ok: true, distance: dist(fromX, fromY, toX, toY), crossedPasses: [targetId] };
    }
    if (!plan.ok) throw new Error(plan.reason || "illegal_path");
    const marchSpeedMod = 1 + (await this.fetchMarchSpeedMod(playerId));
    march.fromX = fromX;
    march.fromY = fromY;
    march.toX = toX;
    march.toY = toY;
    march.startMs = now;
    march.etaMs = now + marchDurationMs(plan.distance, 40 * marchSpeedMod);
    march.targetType = targetType;
    march.targetId = targetId;
    this.persistMarch(march);
    this.ensureAlarm();
    this.broadcast({ type: "march_redirected", march });
    return march;
  }

  /** P2-T1 (+P8-T1+P8-T2): جلب القائد المرافق لمسيرة من D1 مع مستواه ومهاراته ومواهبه */
  private async fetchMarchCommander(marchId: string): Promise<CommanderInstance | undefined> {
    try {
      const mc = await this.env.DB.prepare(
        "SELECT commander_id, player_id FROM march_commanders WHERE march_id = ?",
      ).bind(marchId).first<{ commander_id: string; player_id: string }>();
      if (!mc) return undefined;
      const pc = await this.env.DB.prepare(
        "SELECT level, skills_json, talents_json, equipment_json FROM player_commanders WHERE player_id = ? AND commander_id = ?",
      ).bind(mc.player_id, mc.commander_id).first<{ level: number; skills_json: string; talents_json: string; equipment_json: string }>();
      if (!pc) return undefined;
      return {
        commanderId: mc.commander_id,
        level: pc.level,
        skills: JSON.parse(pc.skills_json || "[1,1,1]"),
        talentAllocations: JSON.parse(pc.talents_json || "{}"),
        equipmentState: (() => { try { return pc.equipment_json ? JSON.parse(pc.equipment_json) as EquipmentState : undefined; } catch { return undefined; } })(),
      };
    } catch {
      return undefined; // الجدول قد لا يكون مُرحّلاً بعد
    }
  }

  // P8-T2: المعدات تُجلب مع القائد في fetchMarchCommander (equipmentState) —
  // لا حاجة لدالة منفصلة تمنع الوصول المزدوج للـ D1 لكل موضع قتال.
  private fetchMarchEquipment(_marchId: string): never {
    throw new Error("Removed: use commander instance equipmentState from fetchMarchCommander");
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

      await Promise.all(Object.entries(admitted).map(([u, c]) => this.env.DB.prepare(
        `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'severely_wounded', ?)
         ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`,
      ).bind(playerId, u, Number(c)).run()));

      const capacity = hospitalCapacity(hospitalLevel);
      return { admitted, died, capacity };
    } catch {
      return { admitted: zero, died: zero, capacity: 0 };
    }
  }

  /** P2-T2: خصم خسائر المعركة من رصيد 'marching' (الناجون يُخصمون لاحقاً عند العودة للمدينة) */
  private async deductMarchLosses(playerId: string, losses: Troops) {
    await Promise.all(Object.entries(losses)
      .filter(([, c]) => Number(c) > 0)
      .map(([u, c]) => this.env.DB.prepare(
        `UPDATE troops SET count = MAX(0, count - ?) WHERE player_id = ? AND unit_id = ? AND status = 'marching'`,
      ).bind(Number(c), playerId, u).run()));
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
    // playerId الموثّق يصل من الـ router عبر header — لا يُقبل أي playerId من العميل
    const authedPlayerId = request.headers.get("x-rok2-player") || "";
    server.serializeAttachment({ playerId: authedPlayerId, aoi: undefined } satisfies Attach);
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
      // لا نقرأ playerId من الرسالة — الهوية جاءت موثّقة من التوكن عبر header
      ws.send(JSON.stringify({ type: "hello_ok", playerId: att.playerId }));
      ws.send(JSON.stringify({ type: "snapshot", ...this.snapshot(att.playerId) }));
      return;
    }

    if (msg.type === "aoi_sub") {
      att.aoi = { x: Number(msg.x) || 0, y: Number(msg.y) || 0, r: Number(msg.r) || 100 };
      ws.serializeAttachment(att);
      ws.send(JSON.stringify({ type: "aoi_ok", aoi: att.aoi }));
      return;
    }

    if (msg.type === "march_create" || msg.type === "pass_attack") {
      // إنشاء المسيرات عبر WS معطّل — المسار الشرعي الوحيد REST الذي يخصم القوات.
      // ترك هذا المسار مفتوحاً كان يسمح بتوليد قوات مجانية (لا خصم عند الإنشاء،
      // وإضافة عند العودة) وانتحال أي لاعب عبر playerId في الرسالة.
      ws.send(JSON.stringify({ type: "error", error: "use_rest_march" }));
      return;
    }

    if (msg.type === "ping") {
      ws.send(JSON.stringify({ type: "pong", t: nowMs() }));
      return;
    }

    // P6-T6: إرسال رسالة دردشة — يُبث لكل المتصلين ويُحفظ في SQLite
    if (msg.type === "chat_send") {
      const channel = String(msg.channel || "");
      const text = String(msg.text || "").trim();
      const chatCfg = getChatConfig();
      const maxLen = chatCfg.limits?.maxTextLength ?? 200;
      const rateLimit = chatCfg.limits?.rateLimit ?? { windowMs: 5000, maxMessages: 5 };

      // تحقق أساسي
      if (!att.playerId) {
        ws.send(JSON.stringify({ type: "error", error: "auth_required" }));
        return;
      }
      if (channel !== "kingdom" && channel !== "alliance") {
        ws.send(JSON.stringify({ type: "error", error: "bad_channel" }));
        return;
      }
      if (!text || text.length > maxLen) {
        ws.send(JSON.stringify({ type: "error", error: "bad_text_length" }));
        return;
      }

      // قناة التحالف تتطلب عضوية، وتلتقط الهوية قبل أن تتغير العضوية لاحقاً.
      const city = this.cities.get(att.playerId);
      if (channel === "alliance" && !city?.allianceId) {
        ws.send(JSON.stringify({ type: "error", error: "no_alliance" }));
        return;
      }

      // مُحدّد السرعة البسيط
      const now = nowMs();
      const rl = this.chatRateLimit.get(att.playerId);
      if (rl && now - rl.windowStart < rateLimit.windowMs) {
        if (rl.count >= rateLimit.maxMessages) {
          ws.send(JSON.stringify({ type: "error", error: "rate_limited" }));
          return;
        }
        rl.count++;
      } else {
        this.chatRateLimit.set(att.playerId, { count: 1, windowStart: now });
      }

      // بناء الرسالة
      const chatMsg: ChatMessage = {
        id: newId("msg"),
        channel: channel as "kingdom" | "alliance",
        allianceId: channel === "alliance" ? city?.allianceId ?? null : null,
        playerId: att.playerId,
        playerName: city?.name ?? att.playerId,
        civ: "",
        text,
        timestampMs: now,
      };

      // حفظ في SQLite
      this.ctx.storage.sql.exec(
        `INSERT INTO chat_messages (id, channel, alliance_id, player_id, text, created_at) VALUES (?, ?, ?, ?, ?, ?)`,
        chatMsg.id, chatMsg.channel, chatMsg.allianceId, chatMsg.playerId, chatMsg.text, chatMsg.timestampMs,
      );

      // إضافة للذاكرة (حلقة مُغلقة)
      const maxHistory = chatCfg.limits?.maxHistoryPerChannel ?? 100;
      this.chatHistory.push(chatMsg);
      if (this.chatHistory.length > maxHistory) {
        this.chatHistory = this.chatHistory.slice(-maxHistory);
      }

      // بث قناة المملكة عام، أما التحالف فمقيد بعضوية التحالف المُثبتة وقت الإرسال.
      this.broadcastChat(chatMsg);
      ws.send(JSON.stringify({ type: "chat_sent", messageId: chatMsg.id }));
      return;
    }

    // P6-T6: طلب سجل الدردشة — يُرسل آخر N رسالة للقنوات المتاحة
    if (msg.type === "chat_history") {
      const city = this.cities.get(att.playerId);
      const allianceId = city?.allianceId;
      const visible = this.chatHistory.filter((m) => {
        if (m.channel === "kingdom") return true;
        return m.channel === "alliance" && Boolean(allianceId && m.allianceId === allianceId);
      });
      ws.send(JSON.stringify({ type: "chat_history", messages: visible.slice(-100) }));
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
