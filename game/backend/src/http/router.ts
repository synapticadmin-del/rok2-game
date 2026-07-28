import type { Env, PlayerRow, CityRow, VipRow } from "../env";
import { HttpError, json, readJson } from "../lib/errors";
import { newId, nowMs } from "../lib/ids";
import { signToken, sha256Hex, verifyToken } from "../lib/auth";
import { requireAuth, requirePlayer, requireAdmin } from "../lib/context";
import {
  getMap,
  getCivilizations,
  getBuildings,
  getTroops,
  getCommanders,
  getZones,
  getSoftLaunch,
  starterBuildings,
  upgradeCost,
  trainCost,
  unitPower,
} from "../lib/gameData";
import { applyProduction, canAfford, spend } from "../do/sim/production";
import {
  shopConstants,
  shopCatalog,
  getSpeedup,
  vipTiers,
  vipTierForPoints,
  vipPointsForPurchase,
  utcDay,
} from "../do/sim/shop";
import {
  academyReq,
  getTech,
  getTechTree,
  isValidTech,
  prereqsMet,
  researchBuff,
  researchCost,
  researchDurationSec,
  type ResearchLevels,
} from "../do/sim/research";
import { healCost, healDurationSec, hospitalCapacity } from "../do/sim/hospital";
import {
  ALLIANCE_CONSTANTS,
  canLaunchRally,
  canModerate,
  helpSpeedupSec,
  helpsCapped,
  isValidRank,
  rankHas,
  rallyFull,
  rallyPrepMs,
} from "../do/sim/alliance";
import {
  addXp,
  commanderPassiveMod,
  COMMANDER_CONSTANTS,
  getCommanderDef,
  isValidCommander,
  starterCommanderForCiv,
  xpForLevel,
} from "../do/sim/commanders";
import {
  isKingdomOpen,
  kingdomCapacity,
  retentionDayBuckets,
  retentionTargets,
  utcDay as retentionUtcDay,
  cohortDayOf,
  pct,
} from "../do/sim/retention";
import {
  bpConstants,
  bpSeasonId,
  bpLevels,
  bpXpFor,
  bpLevelForXp,
  bpXpRequiredFor,
  bpProgressInLevel,
  bpRewardFor,
  bpClaimableLevels,
} from "../do/sim/battlepass";

// P4-T1: صف Battle Pass للاعب (يُنشأ عند أول وصول) — متوافق مع قواعد لم تُرحّل بعد
async function getOrCreateBp(env: Env, playerId: string): Promise<{ player_id: string; season_id: string; xp: number; level: number; premium: number; updated_at: number }> {
  const seasonId = bpSeasonId();
  try {
    const row = await env.DB.prepare("SELECT * FROM player_battlepass WHERE player_id = ? AND season_id = ?")
      .bind(playerId, seasonId)
      .first<{ player_id: string; season_id: string; xp: number; level: number; premium: number; updated_at: number }>();
    if (row) return row;
    const now = nowMs();
    await env.DB.prepare(
      `INSERT INTO player_battlepass (player_id, season_id, xp, level, premium, updated_at) VALUES (?, ?, 0, 0, 0, ?)`,
    ).bind(playerId, seasonId, now).run();
    return { player_id: playerId, season_id: seasonId, xp: 0, level: 0, premium: 0, updated_at: now };
  } catch {
    return { player_id: playerId, season_id: seasonId, xp: 0, level: 0, premium: 0, updated_at: 0 };
  }
}

// P4-T1: منح نقاط Battle Pass عن فعل لعب (build/train/research/heal/march/pass_attack).
// يحدّث XP + المستوى المحسوب منه. لا يعطّل الفعل عند فشله (جدول قد لا يكون مرحّلاً).
async function grantBpXp(env: Env, playerId: string, action: string): Promise<void> {
  try {
    const gain = bpXpFor(action);
    if (gain <= 0) return;
    const bp = await getOrCreateBp(env, playerId);
    const newXp = bp.xp + gain;
    const newLevel = bpLevelForXp(newXp);
    await env.DB.prepare("UPDATE player_battlepass SET xp=?, level=?, updated_at=? WHERE player_id=? AND season_id=?")
      .bind(newXp, newLevel, nowMs(), playerId, bpSeasonId()).run();
  } catch {
    // الجدول غير موجود بعد — النقاط اختيارية
  }
}

// P3-T4: صف VIP للاعب (يُنشأ عند أول وصول) — متوافق مع قواعد لم تُرحّل بعد
async function getOrCreateVip(env: Env, playerId: string): Promise<VipRow> {
  try {
    const row = await env.DB.prepare("SELECT * FROM player_vip WHERE player_id = ?")
      .bind(playerId)
      .first<VipRow>();
    if (row) return row;
    const now = nowMs();
    await env.DB.prepare(
      `INSERT INTO player_vip (player_id, points, level, last_daily_gems_day, last_free_speedup_day, updated_at)
       VALUES (?, 0, 0, -1, -1, ?)`,
    ).bind(playerId, now).run();
    return { player_id: playerId, points: 0, level: 0, last_daily_gems_day: -1, last_free_speedup_day: -1, updated_at: now };
  } catch {
    // الجدول غير موجود بعد (migration لم تُطبّق) — مستوى افتراضي 0 بدون مزايا
    return { player_id: playerId, points: 0, level: 0, last_daily_gems_day: -1, last_free_speedup_day: -1, updated_at: 0 };
  }
}

// P3-T4: مضاعف إنتاج الموارد من مستوى VIP (يُقرأ من data/shop.json)
async function vipProductionMod(env: Env, playerId: string): Promise<number> {
  const vip = await getOrCreateVip(env, playerId);
  return vipTierForPoints(vip.points).production_mult;
}

function kingdomStub(env: Env) {
  return env.KINGDOM_SHARD.get(env.KINGDOM_SHARD.idFromName(env.KINGDOM_ID || "kingdom-1"));
}

/** P2-T5: رتبة اللاعب داخل تحالفه (من alliance_members؛ القائد R5 افتراضياً) */
async function getMemberRank(env: Env, playerId: string, allianceId: string): Promise<string> {
  try {
    const row = await env.DB.prepare(
      "SELECT rank FROM alliance_members WHERE alliance_id = ? AND player_id = ?",
    ).bind(allianceId, playerId).first<{ rank: string }>();
    if (row?.rank) return row.rank;
    const a = await env.DB.prepare("SELECT leader_player_id FROM alliances WHERE id = ?")
      .bind(allianceId).first<{ leader_player_id: string }>();
    return a?.leader_player_id === playerId ? "R5" : "R1";
  } catch {
    return "R1"; // الجدول قد لا يكون مُرحّلاً بعد
  }
}

async function getBuildingsMap(env: Env, playerId: string): Promise<Record<string, number>> {
  const rows = await env.DB.prepare("SELECT building_id, level FROM buildings WHERE player_id = ?")
    .bind(playerId)
    .all<{ building_id: string; level: number }>();
  const out: Record<string, number> = {};
  for (const r of rows.results || []) out[r.building_id] = r.level;
  return out;
}

async function getTroopsMap(env: Env, playerId: string): Promise<Record<string, number>> {
  const rows = await env.DB.prepare("SELECT unit_id, count FROM troops WHERE player_id = ? AND status = 'home'")
    .bind(playerId)
    .all<{ unit_id: string; count: number }>();
  const out: Record<string, number> = {};
  for (const r of rows.results || []) out[r.unit_id] = r.count;
  return out;
}

/** P2-T3: مستويات أبحاث اللاعب المكتملة */
async function getResearchLevels(env: Env, playerId: string): Promise<ResearchLevels> {
  try {
    const rows = await env.DB.prepare("SELECT tech_id, level FROM player_research WHERE player_id = ?")
      .bind(playerId).all<{ tech_id: string; level: number }>();
    const out: ResearchLevels = {};
    for (const r of rows.results || []) out[r.tech_id] = r.level;
    return out;
  } catch {
    return {}; // الجدول قد لا يكون مُرحّلاً بعد
  }
}

/** P2-T1: قائد مملوك للاعب */
type OwnedCommanderRow = {
  id: string;
  player_id: string;
  commander_id: string;
  level: number;
  xp: number;
  tomes: number;
  skills_json: string;
  created_at: number;
};

async function getOwnedCommanders(env: Env, playerId: string): Promise<OwnedCommanderRow[]> {
  try {
    const rows = await env.DB.prepare(
      "SELECT * FROM player_commanders WHERE player_id = ? ORDER BY created_at ASC",
    ).bind(playerId).all<OwnedCommanderRow>();
    return rows.results || [];
  } catch {
    return []; // الجدول قد لا يكون مُرحّلاً بعد
  }
}

async function getOwnedCommander(env: Env, playerId: string, commanderId: string): Promise<OwnedCommanderRow | null> {
  try {
    return await env.DB.prepare(
      "SELECT * FROM player_commanders WHERE player_id = ? AND commander_id = ?",
    ).bind(playerId, commanderId).first<OwnedCommanderRow>();
  } catch {
    return null;
  }
}

function commanderJson(row: OwnedCommanderRow) {
  const def = getCommanderDef(row.commander_id);
  const skills = JSON.parse(row.skills_json || "[1,1,1]") as number[];
  return {
    instanceId: row.id,
    commanderId: row.commander_id,
    name: def?.name || row.commander_id,
    rarity: def?.rarity || "elite",
    nation: def?.nation || null,
    level: row.level,
    xp: row.xp,
    xpToNext: xpForLevel(row.level),
    tomes: row.tomes,
    skills: (def?.skills || []).map((s, i) => ({
      id: s.id,
      name: s.name,
      type: s.type,
      level: skills[i] || 0,
      maxLevel: s.max_level,
      effects: s.effects,
    })),
    marchSpeedMod: commanderPassiveMod({ commanderId: row.commander_id, level: row.level, skills }, "march_speed"),
  };
}

