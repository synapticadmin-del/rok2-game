// P10-T5: الأحداث الكبرى — Mightiest Governor (6 أيام × 6 مراحل) + Wheel of Fortune.
// منطق نقي. الثوابت كلها من data/events.json.

export interface MGPhase { day: number; stage: string; name: string; criteria: string }
export interface MGSpec { id: string; name: string; durationDays: number; phases: MGPhase[]; leaderboardSize: number;
  rewards: { rankCeil: number; title: string; gems: number; sculptureShards: number }[] }

export interface WheelSlot { weight: number; kind: string; value: number | string }
export interface WheelSpec { id: string; name: string; durationDays: number; intervalDays: number; cosmicCycleDays: number;
  spins: { maxPerDay: number; costPerSpinGems: number; freeSpinAfterSpins: number }; slots: WheelSlot[] }

export interface MGScoreState { phaseScores: Record<string, number>; total: number; phase: string }
export interface WheelState { spinsToday: number; paidSpinsSinceFree: number; totalSpins: number; resetDayKey: string }

/** مرحلة اليوم الحالي من حدث الحاكم الأقوى (يوم الحدث 1-6). */
export function currentPhase(spec: MGSpec, eventDay: number): MGPhase | null {
  return spec.phases.find(p => p.day === eventDay) ?? null;
}

/** تسجيل نقاط في المرحلة النشطة — تُرفض النقاط خارج المرحلة النشطة. */
export function addMGScore(state: MGScoreState, spec: MGSpec, eventDay: number, points: number):
  { error?: string; newState: MGScoreState } {
  const phase = currentPhase(spec, eventDay);
  if (!phase) return { error: "event_day_out_of_range", newState: state };
  if (phase.stage !== state.phase) return { error: "not_active_phase", newState: state };
  if (points < 0 || points > 1000000) return { error: "score_overflow", newState: state };
  return { newState: {
    phaseScores: { ...state.phaseScores, [phase.stage]: (state.phaseScores[phase.stage] ?? 0) + points },
    total: state.total + points,
    phase: state.phase
  } };
}

/** نتيجة دوران عجلة الحظ بوزن مرجح + عداد الدوران المجاني. */
export function spinWheel(spec: WheelSpec, state: WheelState, gems: number, rand: () => number, dayKey: string):
  { error?: string; result?: { kind: string; value: number | string; free: boolean }; newState: WheelState } {
  if (state.resetDayKey !== dayKey) state = { ...state, spinsToday: 0, resetDayKey: dayKey };
  if (state.spinsToday >= spec.spins.maxPerDay) return { error: "daily_spins_exhausted", newState: state };
  let free = false;
  if (state.spinsToday >= spec.spins.freeSpinAfterSpins && state.paidSpinsSinceFree >= spec.spins.freeSpinAfterSpins) {
    free = true;
    state = { ...state, paidSpinsSinceFree: 0 };
  } else if (gems < spec.spins.costPerSpinGems) {
    return { error: "insufficient_gems", newState: state };
  }
  const total = spec.slots.reduce((s, slot) => s + slot.weight, 0);
  let r = rand() * total;
  let chosen: WheelSlot = spec.slots[spec.slots.length - 1];
  for (const slot of spec.slots) { r -= slot.weight; if (r <= 0) { chosen = slot; break; } }
  return {
    result: { kind: chosen.kind, value: chosen.value, free },
    newState: { ...state, spinsToday: state.spinsToday + 1, totalSpins: state.totalSpins + 1,
      paidSpinsSinceFree: free ? 0 : state.paidSpinsSinceFree + 1 }
  };
}

/** جائزة الحاكم الأقوى حسب الترتيب النهائي. */
export function mgReward(spec: MGSpec, rank: number): { title: string; gems: number; sculptureShards: number } | null {
  const entry = spec.rewards.find(r => rank <= r.rankCeil);
  return entry ? { title: entry.title, gems: entry.gems, sculptureShards: entry.sculptureShards } : null;
}

/** ترتيب تصنيف الحاكم الأقوى (تنازليًا) مع سقف اللوحة. */
export function mgLeaderboard(spec: MGSpec, scores: { playerId: string; total: number }[]):
  { rank: number; playerId: string; total: number }[] {
  return scores.sort((a, b) => b.total - a.total).slice(0, spec.leaderboardSize)
    .map((e, i) => ({ rank: i + 1, playerId: e.playerId, total: e.total }));
}
