// P8-T7: فحص بنيوي لمسار P8 للعميل C++ — مواهب القادة، معدات الحدادة، حماية
// المدينة (AP/dragons shield + تهجير)، المهام اليومية والجوائز، وملك المملكة.
// لا يوجد UE SDK في بيئة التنفيذ؛ الفحص بُنيوي على مصدر Rok2 فقط (Rok2Types.h،
// Rok2Api.h، Rok2Api.cpp، Rok2WorldRenderer.cpp/h، Rok2.h). كل بند يجب أن يجد
// تعريفات struct + إعلان function + implementation مطابق.
//
// endpoints الباك اند المستهدفة (router.ts الفعلية):
// GET /v1/commanders | POST /v1/commander/talent/allocate | POST /v1/commander/talent/reset
// GET /v1/commander/equipment | POST /v1/commander/equipment/craft | POST /v1/commander/equipment/equip
// POST /v1/commander/equipment/unequip | POST /v1/commander/equipment/merge
// GET /v1/ap/state | POST /v1/shield/activate | POST /v1/city/relocate
// GET /v1/quests | POST /v1/quests/claim | POST /v1/quests/redeem-golden-key | POST /v1/quests/redeem-weekly-chest
// Exit code 0 عند نجاح كل الفحوصات.
import { readFileSync, existsSync } from 'node:fs';
import { join } from 'node:path';
const root = join(import.meta.dirname ?? process.cwd(), '..', '..', '..');
const src = join(root, 'game', 'client-unreal', 'Source', 'Rok2');
const T = join(src, 'Public', 'Rok2Types.h');
const H = join(src, 'Public', 'Rok2Api.h');
const C = join(src, 'Private', 'Rok2Api.cpp');
const RW = join(src, 'Private', 'Rok2WorldRenderer.cpp');
const RWH = join(src, 'Public', 'Rok2WorldRenderer.h');
const M = join(src, 'Rok2.h');
let pass = 0, fail = 0;
function check(name, ok, detail = '') {
  if (ok) { pass++; console.log(`CHECK-PASS ${name}`); }
  else { fail++; console.log(`CHECK-FAIL ${name}${detail ? ' — ' + detail : ''}`); }
}
const types = readFileSync(T, 'utf8');
const apiH = readFileSync(H, 'utf8');
const apiC = readFileSync(C, 'utf8');
const renderer = readFileSync(RW, 'utf8');
const rendererH = readFileSync(RWH, 'utf8');
const module = existsSync(M) ? readFileSync(M, 'utf8') : '';
const hasDecl = (decl) => apiH.includes(decl);
const hasImpl = (impl) => apiC.includes(impl);
// ---------------------------------------------------------------------------
// 1. أنواع P8-T7 في Rok2Types.h — structs المواهب/المعدات/الدرع/المهام/الملك.
// ---------------------------------------------------------------------------
check('type-talent-node', types.includes('FRok2TalentNode') && types.includes('PointsAvailable'), 'FRok2TalentNode مع نقاط متاحة في Types.h');
check('type-talent-tree', /FRok2TalentTree/.test(types), 'FRok2TalentTree شجرة مواهب كاملة');
check('type-equipment-item', /FRok2EquipmentItem/.test(types), 'FRok2EquipmentItem قطعة معدات مع stats');
check('type-equipment-blueprint', /FRok2EquipmentBlueprint/.test(types), 'FRok2EquipmentBlueprint مواصفات التصنيع');
check('type-shield-option', /FRok2ShieldOption/.test(types), 'FRok2ShieldOption خيار درع Gems/دقائق');
check('type-ap-state', /FRok2ActionPointState/.test(types) && types.includes('ApCap') && types.includes('ShieldUntilMs'),
  'FRok2ActionPointState حالة AP للمدينة (cap/درع/حمى الحرب/تهجير)');
