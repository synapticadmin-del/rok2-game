// P11-T3/T4: Lost Kingdom — هجرة الممالك + منشآت KvK + Great Ziggurat. منطق نقي، كل الأرقام من data/lost_kingdom.json.
import { MS_PER_HOUR } from "../../lib/timeConstants";

export interface LostKingdomStructures {
  hierons: { count: number; capture_points: number; hold_score_per_hour: number; reward_coins: number; spawn_day: number };
  sanctuaries: { count: number; severe_heal_ratio: number; cooldown_hours: number; spawn_day: number };
  ancient_ruins: { count: number; exploration_minutes: number; xp_reward: number; spawn_day: number };
  circles: { count: number; buff_atk_def: number; duration_hours: number; spawn_day: number };
  altars_of_darkness: { count: number; horde_level: number; horde_count: number; spawn_day: number };
  citadels: { count: number; hp: number; ziggurat_damage_pct: number; reward_coins: number; spawn_day: number };
  great_ziggurat: { id: string; name: string; hp: number; opens_after_citadels: number; final_battle_duration_hours: number; destroy_reward_coins: number; crown_points: number; pos: number[] };
}
export interface LostKingdomMigration {
  min_city_hall_level: number; cooldown_days: number; requirements: string[];
  vault_protection_hours: number; cost: { gems: number };
}
export interface LostKingdomSpec {
  constants: {
    season_duration_days: number; migration_min_city_hall_level: number; migration_cooldown_days: number;
    migration_vault_protection_hours: number; ziggurat_total_hp: number; ziggurat_open_after_citadels: number;
    ziggurat_final_battle_duration_hours: number; kvk_coin_name: string; kvk_coin_symbol: string;
    final_crown_points: number; ziggurat_destroy_reward_coins: number; citadel_destroy_reward_coins: number;
    hieron_capture_reward_coins: number;
    season_store: { items: { id: string; name: string; cost: number; reward: Record<string, number | string> }[] };
  };
  structures: LostKingdomStructures;
  migration: LostKingdomMigration;
  season_schedule: { day: number; feature: string }[];
}

export interface LKStructureState { id: string; owner: string; hp: number; captured_ms: number }
export interface LKCitadel { id: string; hp: number; destroyed: boolean; destroyed_by: string }
export interface LKZiggurat { hp: number; open: boolean; finalBattleStartedMs: number; destroyed: boolean; destroyed_by: string }
export interface LKMigrationState { migrated: boolean; migrated_ms: number; last_migrated_ms: number }
export interface LostKingdomState {
  structures: LKStructureState[];
  citadels: LKCitadel[];
  ziggurat: LKZiggurat;
  migration: LKMigrationState;
  kvk_coins: number;
  crown_points: number;
  kingdom_points: number;
  season_id: string;
}

export function defaultLostKingdomState(spec: LostKingdomSpec, seasonId: string): LostKingdomState {
  const structures: LKStructureState[] = [
    ...Array.from({ length: spec.structures.hierons.count }, (_, i) => ({
      id: `HIER_${i}`, owner: "", hp: spec.structures.hierons.capture_points, captured_ms: 0,
    })),
    ...Array.from({ length: spec.structures.sanctuaries.count }, (_, i) => ({
      id: `SANC_${i}`, owner: "", hp: spec.structures.hierons.capture_points, captured_ms: 0,
    })),
  ];
  const citadels: LKCitadel[] = Array.from({ length: spec.structures.citadels.count }, (_, i) => ({
    id: `CIT_${i}`, hp: spec.structures.citadels.hp, destroyed: false, destroyed_by: "",
  }));
  return {
    structures, citadels,
    ziggurat: { hp: spec.constants.ziggurat_total_hp, open: false, finalBattleStartedMs: 0, destroyed: false, destroyed_by: "" },
    migration: { migrated: false, migrated_ms: 0, last_migrated_ms: 0 },
    kvk_coins: 0, crown_points: 0, kingdom_points: 0, season_id: seasonId,
  };
}

/** P11-T3: شروط الهجرة إلى Lost Kingdom — CH16+، لا مسيرات، cooldown 30 يومًا. */
export function canMigrate(spec: LostKingdomSpec, state: LostKingdomState, cityHallLevel: number,
  hasActiveMarches: boolean, nowMs: number): { allowed: boolean; reason?: string } {
  if (cityHallLevel < spec.migration.min_city_hall_level) return { allowed: false, reason: "city_hall_too_low" };
  if (hasActiveMarches) return { allowed: false, reason: "active_marches" };
  const cooldownMs = spec.migration.cooldown_days * MS_PER_HOUR * 24;
  if (state.migration.last_migrated_ms > 0 && nowMs - state.migration.last_migrated_ms < cooldownMs) {
    return { allowed: false, reason: "migration_cooldown" };
  }
  if (state.migration.migrated && nowMs - state.migration.migrated_ms < spec.migration.vault_protection_hours * MS_PER_HOUR) {
    return { allowed: false, reason: "vault_protection" };
  }
  return { allowed: true };
}

/** تنفيذ الهجرة: نقل اللاعب إلى Lost Kingdom (يُنفذ في الشارد: تجميد المخزن 48 ساعة + خصم gems). */
export function migratePlayer(spec: LostKingdomSpec, state: LostKingdomState, gems: number, nowMs: number):
  { error?: string; newState: LostKingdomState } {
  if (gems < spec.migration.cost.gems) return { error: "insufficient_gems", newState: state };
  const migrated: LKMigrationState = { migrated: true, migrated_ms: nowMs, last_migrated_ms: nowMs };
  return { newState: { ...state, migration: migrated } };
}

