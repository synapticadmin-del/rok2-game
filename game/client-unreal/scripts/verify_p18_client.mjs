// P18-T1/T2 guard: سلسلة البحث وأزرار المباني الثانوية من الخادم إلى الشاشة.
// يفحص البنية لا السلوك (بناء UE الفعلي يبقى شرط القبول النهائي):
//   1) عقود API: تصاريح وتنفيذات FetchResearch/StartResearch و_delegate.
//   2) تحليل المتطلبات ككائنات {id, level} — AsString على كائن يعيد ""
//      فكانت قائمة المتطلبات تُملأ فراغاً (عطل انحدار ممنوع).
//   3) جرحى/مستشفى في ParseCity — مصدر ورقة الشفاء.
//   4) الوصول: بطاقة المبنى تبث ← CityBuilder يربط ← GameMode يوجّه ←
//      شاشة البحث أو ورقة التدريب/الشفاء. قبل P18 كان الحدث بلا أي مشترك.

import { readFileSync, existsSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

const scriptDir = dirname(fileURLToPath(import.meta.url));
const clientRoot = resolve(scriptDir, '..');
const src = (p) => readFileSync(resolve(clientRoot, p), 'utf8').replace(/\r\n/g, '\n');

const apiH = src('Source/Rok2/Public/Rok2Api.h');
const apiCpp = src('Source/Rok2/Private/Rok2Api.cpp');
const typesH = src('Source/Rok2/Public/Rok2Types.h');
const researchH = src('Source/Rok2/Public/Rok2ResearchWidget.h');
const researchCpp = src('Source/Rok2/Private/Rok2ResearchWidget.cpp');
const gmH = src('Source/Rok2/Public/Rok2GameMode.h');
const gmCpp = src('Source/Rok2/Private/Rok2GameMode.cpp');
const cbCpp = src('Source/Rok2/Private/Rok2CityBuilder.cpp');
const sheetHPath = 'Source/Rok2/Public/Rok2TrainHealSheetWidget.h';
const sheetCppPath = 'Source/Rok2/Private/Rok2TrainHealSheetWidget.cpp';

let pass = 0;
const failures = [];
function check(name, cond) {
  if (cond) { pass++; }
  else { failures.push(name); }
}

// ── 1) عقود API ──
check('Api.h: FOnResearchLoaded delegate', apiH.includes('DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResearchLoaded)'));
check('Api.h: FetchResearch declaration', apiH.includes('void FetchResearch();'));
check('Api.h: StartResearch declaration', apiH.includes('void StartResearch(const FString& TechId);'));
check('Api.h: GetResearchState accessor', apiH.includes('GetResearchState()'));
check('Api.h: OnResearchLoaded BlueprintAssignable', apiH.includes('FOnResearchLoaded OnResearchLoaded;'));
check('Api.cpp: FetchResearch implementation', apiCpp.includes('void URok2Api::FetchResearch()'));
check('Api.cpp: StartResearch implementation', apiCpp.includes('void URok2Api::StartResearch('));
check('Api.cpp: FetchResearch على /v1/research', /FetchResearch\(\)[\s\S]{0,400}\/v1\/research/.test(apiCpp));
check('Api.cpp: StartResearch على /v1/city/research', /StartResearch\([\s\S]{0,300}\/v1\/city\/research/.test(apiCpp));
check('Api.cpp: بدء البحث يعيد قراءة الشجرة (FetchResearch في callback)', /StartResearch[\s\S]{0,900}FetchResearch\(\);/.test(apiCpp));

// ── 2) تحليل المتطلبات (منع انحدار عطل AsString) ──
check('Api.cpp: prerequisites تُقرأ ككائنات', apiCpp.includes('Prereq->AsObject()'));
check('Api.cpp: معرف المتطلب من حقل id', /Rok2Json::Str\(PrereqObj, TEXT\("id"\)\)/.test(apiCpp));
check('Api.cpp: لا AsString على المتطلبات (انحدار ممنوع)', !/Prereq->AsString\(\)/.test(apiCpp));

// ── 3) أنواع + جرحى/مستشفى ──
check('Types.h: FRok2TechNode', typesH.includes('struct FRok2TechNode'));
check('Types.h: FRok2ResearchState', typesH.includes('struct FRok2ResearchState'));
check('Types.h: bHasNextLevel', typesH.includes('bool bHasNextLevel'));
check('Types.h: NextAcademyRequirement', typesH.includes('NextAcademyRequirement'));
check('Types.h: City.Wounded خريطة الجرحى', /struct FRok2City[\s\S]{0,2000}TMap<FString, int32> Wounded;/.test(typesH));
check('Types.h: HospitalCapacity/Used', typesH.includes('HospitalCapacity') && typesH.includes('HospitalUsed'));
check('Api.cpp: ParseCity يقرأ wounded', /ParseCity[\s\S]{0,2500}TEXT\("wounded"\)/.test(apiCpp));
check('Api.cpp: ParseCity يقرأ hospital{capacity,used}', /ParseCity[\s\S]{0,3000}TEXT\("hospital"\)[\s\S]{0,200}TEXT\("used"\)/.test(apiCpp));

// ── 4) شاشة البحث ──
check('ResearchWidget.h: Setup(URok2Api*)', researchH.includes('void Setup(URok2Api* InApi);'));
check('ResearchWidget.h: RequestResearch', researchH.includes('RequestResearch(const FString& TechId);'));
check('ResearchWidget.cpp: تُبنى من GetResearchState (لا JSON محلي)', researchCpp.includes('GetResearchState()'));
check('ResearchWidget.cpp: تبويبات الفروع من أسماء الخادم', researchCpp.includes('BranchEconomy') && researchCpp.includes('BranchMilitary'));
check('ResearchWidget.cpp: حالة «متطلب ناقص» تعتمد Prerequisites', researchCpp.includes('Prerequisites'));

// ── 5) سلسلة الوصول: بطاقة ← CityBuilder ← GameMode ← الشاشات ──
check('BuildingDetail.h: الحدث موجود أصلاً', src('Source/Rok2/Public/Rok2BuildingDetailWidget.h').includes('FOnBuildingAction OnBuildingAction;'));
check('CityBuilder.cpp: اشتراك على OnBuildingAction', cbCpp.includes('OnBuildingAction.AddDynamic(GM, &ARok2GameMode::HandleBuildingAction)'));
check('GameMode.h: HandleBuildingAction معلن UFUNCTION', gmH.includes('void HandleBuildingAction(const FString& BuildingId, const FString& ActionKind);'));
check('GameMode.h: OpenResearchScreen معلن', gmH.includes('void OpenResearchScreen();'));
check('GameMode.h: عضو ResearchWidget', gmH.includes('URok2ResearchWidget* ResearchWidget;'));
check('GameMode.cpp: تنفيذ HandleBuildingAction', gmCpp.includes('void ARok2GameMode::HandleBuildingAction('));
check('GameMode.cpp: مسار research يفتح الشاشة', /HandleBuildingAction[\s\S]{0,600}OpenResearchScreen\(\);/.test(gmCpp));
check('GameMode.cpp: مسارا train/heal يفتحان الورقة', /HandleBuildingAction[\s\S]{0,900}URok2TrainHealSheetWidget/.test(gmCpp));
// P19-T4: الشرط كان «توست صادق» لأن شاشة الحانة لم تكن موجودة — والعطل الذي
// يحرسه هذا الفحص هو **الصمت** لا غياب التوست. الآن يفتح المسار الشاشة فعلاً،
// وهو ما كان التوست وعداً به. فيقبل الفحص الوفاء (شاشة) أو الوعد (توست) ويرفض
// الصمت وحده.
check('GameMode.cpp: chests لا يصمت (شاشة الحانة أو توست صادق)',
  /HandleBuildingAction[\s\S]{0,1400}(OpenTavernScreen\(\)|EmitToast)/.test(gmCpp));
check('GameMode.cpp: OpenResearchScreen ينشئ كسولاً ويجلب الشجرة', /OpenResearchScreen[\s\S]{0,800}FetchResearch\(\);/.test(gmCpp));

// ── 6) ورقة التدريب/الشفاء ──
check('Sheet: الملفان موجودان', existsSync(resolve(clientRoot, sheetHPath)) && existsSync(resolve(clientRoot, sheetCppPath)));
if (existsSync(resolve(clientRoot, sheetHPath)) && existsSync(resolve(clientRoot, sheetCppPath))) {
  const sheetH = src(sheetHPath);
  const sheetCpp = src(sheetCppPath);
  check('Sheet.h: Setup(نمط+مبنى)', sheetH.includes('void Setup(URok2Api* InApi, const FString& InMode, const FString& InBuildingId);'));
  check('Sheet.h: AdjustCount/HandleUnitAction عامان للوسيط', sheetH.includes('void AdjustCount(') && sheetH.includes('void HandleUnitAction('));
  check('Sheet.cpp: تدريب عبر Api->Train', sheetCpp.includes('Api->Train('));
  check('Sheet.cpp: شفاء عبر Api->HealWounded', sheetCpp.includes('Api->HealWounded('));
  check('Sheet.cpp: شفاء الكل', sheetCpp.includes('OnHealAllClicked'));
  check('Sheet.cpp: فرع المبنى من أسماء buildings.json', sheetCpp.includes('BranchForBuilding'));
  check('Sheet.cpp: عدّاد لمسي −/+ (لا SpinBox)', sheetCpp.includes('MakeStep') && !sheetCpp.includes('USpinBox'));
  check('Sheet.cpp: أسطح وحركة من نظام الرموز', sheetCpp.includes('Rok2Surface::') && sheetCpp.includes('URok2MotionLibrary::'));
  check('Sheet.cpp: تكرار التوقيع مطابق للهيدر', sheetCpp.includes('void URok2TrainHealSheetWidget::Setup('));
}

// ── 7) PLAN ──
const plan = readFileSync(resolve(clientRoot, '..', '..', 'PLAN.md'), 'utf8').replace(/\r\n/g, '\n');
check('PLAN.md: مرحلة P18 موجودة', plan.includes('المرحلة P18'));

console.log(`verify_p18_client: ${pass} PASSED, ${failures.length} FAILED`);
if (failures.length > 0) {
  for (const f of failures) console.error(`  FAILED: ${f}`);
  process.exit(1);
}
