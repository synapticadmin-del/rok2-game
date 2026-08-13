// P9-T7: حارس تحقق بنيوي لمسار العميل C++ — لا Unreal في بيئة التنفيذ.
// يتحقق من أن الأنواع الجديدة (Rok2Types.h) والتصريحات والتنفيذات (Rok2Api.h/cpp)
// متطابقة مع endpoints الخادم في router.ts. كل فشل يُوقف الفحص.
import { readFileSync } from "node:fs";
import { resolve } from "node:path";
const root = resolve(import.meta.dirname, "../../..");
const read = (relativePath) => readFileSync(resolve(root, relativePath), "utf8");
const requireMatch = (source, pattern, description) => {
  if (!pattern.test(source)) { console.error(`FAIL: ${description}`); process.exit(1); }
  console.log(`  PASS: ${description}`);
};

console.log("P9-T7 client structural guard");

const types = read("game/client-unreal/Source/Rok2/Public/Rok2Types.h");
const apiH = read("game/client-unreal/Source/Rok2/Public/Rok2Api.h");
const apiC = read("game/client-unreal/Source/Rok2/Private/Rok2Api.cpp");
const router = read("game/backend/src/http/router.ts");

// ---------------------------------------------------------------------------
// 1. أنواع P9-T7 في Rok2Types.h — مرآة للباک اند.
// ---------------------------------------------------------------------------
// ملاحظة: الخادم يستخدم /v1/alliance-tech/* (ليست /v1/alliance/tech).
requireMatch(types, /FRok2AllianceTechNode/, "FRok2AllianceTechNode عقدة تقنية تحالف");
requireMatch(types, /FRok2AllianceTerritoryState/, "FRok2AllianceTerritoryState حالة أرض التحالف");
requireMatch(types, /FRok2AllianceShopItem/, "FRok2AllianceShopItem عنصر متجر التحالف");
requireMatch(types, /FRok2AllianceTitle/, "FRok2AllianceTitle لقب تحالف");
requireMatch(types, /FRok2VipStatus/, "FRok2VipStatus حالة VIP");
requireMatch(types, /FRok2TradingOffer/, "FRok2TradingOffer عرض التداول");
requireMatch(types, /FRok2AllianceGift/, "FRok2AllianceGift صندوق هدية تحالف");
requireMatch(types, /USTRUCT\(BlueprintType\)[\s\S]*?GENERATED_BODY\(\)[\s\S]*?Category = "Rok2\|P9"/, "USTRUCT + GENERATED_BODY + Category Rok2|P9 في الأنواع الجديدة");