/** الاستيلاء على هيرون (P11-T4): منح عملات KvK + نقاط للمملكة. */
export function captureHieron(spec: LostKingdomSpec, state: LostKingdomState, hieronId: string,
  captorKingdom: string, nowMs: number): { error?: string; reward_coins: number; newState: LostKingdomState } {
  const idx = state.structures.findIndex(s => s.id === hieronId && s.id.startsWith("HIER_"));
  if (idx < 0) return { error: "unknown_hieron", reward_coins: 0, newState: state };
  const struct = state.structures[idx];
  if (struct.owner === captorKingdom) return { error: "already_owned", reward_coins: 0, newState: state };
  const updated = [...state.structures];
  updated[idx] = { ...struct, owner: captorKingdom, captured_ms: nowMs };
  return { reward_coins: spec.structures.hierons.reward_coins,
    newState: { ...state, structures: updated, kvk_coins: state.kvk_coins + spec.structures.hierons.reward_coins,
      kingdom_points: state.kingdom_points + spec.structures.hierons.hold_score_per_hour } };
}

/** P11-T4: تدمير قلعة يقصف Great Ziggurat — كل قلعة تزيل 25% من HP الزيقورة. */
export function destroyCitadel(spec: LostKingdomSpec, state: LostKingdomState, citadelId: string,
  attackerKingdom: string, damage: number, nowMs: number):
  { error?: string; zigguratHp?: number; citadelDestroyed?: boolean; reward_coins: number; newState: LostKingdomState } {
  const idx = state.citadels.findIndex(c => c.id === citadelId);
  if (idx < 0) return { error: "unknown_citadel", reward_coins: 0, newState: state };
  const citadel = state.citadels[idx];
  if (citadel.destroyed) return { error: "already_destroyed", reward_coins: 0, newState: state };
  const newHp = Math.max(0, citadel.hp - damage);
  const destroyed = newHp <= 0;
  const updatedCitadels = [...state.citadels];
  updatedCitadels[idx] = { ...citadel, hp: newHp, destroyed, destroyed_by: destroyed ? attackerKingdom : "" };
  const destroyedCount = updatedCitadels.filter(c => c.destroyed).length;
  const zigguratOpen = destroyedCount >= spec.constants.ziggurat_open_after_citadels;
  const zigguratDamagePct = destroyed ? spec.structures.citadels.ziggurat_damage_pct : 0;
  const zigguratDamage = destroyed ? Math.ceil(state.ziggurat.hp * zigguratDamagePct / 100) : 0;
  const ziggurat: LKZiggurat = { ...state.ziggurat, hp: state.ziggurat.hp - zigguratDamage,
    open: state.ziggurat.open || zigguratOpen };
  const coins = destroyed ? spec.structures.citadels.reward_coins : 0;
  return { zigguratHp: ziggurat.hp, citadelDestroyed: destroyed, reward_coins: coins,
    newState: { ...state, citadels: updatedCitadels, ziggurat, kvk_coins: state.kvk_coins + coins } };
}

/** P11-T4: المعركة النهائية على Great Ziggurat — تتويج المملكة الفائزة. */
export function attackZiggurat(spec: LostKingdomSpec, state: LostKingdomState, attackerKingdom: string,
  damage: number, nowMs: number): { error?: string; crowned?: string; crown_points: number; newState: LostKingdomState } {
  const zg = state.ziggurat;
  if (!zg.open) return { error: "ziggurat_not_open", crown_points: 0, newState: state };
  if (zg.destroyed) return { error: "ziggurat_already_destroyed", crown_points: 0, newState: state };
  const finalBattleWindowMs = spec.constants.ziggurat_final_battle_duration_hours * MS_PER_HOUR;
  if (!zg.finalBattleStartedMs) {
    // فتح نافذة المعركة النهائية
  }
  const newHp = Math.max(0, zg.hp - damage);
  const destroyed = newHp <= 0;
  const ziggurat: LKZiggurat = { ...zg, hp: newHp, destroyed,
    finalBattleStartedMs: zg.finalBattleStartedMs || nowMs, destroyed_by: destroyed ? attackerKingdom : "" };
  const crown_points = destroyed ? spec.constants.final_crown_points : 0;
  const coins = destroyed ? spec.constants.ziggurat_destroy_reward_coins : 0;
  return { crowned: destroyed ? attackerKingdom : undefined, crown_points,
    newState: { ...state, ziggurat, crown_points: state.crown_points + crown_points,
      kvk_coins: state.kvk_coins + coins } };
}

/** متجر عملات KvK — العناصر من JSON فقط. */
export function buySeasonItem(spec: LostKingdomSpec, state: LostKingdomState, itemId: string):
  { error?: string; item?: { id: string; name: string; reward: Record<string, number | string> }; newState: LostKingdomState } {
  const item = spec.constants.season_store.items.find(i => i.id === itemId);
  if (!item) return { error: "unknown_item", newState: state };
  if (state.kvk_coins < item.cost) return { error: "insufficient_kvk_coins", newState: state };
  return { item: { id: item.id, name: item.name, reward: item.reward },
    newState: { ...state, kvk_coins: state.kvk_coins - item.cost } };
}