async function refreshCity(env: Env, playerId: string): Promise<CityRow> {
  const city = await env.DB.prepare("SELECT * FROM cities WHERE player_id = ?")
    .bind(playerId)
    .first<CityRow>();
  if (!city) throw new HttpError(404, "City not found");
  // P3-T4: توافقية مع قواعد لم تُرحّل بعد (بدون عمود gems)
  if (city.gems === undefined || city.gems === null) city.gems = shopConstants().sandbox_starting_gems;
  const buildings = await getBuildingsMap(env, playerId);
  const now = nowMs();
  // P2-T3: باف إنتاج الموارد من أبحاث الاقتصاد
  const research = await getResearchLevels(env, playerId);
  // P3-T4: مضاعف إنتاج VIP (تراكمي مع الأبحاث)
  const vipMod = await vipProductionMod(env, playerId);
  const productionMod = (1 + researchBuff(research, "resource_production")) * vipMod;
  const next = applyProduction(
    {
      food: city.food,
      wood: city.wood,
      stone: city.stone,
      gold: city.gold,
      updated_at: city.updated_at,
    },
    buildings,
    now,
    productionMod,
  );
  await env.DB.prepare(
    `UPDATE cities SET food=?, wood=?, stone=?, gold=?, updated_at=? WHERE player_id=?`,
  )
    .bind(next.food, next.wood, next.stone, next.gold, next.updated_at, playerId)
    .run();
  return {
    ...city,
    food: next.food,
    wood: next.wood,
    stone: next.stone,
    gold: next.gold,
    updated_at: next.updated_at,
  };
}

function pickSpawn(envMap = getMap()) {
  const zone1 = envMap.regions.filter((r) => r.zone_id === 1);
  const r = zone1[Math.floor(Math.random() * zone1.length)];
  const anchor = r.spawn_anchor || [
    (r.aabb[0] + r.aabb[2]) / 2,
    (r.aabb[1] + r.aabb[3]) / 2,
  ];
  const jitter = () => (Math.random() - 0.5) * 40;
  return {
    regionId: r.id,
    x: anchor[0] + jitter(),
    y: anchor[1] + jitter(),
  };
}

