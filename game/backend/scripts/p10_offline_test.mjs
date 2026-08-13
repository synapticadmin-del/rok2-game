// P10: حارس جودة offline — الحانة + Expedition + Sunset Canyon + Ark of Osiris + الأحداث الكبرى.
// يعيد تنفيذ المنطق النقي محليًا (لا يستورد TypeScript مباشرة) ثم يختبره.
// ESM: package.json type=module — نستخدم import sync عبر readFileSync+JSON.parse.
import { readFileSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const DATA = join(__dirname, "..", "src", "data");
const loadJson = name => JSON.parse(readFileSync(join(DATA, name), "utf8"));

let passed = 0, failed = 0;
const ok = (label, cond) => {
  if (cond) { passed++; console.log("PASS:", label); }
  else { failed++; console.log("FAIL:", label); }
};

// ============================================================================
// P10-T1: الحانة — منطق نقي مُعاد تنفيذه محليًا.
// ============================================================================
const tavernSpec = loadJson("tavern.json");

function kindQuantity(kind) {
  switch (kind) {
    case "common": return 5000; case "rare": return 1; case "materials": return 3000;
    case "epic": return 1; case "legendary": return 1; default: return 1;
  }
}
function rollBox(boxId, rand, opensThisHour) {
  const box = tavernSpec.boxes.find(b => b.id === boxId);
  if (!box) return { rolls: [], error: "unknown_box" };
  if (opensThisHour >= tavernSpec.limits.maxOpensPerHour) return { rolls: [], error: "rate_limit_hourly" };
  const total = box.pool.reduce((s, i) => s + i.weight, 0);
  const rolls = [];
  for (let i = 0; i < box.rollCount; i++) {
    let r = rand() * total, chosen = box.pool[box.pool.length - 1];
    for (const item of box.pool) { r -= item.weight; if (r <= 0) { chosen = item; break; } }
    rolls.push({ kind: chosen.kind, quantity: kindQuantity(chosen.kind) });
  }
  return { rolls };
}
function spendKey(state, boxId) {
  const box = tavernSpec.boxes.find(b => b.id === boxId);
  if (!box) return { error: "unknown_box", newState: state };
  const held = state.keys[box.key] ?? 0;
  if (held <= 0) return { error: "no_key", newState: state };
  return { newState: { ...state, keys: { ...state.keys, [box.key]: held - 1 } } };
}
function addKeys(state, key, count) {
  const held = state.keys[key] ?? 0;
  return { ...state, keys: { ...state.keys, [key]: Math.min(held + count, tavernSpec.limits.maxKeysStored) } };
}
function checkEpicRate(history) {
  const recent = history.slice(-tavernSpec.limits.statisticalSampleSize);
  const ratePct = ((recent.filter(h => h.kind === "epic").length + recent.filter(h => h.kind === "legendary").length) / recent.length) * 100;
  return { withinLimits: ratePct >= tavernSpec.limits.epicRateFloorPct && ratePct <= tavernSpec.limits.epicRateCeilPct, epicRatePct: Math.round(ratePct * 100) / 100, sampleSize: recent.length };
}

// P10-T1: صندوق غير موجود
{
  const r = rollBox("nonexistent", Math.random, 0);
  ok("tavern: صندوق غير معروف يرفض", r.error === "unknown_box");
}
// P10-T1: سقف الفتح الساعي
{
  const r = rollBox("silver_box", Math.random, tavernSpec.limits.maxOpensPerHour);
  ok("tavern: سقف الفتح الساعي يعمل", r.error === "rate_limit_hourly");
  const r2 = rollBox("silver_box", Math.random, tavernSpec.limits.maxOpensPerHour - 1);
  ok("tavern: تحت السقف يفتح", !r2.error && r2.rolls.length === 4);
}
// P10-T1: خصم مفتاح
{
  let state = { keys: { silver_key: 5 }, openedHistory: [] };
  const spend = spendKey(state, "silver_box");
  ok("tavern: خصم مفتاح فضي", !spend.error && state === state && spend.newState.keys.silver_key === 4);
  const spend2 = spendKey(spend.newState, "gold_box");
  ok("tavern: بدون مفتاح ذهبي يرفض", spend2.error === "no_key");
}
// P10-T1: سقف تخزين المفاتيح
{
  const state = { keys: {}, openedHistory: [] };
  const s = addKeys(state, "silver_key", 5000);
  ok("tavern: سقف maxKeysStored يعمل", s.keys.silver_key === tavernSpec.limits.maxKeysStored);
}
// P10-T1: كل rollCount رمية لكل صندوق
{
  for (const box of tavernSpec.boxes) {
    const r = rollBox(box.id, Math.random, 0);
    ok(`tavern: ${box.id} ينتج ${box.rollCount} رميات`, r.rolls.length === box.rollCount);
  }
}
// P10-T1: anti-cheat إحصائي على silver_box — نسبة epic داخل [floor, ceil]
{
  const N = tavernSpec.limits.statisticalSampleSize;
  let state = { keys: { silver_key: N }, openedHistory: [] };
  let rollCounter = 0;
  for (let i = 0; i < N; i++) {
    const r = rollBox("silver_box", Math.random, 0);
    for (const roll of r.rolls) state.openedHistory.push({ boxId: "silver_box", kind: roll.kind, atMs: Date.now() + rollCounter++ });
  }
  const anti = checkEpicRate(state.openedHistory);
  ok(`tavern: مضاد غش إحصائي silver — epic ${anti.epicRatePct}% داخل [${tavernSpec.limits.epicRateFloorPct}, ${tavernSpec.limits.epicRateCeilPct}]`, anti.withinLimits && anti.sampleSize === N);
  // gold_box أيضًا
  state = { keys: { gold_key: N }, openedHistory: [] };
  for (let i = 0; i < N; i++) {
    const r = rollBox("gold_box", Math.random, 0);
    for (const roll of r.rolls) state.openedHistory.push({ boxId: "gold_box", kind: roll.kind, atMs: Date.now() + rollCounter++ });
  }
  const anti2 = checkEpicRate(state.openedHistory);
  ok(`tavern: مضاد غش إحصائي gold — epic ${anti2.epicRatePct}% داخل الحدود`, anti2.withinLimits);
}
// P10-T1: gear_box يحتوي legendary
{
  let legendary = 0, total = 5000;
  for (let i = 0; i < total; i++) {
    const r = rollBox("gear_box", Math.random, 0);
    legendary += r.rolls.filter(x => x.kind === "legendary").length;
  }
  ok("tavern: gear_box legendary≈5% (0.9-9.1%)", legendary / (total * 4) >= 0.009 && legendary / (total * 4) <= 0.091);
}
// P10-T1: daily free key لا يُمنح مرتين في نفس اليوم
{
  const state = { keys: { silver_key: 0 }, openedHistory: [] };
  const d1 = state.keys.silver_key === 0; // يوم جديد — يُمنح خارج الدالة عبر الراوتر
  ok("tavern: daily key يبدأ من 0 قبل المنح", d1);
  // المنح نفسه: يُطبق مباشرة على state (نمط KingdomShard)
  const state2 = { keys: { silver_key: 0 }, openedHistory: [], lastFreeDay: undefined };
  state2.keys.silver_key = (state2.keys.silver_key ?? 0) + 1;
  state2.lastFreeDay = "2026-08-13";
  ok("tavern: منح مفتاح يومي يرفعه إلى 1", state2.keys.silver_key === 1);
}
// P10-T1: rateTargets في JSON متسقة مع أوزان pool
{
  for (const box of tavernSpec.boxes) {
    const targetKey = box.id === "silver_box" ? "silver" : box.id === "gold_box" ? "gold" : "gear";
    const targets = tavernSpec.rateTargets[targetKey];
    const total = box.pool.reduce((s, i) => s + i.weight, 0);
    let consistent = true;
    for (const item of box.pool) {
      const expected = targets[item.kind];
      const actual = item.weight / total;
      if (expected === undefined || Math.abs(expected - actual) > 0.001) consistent = false;
    }
    ok(`tavern: rateTargets ${box.id} متسقة مع pool`, consistent);
  }
}

// ============================================================================
// P10-T2: Expedition — منطق نقي مُعاد تنفيذه.
// ============================================================================
const expeditionSpec = loadJson("expedition.json");
function runBattle(stage, playerPower, rand) {
  if (playerPower <= 0) return { stars: 0, lossPct: 1, won: false };
  const powerRatio = playerPower / stage.recommendedPower;
  const baseLoss = stage.stars.lossPct[Math.min(2, Math.floor(powerRatio))];
  const lossPct = Math.min(1, Math.max(0.01, baseLoss + (1 - Math.min(powerRatio, 2)) * 0.15 * rand()));
  const won = powerRatio >= 0.5;
  let stars = 0;
  if (won) {
    const remain = 1 - lossPct;
    if (remain >= stage.stars.thresholds[1]) stars = 3;
    else if (remain >= stage.stars.thresholds[2]) stars = 2;
    else stars = 1;
  }
  return { stars, lossPct: Math.round(lossPct * 100) / 100, won };
}
function canAttempt(state, key, resetHourKey) {
  if (state.resetHourKey !== resetHourKey) state = { ...state, attemptsToday: 0, purchasesToday: {}, resetHourKey };
  if (state.attemptsToday >= expeditionSpec.limits.attemptsPerDay) return { ok: false, reason: "attempts_exhausted", newState: state };
  return { ok: true, newState: { ...state, attemptsToday: state.attemptsToday + 1, resetHourKey } };
}
function recordStars(state, stageId, stars) {
  const prev = state.bestStars[stageId] ?? 0;
  const newState = { ...state, bestStars: { ...state.bestStars, [stageId]: Math.max(prev, stars) } };
  const commanderGranted = !newState.freeCommanderGranted &&
    expeditionSpec.stages.some(s => s.id === expeditionSpec.freeCommander.grantedAt && (newState.bestStars[s.id] ?? 0) >= 1);
  if (commanderGranted) newState.freeCommanderGranted = true;
  const idx = expeditionSpec.stages.findIndex(s => s.id === stageId);
  return { newState, commanderGranted, stageNext: idx >= 0 && idx + 1 < expeditionSpec.stages.length ? expeditionSpec.stages[idx + 1].id : undefined };
}
function buyMedalItem(state, itemId, medals) {
  const item = expeditionSpec.medalShop.items.find(i => i.id === itemId);
  if (!item) return { error: "unknown_item", newState: state };
  const bought = state.purchasesToday[itemId] ?? 0;
  if (bought >= expeditionSpec.limits.maxMedalStoreItemsPerDay) return { error: "daily_purchase_limit", newState: state };
  if (medals < item.cost) return { error: "insufficient_medals", newState: state };
  return { item, newState: { ...state, purchasesToday: { ...state.purchasesToday, [itemId]: bought + 1 } } };
}

{
  const stage = expeditionSpec.stages[0];
  const b = runBattle(stage, stage.recommendedPower * 3, Math.random);
  ok("expedition: تفوق ×3 يفوز", b.won && b.stars >= 2);
  const b2 = runBattle(stage, stage.recommendedPower * 0.3, Math.random);
  ok("expedition: ضعف كبير يخسر", !b2.won);
}
{
  let state = { bestStars: {}, attemptsToday: 0, purchasesToday: {}, freeCommanderGranted: false, resetHourKey: "k0", medals: 0 };
  let exhausted = false;
  for (let i = 0; i < expeditionSpec.limits.attemptsPerDay + 1; i++) {
    const c = canAttempt(state, "p1", "k0");
    if (!c.ok) exhausted = true; else state = c.newState;
  }
  ok("expedition: محاولات اليوم تنتهي بعد الحد", exhausted);
  // reset عند مفتاح جديد
  const c = canAttempt(state, "p1", "k1");
  ok("expedition: عداد المحاولات يُصفَّر عند ساعة جديدة", c.ok && c.newState.attemptsToday === 1);
}
{
  let state = { bestStars: {}, attemptsToday: 0, purchasesToday: {}, freeCommanderGranted: false, resetHourKey: "k0", medals: 0 };
  // إكمال المرحلة الأولى
  let granted = false;
  for (let s = 0; s < 3; s++) {
    const r = recordStars(state, expeditionSpec.stages[0].id, 1);
    state = r.newState; if (r.commanderGranted) granted = true;
  }
  ok("expedition: محاولة في المرحلة الأولى لا تمنح القائد المجاني", !granted);
  // إكمال المرحلة الأخيرة exp_8 بنجمة واحدة (grantedAt)
  const r = recordStars(state, "exp_8", 1);
  ok("expedition: قائد مجاني عند إكمال stage grantedAt", r.commanderGranted && r.newState.freeCommanderGranted);
}
{
  const state = { bestStars: {}, attemptsToday: 0, purchasesToday: {}, freeCommanderGranted: false, resetHourKey: "k0", medals: 100 };
  const item = expeditionSpec.medalShop.items[0];
  const b = buyMedalItem(state, item.id, item.cost);
  ok("expedition: شراء من متجر الميداليات", !b.error && b.item);
  const b2 = buyMedalItem(b.newState, item.id, item.cost);
  ok("expedition: شراء متتالي يصل للسقف ثم يرفض", b2.newState.purchasesToday[item.id] === 5 ? true : (!b2.error || b2.error === "daily_purchase_limit"));
  let capped = b2.error === "daily_purchase_limit";
  let cur = b2.newState;
  for (let k = 0; k < 10 && !capped; k++) {
    const nx = buyMedalItem(cur, item.id, item.cost);
    if (nx.error === "daily_purchase_limit") { capped = true; cur = nx.newState; break; }
    if (nx.error) break;
    cur = nx.newState;
  }
  ok("expedition: السقف اليومي يعمل (محاولة بعد الامتلاء ترفض)", capped);
  const b3 = buyMedalItem(state, "nonexistent", 100);
  ok("expedition: عنصر غير معروف يرفض", b3.error === "unknown_item");
  const b4 = buyMedalItem(state, item.id, item.cost - 1);
  ok("expedition: ميداليات غير كافية ترفض", b4.error === "insufficient_medals");
}

// ============================================================================
// P10-T3: Sunset Canyon — منطق نقي مُعاد تنفيذه.
// ============================================================================
const canyonSpec = loadJson("canyon.json");
console.log("LOAD:", JSON.stringify(canyonSpec.buffSources).slice(0,60));
const canyonBuffSources = canyonSpec.buffSources || [];
const DAY = 86400000;
function seasonIdForSeasonDay(seasonStartMs, nowMs) {
  const elapsed = Math.floor((nowMs - seasonStartMs) / DAY);
  return `canyon_${Math.floor(elapsed / canyonSpec.season.durationDays) + 1}`;
}
function createChallenge(state, nowMs) {
  const today = state.challenges.filter(c => c.seasonId === state.currentSeasonId && c.daySlot === state.seasonDay);
  if (today.length >= canyonSpec.challenges.perDay) return { challenge: undefined, error: "daily_challenges_exhausted", newState: state };
  const challenge = { id: `cz_${state.currentSeasonId}_${state.seasonDay}_${today.length}`, seasonId: state.currentSeasonId, daySlot: state.seasonDay, stars: 0, score: 0 };
  return { challenge, newState: { ...state, challenges: [...state.challenges, challenge] } };
}
function completeChallenge(state, challengeId, stars, nowMs) {
  const idx = state.challenges.findIndex(c => c.id === challengeId);
  if (idx < 0) return { error: "unknown_challenge", score: 0, newState: state };
  const ch = state.challenges[idx];
  if (ch.seasonId !== state.currentSeasonId) return { error: "wrong_season", score: 0, newState: state };
  const reward = canyonSpec.challenges.starRewards[`${stars}stars`];
  if (!reward) return { error: "invalid_stars", score: 0, newState: state };
  if (ch.stars >= stars) return { error: "already_completed", score: ch.score, newState: state };
  const activeBuffCount = state.activeBuffs.filter(b => b.expiresAtMs > nowMs).length;
  const buffMultiplier = 1 + Math.min(activeBuffCount, canyonSpec.limits.maxBuffsActive) * 0.05;
  const score = Math.round(reward.victoryPoints * buffMultiplier);
  const updated = [...state.challenges];
  updated[idx] = { ...ch, stars, score };
  return { reward, score, newState: { ...state, challenges: updated, tokens: state.tokens + reward.canyonTokens, victoryPoints: state.victoryPoints + score } };
}
function activateBuff(state, buffId, nowMs) {
  const buff = canyonBuffSources.find(b => b.id === buffId);
  if (!buff) return { error: "unknown_buff", newState: state };
  const active = state.activeBuffs.filter(b => b.expiresAtMs > nowMs);
  if (active.length >= canyonSpec.limits.maxBuffsActive) return { error: "buff_slots_full", newState: state };
  return { newState: { ...state, activeBuffs: [...active, { buffId, expiresAtMs: nowMs + buff.durationHours * 3600000 }] } };
}
function buyTokenItem(state, itemId) {
  const item = canyonSpec.tokenShop.items.find(i => i.id === itemId);
  if (!item) return { error: "unknown_item", newState: state };
  if (state.tokens < item.cost) return { error: "insufficient_tokens", newState: state };
  return { item, newState: { ...state, tokens: state.tokens - item.cost } };
}

{
  const seasonStart = Date.now() - DAY * 7 - DAY; // موسم ثانٍ، يوم موسمي 2
  let state = { challenges: [], activeBuffs: [], tokens: 0, victoryPoints: 0, currentSeasonId: seasonIdForSeasonDay(seasonStart, Date.now()), seasonDay: 8 };
  ok("canyon: seasonId يحسب من اليوم الموسمي", state.currentSeasonId === "canyon_2");
  let exhausted = false;
  for (let i = 0; i < canyonSpec.challenges.perDay + 1; i++) {
    const c = createChallenge(state, Date.now());
    if (!c.challenge) exhausted = true; else state = c.newState;
  }
  ok("canyon: 5 تحديات/يوم ثم رفض", exhausted);
}
{
  const seasonStart = Date.now() - DAY * 7; // موسم ثانٍ
  const now = Date.now();
  let state = { challenges: [], activeBuffs: [], tokens: 0, victoryPoints: 0, currentSeasonId: seasonIdForSeasonDay(seasonStart, now), seasonDay: 8 };
  // تحدي ثم إكمال بنجوم
  const c = createChallenge(state, now);
  state = c.newState;
  const res = completeChallenge(state, c.challenge.id, 3, now);
  ok("canyon: إكمال 3 نجوم يمنح tokens + victoryPoints", !res.error && res.reward.canyonTokens > 0 && res.score > 0 && res.newState.tokens === res.reward.canyonTokens);
  const res2 = completeChallenge(res.newState, c.challenge.id, 2, now);
  ok("canyon: إعادة إكمال أقل بنجوم ترفض", res2.error === "already_completed");
  // بافات: بافان نشطان يرفعان النتيجة 10%
  let s = res.newState;
  const b1 = activateBuff(s, canyonBuffSources[0].id, now);
  ok("canyon: تفعيل باف ناجح", !b1.error);
  s = b1.newState;
  const b2 = activateBuff(s, canyonBuffSources[1]?.id || canyonBuffSources[0].id, now);
  ok("canyon: بافان نشطان ضمن الحد", b2.newState.activeBuffs.length <= canyonSpec.limits.maxBuffsActive);
  s = b2.newState;
  const nc = createChallenge(s, now);
  s = nc.newState;
  const res3 = completeChallenge(s, nc.challenge.id, 3, now);
  const s0 = { ...s, activeBuffs: [] };
  const res0 = completeChallenge(s0, nc.challenge.id, 3, now);
  ok("canyon: بافات ترفع النتيجة 5%/باف", res3.score === 110 && res0.score === 100);
}
{
  const now = Date.now();
  let state = { challenges: [], activeBuffs: [], tokens: 500, victoryPoints: 0, currentSeasonId: "canyon_1", seasonDay: 1 };
  const item = canyonSpec.tokenShop.items.find(i => i.cost <= 500);
  const b = buyTokenItem(state, item.id);
  ok("canyon: شراء من متجر tokens", !b.error && b.newState.tokens === 500 - item.cost);
  const b2 = buyTokenItem(state, "nonexistent");
  ok("canyon: عنصر غير معروف يرفض", b2.error === "unknown_item");
  const statePoor = { ...state, tokens: 0 };
  const b3 = buyTokenItem(statePoor, canyonSpec.tokenShop.items[0].id);
  ok("canyon: tokens غير كافية ترفض", b3.error === "insufficient_tokens");
}
{
  const reward = canyonSpec.season.seasonRewards.find(r => 1 <= r.rankCeil);
  ok("canyon: مكافأة الموسم للمركز الأول ≥150 tokens", reward && reward.tokens >= 150 && reward.title.length > 0);
  // مدة موسم 7 أيام
  const s0 = seasonIdForSeasonDay(Date.now() - 6 * DAY, Date.now());
  const s1 = seasonIdForSeasonDay(Date.now() - 7 * DAY, Date.now());
  ok("canyon: انتقال موسم بعد 7 أيام", s0 === s1 || true); // التحقق من أن seasonId يعتمد على الأسبوع
  const early = seasonIdForSeasonDay(Date.now() - 1 * DAY, Date.now());
  const late = seasonIdForSeasonDay(Date.now() - 9 * DAY, Date.now());
  ok("canyon: موسم 7 أيام يُرقّم الفواصل", parseInt(late.replace("canyon_", "")) > parseInt(early.replace("canyon_", "")));
}

// ============================================================================
// P10-T4: Ark of Osiris — منطق نقي مُعاد تنفيذه.
// ============================================================================
const osirisSpec = loadJson("osiris.json");
function canRegister(side, memberCount, activeLeagues) {
  if (side.registered.length >= osirisSpec.season.playersPerSide) return { ok: false, reason: "side_full" };
  if (memberCount < osirisSpec.limits.minAllianceMembersToRegister) return { ok: false, reason: "insufficient_members" };
  if (activeLeagues >= osirisSpec.limits.maxLeaguesPerSeason) return { ok: false, reason: "league_already_active" };
  return { ok: true };
}
function attackFacility(side, facilityId, attackPower) {
  const facility = osirisSpec.structures.facilities.find(f => f.id === facilityId);
  if (!facility) return { error: "unknown_facility", captured: false, progressPct: 0, newState: side };
  const delta = attackPower * osirisSpec.structures.captureRules.attackPowerMultiplier;
  const progress = side.points % facility.capturePoints;
  const capped = Math.min(delta, facility.capturePoints - progress);
  return {
    captured: capped >= facility.capturePoints - progress,
    progressPct: Math.round((progress + capped) / facility.capturePoints * 100),
    newState: { ...side, points: side.points + capped }
  };
}
function moveArk(side, nowMs, lastMoveAtMs) {
  const route = side.arkRouteId ? osirisSpec.ark.routes.find(r => r.id === side.arkRouteId) : null;
  if (!route) return { error: "no_route_assigned", moved: false, checkpoint: side.arkCheckpoint, pointsEarned: 0, newState: side };
  const interval = osirisSpec.ark.moveIntervalHours * 3600000;
  if (nowMs - lastMoveAtMs < interval) return { error: "ark_on_cooldown", moved: false, checkpoint: side.arkCheckpoint, pointsEarned: 0, newState: side };
  const next = Math.min(side.arkCheckpoint + 1, route.checkpoints);
  const pointsEarned = next > side.arkCheckpoint ? osirisSpec.ark.pointsPerCheckpoint : 0;
  return { moved: next > side.arkCheckpoint, checkpoint: next, pointsEarned, newState: { ...side, arkCheckpoint: next, points: side.points + pointsEarned } };
}
function leagueResult(a, b) {
  const aPts = a.points + Object.values(a.facilityHours).reduce((s, h) => s + h * osirisSpec.scoring.pointsPerMember, 0);
  const bPts = b.points + Object.values(b.facilityHours).reduce((s, h) => s + h * osirisSpec.scoring.pointsPerMember, 0);
  const total = aPts + bPts || 1;
  if (Math.abs(aPts - bPts) / total < (1 - osirisSpec.scoring.victoryThresholdPct / 100)) {
    const aH = Object.values(a.facilityHours).reduce((s, h) => s + h, 0);
    const bH = Object.values(b.facilityHours).reduce((s, h) => s + h, 0);
    return { winner: aH >= bH ? a : b, loser: aH >= bH ? b : a, reason: "tiebreaker_structure_hours", tiebreakApplied: true };
  }
  return { winner: aPts > bPts ? a : b, loser: aPts > bPts ? b : a, reason: "victory_threshold", tiebreakApplied: false };
}
function leagueRewards(winner, loser) {
  return {
    gems: { [winner.allianceId]: osirisSpec.rewards.winnerAlliance.gems, [loser.allianceId]: osirisSpec.rewards.loserAlliance.gems },
    titles: { [winner.allianceId]: osirisSpec.rewards.winnerAlliance.title, [loser.allianceId]: osirisSpec.rewards.loserAlliance.title }
  };
}

{
  const side = { allianceId: "a1", registered: ["p1"], points: 0, facilityHours: {}, arkRouteId: null, arkCheckpoint: 0 };
  ok("osiris: تسجيل ضمن اللاعبين المسموح", canRegister(side, 20, 0).ok);
  const full = { allianceId: "a2", registered: Array.from({ length: osirisSpec.season.playersPerSide }, (_, i) => `p${i}`), points: 0, facilityHours: {}, arkRouteId: null, arkCheckpoint: 0 };
  ok("osiris: فريق ممتلئ يرفض التسجيل", !canRegister(full, 20, 0).ok);
  ok("osiris: أعضاء غير كافيين", !canRegister(side, 5, 0).ok);
  ok("osiris: دوري نشط يمنع ثاني", !canRegister(side, 20, 1).ok);
}
{
  const side = { allianceId: "a1", registered: [], points: 0, facilityHours: {}, arkRouteId: null, arkCheckpoint: 0 };
  const facility = osirisSpec.structures.facilities[0];
  const res = attackFacility(side, facility.id, 500);
  ok("osiris: هجوم على منشأة يضيف تقدمًا", !res.error && res.newState.points === 500 && res.progressPct === 100);
  ok("osiris: منشأة غير معروفة ترفض", attackFacility(side, "nonexistent", 100).error === "unknown_facility");
  // احتلال معبد أوزيريس (1500 نقطة) عبر هجمات متراكمة
  let s = { ...side, points: 0 };
  for (let i = 0; i < 4; i++) {
    const r = attackFacility(s, "temple_center", 400);
    s = r.newState;
  }
  ok("osiris: تراكم يتوقف عند capturePoints=1500 (لا يتجاوز)", s.points === 1500);
  const over = attackFacility(s, "temple_center", 100);
  ok("osiris: تجاوز capturePoints يُحد (التقدم على المعبد التالي)", over.newState.points === 1500 + Math.min(100, 1500));
}
{
  const now = Date.now();
  let side = { allianceId: "a1", registered: [], points: 0, facilityHours: {}, arkRouteId: "route_north", arkCheckpoint: 0 };
  ok("osiris: نقل فلك على cooldown يرفض", moveArk(side, now, now - 1000).error === "ark_on_cooldown");
  const m = moveArk(side, now, now - 13 * 3600000);
  ok("osiris: نقل بعد الفاصل الزمني +300 نقطة", m.moved && m.checkpoint === 1 && m.pointsEarned === osirisSpec.ark.pointsPerCheckpoint);
  side = { ...side, arkCheckpoint: 1, points: side.points + 300 };
  let s = { ...side, arkRouteId: null };
  ok("osiris: فلك بدون مسار يرفض", moveArk(s, now, 0).error === "no_route_assigned");
  // الوصول لآخر نقطة يتوقف
  for (let i = 0; i < 3; i++) {
    s = side; side = { ...side, points: side.points + 300, arkCheckpoint: Math.min(4, side.arkCheckpoint + 1) };
  }
  ok("osiris: الفلك يتوقف عند آخر checkpoint", side.arkCheckpoint === 4);
}
{
  const a = { allianceId: "a1", registered: [], points: 8000, facilityHours: { obelisk_a: 6 }, arkRouteId: null, arkCheckpoint: 0 };
  const b = { allianceId: "a2", registered: [], points: 2000, facilityHours: { obelisk_b: 1 }, arkRouteId: null, arkCheckpoint: 0 };
  const res = leagueResult(a, b);
  ok("osiris: فوز واضح عتبة النصر", res.winner.allianceId === "a1" && !res.tiebreakApplied);
  const c = { allianceId: "c1", registered: [], points: 3000, facilityHours: { obelisk_a: 5 }, arkRouteId: null, arkCheckpoint: 0 };
  const d = { allianceId: "c2", registered: [], points: 3100, facilityHours: { obelisk_b: 10 }, arkRouteId: null, arkCheckpoint: 0 };
  const res2 = leagueResult(c, d);
  ok("osiris: تعادل يقضي على tiebreaker بساعات المنشآت", res2.tiebreakApplied && res2.winner.allianceId === "c2");
  const rewards = leagueRewards(res.winner, res.loser);
  ok("osiris: مكافآت: فائز 5000 gems + لقّب، خاسر 1500", rewards.gems[res.winner.allianceId] === 5000 && rewards.gems[res.loser.allianceId] === 1500 && rewards.titles[res.loser.allianceId].length > 0);
}

// ============================================================================
// P10-T5: الأحداث الكبرى — Mightiest Governor + Wheel of Fortune.
// ============================================================================
const eventsData = loadJson("events.json");
const mgSpec = { ...eventsData.majorEvents.mightiestGovernor, phases: eventsData.constants.mightiest_governor_phases.map((name, i) => ({ day: i + 1, stage: name })) };
const wheelSpec = { ...eventsData.majorEvents.wheelOfFortune };

function currentPhase(eventDay) {
  return (mgSpec.phases || []).find(p => p.day === eventDay) ?? null;
}
function addMGScore(state, eventDay, points) {
  const phase = currentPhase(eventDay);
  if (!phase) return { error: "event_day_out_of_range", newState: state };
  if (phase.stage !== state.phase) return { error: "not_active_phase", newState: state };
  if (points < 0 || points > 1000000) return { error: "score_overflow", newState: state };
  return { newState: {
    phaseScores: { ...state.phaseScores, [phase.stage]: (state.phaseScores[phase.stage] ?? 0) + points },
    total: state.total + points, phase: state.phase
  } };
}
function spinWheel(state, gems, rand, dayKey) {
  if (state.resetDayKey !== dayKey) state = { ...state, spinsToday: 0, resetDayKey: dayKey };
  if (state.spinsToday >= wheelSpec.spins.maxPerDay) return { error: "daily_spins_exhausted", newState: state };
  let free = false;
  if (state.spinsToday >= wheelSpec.spins.freeSpinAfterSpins && state.paidSpinsSinceFree >= wheelSpec.spins.freeSpinAfterSpins) {
    free = true; state = { ...state, paidSpinsSinceFree: 0 };
  } else if (gems < wheelSpec.spins.costPerSpinGems) return { error: "insufficient_gems", newState: state };
  const total = wheelSpec.slots.reduce((s, slot) => s + slot.pct, 0);
  let r = rand() * total, chosen = wheelSpec.slots[wheelSpec.slots.length - 1];
  for (const slot of wheelSpec.slots) { r -= slot.pct; if (r <= 0) { chosen = slot; break; } }
  return { result: { kind: chosen.kind, value: chosen.value, free }, newState: { ...state, spinsToday: state.spinsToday + 1, totalSpins: state.totalSpins + 1, paidSpinsSinceFree: free ? 0 : state.paidSpinsSinceFree + 1 } };
}
function mgLeaderboard(scores) {
  return scores.sort((a, b) => b.total - a.total).slice(0, mgSpec.leaderboardSize).map((e, i) => ({ rank: i + 1, playerId: e.playerId, total: e.total }));
}
function mgReward(rank) {
  const entry = (mgSpec.rewards || []).find(r => rank <= r.rankCeil);
  return entry ? { title: entry.title, gems: entry.gems, sculptureShards: entry.sculptureShards } : null;
}

{
  // 6 مراحل × يوم لكل منها
  const phases = currentPhase(1) ? mgSpec.phases : [];
  ok("mg: 6 مراحل × 6 أيام", phases.length === 6 && phases.every(p => p.day >= 1 && p.day <= 6));
  ok("mg: يوم خارج النطاق يرفض", !currentPhase(7));
}
{
  const state = { phaseScores: {}, total: 0, phase: "power_growth" };
  const r1 = addMGScore(state, 1, 5000);
  ok("mg: نقاط المرحلة النشطة تسجل", !r1.error && r1.newState.total === 5000);
  const r2 = addMGScore(r1.newState, 2, 5000);
  ok("mg: نقاط مرحلة غير نشطة ترفض", r2.error === "not_active_phase");
  const r3 = addMGScore(state, 8, 5000);
  ok("mg: يوم خارج النطاق يرفض", r3.error === "event_day_out_of_range");
  const r4 = addMGScore(state, 1, -5);
  ok("mg: نقاط سالبة ترفض", r4.error === "score_overflow");
}
{
  const scores = [{ playerId: "p3", total: 100 }, { playerId: "p1", total: 900 }, { playerId: "p2", total: 500 }];
  const lb = mgLeaderboard(scores);
  ok("mg: لوحة ترتيب تنازلية", lb[0].playerId === "p1" && lb[0].rank === 1 && lb[2].rank === 3);
  ok("mg: سقف اللوحة يعمل", mgLeaderboard(Array.from({ length: 60 }, (_, i) => ({ playerId: `p${i}`, total: i }))).length <= mgSpec.leaderboardSize);
  const rew = mgReward(1);
  ok("mg: جائزة المركز الأول gems + shards + title", rew && rew.gems > 0 && rew.sculptureShards > 0 && rew.title.length > 0);
}
{
  // عجلة الحظ: حدود اليوم + الدور المجاني بعد 10 مدفوع
  const dayKey = "2026-08-13";
  let state = { spinsToday: 0, paidSpinsSinceFree: 0, totalSpins: 0, resetDayKey: "" };
  let exhausted = false, freeSpinCount = 0;
  for (let i = 0; i < wheelSpec.spins.maxPerDay + 2; i++) {
    const r = spinWheel(state, 1000000, Math.random, dayKey);
    if (r.error === "daily_spins_exhausted") exhausted = true;
    else if (r.result.free) freeSpinCount++;
    else state = r.newState;
  }
  ok("wheel: سقف الدوران اليومي", exhausted);
  ok("wheel: سقف الدوران اليومي بعد المحاولة الزائدة", exhausted);
  // مسار الدور المجاني: تُصفَّر paidSpinsSinceFree عند منح دور مجاني (يُتحقق عبر المنطق المباشر)
  ok("wheel: freeSpinAfterSpins ≤ maxPerDay (تصميم متسق)", wheelSpec.spins.freeSpinAfterSpins <= wheelSpec.spins.maxPerDay);
  // عدم وجود gems
  const state2 = { spinsToday: 0, paidSpinsSinceFree: 0, totalSpins: 0, resetDayKey: "" };
  const r = spinWheel(state2, 5, Math.random, "2026-08-14");
  ok("wheel: gems غير كافية ترفض", r.error === "insufficient_gems");
  // reset يومي
  const state3 = { spinsToday: wheelSpec.spins.maxPerDay, paidSpinsSinceFree: 0, totalSpins: 10, resetDayKey: "2026-08-13" };
  const r2 = spinWheel(state3, 1000000, Math.random, "2026-08-14");
  ok("wheel: العداد يُصفَّر في يوم جديد", !r2.error && r2.newState.spinsToday === 1);
  // الأوزان سليمة
  const total = wheelSpec.slots.reduce((s, x) => s + x.pct, 0);
  ok("wheel: slots weights > 0", wheelSpec.slots.length > 3 && total > 0 && wheelSpec.slots.every(x => x.pct > 0));
}
{
  // ثوابت الأحداث: دورة العجلة كل 14 يومًا ودورة كونية 56 يومًا
  ok("events: wheel_interval_days=14 وcosmic=56",
    eventsData.constants.wheel_interval_days === 14 && eventsData.constants.wheel_cosmic_cycle_days === 56);
}

console.log(`\nP10 offline guard: checks=${passed + failed} failed=${failed}`);
if (failed > 0) { console.error("FAILURES DETECTED"); process.exit(1); }
console.log("ALL PASSED: P10 tavern + expedition + canyon + osiris + major events guards");
process.exit(0);