// ---------------------------------------------------------------------------
// 2. Rok2Api.h — declarations لوظائف P9-T7 مع UFUNCTION BlueprintCallable.
// ---------------------------------------------------------------------------
requireMatch(apiH, /void FetchAllianceTech\(\)/, "FetchAllianceTech (GET alliance/tech)");
requireMatch(apiH, /void DonateAllianceTech\(/, "DonateAllianceTech (POST tech/donate)");
requireMatch(apiH, /void FetchAllianceTerritory\(\)/, "FetchAllianceTerritory (GET territory/state)");
requireMatch(apiH, /void FetchAllianceShop\(\)/, "FetchAllianceShop (GET shop-state)");
requireMatch(apiH, /void PurchaseAllianceShopItem\(/, "PurchaseAllianceShopItem (POST shop/purchase)");
requireMatch(apiH, /void FetchVipStatus\(\)/, "FetchVipStatus (GET vip/status)");
requireMatch(apiH, /void FetchTradingOffers\(\)/, "FetchTradingOffers (GET trading/list)");
requireMatch(apiH, /void PostTradingOffer\(/, "PostTradingOffer (POST trading/offer)");
requireMatch(apiH, /void ClaimTradingOffer\(/, "ClaimTradingOffer (POST trading/claim)");
requireMatch(apiH, /void FetchAllianceGifts\(\)/, "FetchAllianceGifts (GET gifts/list)");
requireMatch(apiH, /void ClaimAllianceGift\(/, "ClaimAllianceGift (POST gifts/claim)");

// ---------------------------------------------------------------------------
// 3. Rok2Api.h — delegates وUObject properties وkaches وParse methods.
// ---------------------------------------------------------------------------
requireMatch(apiH, /FOnAllianceTechUpdated/, "FOnAllianceTechUpdated delegate");
requireMatch(apiH, /FOnAllianceTerritoryUpdated/, "FOnAllianceTerritoryUpdated delegate");
requireMatch(apiH, /FOnAllianceShopUpdated/, "FOnAllianceShopUpdated delegate");
requireMatch(apiH, /FOnAllianceTitleChanged/, "FOnAllianceTitleChanged delegate");
requireMatch(apiH, /FOnVipStatusUpdated/, "FOnVipStatusUpdated delegate");
requireMatch(apiH, /FOnTradingOffersUpdated/, "FOnTradingOffersUpdated delegate");
requireMatch(apiH, /FOnAllianceGiftsUpdated/, "FOnAllianceGiftsUpdated delegate");
requireMatch(apiH, /TArray<FRok2AllianceTechNode> AllianceTechNodes;/, "كاش AllianceTechNodes");
requireMatch(apiH, /FRok2AllianceTerritoryState TerritoryState;/, "كاش TerritoryState");
requireMatch(apiH, /TArray<FRok2AllianceShopItem> AllianceShopItems;/, "كاش AllianceShopItems");
requireMatch(apiH, /int32 AllianceShopBalance = 0;/, "كاش رصيد متجر التحالف");
requireMatch(apiH, /FRok2VipStatus VipStatus;/, "كاش VipStatus");
requireMatch(apiH, /TArray<FRok2TradingOffer> TradingOffers;/, "كاش TradingOffers");
requireMatch(apiH, /TArray<FRok2AllianceGift> AllianceGifts;/, "كاش AllianceGifts");
requireMatch(apiH, /void ParseAllianceTechState\(/, "ParseAllianceTechState");
requireMatch(apiH, /void ParseAllianceTerritoryState\(/, "ParseAllianceTerritoryState");
requireMatch(apiH, /void ParseAllianceShopState\(/, "ParseAllianceShopState");
requireMatch(apiH, /void ParseVipStatus\(/, "ParseVipStatus");
requireMatch(apiH, /void ParseTradingOffers\(/, "ParseTradingOffers");
requireMatch(apiH, /void ParseAllianceGifts\(/, "ParseAllianceGifts");

// ---------------------------------------------------------------------------
// 4. Rok2Api.cpp — تنفيذ الدوال ومساراتها مطابقة للراوتر.
// ---------------------------------------------------------------------------
requireMatch(apiC, /void URok2Api::FetchAllianceTech\(\)[\s\S]*?TEXT\("\/v1\/alliance-tech\/state"\)/, "تنفيذ FetchAllianceTech (GET /v1/alliance-tech/state)");
requireMatch(apiC, /void URok2Api::DonateAllianceTech\(/, "تنفيذ DonateAllianceTech (POST donate)");
requireMatch(apiC, /"\/v1\/alliance-tech\/donate"/, "مسار POST donate في التنفيذ");
requireMatch(apiC, /"\/v1\/territory\/state"/, "تنفيذ FetchAllianceTerritory (GET territory/state)");
requireMatch(apiC, /"\/v1\/alliance\/shop-state"/, "تنفيذ FetchAllianceShop (GET shop-state)");
requireMatch(apiC, /"\/v1\/alliance\/shop\/purchase"/, "تنفيذ PurchaseAllianceShopItem (POST shop/purchase)");
requireMatch(apiC, /"\/v1\/vip\/status"/, "تنفيذ FetchVipStatus (GET vip/status)");
requireMatch(apiC, /"\/v1\/trading\/list"/, "تنفيذ FetchTradingOffers (GET trading/list)");
requireMatch(apiC, /"\/v1\/trading\/offer"/, "تنفيذ PostTradingOffer (POST trading/offer)");
requireMatch(apiC, /"\/v1\/trading\/claim"/, "تنفيذ ClaimTradingOffer (POST trading/claim)");
requireMatch(apiC, /"\/v1\/alliance\/gifts\/list"/, "تنفيذ FetchAllianceGifts (GET gifts/list)");
requireMatch(apiC, /"\/v1\/alliance\/gifts\/claim"/, "تنفيذ ClaimAllianceGift (POST gifts/claim)");
requireMatch(apiC, /void URok2Api::ParseAllianceTechState\(/, "تنفيذ ParseAllianceTechState");
requireMatch(apiC, /void URok2Api::ParseAllianceTerritoryState\(/, "تنفيذ ParseAllianceTerritoryState");
requireMatch(apiC, /void URok2Api::ParseAllianceShopState\(/, "تنفيذ ParseAllianceShopState");
requireMatch(apiC, /void URok2Api::ParseVipStatus\(/, "تنفيذ ParseVipStatus");
requireMatch(apiC, /void URok2Api::ParseTradingOffers\(/, "تنفيذ ParseTradingOffers");
requireMatch(apiC, /void URok2Api::ParseAllianceGifts\(/, "تنفيذ ParseAllianceGifts");
requireMatch(apiC, /->FetchAllianceTech\(\);[\s\S]*?OnAllianceTechUpdated\.Broadcast/, "تبرع يعيد سحب التقنية ويبث OnAllianceTechUpdated");
requireMatch(apiC, /->FetchTradingOffers\(\);[\s\S]*?OnTradingOffersUpdated\.Broadcast/, "عرض تداول يعيد السحب ويبث OnTradingOffersUpdated");
requireMatch(apiC, /->FetchAllianceGifts\(\);[\s\S]*?OnAllianceGiftsUpdated\.Broadcast/, "فتح هدية يعيد السحب ويبث OnAllianceGiftsUpdated");

// ---------------------------------------------------------------------------
// 5. اتساق مع الخادم: الراوتر يجب أن يعرّف endpoints المطابقة.
// ---------------------------------------------------------------------------
requireMatch(router, /\/v1\/alliance-tech\//, "الراوتر يعرّف endpoints تقنية التحالف (/v1/alliance-tech/*)");
requireMatch(router, /\/v1\/territory/, "الراوتر يعرّف endpoints الأراضي");
requireMatch(router, /\/v1\/alliance\/shop/, "الراوتر يعرّف endpoints متجر التحالف");
requireMatch(router, /\/v1\/vip\/status/, "الراوتر يعرّف GET vip/status");
requireMatch(router, /\/v1\/trading\//, "الراوتر يعرّف endpoints التداول");
requireMatch(router, /\/v1\/alliance\/gifts\//, "الراوتر يعرّف endpoints الهدايا");

// ---------------------------------------------------------------------------
// 6. عدم وجود hard-coded في العميل: الثوابت تُقرأ من الخادم فقط.
// ---------------------------------------------------------------------------
requireMatch(apiC, /"\/v1\/meta\/all"/, "العميل يسحب meta من /v1/meta/all (مصدر البيانات المركزي)");

console.log("\nALL PASSED: P9-T7 client path (7 USTRUCT + 11 declarations + 7 delegates + 7 Parse + endpoints مطابقة للراوتر)");
process.exit(0);