check('type-daily-quest', /FRok2DailyQuest/.test(types), 'FRok2DailyQuest مهمة يومية بمهامها');
check('type-quest-state', /FRok2QuestState/.test(types), 'FRok2QuestState نقاط + مفتاح ذهبي + صندوق أسبوعي');
check('type-king-marker', /FRok2KingMarker/.test(types), 'FRok2KingMarker موقع الملك على العرش');
check('type-game-meta', /FRok2GameMeta/.test(types), 'FRok2GameMeta خزينة meta للمواهب والمعدات');
check('snapshot-king', /King/.test(types) && /FRok2KingMarker/.test(types), 'FRok2WorldSnapshot يحمل KingMarker');
// ---------------------------------------------------------------------------
// 2. Rok2Api.h — declarations لوظائف P8-T7 مع UFUNCTION BlueprintCallable.
// ---------------------------------------------------------------------------
check('decl-fetch-talents', hasDecl('FetchTalents'), 'FetchTalents في Rok2Api.h');
check('decl-allocate-talent', hasDecl('AllocateTalent'), 'AllocateTalent (POST talent/allocate)');
check('decl-respec-talents', hasDecl('RespecTalents'), 'RespecTalents (POST talent/reset)');
check('decl-fetch-equipment', hasDecl('FetchEquipment'), 'FetchEquipment (GET commander/equipment)');
check('decl-craft-equipment', hasDecl('CraftEquipment'), 'CraftEquipment (POST equipment/craft)');
check('decl-equip-item', hasDecl('EquipItem'), 'EquipItem (POST equipment/equip)');
check('decl-unequip-item', hasDecl('UnequipItem'), 'UnequipItem (POST equipment/unequip)');
check('decl-merge-items', hasDecl('MergeItems'), 'MergeItems (POST equipment/merge)');
check('decl-fetch-shield', hasDecl('FetchShieldOptions'), 'FetchShieldOptions (GET ap/state)');
check('decl-activate-shield', hasDecl('ActivateShield'), 'ActivateShield (POST shield/activate)');
check('decl-relocate-city', hasDecl('RelocateCity'), 'RelocateCity (POST city/relocate بوضع random/targeted)');
check('decl-fetch-quests', hasDecl('FetchQuests'), 'FetchQuests (GET quests)');
check('decl-claim-quest', hasDecl('ClaimQuest'), 'ClaimQuest (POST quests/claim)');
check('decl-golden-key', hasDecl('RedeemGoldenKey'), 'RedeemGoldenKey (POST quests/redeem-golden-key)');
check('decl-weekly-chest', hasDecl('RedeemWeeklyChest'), 'RedeemWeeklyChest (POST quests/redeem-weekly-chest)');
check('decl-fetch-king', hasDecl('FetchKing'), 'FetchKing (RefreshWorld → UpsertKing)');
check('decl-march-holy-site', hasDecl('MarchToHolySite'), 'MarchToHolySite (P8-T4/T5 targetType=holy_site)');
// ---------------------------------------------------------------------------
// 3. Rok2Api.cpp — implementations تطابق endpoints الباك اند الفعلية.
// ---------------------------------------------------------------------------
check('impl-fetch-talents', hasImpl('URok2Api::FetchTalents'), 'FetchTalents من GET /v1/commanders');
check('impl-allocate-talent', hasImpl('URok2Api::AllocateTalent') && apiC.includes('/v1/commander/talent/allocate'),
  'AllocateTalent → POST /v1/commander/talent/allocate {commanderId,nodeId,points}');
check('impl-respec-talents', hasImpl('URok2Api::RespecTalents') && apiC.includes('/v1/commander/talent/reset'),
  'RespecTalents → POST /v1/commander/talent/reset');
check('impl-fetch-equipment', hasImpl('URok2Api::FetchEquipment') && apiC.includes('/v1/commander/equipment?commanderId='),
  'FetchEquipment → GET /v1/commander/equipment');
check('impl-craft-equipment', hasImpl('URok2Api::CraftEquipment') && apiC.includes('/v1/commander/equipment/craft'),
  'CraftEquipment → POST equipment/craft {commanderId,slot,quality}');
check('impl-equip-item', hasImpl('URok2Api::EquipItem') && apiC.includes('/v1/commander/equipment/equip'),
  'EquipItem → POST equipment/equip {commanderId,itemId}');
check('impl-unequip-item', hasImpl('URok2Api::UnequipItem') && apiC.includes('/v1/commander/equipment/unequip'),
  'UnequipItem → POST equipment/unequip {commanderId,slot}');
check('impl-merge-items', hasImpl('URok2Api::MergeItems') && apiC.includes('/v1/commander/equipment/merge'),
  'MergeItems → POST equipment/merge {commanderId,itemIds}');
check('impl-activate-shield', hasImpl('URok2Api::ActivateShield') && apiC.includes('/v1/shield/activate'),
  'ActivateShield → POST /v1/shield/activate {duration_minutes}');
check('impl-relocate-city', hasImpl('URok2Api::RelocateCity') && apiC.includes('/v1/city/relocate'),
  'RelocateCity → POST /v1/city/relocate {mode,x,y}');
check('impl-fetch-quests', hasImpl('URok2Api::FetchQuests') && apiC.includes('"/v1/quests"'),
  'FetchQuests → GET /v1/quests');
