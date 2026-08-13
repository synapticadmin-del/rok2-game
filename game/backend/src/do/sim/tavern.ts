// P10-T1: الحانة والصناديق — منطق نقي (pure) بدون أي اعتماد على Cloudflare.
// يُنفَّذ في KingdomShard ويُختبر محليًا في حارس offline. الثوابت كلها من data/tavern.json.

export interface TavernPoolItem { weight: number; kind: string; description: string }
export interface TavernBox { id: string; name: string; key: string; rollCount: number; pool: TavernPoolItem[] }
export interface TavernLimits { maxOpensPerHour: number; maxKeysStored: number; dailyFreeSilverKeys: number;
  epicRateFloorPct: number; epicRateCeilPct: number; statisticalSampleSize: number; chiSquarePctThreshold: number }
export interface TavernSpec { boxes: TavernBox[]; rateTargets: Record<string, Record<string, number>>; limits: TavernLimits }

export interface TavernRoll { boxId: string; rolls: { kind: string; quantity: number }[]; openAtMs: number }
export interface TavernState { keys: Record<string, number>; openedHistory: { boxId: string; kind: string; atMs: number }[] }

/** مرجح عشوائي: rollCount من pool حسب الأوزان (لا hard-coded — البيانات من JSON). */
export function rollBox(spec: TavernSpec, boxId: string, rand: () => number, opensThisHour: number):
  { rolls: TavernRoll["rolls"]; error?: string } {
  const box = spec.boxes.find(b => b.id === boxId);
  if (!box) return { rolls: [], error: "unknown_box" };
  if (opensThisHour >= spec.limits.maxOpensPerHour) return { rolls: [], error: "rate_limit_hourly" };
  const total = box.pool.reduce((s, i) => s + i.weight, 0);
  const rolls: TavernRoll["rolls"] = [];
  for (let i = 0; i < box.rollCount; i++) {
    let r = rand() * total, chosen = box.pool[box.pool.length - 1];
    for (const item of box.pool) { r -= item.weight; if (r <= 0) { chosen = item; break; } }
    rolls.push({ kind: chosen.kind, quantity: kindQuantity(chosen.kind) });
  }
  return { rolls };
}

function kindQuantity(kind: string): number {
  switch (kind) {
    case "common": return 5000;
    case "rare": return 1;
    case "materials": return 3000;
    case "epic": return 1;
    case "legendary": return 1;
    default: return 1;
  }
}

/** تحقق مفتاح الصناديق: يخصم مفتاحًا أو يعيد خطأ. */
export function spendKey(state: TavernState, spec: TavernSpec, boxId: string): { error?: string; newState: TavernState } {
  const box = spec.boxes.find(b => b.id === boxId);
  if (!box) return { error: "unknown_box", newState: state };
  const held = state.keys[box.key] ?? 0;
  if (held <= 0) return { error: "no_key", newState: state };
  return { newState: { ...state, keys: { ...state.keys, [box.key]: held - 1 } } };
}

/** إضافة مفاتيح (من المهام اليومية مثلًا) مع سقف maxKeysStored. */
export function addKeys(state: TavernState, spec: TavernSpec, key: string, count: number): TavernState {
  const held = state.keys[key] ?? 0;
  return { ...state, keys: { ...state.keys, [key]: Math.min(held + count, spec.limits.maxKeysStored) } };
}

/** فحص إحصائي anti-cheat: نسبة epic الفعلية يجب أن تبقى داخل [floor, ceil] على عينة ≥sampleSize. */
export function checkEpicRate(state: TavernState, spec: TavernSpec): {
  withinLimits: boolean; epicRatePct: number; sampleSize: number
} {
  const opens = state.openedHistory.length;
  const epic = state.openedHistory.filter(h => h.kind === "epic").length;
  const legendary = state.openedHistory.filter(h => h.kind === "legendary").length;
  const sampleSize = Math.min(opens, spec.limits.statisticalSampleSize);
  if (sampleSize < 100) return { withinLimits: true, epicRatePct: 0, sampleSize };
  const recent = state.openedHistory.slice(-spec.limits.statisticalSampleSize);
  const ratePct = ((recent.filter(h => h.kind === "epic").length + recent.filter(h => h.kind === "legendary").length) / recent.length) * 100;
  return {
    withinLimits: ratePct >= spec.limits.epicRateFloorPct && ratePct <= spec.limits.epicRateCeilPct,
    epicRatePct: Math.round(ratePct * 100) / 100,
    sampleSize
  };
}

/** مفتاح فضي يومي مجاني (من المهام اليومية). */
export function dailyFreeKey(state: TavernState, dayString: string, lastFreeDay?: string): { granted: boolean; newState: TavernState } {
  if (lastFreeDay === dayString) return { granted: false, newState: state };
  return { granted: true, newState: { ...state, keys: { ...state.keys, silver_key: (state.keys.silver_key ?? 0) + 1 } } };
}
