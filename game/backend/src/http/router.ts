import type { Env, PlayerRow, CityRow } from "../env";
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
  getTechTree,
  starterBuildings,
  upgradeCost,
  trainCost,
  unitPower,
} from "../lib/gameData";
import { applyProduction, canAfford, spend } from "../do/sim/production";
import { TECHNOLOGIES } from "../do/sim/research";
import { healCost, healDurationSec, hospitalCapacity } from "../do/sim/hospital";
import {
  addXp,
  commanderPassiveMod,
  COMMANDER_CONSTANTS,
  getCommanderDef,
  isValidCommander,
  starterCommanderForCiv,
  xpForLevel,
} from "../do/sim/commanders";

function kingdomStub(env: Env) {
  return env.KINGDOM_SHARD.get(env.KINGDOM_SHARD.idFromName(env.KINGDOM_ID || "kingdom-1"));
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
  const buildings = await getBuildingsMap(env, playerId);
  const now = nowMs();
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
    // P1-T6: endpoint موحد لكل بيانات التوازن — يقرأها العميل مرة واحدة عند البدء
    if (path === "/v1/meta/all" && request.method === "GET") {
      return json({
        version: 1,
        civilizations: getCivilizations(),
        buildings: getBuildings(),
        troops: getTroops(),
        commanders: getCommanders(),
        techTree: getTechTree(),
        constants: {
          productionBase: { farm: 100, lumber_mill: 100, quarry: 70, goldmine: 40 },
          productionLevelMult: 1.2,
          commanders: COMMANDER_CONSTANTS,
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

      const duration = 10 * count;
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
      return json({ ok: true, unit, count: count, queueId, city, troops: all });
    }

    // Collect / refresh
    if (path === "/v1/city/collect" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const city = await refreshCity(env, player.id);
      return json({ ok: true, city });
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
      return json({ ok: true, queueId, healSeconds: duration, cost, city });
    }

    // Research
    if (path === "/v1/city/research" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      const body = await readJson<{ techId: string }>(request);
      if (!body.techId) throw new HttpError(400, "techId required");

      const tech = TECHNOLOGIES[body.techId];
      if (!tech) throw new HttpError(404, "Technology not found");

      let city = await refreshCity(env, player.id);
      const buildings = await getBuildingsMap(env, player.id);
      const academyLvl = buildings["academy"] || 0;

      // Dummy check for current tech level. (Usually from DB, assume 0 for now)
      const currentTechLevel = 0; // Prototype stub
      const nextLevel = currentTechLevel + 1;
      
      if (nextLevel > tech.maxLevel) throw new HttpError(400, "Max tech level reached");
      if (academyLvl < tech.academyLevelReq(nextLevel)) throw new HttpError(400, "Academy level too low");

      const cost = tech.cost(nextLevel);
      if (!canAfford(city, cost)) throw new HttpError(400, "Not enough resources");

      const spent = spend(city, cost);
      await env.DB.prepare(
        `UPDATE cities SET food=?, wood=?, stone=?, gold=?, updated_at=? WHERE player_id=?`,
      ).bind(spent.food, spent.wood, spent.stone, spent.gold, nowMs(), player.id).run();

      const duration = tech.duration(nextLevel);
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
      return json({ ok: true, techId: body.techId, level: nextLevel, queueId, city });
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
      await env.DB.prepare(`UPDATE players SET alliance_id=? WHERE id=?`)
        .bind(body.allianceId, player.id)
        .run();

      const stub = kingdomStub(env);
      await stub.fetch("https://do/set-alliance", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ playerId: player.id, allianceId: body.allianceId }),
      });
      return json({ ok: true, allianceId: body.allianceId });
    }

    // Alliance get
    if (path.startsWith("/v1/alliance/") && request.method === "GET") {
      const id = path.split("/").pop()!;
      const a = await env.DB.prepare("SELECT * FROM alliances WHERE id = ?").bind(id).first();
      if (!a) throw new HttpError(404, "Alliance not found");
      const members = await env.DB.prepare(
        "SELECT id, name, power, region_id FROM players WHERE alliance_id = ?",
      )
        .bind(id)
        .all();
      return json({ alliance: a, members: members.results || [] });
    }

    // Alliance Invite
    if (path === "/v1/alliance/invite" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      return json({ ok: true, message: "Invite sent" });
    }

    // Alliance Promote
    if (path === "/v1/alliance/promote" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      return json({ ok: true, message: "Promoted member" });
    }

    // Alliance Kick
    if (path === "/v1/alliance/kick" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      return json({ ok: true, message: "Kicked member" });
    }

    // Alliance Help
    if (path === "/v1/alliance/help" && request.method === "POST") {
      const { player } = await requirePlayer(request, env);
      if (!player.alliance_id) throw new HttpError(400, "Not in an alliance");
      return json({ ok: true, message: "Help sent" });
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

    return json({ error: "Not found", path }, 404);
  } catch (err: any) {
    if (err instanceof HttpError) {
      return json({ error: err.message, details: err.details }, err.status);
    }
    console.error(err);
    return json({ error: err?.message || "Internal error" }, 500);
  }
}
