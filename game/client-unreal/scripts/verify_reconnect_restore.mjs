#!/usr/bin/env node
/**
 * P7-T5 — حارس بنيوي لاستعادة الحالة بعد إعادة اتصال WebSocket.
 * لا يحاكي Unreal؛ يثبت عقد المصدر الذي يضمن أن الطلبات السلطوية تُطلق
 * بعد اتصالٍ مستعاد، وأن كل حالة خاصة لها طلب مستقل وآمن للتكرار.
 */
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';

const clientRoot = resolve(import.meta.dirname, '..');
const read = (relativePath) => readFileSync(resolve(clientRoot, relativePath), 'utf8');
const header = read('Source/Rok2/Public/Rok2Api.h');
const source = read('Source/Rok2/Private/Rok2Api.cpp');

const failures = [];
function requireText(haystack, needle, label) {
  if (!haystack.includes(needle)) failures.push(label);
}

requireText(header, 'void RestoreAuthoritativeState();', 'غياب واجهة RestoreAuthoritativeState العامة');
requireText(header, 'bool bRestoreOnNextWsConnection = false;', 'غياب راية الاستعادة بعد الاتصال');
requireText(header, 'bool bStateRestoreInFlight = false;', 'غياب مزلاج منع استعادة موازية');
requireText(header, 'int32 StateRestorePendingRequests = 0;', 'غياب عداد طلبات الاستعادة');
requireText(header, 'TFunction<void(const FString&)> OnErr = nullptr', 'غياب مسار فشل لطلبات GET');

requireText(source, 'void URok2Api::RestoreAuthoritativeState()', 'غياب تنفيذ دورة الاستعادة');
requireText(source, 'StateRestorePendingRequests = 5;', 'عدد طلبات الاستعادة لا يغطي الحزم السلطوية الخمس');
for (const endpoint of ['/v1/city', '/v1/world/snapshot', '/v1/commanders', '/v1/combat/reports', '/v1/alliance/rallies']) {
  requireText(source, endpoint, `الاستعادة لا تغطي ${endpoint}`);
}
for (const helper of ['FetchCommandersInternal(OnPartFinished);', 'FetchBattleReportsInternal(OnPartFinished);', 'FetchAllianceRalliesInternal(OnPartFinished);']) {
  requireText(source, helper, `الاستعادة لا تنتظر ${helper}`);
}
requireText(source, 'void URok2Api::CompleteAuthoritativeStateRestore()', 'غياب إنهاء دورة الاستعادة');
requireText(source, 'if (bStateRestoreInFlight) return;', 'غياب منع التزامن في دورة الاستعادة');
requireText(source, 'PushNotification(TEXT("connection"), TEXT("اكتملت المزامنة")', 'غياب تأكيد مزامنة الحالة للمستخدم');

requireText(source, 'const bool bShouldRestore = Self->bRestoreOnNextWsConnection', 'غياب شرط استعادة عند OnConnected');
requireText(source, 'if (bShouldRestore)\n\t\t\t{\n\t\t\t\tSelf->RestoreAuthoritativeState();', 'لا تُطلق الاستعادة من اتصال WebSocket العائد');
requireText(source, 'const bool bWasLive = Self->bWsConnected;', 'غياب تمييز الاتصال الحي السابق قبل إعادة الاتصال');
requireText(source, 'bRestoreOnNextWsConnection = false;\n\tbStateRestoreInFlight = false;', 'الفصل اليدوي لا يمسح حالة الاستعادة');

for (const helper of ['FetchCommandersInternal', 'FetchBattleReportsInternal', 'FetchAllianceRalliesInternal']) {
  const start = source.indexOf(`void URok2Api::${helper}`);
  const segment = start >= 0 ? source.slice(start, start + 2200) : '';
  if (!segment.includes('OnFinished')) failures.push(`${helper} لا ينهي عداد الاستعادة`);
  if (!segment.includes('const FString&')) failures.push(`${helper} لا يعالج فشل طلب القراءة`);
}

if (failures.length) {
  console.error('P7-T5 reconnect restore verification failed:');
  for (const failure of failures) console.error(`- ${failure}`);
  process.exit(1);
}

console.log('P7-T5 reconnect restore verification passed (authoritative 5-part bundle + reconnect guards).');