check('impl-claim-quest', hasImpl('URok2Api::ClaimQuest') && apiC.includes('/v1/quests/claim'),
  'ClaimQuest → POST /v1/quests/claim {id}');
check('impl-golden-key', hasImpl('URok2Api::RedeemGoldenKey') && apiC.includes('/v1/quests/redeem-golden-key'),
  'RedeemGoldenKey → POST /v1/quests/redeem-golden-key');
check('impl-weekly-chest', hasImpl('URok2Api::RedeemWeeklyChest') && apiC.includes('/v1/quests/redeem-weekly-chest'),
  'RedeemWeeklyChest → POST /v1/quests/redeem-weekly-chest');
check('impl-fetch-king', hasImpl('URok2Api::FetchKing'), 'FetchKing → RefreshWorld');
check('impl-parse-talent-node', hasImpl('URok2Api::ParseTalentNode'), 'ParseTalentNode يحلل node المواهب من JSON');
check('impl-parse-equipment-item', hasImpl('URok2Api::ParseEquipmentItem'), 'ParseEquipmentItem يحلل id/slot/blueprint/quality/stats');
check('impl-parse-quest', hasImpl('URok2Api::ParseQuest'), 'ParseQuest يحلل daily/weekly {id,goal,progress,points,claimed}');
check('impl-upsert-king', hasImpl('URok2Api::UpsertKing'), 'UpsertKing يبث OnKingUpdated');
check('impl-parse-shield-state', hasImpl('URok2Api::ParseShieldState'), 'ParseShieldState يحلل ap/shield_options/relocation');
check('impl-parse-quest-state', hasImpl('URok2Api::ParseQuestState'), 'ParseQuestState يبث OnQuestsUpdated');
// ---------------------------------------------------------------------------
// 4. ParseWorld يقرأ king/throne/holySites من اللقطة السلطوية.
// ---------------------------------------------------------------------------
check('parseworld-king', apiC.includes('Obj->TryGetObjectField(TEXT("king")') && apiC.includes('UpsertKing(NewKing)'),
  'ParseWorld يقرأ snapshot.king ويبث UpsertKing');
check('parseworld-throne', apiC.includes('Obj->TryGetObjectField(TEXT("throne")'),
  'ParseWorld يقرأ snapshot.throne لموقع العرش');
check('parseworld-holy-sites', apiC.includes('Obj->TryGetArrayField(TEXT("holySites")') && apiC.includes('CapturedSiteIds'),
  'ParseWorld يسجل المواقع المقدسة المحتلة');
// ---------------------------------------------------------------------------
// 5. FetchMeta يسحب meta للمواهب والمعدات (خزينة المواصفات).
// ---------------------------------------------------------------------------
check('meta-talents', apiC.includes('/v1/meta/talents'), 'FetchMeta يسحب /v1/meta/talents');
check('meta-equipment', apiC.includes('/v1/meta/equipment'), 'FetchMeta يسحب /v1/meta/equipment');
// ---------------------------------------------------------------------------
// 6. WorldRenderer يرسم علامة الملك على العرش (P8-T7).
// ---------------------------------------------------------------------------
check('renderer-king-marker', rendererH.includes('DrawKingMarker'), 'WorldRenderer.h يعلن DrawKingMarker');
check('renderer-king-impl', renderer.includes('DrawKingMarker'), 'DrawKingMarker تُستدعى من RefreshFromApi');
check('renderer-throne', /SpawnedThrone/.test(renderer) || /Throne/.test(renderer), 'العرش يُعرض في RefreshFromApi');
// ---------------------------------------------------------------------------
// 7. موديول Rok2 يدرج الملفات + لا تناقض بين decl/impl.
// ---------------------------------------------------------------------------
const decls = ['FetchTalents', 'AllocateTalent', 'RespecTalents', 'FetchEquipment', 'CraftEquipment',
  'EquipItem', 'UnequipItem', 'MergeItems', 'FetchShieldOptions', 'ActivateShield', 'RelocateCity',
  'FetchQuests', 'ClaimQuest', 'RedeemGoldenKey', 'RedeemWeeklyChest', 'FetchKing', 'MarchToHolySite'];
let declImplMatch = true;
for (const d of decls) {
  const inH = hasDecl(`void ${d}`) || apiH.includes(d);
  const inC = apiC.includes(`URok2Api::${d}`);
  if (!inH || !inC) { declImplMatch = false; console.log(`  mismatch: ${d} H=${inH} C=${inC}`); }
}
check('decl-impl-match', declImplMatch, 'كل declarations لها implementation مطابق');
console.log(`\n${pass} PASSED, ${fail} FAILED (client-unreal)`);
process.exit(fail > 0 ? 1 : 0);