export async function handleRequest(request: Request, env: Env): Promise<Response> {
  if (request.method === "OPTIONS") {
    return json({ ok: true });
  }

  const url = new URL(request.url);
  const path = url.pathname.replace(/\/+$/, "") || "/";

  try {
    // P2-T5: إطلاق حملات rally المستحقة — poller رخيص يعمل مع أي طلب (fire-and-forget)
    try {
      const due = await env.DB.prepare(
        "SELECT COUNT(*) as c FROM rallies WHERE status = 'forming' AND launch_ms <= ?",
      ).bind(nowMs()).first<{ c: number }>();
      if ((due?.c || 0) > 0) {
        void kingdomStub(env).fetch("https://do/process-rallies", { method: "POST" });
      }
    } catch {
      // جدول rallies قد لا يكون مُرحّلاً بعد
    }

    // Health
    if (path === "/v1/health" || path === "/health") {
      return json({
        ok: true,
        service: "rok2-api",
        kingdom: env.KINGDOM_ID,
        time: nowMs(),
      });
    }

    // Meta
    if (path === "/v1/meta/map" && request.method === "GET") {
      const map = getMap();
      return json({
        mapId: map.mapId,
        width: map.width,
        height: map.height,
        regions: map.regions,
        passes: map.passes,
        zone3: map.zone3,
        season_unlock_schedule: map.season_unlock_schedule,
      });
    }
    if (path === "/v1/meta/civilizations" && request.method === "GET") {
      return json(getCivilizations());
    }
    if (path === "/v1/meta/buildings" && request.method === "GET") {
      return json(getBuildings());
    }
    if (path === "/v1/meta/troops" && request.method === "GET") {
      return json(getTroops());
    }
    if (path === "/v1/meta/commanders" && request.method === "GET") {
      return json(getCommanders());
    }
    if (path === "/v1/meta/techtree" && request.method === "GET") {
      return json(getTechTree());
    }
    // P2-T4: مواصفة المناطق (فتح زمني + نطاقات موارد) من data/zones.json
    if (path === "/v1/meta/zones" && request.method === "GET") {
      return json(getZones());
    }
    // P1-T6: endpoint موحد لكل بيانات التوازن — يقرأها العميل مرة واحدة عند البدء
    if (path === "/v1/meta/all" && request.method === "GET") {
      return json({
        version: 1,
        civilizations: getCivilizations(),
        buildings: getBuildings(),
        troops: getTroops(),
        commanders: getCommanders(),
        techTree: getTechTree(),
        zones: getZones(),
        constants: {
          productionBase: { farm: 100, lumber_mill: 100, quarry: 70, goldmine: 40 },
          productionLevelMult: 1.2,
          commanders: COMMANDER_CONSTANTS,
          alliance: ALLIANCE_CONSTANTS,
          // P3-T1: ثوابت خدمة الموسم (طول اليوم + سقف الأيام) للعميل — من zones.json
          season: {
            dayMs: (getZones() as any).constants?.season_day_ms ?? 86_400_000,
            maxDay: (getZones() as any).constants?.season_max_day ?? 60,
            service: (getZones() as any).season_service ?? null,
          },
          trainableUnits: [
            { id: "infantry_t1", name: "مشاة T1", branch: "infantry" },
            { id: "cavalry_t1", name: "فرسان T1", branch: "cavalry" },
            { id: "archer_t1", name: "رماة T1", branch: "archer" },
          ],
        },
      });
    }

    // Auth guest
    if (path === "/v1/auth/guest" && request.method === "POST") {
      const body = await readJson<{ deviceId?: string; name?: string }>(request).catch(() => ({} as any));
      const deviceId = body.deviceId || newId("dev");
      const now = nowMs();

      let account = await env.DB.prepare("SELECT id FROM accounts WHERE device_id = ?")
        .bind(deviceId)
        .first<{ id: string }>();

      if (!account) {
        const accountId = newId("acc");
        await env.DB.prepare("INSERT INTO accounts (id, device_id, created_at) VALUES (?, ?, ?)")
          .bind(accountId, deviceId, now)
          .run();
        account = { id: accountId };
      }

      let player = await env.DB.prepare("SELECT * FROM players WHERE account_id = ?")
        .bind(account.id)
        .first<PlayerRow>();

      // player may not exist yet until city/init
      const token = await signToken(
        {
          accountId: account.id,
          playerId: player?.id ?? null,
          exp: now + 1000 * 60 * 60 * 24 * 30,
        },
        env.AUTH_SECRET,
      );
      const tokenHash = await sha256Hex(token);
      await env.DB.prepare(
        `INSERT OR REPLACE INTO sessions (token_hash, account_id, player_id, expires_at) VALUES (?, ?, ?, ?)`,
      )
        .bind(tokenHash, account.id, player?.id ?? null, now + 1000 * 60 * 60 * 24 * 30)
        .run();

      return json({
        token,
        accountId: account.id,
        deviceId,
        player: player
          ? {
              id: player.id,
              name: player.name,
              civ: player.civ,
              allianceId: player.alliance_id,
              regionId: player.region_id,
              x: player.x,
              y: player.y,
            }
          : null,
      });
    }

    // Me
    if (path === "/v1/me" && request.method === "GET") {
      const auth = await requireAuth(request, env);
      const account = await env.DB.prepare("SELECT id, device_id, created_at FROM accounts WHERE id = ?")
        .bind(auth.accountId)
        .first();
      let player = null as PlayerRow | null;
      if (auth.playerId) {
        player = await env.DB.prepare("SELECT * FROM players WHERE id = ?")
          .bind(auth.playerId)
          .first<PlayerRow>();
      }
      return json({ account, player });
    }

    // City init
    if (path === "/v1/city/init" && request.method === "POST") {
      const auth = await requireAuth(request, env);
      const body = await readJson<{ civ?: string; name?: string }>(request);
      const civs = (getCivilizations() as any).civilizations as any[];
      const civ = body.civ || "rome";
      if (!civs.some((c) => c.id === civ)) throw new HttpError(400, "Unknown civilization");

      const existing = await env.DB.prepare("SELECT * FROM players WHERE account_id = ?")
        .bind(auth.accountId)
        .first<PlayerRow>();
      if (existing) {
        return json({ ok: true, already: true, playerId: existing.id });
      }

      // P3-T5: بوابة Soft launch — الانضمام مقيد بممالك الإطلاق المفتوحة وسعتها
      const kingdomId = env.KINGDOM_ID || "kingdom-1";
      if (!isKingdomOpen(kingdomId)) {
        throw new HttpError(403, "kingdom_not_open_for_launch", { kingdom: kingdomId });
      }
      const cap = kingdomCapacity(kingdomId);
      if (cap !== null) {
        const cnt = await env.DB.prepare("SELECT COUNT(*) as c FROM players")
          .first<{ c: number }>();
        if ((cnt?.c || 0) >= cap) {
          throw new HttpError(403, "kingdom_full", { kingdom: kingdomId, max_players: cap });
        }
      }

      const playerId = newId("plr");
      const name = (body.name || `Governor-${playerId.slice(-4)}`).slice(0, 24);
      const spawn = pickSpawn();
      const now = nowMs();

      await env.DB.batch([
        env.DB.prepare(
          `INSERT INTO players (id, account_id, name, civ, alliance_id, power, region_id, x, y, created_at)
           VALUES (?, ?, ?, ?, NULL, 0, ?, ?, ?, ?)`,
        ).bind(playerId, auth.accountId, name, civ, spawn.regionId, spawn.x, spawn.y, now),
        env.DB.prepare(
          `INSERT INTO cities (player_id, hall_level, food, wood, stone, gold, updated_at)
           VALUES (?, 1, 5000, 5000, 3000, 2000, ?)`,
        ).bind(playerId, now),
      ]);

      for (const b of starterBuildings()) {
        await env.DB.prepare(
          `INSERT INTO buildings (player_id, building_id, level) VALUES (?, ?, 1)`,
        )
          .bind(playerId, b)
          .run();
      }
      // starter troops
      for (const u of ["infantry_t1", "cavalry_t1", "archer_t1"]) {
        await env.DB.prepare(`INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)`)
          .bind(playerId, u, u === "infantry_t1" ? 100 : 50)
          .run();
      }

      // P2-T1: قائد البداية للحضارة المختارة (يُقرأ من data/commanders.json)
      const starterCmdId = starterCommanderForCiv(civ);
      let starterCommander: any = null;
      if (starterCmdId) {
        try {
          const now2 = nowMs();
          await env.DB.prepare(
            `INSERT INTO player_commanders (id, player_id, commander_id, level, xp, tomes, skills_json, created_at)
             VALUES (?, ?, ?, 1, 0, ?, '[1,1,1]', ?)`,
          ).bind(newId("pc"), playerId, starterCmdId, COMMANDER_CONSTANTS.starter_tomes, now2).run();
          const def = getCommanderDef(starterCmdId);
          starterCommander = { commanderId: starterCmdId, name: def?.name, level: 1, tomes: COMMANDER_CONSTANTS.starter_tomes };
        } catch {
          // الجدول قد لا يكون مُرحّلاً بعد — لا نفشل إنشاء المدينة
        }
      }

      // P3-T4: منحة gems الأولى (sandbox) — تُقرأ من data/shop.json؛ متوافق مع قواعد لم تُرحّل بعد
      try {
        await env.DB.prepare("UPDATE cities SET gems = ? WHERE player_id = ?")
          .bind(shopConstants().sandbox_starting_gems, playerId).run();
      } catch {
        // عمود gems غير موجود بعد — لا نفشل إنشاء المدينة
      }

      // re-sign token with player id
      const token = await signToken(
        { accountId: auth.accountId, playerId, exp: now + 1000 * 60 * 60 * 24 * 30 },
        env.AUTH_SECRET,
      );

      // push city to kingdom shard
      const stub = kingdomStub(env);
      await stub.fetch("https://do/upsert-city", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({
          playerId,
          name,
          allianceId: null,
          x: spawn.x,
          y: spawn.y,
          hallLevel: 1,
          regionId: spawn.regionId,
        }),
      });

      return json({
        ok: true,
        token,
        player: {
          id: playerId,
          name,
          civ,
          regionId: spawn.regionId,
          x: spawn.x,
          y: spawn.y,
        },
        starterCommander,
      });
    }

    // City get
    if (path === "/v1/city" && request.method === "GET") {
      const { player } = await requirePlayer(request, env);
      const city = await refreshCity(env, player.id);
      const buildings = await getBuildingsMap(env, player.id);
      const troops = await getTroopsMap(env, player.id);

      // P2-T2: الجرحى وسعة المستشفى
      const wRows = await env.DB.prepare("SELECT unit_id, count FROM troops WHERE player_id = ? AND status = 'severely_wounded'")
        .bind(player.id).all<{ unit_id: string; count: number }>();
      const wounded: Record<string, number> = {};
      let woundedTotal = 0;
      for (const r of wRows.results || []) { wounded[r.unit_id] = r.count; woundedTotal += r.count; }
      const hospitalLevel = buildings["hospital"] || 0;
      const capacity = hospitalCapacity(hospitalLevel);

      return json({
        player,
        city,
        buildings,
        troops,
        wounded,
        hospital: {
          level: hospitalLevel,
          capacity,
          used: woundedTotal,
          free: Math.max(0, capacity - woundedTotal),
        },
      });
    }

    // City upgrade
    if (path === "/v1/city/upgrade" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ buildingId: string }>(request);
      if (!body.buildingId) throw new HttpError(400, "buildingId required");

      let city = await refreshCity(env, player.id);
      const buildings = await getBuildingsMap(env, player.id);
      const cur = buildings[body.buildingId] || 0;
      if (cur <= 0) throw new HttpError(400, "Building not owned");
      const nextLevel = cur + 1;
      if (body.buildingId !== "city_hall") {
        const hall = buildings.city_hall || 1;
        if (nextLevel > hall + 1) throw new HttpError(400, "City Hall too low for this upgrade");
      }
      const cost = upgradeCost(body.buildingId, nextLevel);
      if (!canAfford(city, cost)) throw new HttpError(400, "Not enough resources", { cost, city });

      const spent = spend(city, cost);
      await env.DB.prepare(
        `UPDATE cities SET food=?, wood=?, stone=?, gold=?, updated_at=? WHERE player_id=?`,
      ).bind(
        spent.food,
        spent.wood,
        spent.stone,
        spent.gold,
        nowMs(),
        player.id,
      ).run();

      const duration = 30 * Math.pow(1.35, nextLevel - 1);
      const queueId = newId("q");
      const stub = kingdomStub(env);
      await stub.fetch("https://do/queue/add", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({
          id: queueId,
          playerId: player.id,
          type: "build",
          data: { buildingId: body.buildingId, level: nextLevel },
          startMs: nowMs(),
          etaMs: nowMs() + duration * 1000,
        })
      });

      city = await refreshCity(env, player.id);
      // P4-T1: نقاط Battle Pass عن البناء
      await grantBpXp(env, player.id, "build");
      return json({
        ok: true,
        buildingId: body.buildingId,
        level: nextLevel,
        city,
        buildings: await getBuildingsMap(env, player.id),
      });
    }

    // Train
    if (path === "/v1/city/train" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ unit: string; count: number }>(request);
      const unit = body.unit;
      const count = Math.floor(Number(body.count) || 0);
      if (!["infantry_t1", "cavalry_t1", "archer_t1"].includes(unit)) {
        throw new HttpError(400, "Unsupported unit for prototype");
      }
      if (count <= 0 || count > 10000) throw new HttpError(400, "Invalid count");

      let city = await refreshCity(env, player.id);
      const cost = trainCost(unit, count);
      if (!canAfford(city, cost)) throw new HttpError(400, "Not enough resources", { cost });
      const spent = spend(city, cost);
      
      await env.DB.prepare(
        `UPDATE cities SET food=?, wood=?, stone=?, gold=?, updated_at=? WHERE player_id=?`,
      ).bind(spent.food, spent.wood, spent.stone, spent.gold, nowMs(), player.id).run();

      // P3-T4: مضاعف سرعة التدريب من مستوى VIP (تراكمي مع أبحاث training_speed)
      const vipT = vipTierForPoints((await getOrCreateVip(env, player.id)).points);
      const duration = Math.max(1, Math.floor(10 * count / ((1 + researchBuff(await getResearchLevels(env, player.id), "training_speed")) * vipT.train_speed_mult)));
      const queueId = newId("q");
      const stub = kingdomStub(env);
      await stub.fetch("https://do/queue/add", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({
          id: queueId,
          playerId: player.id,
          type: "train",
          data: { troops: { [unit]: count } },
          startMs: nowMs(),
          etaMs: nowMs() + duration * 1000,
        })
      });

      const all = await getTroopsMap(env, player.id);
      city = await refreshCity(env, player.id);
      // P4-T1: نقاط Battle Pass عن التدريب
      await grantBpXp(env, player.id, "train");
      return json({ ok: true, unit, count: count, queueId, city, troops: all });
    }

    // Collect / refresh
    if (path === "/v1/city/collect" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const city = await refreshCity(env, player.id);
      return json({ ok: true, city });
    }

    // ═══ P3-T4: متجر sandbox + speedups + VIP (بدون مدفوعات حقيقية) ═══

    // كتالوج المتجر: speedups + رصيد gems + مخزون اللاعب
    if (path === "/v1/shop/catalog" && request.method === "GET") {
      const { player } = await requirePlayer(request, env);
      const city = await refreshCity(env, player.id);
      const invRows = await env.DB.prepare("SELECT item_id, count FROM player_inventory WHERE player_id = ?")
        .bind(player.id).all<{ item_id: string; count: number }>().catch(() => ({ results: [] as any[] }));
      const inventory: Record<string, number> = {};
      for (const r of invRows.results || []) inventory[r.item_id] = r.count;
      const vip = await getOrCreateVip(env, player.id);
      return json({
        ok: true,
        gems: city.gems,
        speedups: shopCatalog(),
        inventory,
        vip: { ...vipTierForPoints(vip.points), points: vip.points },
        constants: shopConstants(),
      });
    }

    // حالة VIP: المستوى الحالي + المزايا + التقدم نحو المستوى التالي
    if (path === "/v1/vip/status" && request.method === "GET") {
      const { player } = await requirePlayer(request, env);
      const vip = await getOrCreateVip(env, player.id);
      const tier = vipTierForPoints(vip.points);
      const nextTier = vipTiers().find((t) => t.points_required > vip.points) || null;
      const day = utcDay(nowMs());
      return json({
        ok: true,
        points: vip.points,
        level: tier.level,
        perks: tier,
        next: nextTier ? { level: nextTier.level, points_required: nextTier.points_required, points_to_go: nextTier.points_required - vip.points } : null,
        daily_gems_available: vip.last_daily_gems_day < day,
        free_speedup_available: tier.free_speedup_sec_per_day > 0 && vip.last_free_speedup_day < day,
      });
    }

    // المنحة اليومية المجانية من gems (sandbox)
    if (path === "/v1/shop/daily-gems" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const vip = await getOrCreateVip(env, player.id);
      const day = utcDay(nowMs());
      if (vip.last_daily_gems_day >= day) throw new HttpError(400, "Daily gems already claimed");
      const grant = shopConstants().sandbox_daily_gems;
      const city = await refreshCity(env, player.id);
      const now = nowMs();
      await env.DB.batch([
        env.DB.prepare("UPDATE cities SET gems=?, updated_at=? WHERE player_id=?")
          .bind(city.gems + grant, now, player.id),
        env.DB.prepare("UPDATE player_vip SET last_daily_gems_day=?, updated_at=? WHERE player_id=?")
          .bind(day, now, player.id),
      ]);
      return json({ ok: true, granted: grant, gems: city.gems + grant });
    }

    // شراء speedup من المتجر بالـ gems — كل عملية شراء تمنح نقاط VIP
    if (path === "/v1/shop/buy" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ itemId: string; count?: number }>(request);
      const item = getSpeedup(body.itemId);
      if (!item) throw new HttpError(400, "Unknown shop item");
      const count = Math.max(1, Math.min(99, Math.floor(Number(body.count) || 1)));
      const totalCost = item.cost_gems * count;

      const city = await refreshCity(env, player.id);
      if (city.gems < totalCost) throw new HttpError(400, "Not enough gems", { cost: totalCost, gems: city.gems });

      const vip = await getOrCreateVip(env, player.id);
      const pointsGain = vipPointsForPurchase(totalCost);
      const newPoints = vip.points + pointsGain;
      const newTier = vipTierForPoints(newPoints);
      const now = nowMs();

      await env.DB.batch([
        env.DB.prepare("UPDATE cities SET gems=?, updated_at=? WHERE player_id=?")
          .bind(city.gems - totalCost, now, player.id),
        env.DB.prepare(
          `INSERT INTO player_inventory (player_id, item_id, count, updated_at) VALUES (?, ?, ?, ?)
           ON CONFLICT(player_id, item_id) DO UPDATE SET count=count+?, updated_at=?`,
        ).bind(player.id, item.id, count, now, count, now),
        env.DB.prepare("UPDATE player_vip SET points=?, level=?, updated_at=? WHERE player_id=?")
          .bind(newPoints, newTier.level, now, player.id),
      ]);

      return json({
        ok: true,
        itemId: item.id,
        count,
        spent_gems: totalCost,
        gems: city.gems - totalCost,
        vip: { points: newPoints, level: newTier.level, leveled_up: newTier.level > vipTierForPoints(vip.points).level },
      });
    }

    // استخدام speedup من المخزون على طابور جاري + المطالبة بالتسريع المجاني اليومي من VIP
    if (path === "/v1/shop/use-speedup" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ queueId: string; itemId?: string; useFreeDaily?: boolean }>(request);
      if (!body.queueId) throw new HttpError(400, "queueId required");

      let seconds = 0;
      let source: string;
      const now = nowMs();

      if (body.useFreeDaily) {
        // التسريع المجاني اليومي من مزايا VIP
        const vip = await getOrCreateVip(env, player.id);
        const tier = vipTierForPoints(vip.points);
        const day = utcDay(now);
        if (tier.free_speedup_sec_per_day <= 0) throw new HttpError(400, "No free daily speedup at your VIP level");
        if (vip.last_free_speedup_day >= day) throw new HttpError(400, "Free daily speedup already used");
        seconds = tier.free_speedup_sec_per_day;
        source = "vip_free_daily";
        await env.DB.prepare("UPDATE player_vip SET last_free_speedup_day=?, updated_at=? WHERE player_id=?")
          .bind(day, now, player.id).run();
      } else {
        const item = getSpeedup(body.itemId || "");
        if (!item) throw new HttpError(400, "Unknown speedup item");
        const inv = await env.DB.prepare("SELECT count FROM player_inventory WHERE player_id = ? AND item_id = ?")
          .bind(player.id, item.id).first<{ count: number }>();
        if (!inv || inv.count < 1) throw new HttpError(400, "Item not in inventory");
        seconds = item.seconds;
        source = item.id;
        await env.DB.prepare("UPDATE player_inventory SET count=count-1, updated_at=? WHERE player_id=? AND item_id=?")
          .bind(now, player.id, item.id).run();
      }

      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/queue/speedup", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ queueId: body.queueId, seconds }),
      });
      const data = await res.json<any>();
      if (!res.ok) throw new HttpError(res.status, data.error || "speedup_failed");
      return json({ ok: true, queueId: body.queueId, seconds, source });
    }

    // ═══ P4-T1: Battle Pass (sandbox — مسار مجاني + مدفوع بالـ gems) ═══

    // حالة Battle Pass: XP + مستوى + تقدم + مكافآت قابلة للمطالبة لكل مسار
    if (path === "/v1/battlepass" && request.method === "GET") {
      const { player } = await requirePlayer(request, env);
      const bp = await getOrCreateBp(env, player.id);
      const progress = bpProgressInLevel(bp.xp);
      const claimRows = await env.DB.prepare(
        "SELECT level, track FROM battlepass_claims WHERE player_id = ? AND season_id = ?",
      ).bind(player.id, bpSeasonId()).all<{ level: number; track: string }>()
        .catch(() => ({ results: [] as any[] }));
      const claimed = new Set((claimRows.results || []).map((r) => `${r.level}:${r.track}`));
      const claimable = bpClaimableLevels(bp.level);
      return json({
        ok: true,
        season_id: bp.season_id,
        xp: bp.xp,
        level: bp.level,
        premium: bp.premium === 1,
        progress,
        xp_for_next: progress.atMax ? null : bpXpRequiredFor(bp.level + 1) - bp.xp,
        levels: bpLevels().map((l) => ({
          level: l.level,
          unlocked: bp.level >= l.level,
          free: { reward: l.free, claimed: claimed.has(`${l.level}:free`), claimable: bp.level >= l.level && !claimed.has(`${l.level}:free`) },
          premium: { reward: l.premium, claimed: claimed.has(`${l.level}:premium`), claimable: bp.premium === 1 && bp.level >= l.level && !claimed.has(`${l.level}:premium`) },
        })),
        claimable_levels: claimable,
        constants: bpConstants(),
      });
    }

    // فتح المسار المدفوع بالـ gems (sandbox)
    if (path === "/v1/battlepass/unlock-premium" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const bp = await getOrCreateBp(env, player.id);
      if (bp.premium === 1) throw new HttpError(400, "Premium already unlocked");
      const cost = bpConstants().premium_cost_gems;
      const city = await refreshCity(env, player.id);
      if (city.gems < cost) throw new HttpError(400, "Not enough gems", { cost, gems: city.gems });
      const now = nowMs();
      await env.DB.batch([
        env.DB.prepare("UPDATE cities SET gems=?, updated_at=? WHERE player_id=?")
          .bind(city.gems - cost, now, player.id),
        env.DB.prepare("UPDATE player_battlepass SET premium=1, updated_at=? WHERE player_id=? AND season_id=?")
          .bind(now, player.id, bpSeasonId()),
      ]);
      return json({ ok: true, premium: true, spent_gems: cost, gems: city.gems - cost });
    }

    // المطالبة بمكافأة مستوى (مسار مجاني أو مدفوع)
    if (path === "/v1/battlepass/claim" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ level: number; track: string }>(request);
      const level = Math.floor(Number(body.level) || 0);
      const track = body.track === "premium" ? "premium" : "free";
      if (level < 1 || level > bpConstants().max_level) throw new HttpError(400, "Invalid level");

      const bp = await getOrCreateBp(env, player.id);
      if (bp.level < level) throw new HttpError(400, "Level not reached", { have: bp.level, need: level });
      if (track === "premium" && bp.premium !== 1) throw new HttpError(400, "Premium not unlocked");

      const reward = bpRewardFor(level, track);
      if (!reward) throw new HttpError(404, "No reward for this level/track");

      // المطالبة مرة واحدة لكل (مستوى, مسار)
      const now = nowMs();
      try {
        await env.DB.prepare(
          "INSERT INTO battlepass_claims (player_id, season_id, level, track, claimed_at) VALUES (?, ?, ?, ?, ?)",
        ).bind(player.id, bpSeasonId(), level, track, now).run();
      } catch {
        throw new HttpError(409, "Reward already claimed");
      }

      // منح المكافأة حسب نوعها
      const city = await refreshCity(env, player.id);
      if (reward.type === "speedup") {
        const itemId = reward.item_id!;
        const count = reward.count || 1;
        await env.DB.prepare(
          `INSERT INTO player_inventory (player_id, item_id, count, updated_at) VALUES (?, ?, ?, ?)
           ON CONFLICT(player_id, item_id) DO UPDATE SET count=count+?, updated_at=?`,
        ).bind(player.id, itemId, count, now, count, now).run();
        return json({ ok: true, level, track, reward: { type: "speedup", item_id: itemId, count } });
      }

      const res = ["food", "wood", "stone", "gold", "gems"];
      if (!res.includes(reward.type)) throw new HttpError(400, "Unknown reward type");
      const amount = reward.amount || 0;
      const next = { ...city, [reward.type]: (city as any)[reward.type] + amount };
      await env.DB.prepare(
        `UPDATE cities SET food=?, wood=?, stone=?, gold=?, gems=?, updated_at=? WHERE player_id=?`,
      ).bind(next.food, next.wood, next.stone, next.gold, (next as any).gems, now, player.id).run();
      return json({ ok: true, level, track, reward: { type: reward.type, amount }, city: next });
    }

    // Heal — P2-T2: شفاء الجرحى الخطيرين مقابل نصف تكلفة التدريب + مدة من data/buildings.json
    if (path === "/v1/city/heal" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ troops: Record<string, number> }>(request);
      let count = 0;
      for (const [u, c] of Object.entries(body.troops || {})) count += Number(c);
      if (count <= 0) throw new HttpError(400, "No troops to heal");

      const rows = await env.DB.prepare("SELECT unit_id, count FROM troops WHERE player_id = ? AND status = 'severely_wounded'")
        .bind(player.id).all<{ unit_id: string; count: number }>();
      const wounded: Record<string, number> = {};
      for (const r of rows.results || []) wounded[r.unit_id] = r.count;
      for (const [u, c] of Object.entries(body.troops || {})) {
        if ((wounded[u] || 0) < Number(c)) throw new HttpError(400, `Not enough severely wounded ${u}`);
      }

      // تكلفة الشفاء من الموارد (نصف تكلفة التدريب — من data/buildings.json)
      let city = await refreshCity(env, player.id);
      const cost = healCost(body.troops as Record<string, number>);
      if (!canAfford(city, cost)) throw new HttpError(400, "Not enough resources to heal", { cost });
      const spent = spend(city, cost);
      await env.DB.prepare(
        `UPDATE cities SET food=?, wood=?, stone=?, gold=?, updated_at=? WHERE player_id=?`,
      ).bind(spent.food, spent.wood, spent.stone, spent.gold, nowMs(), player.id).run();

      for (const [u, c] of Object.entries(body.troops || {})) {
        await env.DB.prepare(
          `UPDATE troops SET count=count-? WHERE player_id=? AND unit_id=? AND status='severely_wounded'`
        ).bind(Number(c), player.id, u).run();
      }

      const duration = healDurationSec(count);
      const queueId = newId("q");
      const stub = kingdomStub(env);
      await stub.fetch("https://do/queue/add", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({
          id: queueId,
          playerId: player.id,
          type: "heal",
          data: { troops: body.troops },
          startMs: nowMs(),
          etaMs: nowMs() + duration * 1000,
        })
      });

      city = await refreshCity(env, player.id);
      // P4-T1: نقاط Battle Pass عن الشفاء
      await grantBpXp(env, player.id, "heal");
      return json({ ok: true, queueId, healSeconds: duration, cost, city });
    }

    // P2-T3: شجرة البحث مع مستويات اللاعب المكتملة
    if (path === "/v1/research" && request.method === "GET") {
      const { player } = await requirePlayer(request, env);
      const levels = await getResearchLevels(env, player.id);
      const buildings = await getBuildingsMap(env, player.id);
      const academyLevel = buildings["academy"] || 0;
      const tree = (getTechTree() as any).technologies as any[];
      return json({
        academyLevel,
        technologies: tree.map((t) => ({
          id: t.id,
          name: t.name,
          branch: t.branch,
          maxLevel: t.max_level,
          level: levels[t.id] || 0,
          buff: t.buff,
          description: t.description,
          prerequisites: t.prerequisites,
          nextLevel: (levels[t.id] || 0) < t.max_level ? {
            level: (levels[t.id] || 0) + 1,
            cost: researchCost(t.id, (levels[t.id] || 0) + 1),
            durationSec: researchDurationSec(t.id, (levels[t.id] || 0) + 1),
            academyReq: academyReq(t.id, (levels[t.id] || 0) + 1),
          } : null,
        })),
      });
    }

    // Research — P2-T3: يقرأ المستوى من D1 + يتحقق prerequisites + تكلفة/مدة من data/research.json
    if (path === "/v1/city/research" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ techId: string }>(request);
      if (!body.techId) throw new HttpError(400, "techId required");
      if (!isValidTech(body.techId)) throw new HttpError(404, "Technology not found");

      const tech = getTech(body.techId)!;
      let city = await refreshCity(env, player.id);
      const buildings = await getBuildingsMap(env, player.id);
      const academyLvl = buildings["academy"] || 0;

      const levels = await getResearchLevels(env, player.id);
      const currentTechLevel = levels[body.techId] || 0;
      const nextLevel = currentTechLevel + 1;

      if (nextLevel > tech.max_level) throw new HttpError(400, "Max tech level reached");
      if (academyLvl < academyReq(body.techId, nextLevel)) {
        throw new HttpError(400, "Academy level too low", { required: academyReq(body.techId, nextLevel), have: academyLvl });
      }
      const pre = prereqsMet(body.techId, levels);
      if (!pre.ok) throw new HttpError(400, "Prerequisites not met", { missing: pre.missing });

      const cost = researchCost(body.techId, nextLevel);
      if (!canAfford(city, cost)) throw new HttpError(400, "Not enough resources", { cost });

      const spent = spend(city, cost);
      await env.DB.prepare(
        `UPDATE cities SET food=?, wood=?, stone=?, gold=?, updated_at=? WHERE player_id=?`,
      ).bind(spent.food, spent.wood, spent.stone, spent.gold, nowMs(), player.id).run();

      const duration = researchDurationSec(body.techId, nextLevel);
      const queueId = newId("q");
      const stub = kingdomStub(env);
      await stub.fetch("https://do/queue/add", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({
          id: queueId,
          playerId: player.id,
          type: "research",
          data: { techId: body.techId, level: nextLevel },
          startMs: nowMs(),
          etaMs: nowMs() + duration * 1000,
        })
      });

      city = await refreshCity(env, player.id);
      // P4-T1: نقاط Battle Pass عن البحث
      await grantBpXp(env, player.id, "research");
      return json({ ok: true, techId: body.techId, level: nextLevel, durationSec: duration, queueId, cost, city });
    }

    // P2-T1: قائمة قادة اللاعب المملوكين (مع بياناتهم من data/commanders.json)
    if (path === "/v1/commanders" && request.method === "GET") {
      const { player } = await requirePlayer(request, env);
      const owned = await getOwnedCommanders(env, player.id);
      return json({
        commanders: owned.map(commanderJson),
        roster: getCommanders(),
        constants: COMMANDER_CONSTANTS,
      });
    }

    // P2-T1: استدعاء قائد جديد مقابل ذهب (sandbox — لا gacha حقيقي بعد)
    if (path === "/v1/commander/summon" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ commanderId: string }>(request);
      if (!body.commanderId || !isValidCommander(body.commanderId)) {
        throw new HttpError(400, "Unknown commanderId");
      }
      const existing = await getOwnedCommander(env, player.id, body.commanderId);
      if (existing) throw new HttpError(409, "Commander already owned");

      let city = await refreshCity(env, player.id);
      const cost = { food: 0, wood: 0, stone: 0, gold: COMMANDER_CONSTANTS.summon_cost_gold };
      if (!canAfford(city, cost)) throw new HttpError(400, "Not enough gold", { cost });
      const spent = spend(city, cost);
      await env.DB.prepare(
        `UPDATE cities SET food=?, wood=?, stone=?, gold=?, updated_at=? WHERE player_id=?`,
      ).bind(spent.food, spent.wood, spent.stone, spent.gold, nowMs(), player.id).run();

      await env.DB.prepare(
        `INSERT INTO player_commanders (id, player_id, commander_id, level, xp, tomes, skills_json, created_at)
         VALUES (?, ?, ?, 1, 0, 0, '[1,1,1]', ?)`,
      ).bind(newId("pc"), player.id, body.commanderId, nowMs()).run();

      const row = await getOwnedCommander(env, player.id, body.commanderId);
      return json({ ok: true, commander: row ? commanderJson(row) : null, city: await refreshCity(env, player.id) });
    }

    // P2-T1: رفع مستوى القائد باستهلاك تومات خبرة
    if (path === "/v1/commander/levelup" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ commanderId: string; tomes: number }>(request);
      if (!body.commanderId) throw new HttpError(400, "commanderId required");
      const tomes = Math.floor(Number(body.tomes) || 0);
      if (tomes <= 0 || tomes > 100) throw new HttpError(400, "Invalid tomes count");

      const row = await getOwnedCommander(env, player.id, body.commanderId);
      if (!row) throw new HttpError(404, "Commander not owned");
      if (row.tomes < tomes) throw new HttpError(400, "Not enough tomes", { have: row.tomes });
      if (row.level >= COMMANDER_CONSTANTS.max_level) throw new HttpError(400, "Max level reached");

      const next = addXp(
        { commanderId: row.commander_id, level: row.level, xp: row.xp, skills: JSON.parse(row.skills_json) },
        tomes * COMMANDER_CONSTANTS.tome_xp,
      );
      await env.DB.prepare(
        `UPDATE player_commanders SET level=?, xp=?, tomes=tomes-? WHERE id=?`,
      ).bind(next.level, next.xp, tomes, row.id).run();

      const updated = await getOwnedCommander(env, player.id, body.commanderId);
      return json({ ok: true, commander: updated ? commanderJson(updated) : null });
    }

    // P2-T1: رفع مهارة قائد (attack/defense/passive) مقابل تومات
    if (path === "/v1/commander/skill" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ commanderId: string; skillSlot: number }>(request);
      if (!body.commanderId) throw new HttpError(400, "commanderId required");
      const slot = Math.floor(Number(body.skillSlot) || 0);
      if (slot < 1 || slot > 3) throw new HttpError(400, "skillSlot must be 1..3");

      const row = await getOwnedCommander(env, player.id, body.commanderId);
      if (!row) throw new HttpError(404, "Commander not owned");
      const def = getCommanderDef(row.commander_id);
      if (!def) throw new HttpError(404, "Commander def missing");

      const skills = JSON.parse(row.skills_json || "[1,1,1]") as number[];
      const skillDef = def.skills[slot - 1];
      const cur = skills[slot - 1] || 0;
      if (cur >= skillDef.max_level) throw new HttpError(400, "Skill maxed");
      // شرط مستوى القائد: مهارة أعلى تحتاج مستوى قائد أعلى (10 لكل مستوى مهارة)
      const levelReq = (cur + 1) * 10;
      if (row.level < levelReq) throw new HttpError(400, `Commander level ${levelReq} required`);

      const cost = COMMANDER_CONSTANTS.skill_upgrade_tome_cost;
      if (row.tomes < cost) throw new HttpError(400, "Not enough tomes", { have: row.tomes, cost });

      skills[slot - 1] = cur + 1;
      await env.DB.prepare(
        `UPDATE player_commanders SET skills_json=?, tomes=tomes-? WHERE id=?`,
      ).bind(JSON.stringify(skills), cost, row.id).run();

      const updated = await getOwnedCommander(env, player.id, body.commanderId);
      return json({ ok: true, commander: updated ? commanderJson(updated) : null });
    }

    // P2-T1: تعيين قائد على مسيرة نشطة مملوكة للاعب
    if (path === "/v1/commander/assign" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ marchId: string; commanderId: string }>(request);
      if (!body.marchId || !body.commanderId) throw new HttpError(400, "marchId and commanderId required");

      const row = await getOwnedCommander(env, player.id, body.commanderId);
      if (!row) throw new HttpError(404, "Commander not owned");

      const stub = kingdomStub(env);
      const snap = await (await stub.fetch("https://do/snapshot")).json<any>();
      const march = (snap.marches || []).find((m: any) => m.id === body.marchId);
      if (!march) throw new HttpError(404, "March not found or not active");
      if (march.ownerPlayerId !== player.id) throw new HttpError(403, "Not your march");

      await env.DB.prepare(
        `INSERT OR REPLACE INTO march_commanders (march_id, player_id, commander_id, skills_json, created_at)
         VALUES (?, ?, ?, ?, ?)`,
      ).bind(body.marchId, player.id, body.commanderId, row.skills_json, nowMs()).run();

      return json({ ok: true, marchId: body.marchId, commander: commanderJson(row) });
    }

    // Speedup
    if (path === "/v1/city/speedup" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ queueId: string; seconds: number }>(request);
      if (!body.queueId || !body.seconds) throw new HttpError(400, "Missing queueId or seconds");
      
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/queue/speedup", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ queueId: body.queueId, seconds: body.seconds }),
      });
      const data = await res.json<any>();
      if (!res.ok) throw new HttpError(res.status, data.error || "speedup_failed");
      return json({ ok: true });
    }

    // Alliance create
    if (path === "/v1/alliance/create" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (player.alliance_id) throw new HttpError(400, "Already in an alliance");
      const body = await readJson<{ name: string; tag: string }>(request);
      const name = (body.name || "").trim().slice(0, 24);
      const tag = (body.tag || "").trim().toUpperCase().slice(0, 4);
      if (name.length < 3 || tag.length < 2) throw new HttpError(400, "Invalid name/tag");

      const id = newId("all");
      const now = nowMs();
      try {
        await env.DB.batch([
          env.DB.prepare(
            `INSERT INTO alliances (id, name, tag, leader_player_id, created_at) VALUES (?, ?, ?, ?, ?)`,
          ).bind(id, name, tag, player.id, now),
          env.DB.prepare(`UPDATE players SET alliance_id=? WHERE id=?`).bind(id, player.id),
          env.DB.prepare(
            `INSERT INTO alliance_members (alliance_id, player_id, rank, joined_at) VALUES (?, ?, 'R5', ?)`,
          ).bind(id, player.id, now),
        ]);
      } catch {
        throw new HttpError(409, "Tag already exists");
      }

      const stub = kingdomStub(env);
      await stub.fetch("https://do/set-alliance", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ playerId: player.id, allianceId: id }),
      });

      return json({ ok: true, alliance: { id, name, tag, leaderPlayerId: player.id } });
    }

    // Alliance join
    if (path === "/v1/alliance/join" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (player.alliance_id) throw new HttpError(400, "Already in an alliance");
      const body = await readJson<{ allianceId: string }>(request);
      const a = await env.DB.prepare("SELECT * FROM alliances WHERE id = ?")
        .bind(body.allianceId)
        .first();
      if (!a) throw new HttpError(404, "Alliance not found");
      const count = await env.DB.prepare(
        "SELECT COUNT(*) as c FROM alliance_members WHERE alliance_id = ?",
      ).bind(body.allianceId).first<{ c: number }>();
      if ((count?.c || 0) >= ALLIANCE_CONSTANTS.maxMembers) throw new HttpError(400, "alliance_full");
      await env.DB.prepare(`UPDATE players SET alliance_id=? WHERE id=?`)
        .bind(body.allianceId, player.id)
        .run();
      try {
        await env.DB.prepare(
          `INSERT INTO alliance_members (alliance_id, player_id, rank, joined_at) VALUES (?, ?, 'R1', ?)`,
        ).bind(body.allianceId, player.id, nowMs()).run();
      } catch {
        // الجدول قد لا يكون مُرحّلاً بعد
      }

      const stub = kingdomStub(env);
      await stub.fetch("https://do/set-alliance", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ playerId: player.id, allianceId: body.allianceId }),
      });
      return json({ ok: true, allianceId: body.allianceId });
    }

    // Alliance Rally status (قبل المسار العام /v1/alliance/:id حتى لا يبتلعه)
    if (path.startsWith("/v1/alliance/rally/") && request.method === "GET") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      const id = path.split("/").pop()!;
      const rally = await env.DB.prepare("SELECT * FROM rallies WHERE id = ?").bind(id).first<any>();
      if (!rally || rally.alliance_id !== player.alliance_id) throw new HttpError(404, "rally_not_found");
      const parts = await env.DB.prepare(
        "SELECT player_id, troops_json FROM rally_participants WHERE rally_id = ?",
      ).bind(id).all<{ player_id: string; troops_json: string }>();
      return json({
        rally,
        participants: (parts.results || []).map((p) => ({ playerId: p.player_id, troops: JSON.parse(p.troops_json) })),
      });
    }

    // Alliance get — يشمل الرتب (P2-T5)
    if (path.startsWith("/v1/alliance/") && request.method === "GET") {
      const id = path.split("/").pop()!;
      const a = await env.DB.prepare("SELECT * FROM alliances WHERE id = ?").bind(id).first();
      if (!a) throw new HttpError(404, "Alliance not found");
      const members = await env.DB.prepare(
        "SELECT id, name, power, region_id FROM players WHERE alliance_id = ?",
      )
        .bind(id)
        .all();
      let ranks: Record<string, string> = {};
      try {
        const rows = await env.DB.prepare(
          "SELECT player_id, rank FROM alliance_members WHERE alliance_id = ?",
        ).bind(id).all<{ player_id: string; rank: string }>();
        for (const r of rows.results || []) ranks[r.player_id] = r.rank;
      } catch {
        // الجدول قد لا يكون مُرحّلاً بعد
      }
      const list = (members.results || []).map((m: any) => ({
        ...m,
        rank: ranks[m.id] || ((a as any).leader_player_id === m.id ? "R5" : "R1"),
      }));
      return json({ alliance: a, members: list });
    }

    // Alliance Promote (P2-T5): ترقية/تنزيل رتبة عضو — يتطلب صلاحية ورتبة أعلى من الهدف
    if (path === "/v1/alliance/promote" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      const body = await readJson<{ playerId: string; rank: string }>(request);
      if (!body.playerId || !body.rank) throw new HttpError(400, "playerId and rank required");
      if (!isValidRank(body.rank)) throw new HttpError(400, "invalid_rank");
      const actorRank = await getMemberRank(env, player.id, player.alliance_id);
      if (!rankHas(actorRank, "promote")) throw new HttpError(403, "insufficient_rank");
      const target = await env.DB.prepare(
        "SELECT id, alliance_id FROM players WHERE id = ?",
      ).bind(body.playerId).first<{ id: string; alliance_id: string | null }>();
      if (!target || target.alliance_id !== player.alliance_id) throw new HttpError(404, "member_not_found");
      const targetRank = await getMemberRank(env, target.id, player.alliance_id);
      if (!canModerate(actorRank, targetRank) || !canModerate(actorRank, body.rank)) {
        throw new HttpError(403, "cannot_moderate_equal_or_higher_rank");
      }
      await env.DB.prepare(
        `INSERT INTO alliance_members (alliance_id, player_id, rank, joined_at) VALUES (?, ?, ?, ?)
         ON CONFLICT(alliance_id, player_id) DO UPDATE SET rank=excluded.rank`,
      ).bind(player.alliance_id, target.id, body.rank, nowMs()).run();
      return json({ ok: true, playerId: target.id, rank: body.rank });
    }

    // Alliance Kick (P2-T5): طرد عضو — رتبة أعلى فقط تطرد أدنى منها
    if (path === "/v1/alliance/kick" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      const body = await readJson<{ playerId: string }>(request);
      if (!body.playerId) throw new HttpError(400, "playerId required");
      const actorRank = await getMemberRank(env, player.id, player.alliance_id);
      if (!rankHas(actorRank, "kick")) throw new HttpError(403, "insufficient_rank");
      const target = await env.DB.prepare(
        "SELECT id, alliance_id FROM players WHERE id = ?",
      ).bind(body.playerId).first<{ id: string; alliance_id: string | null }>();
      if (!target || target.alliance_id !== player.alliance_id) throw new HttpError(404, "member_not_found");
      const targetRank = await getMemberRank(env, target.id, player.alliance_id);
      if (!canModerate(actorRank, targetRank)) throw new HttpError(403, "cannot_kick_equal_or_higher_rank");
      await env.DB.batch([
        env.DB.prepare("UPDATE players SET alliance_id=NULL WHERE id=?").bind(target.id),
        env.DB.prepare("DELETE FROM alliance_members WHERE alliance_id=? AND player_id=?")
          .bind(player.alliance_id, target.id),
      ]);
      const stub = kingdomStub(env);
      await stub.fetch("https://do/set-alliance", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ playerId: target.id, allianceId: null }),
      });
      return json({ ok: true, kicked: target.id });
    }

    // Alliance Leave (P2-T5): مغادرة طوعية — القائد (R5) لا يغادر قبل نقل القيادة
    if (path === "/v1/alliance/leave" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      const rank = await getMemberRank(env, player.id, player.alliance_id);
      if (rank === "R5") throw new HttpError(400, "leader_must_transfer_first");
      const allianceId = player.alliance_id;
      await env.DB.batch([
        env.DB.prepare("UPDATE players SET alliance_id=NULL WHERE id=?").bind(player.id),
        env.DB.prepare("DELETE FROM alliance_members WHERE alliance_id=? AND player_id=?")
          .bind(allianceId, player.id),
      ]);
      const stub = kingdomStub(env);
      await stub.fetch("https://do/set-alliance", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ playerId: player.id, allianceId: null }),
      });
      return json({ ok: true, left: allianceId });
    }

    // Alliance Help (P2-T5): كل مساعدة تسرّع طابور عضو حسب قواعد data/zones.json
    if (path === "/v1/alliance/help" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      const body = await readJson<{ queueId: string }>(request);
      if (!body.queueId) throw new HttpError(400, "queueId required");

      // الطابور الهدف يجب أن يكون لعضو في نفس التحالف (غير المساعد نفسه)
      const stub0 = kingdomStub(env);
      const snap = await (await stub0.fetch("https://do/snapshot")).json<any>();
      const q = (snap.queues || []).find((x: any) => x.id === body.queueId && x.state === "running");
      if (!q) throw new HttpError(404, "queue_not_found");
      if (q.playerId === player.id) throw new HttpError(400, "cannot_help_own_queue");
      const owner = await env.DB.prepare("SELECT alliance_id FROM players WHERE id = ?")
        .bind(q.playerId).first<{ alliance_id: string | null }>();
      if (!owner || owner.alliance_id !== player.alliance_id) throw new HttpError(403, "not_same_alliance");

      // مساعدة واحدة لكل لاعب على الطابور + سقف العدد
      try {
        await env.DB.prepare(
          `INSERT INTO alliance_helps (queue_id, helper_player_id, created_at) VALUES (?, ?, ?)`,
        ).bind(body.queueId, player.id, nowMs()).run();
      } catch {
        throw new HttpError(409, "already_helped");
      }
      const cnt = await env.DB.prepare(
        "SELECT COUNT(*) as c FROM alliance_helps WHERE queue_id = ?",
      ).bind(body.queueId).first<{ c: number }>();
      const helpsCount = cnt?.c || 1;

      const remainingMs = Math.max(0, q.etaMs - nowMs());
      const sec = helpSpeedupSec(remainingMs, helpsCount);
      // التخفيض التراكمي: الفرق بين ما تستحقه المساعدات الحالية وما طُبّق سابقاً
      const prevSec = helpSpeedupSec(remainingMs, helpsCount - 1);
      const deltaSec = Math.max(0, sec - prevSec);

      let queue = null;
      if (deltaSec > 0) {
        const stub = kingdomStub(env);
        const res = await stub.fetch("https://do/queue/speedup", {
          method: "POST",
          headers: { "content-type": "application/json" },
          body: JSON.stringify({ queueId: body.queueId, seconds: deltaSec }),
        });
        const data = await res.json<any>();
        if (!res.ok) throw new HttpError(res.status, data.error || "help_failed", data);
        queue = data.queue;
      }
      return json({
        ok: true,
        queueId: body.queueId,
        helpsCount,
        capped: helpsCapped(helpsCount),
        speedupSec: deltaSec,
        totalReductionSec: sec,
        queue,
      });
    }

    // Alliance Rally launch (P2-T5): قائد R3+ يفتح حملة على ممر/عرش + ينضم بقواته
    if (path === "/v1/alliance/rally" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      const body = await readJson<{
        targetType: string; targetId: string; troops: Record<string, number>; primaryCommanderId?: string;
      }>(request);
      if (!body.targetType || !body.targetId) throw new HttpError(400, "targetType and targetId required");
      const rank = await getMemberRank(env, player.id, player.alliance_id);
      if (!canLaunchRally(rank, body.targetType)) {
        throw new HttpError(403, `rally requires rank ${ALLIANCE_CONSTANTS.rally.min_rank}+ and target in ${ALLIANCE_CONSTANTS.rally.allowed_targets.join("/")}`);
      }
      const troops = body.troops || {};
      const owned = await getTroopsMap(env, player.id);
      let total = 0;
      for (const [u, c] of Object.entries(troops)) {
        if ((owned[u] || 0) < Number(c)) throw new HttpError(400, `Not enough ${u}`);
        total += Number(c);
      }
      if (total <= 0) throw new HttpError(400, "no_troops");

      let commanderSkills: number[] | undefined;
      if (body.primaryCommanderId) {
        const cmd = await getOwnedCommander(env, player.id, String(body.primaryCommanderId));
        if (!cmd) throw new HttpError(400, "Commander not owned");
        commanderSkills = JSON.parse(cmd.skills_json || "[1,1,1]");
      }

      // نقل قوات القائد إلى marching
      for (const [u, c] of Object.entries(troops)) {
        const left = (owned[u] || 0) - Number(c);
        await env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=excluded.count`,
        ).bind(player.id, u, left).run();
        await env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'marching', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`,
        ).bind(player.id, u, Number(c)).run();
      }

      const id = newId("rally");
      const now = nowMs();
      await env.DB.prepare(
        `INSERT INTO rallies (id, alliance_id, leader_player_id, target_type, target_id, status, start_ms, launch_ms, commander_id, commander_skills_json, created_at)
         VALUES (?, ?, ?, ?, ?, 'forming', ?, ?, ?, ?, ?)`,
      ).bind(
        id, player.alliance_id, player.id, body.targetType, body.targetId,
        now, now + rallyPrepMs(),
        body.primaryCommanderId || null,
        commanderSkills ? JSON.stringify(commanderSkills) : null,
        now,
      ).run();
      await env.DB.prepare(
        `INSERT INTO rally_participants (rally_id, player_id, troops_json, joined_at) VALUES (?, ?, ?, ?)`,
      ).bind(id, player.id, JSON.stringify(troops), now).run();

      return json({ ok: true, rally: { id, targetType: body.targetType, targetId: body.targetId, launchMs: now + rallyPrepMs(), status: "forming" } });
    }

    // Alliance Rally join (P2-T5): عضو ينضم بقواته حتى اكتمال العدد
    if (path === "/v1/alliance/rally/join" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      const body = await readJson<{ rallyId: string; troops: Record<string, number> }>(request);
      if (!body.rallyId) throw new HttpError(400, "rallyId required");
      const rally = await env.DB.prepare("SELECT * FROM rallies WHERE id = ?")
        .bind(body.rallyId).first<any>();
      if (!rally || rally.alliance_id !== player.alliance_id) throw new HttpError(404, "rally_not_found");
      if (rally.status !== "forming") throw new HttpError(400, "rally_not_forming");
      const existing = await env.DB.prepare(
        "SELECT COUNT(*) as c FROM rally_participants WHERE rally_id = ?",
      ).bind(body.rallyId).first<{ c: number }>();
      if (rallyFull(existing?.c || 0)) throw new HttpError(400, "rally_full");
      const dup = await env.DB.prepare(
        "SELECT 1 as x FROM rally_participants WHERE rally_id = ? AND player_id = ?",
      ).bind(body.rallyId, player.id).first();
      if (dup) throw new HttpError(409, "already_joined");

      const troops = body.troops || {};
      const owned = await getTroopsMap(env, player.id);
      let total = 0;
      for (const [u, c] of Object.entries(troops)) {
        if ((owned[u] || 0) < Number(c)) throw new HttpError(400, `Not enough ${u}`);
        total += Number(c);
      }
      if (total <= 0) throw new HttpError(400, "no_troops");
      for (const [u, c] of Object.entries(troops)) {
        const left = (owned[u] || 0) - Number(c);
        await env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=excluded.count`,
        ).bind(player.id, u, left).run();
        await env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'marching', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`,
        ).bind(player.id, u, Number(c)).run();
      }
      await env.DB.prepare(
        `INSERT INTO rally_participants (rally_id, player_id, troops_json, joined_at) VALUES (?, ?, ?, ?)`,
      ).bind(body.rallyId, player.id, JSON.stringify(troops), nowMs()).run();
      return json({ ok: true, rallyId: body.rallyId, participants: (existing?.c || 0) + 1 });
    }

    // Alliance flag build
    if (path === "/v1/alliance/flag/build" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      const body = await readJson<{ x: number; y: number }>(request);
      
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/build-flag", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ allianceId: player.alliance_id, x: body.x, y: body.y })
      });
      const data = await res.json<any>();
      if (!res.ok) throw new HttpError(res.status, data.error || "build_failed", data);
      return json(data);
    }

    // World snapshot
    if (path === "/v1/world/snapshot" && request.method === "GET") {
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/snapshot");
      const data = await res.json();
      return json(data);
    }

    // Season Leaderboard
    if (path === "/v1/season/leaderboard" && request.method === "GET") {
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/leaderboard");
      const data = await res.json();
      return json(data);
    }

    // P3-T1: جدول فتح الموسم الكامل على السيرفر (Zone unlock service)
    if (path === "/v1/season/schedule" && request.method === "GET") {
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/season/schedule");
      const data = await res.json();
      return json(data);
    }

    // P3-T2: لوحة نقاط الموسم — أهداف قلب Zone 3 + النقاط + المتصدر
    if (path === "/v1/season/scoreboard" && request.method === "GET") {
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/season/scoreboard");
      const data = await res.json();
      return json(data);
    }

    // P3-T3: الأحداث النشطة والمجدولة اليوم (barbarians / resource_rush / war_fever)
    if (path === "/v1/events/active" && request.method === "GET") {
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/events");
      const data = await res.json();
      return json(data);
    }

    // P2-T4: حالة فتح/قفل المناطق (مؤقت Zone 2 stubs)
    if (path === "/v1/world/zones" && request.method === "GET") {
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/zones-status");
      const data = await res.json();
      return json(data);
    }

    // World WS proxy
    if (path === "/v1/world/ws" && request.headers.get("Upgrade") === "websocket") {
      const token = url.searchParams.get("token") || request.headers.get("Sec-WebSocket-Protocol");
      if (token) {
        await verifyToken(token, env.AUTH_SECRET);
      } else {
        await requireAuth(request, env);
      }
      const stub = kingdomStub(env);
      return stub.fetch("https://do/ws", request);
    }

    // March REST
    if (path === "/v1/world/march" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<any>(request);
      // validate troops ownership
      const owned = await getTroopsMap(env, player.id);
      const troops = body.troops || {};
      for (const [u, c] of Object.entries(troops)) {
        if ((owned[u] || 0) < Number(c)) throw new HttpError(400, `Not enough ${u}`);
      }
      // P2-T1: القائد المرافق (اختياري) — تحقق من الملكية
      let commanderSkills: number[] | undefined;
      if (body.primaryCommanderId) {
        const cmd = await getOwnedCommander(env, player.id, String(body.primaryCommanderId));
        if (!cmd) throw new HttpError(400, "Commander not owned");
        commanderSkills = JSON.parse(cmd.skills_json || "[1,1,1]");
      }
      // deduct temporarily (prototype: permanent send)
      for (const [u, c] of Object.entries(troops)) {
        const left = (owned[u] || 0) - Number(c);
        await env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=excluded.count`,
        )
          .bind(player.id, u, left)
          .run();
        await env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'marching', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`,
        )
          .bind(player.id, u, Number(c))
          .run();
      }

      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/march", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({
          playerId: player.id,
          troops,
          targetType: body.targetType,
          targetId: body.targetId,
          passId: body.passId,
          toX: body.toX,
          toY: body.toY,
          primaryCommanderId: body.primaryCommanderId,
          commanderSkills,
        }),
      });
      const data = await res.json<any>();
      if (!res.ok) throw new HttpError(res.status, data.error || "march_failed", data);
      // P4-T1: نقاط Battle Pass عن المسيرة
      await grantBpXp(env, player.id, "march");
      return json(data);
    }

    // Pass attack REST
    if (path === "/v1/world/pass/attack" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Join an alliance before capturing passes");
      const body = await readJson<{ passId: string; troops: Record<string, number>; primaryCommanderId?: string }>(request);
      const owned = await getTroopsMap(env, player.id);
      const troops = body.troops || {};
      for (const [u, c] of Object.entries(troops)) {
        if ((owned[u] || 0) < Number(c)) throw new HttpError(400, `Not enough ${u}`);
      }
      // P2-T1: القائد المرافق (اختياري)
      let commanderSkills: number[] | undefined;
      if (body.primaryCommanderId) {
        const cmd = await getOwnedCommander(env, player.id, String(body.primaryCommanderId));
        if (!cmd) throw new HttpError(400, "Commander not owned");
        commanderSkills = JSON.parse(cmd.skills_json || "[1,1,1]");
      }
      for (const [u, c] of Object.entries(troops)) {
        const left = (owned[u] || 0) - Number(c);
        await env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=excluded.count`,
        )
          .bind(player.id, u, left)
          .run();
        await env.DB.prepare(
          `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'marching', ?)
           ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=count+excluded.count`,
        )
          .bind(player.id, u, Number(c))
          .run();
      }

      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/pass-attack", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({
          playerId: player.id,
          passId: body.passId,
          troops,
          primaryCommanderId: body.primaryCommanderId,
          commanderSkills,
        }),
      });
      const data = await res.json<any>();
      if (!res.ok) throw new HttpError(res.status, data.error || "attack_failed", data);
      // P4-T1: نقاط Battle Pass عن هجوم الممر
      await grantBpXp(env, player.id, "pass_attack");
      return json(data);
    }

    // Admin
    if (path === "/v1/admin/tick" && request.method === "POST") {
      requireAdmin(request, env);
      const body = await readJson<{ force?: boolean }>(request).catch(() => ({ force: true }));
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/admin", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ action: "tick", force: body.force !== false }),
      });
      return json(await res.json());
    }

    if (path === "/v1/admin/set-time" && request.method === "POST") {
      requireAdmin(request, env);
      const body = await readJson<{ day: number }>(request);
      const stub = kingdomStub(env);
      const res = await stub.fetch("https://do/admin", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ action: "set_day", day: body.day }),
      });
      return json(await res.json());
    }

    if (path === "/v1/admin/grant" && request.method === "POST") {
      requireAdmin(request, env);
      const body = await readJson<{
        playerId: string;
        food?: number;
        wood?: number;
        stone?: number;
        gold?: number;
        troops?: Record<string, number>;
      }>(request);
      if (!body.playerId) throw new HttpError(400, "playerId required");
      const city = await refreshCity(env, body.playerId);
      await env.DB.prepare(
        `UPDATE cities SET food=food+?, wood=wood+?, stone=stone+?, gold=gold+?, updated_at=? WHERE player_id=?`,
      )
        .bind(body.food || 0, body.wood || 0, body.stone || 0, body.gold || 0, nowMs(), body.playerId)
        .run();
      if (body.troops) {
        const owned = await getTroopsMap(env, body.playerId);
        for (const [u, c] of Object.entries(body.troops)) {
          const next = (owned[u] || 0) + Number(c);
          await env.DB.prepare(
            `INSERT INTO troops (player_id, unit_id, status, count) VALUES (?, ?, 'home', ?)
             ON CONFLICT(player_id, unit_id, status) DO UPDATE SET count=excluded.count`,
          )
            .bind(body.playerId, u, next)
            .run();
        }
      }
      return json({
        ok: true,
        city: await refreshCity(env, body.playerId),
        troops: await getTroopsMap(env, body.playerId),
      });
    }

    // P3-T5: قياس retention — DAU + رجوع cohorts عند الأيام المحددة في data/softlaunch.json
    if (path === "/v1/admin/retention" && request.method === "GET") {
      requireAdmin(request, env);
      const today = retentionUtcDay(nowMs());
      const buckets = retentionDayBuckets();
      const targets = retentionTargets();
      const activeThreshold = getSoftLaunch().retention.active_threshold_days;

      // DAU: لاعبون نشطوا خلال آخر active_threshold_days يوم
      const dauRow = await env.DB.prepare(
        "SELECT COUNT(DISTINCT player_id) as c FROM player_activity WHERE day >= ?",
      ).bind(today - (activeThreshold - 1)).first<{ c: number }>().catch(() => null);

      // cohorts: حجم كل يوم إنشاء + عدد العائدين منه عند كل bucket
      const cohortsRows = await env.DB.prepare(
        `SELECT p.created_at as created_ms, p.id as pid,
                (SELECT MAX(day) FROM player_activity a WHERE a.player_id = p.id) as last_day,
                (SELECT MIN(day) FROM player_activity a WHERE a.player_id = p.id) as first_active_day
         FROM players p`,
      ).all<{ created_ms: number; pid: string; last_day: number | null; first_active_day: number | null }>()
        .catch(() => ({ results: [] as any[] }));

      type CohortAgg = { size: number; returned: Record<number, number> };
      const byCohort = new Map<number, CohortAgg>();
      for (const r of cohortsRows.results || []) {
        const cDay = cohortDayOf(r.created_ms);
        if (!byCohort.has(cDay)) byCohort.set(cDay, { size: 0, returned: {} });
        const agg = byCohort.get(cDay)!;
        agg.size++;
        const lastDay = r.last_day ?? -1;
        for (const n of buckets) {
          if (cDay + n <= today && lastDay >= cDay + n) {
            agg.returned[n] = (agg.returned[n] || 0) + 1;
          }
        }
      }

      const cohorts = [...byCohort.entries()]
        .sort((a, b) => b[0] - a[0])
        .slice(0, 30)
        .map(([day, agg]) => ({
          cohort_day: day,
          size: agg.size,
          retention: Object.fromEntries(
            buckets
              .filter((n) => day + n <= today)
              .map((n) => [`d${n}`, pct(agg.returned[n] || 0, agg.size)]),
          ),
        }));

      return json({
        ok: true,
        today_utc_day: today,
        dau: dauRow?.c ?? 0,
        buckets,
        targets,
        cohorts,
        tracked_players: (cohortsRows.results || []).length,
      });
    }

    // P3-T5: حالة Soft launch — الممالك المفتوحة + إشغالها (للاعبين والمشرفين)
    if (path === "/v1/launch/status" && request.method === "GET") {
      const spec = getSoftLaunch();
      const cnt = await env.DB.prepare("SELECT COUNT(*) as c FROM players")
        .first<{ c: number }>().catch(() => null);
      const kingdomId = env.KINGDOM_ID || "kingdom-1";
      return json({
        ok: true,
        kingdom: kingdomId,
        players: cnt?.c ?? 0,
        kingdoms: spec.kingdoms,
        current: spec.kingdoms.find((k: any) => k.id === kingdomId) || null,
        success_gate: spec.success_gate,
      });
    }

    return json({ error: "Not found", path }, 404);
  } catch (err: any) {
    if (err instanceof HttpError) {
      return json({ error: err.message, details: err.details }, err.status);
    }
    console.error(err);
    return json({ error: err?.message || "Internal error" }, 500);
  }
}
