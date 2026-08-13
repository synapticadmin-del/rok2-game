// P10-T2: Expedition (حملة PvE) — منطق نقي. الثوابت كلها من data/expedition.json.

export interface ExpeditionStage { id: string; name: string; recommendedPower: number;
  stars: { thresholds: number[]; lossPct: number[] }; rewards: Record<string, number | boolean>; medals: number }
export interface ExpeditionLimits { attemptsPerDay: number; maxMedalStoreItemsPerDay: number; resetHours: number[] }
export interface MedalShopItem { id: string; name: string; cost: number; reward: Record<string, number | boolean> }
export interface ExpeditionSpec { stages: ExpeditionStage[]; freeCommander: { grantedAt: string; name: string };
  medalShop: { items: MedalShopItem[] }; limits: ExpeditionLimits }

export interface ExpeditionState {
  bestStars: Record<string, number>;   // المرحلة -> أفضل نجوم (0-3)
  attemptsToday: number;
  purchasesToday: Record<string, number>; // متجر الميداليات id -> count
  freeCommanderGranted: boolean;
  resetHourKey: string;
  medals: number;                      // ميداليات الحملة المجمعة (تُحفظ في expedition_state)
}

/** نتيجة محاكاة معركة: خسائر + نجوم. thresholds = [0.9,0.7,0.4] نسبة قوات باقية للنجوم. */
export function runBattle(stage: ExpeditionStage, playerPower: number, rand: () => number):
  { stars: number; lossPct: number; won: boolean } {
  if (playerPower <= 0) return { stars: 0, lossPct: 1, won: false };
  const powerRatio = playerPower / stage.recommendedPower;
  // خسارة متوقعة تنخفض كلما فاق اللاعب القوة المقترحة (12%..75% عند النسبة 1)
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

/** فحص محاولات اليوم. */
export function canAttempt(state: ExpeditionState, spec: ExpeditionSpec, key: string, resetHourKey: string): { ok: boolean; reason?: string; newState: ExpeditionState } {
  if (state.resetHourKey !== resetHourKey) state = resetToday(state, resetHourKey);
  if (state.attemptsToday >= spec.limits.attemptsPerDay) return { ok: false, reason: "attempts_exhausted", newState: state };
  return { ok: true, newState: { ...state, attemptsToday: state.attemptsToday + 1, resetHourKey } };
}

function resetToday(state: ExpeditionState, key: string): ExpeditionState {
  return { ...state, attemptsToday: 0, purchasesToday: {}, resetHourKey: key };
}

/** تحديث أفضل نجوم للمرحلة + فحص القائد المجاني. */
export function recordStars(state: ExpeditionState, spec: ExpeditionSpec, stageId: string, stars: number):
  { newState: ExpeditionState; commanderGranted: boolean; stageNext?: string } {
  const prev = state.bestStars[stageId] ?? 0;
  const idx = spec.stages.findIndex(s => s.id === stageId);
  const newState: ExpeditionState = { ...state, bestStars: { ...state.bestStars, [stageId]: Math.max(prev, stars) } };
  const commanderGranted = !newState.freeCommanderGranted &&
    spec.stages.some(s => s.id === spec.freeCommander.grantedAt && (newState.bestStars[s.id] ?? 0) >= 1);
  if (commanderGranted) newState.freeCommanderGranted = true;
  return { newState, commanderGranted, stageNext: idx >= 0 && idx + 1 < spec.stages.length ? spec.stages[idx + 1].id : undefined };
}

/** شراء عنصر من متجر الميداليات. */
export function buyMedalItem(state: ExpeditionState, spec: ExpeditionSpec, itemId: string, medals: number):
  { error?: string; item?: MedalShopItem; newState: ExpeditionState } {
  const item = spec.medalShop.items.find(i => i.id === itemId);
  if (!item) return { error: "unknown_item", newState: state };
  const bought = state.purchasesToday[itemId] ?? 0;
  if (bought >= spec.limits.maxMedalStoreItemsPerDay) return { error: "daily_purchase_limit", newState: state };
  if (medals < item.cost) return { error: "insufficient_medals", newState: state };
  return { item, newState: { ...state, purchasesToday: { ...state.purchasesToday, [itemId]: bought + 1 } } };
}
