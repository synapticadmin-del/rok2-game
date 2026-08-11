#!/usr/bin/env node
// verify_ui_chat.mjs — P6-T6: فحص بنيوي لدردشة HUD الحية
// يتحقق من وجود الملفات والهياكل الأساسية في الخادم والعميل.

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const REPO = join(__dirname, '..');
const DATA = join(REPO, 'data');
const BACKEND = join(REPO, 'game', 'backend', 'src');
const SRC = join(REPO, 'game', 'client-unreal', 'Source', 'Rok2');
const PUB = join(SRC, 'Public');
const PRIV = join(SRC, 'Private');

let passed = 0;
let failed = 0;

function ok(name) { console.log(`  ✅ ${name}`); passed++; }
function fail(name, detail = '') { console.error(`  ❌ ${name}${detail ? ' — ' + detail : ''}`); failed++; }
function check(name, condition, detail = '') { if (condition) ok(name); else fail(name, detail); }
function read(rel) { return readFileSync(rel, 'utf8').replace(/\r\n/g, '\n'); }

// ---------------------------------------------------------------------------
console.log('\n[1] data/chat.json — مواصفة الدردشة');
// ---------------------------------------------------------------------------
const chatJsonPath = join(DATA, 'chat.json');
check('data/chat.json exists', existsSync(chatJsonPath));
if (existsSync(chatJsonPath)) {
  const cfg = JSON.parse(readFileSync(chatJsonPath, 'utf8'));
  check('has channels.kingdom', !!cfg.channels?.kingdom, 'missing channels.kingdom');
  check('has channels.alliance', !!cfg.channels?.alliance, 'missing channels.alliance');
  check('channels.kingdom.requiresAlliance = false', cfg.channels?.kingdom?.requiresAlliance === false);
  check('channels.alliance.requiresAlliance = true', cfg.channels?.alliance?.requiresAlliance === true);
  check('has limits.maxTextLength', typeof cfg.limits?.maxTextLength === 'number', `got ${cfg.limits?.maxTextLength}`);
  check('has limits.rateLimit.windowMs', typeof cfg.limits?.rateLimit?.windowMs === 'number');
  check('has limits.rateLimit.maxMessages', typeof cfg.limits?.rateLimit?.maxMessages === 'number');
  check('has ui.maxBubbleWidthPx', typeof cfg.ui?.maxBubbleWidthPx === 'number');
}

// ---------------------------------------------------------------------------
console.log('\n[2] backend/src/data/chat.json — نسخة مطابقة');
// ---------------------------------------------------------------------------
const backendChatPath = join(BACKEND, 'data', 'chat.json');
check('backend/src/data/chat.json exists', existsSync(backendChatPath));
if (existsSync(chatJsonPath) && existsSync(backendChatPath)) {
  const root = readFileSync(chatJsonPath, 'utf8');
  const back = readFileSync(backendChatPath, 'utf8');
  check('backend chat.json matches root', root === back, 'files differ');
}

// ---------------------------------------------------------------------------
console.log('\n[3] gameData.ts — import chatSpec + getChatConfig');
// ---------------------------------------------------------------------------
const gdPath = join(BACKEND, 'lib', 'gameData.ts');
check('gameData.ts exists', existsSync(gdPath));
if (existsSync(gdPath)) {
  const gd = read(gdPath);
  check('imports chatSpec', gd.includes('chat.json'));
  check('exports getChatConfig', gd.includes('getChatConfig'));
}

// ---------------------------------------------------------------------------
console.log('\n[4] KingdomShard.ts — ChatMessage + migration + WS');
// ---------------------------------------------------------------------------
const ksPath = join(BACKEND, 'do', 'KingdomShard.ts');
check('KingdomShard.ts exists', existsSync(ksPath));
if (existsSync(ksPath)) {
  const ks = read(ksPath);
  check('has ChatMessage type', ks.includes('type ChatMessage'));
  check('has chatHistory array', ks.includes('chatHistory'));
  check('has chatRateLimit', ks.includes('chatRateLimit'));
  check('has migration ver<6', ks.includes('ver < 6'));
  check('has chat_messages table', ks.includes('chat_messages'));
  check('has chatHistory in snapshot()', ks.includes('chatHistory:') || ks.includes('chatHistory :'));
  check('handles chat_send WS', ks.includes('"chat_send"'));
  check('handles chat_history WS', ks.includes('"chat_history"'));
  check('broadcasts chat_message', ks.includes('"chat_message"'));
  check('validates channel', ks.includes('"bad_channel"'));
  check('validates alliance membership', ks.includes('"no_alliance"'));
  check('rate limits chat', ks.includes('"rate_limited"'));
  check('persists to SQLite', ks.includes('INSERT INTO chat_messages'));
  check('imports getChatConfig', ks.includes('getChatConfig'));
}

// ---------------------------------------------------------------------------
console.log('\n[5] Rok2Types.h — FRok2ChatMessage');
// ---------------------------------------------------------------------------
const typesPath = join(PUB, 'Rok2Types.h');
check('Rok2Types.h exists', existsSync(typesPath));
if (existsSync(typesPath)) {
  const t = read(typesPath);
  check('has FRok2ChatMessage struct', t.includes('FRok2ChatMessage'));
  check('has Channel field', t.includes('FString Channel'));
  check('has PlayerId field', t.includes('FString PlayerId'));
  check('has PlayerName field', t.includes('FString PlayerName'));
  check('has Civ field', t.includes('FString Civ'));
  check('has Text field', t.includes('FString Text'));
  check('has TimestampMs field', t.includes('TimestampMs'));
}

// ---------------------------------------------------------------------------
console.log('\n[6] Rok2Api.h — SendChat + OnChatMessage + delegate');
// ---------------------------------------------------------------------------
const apiHPath = join(PUB, 'Rok2Api.h');
check('Rok2Api.h exists', existsSync(apiHPath));
if (existsSync(apiHPath)) {
  const h = read(apiHPath);
  check('has FOnChatMessage delegate', h.includes('FOnChatMessage'));
  check('has OnChatMessage UPROPERTY', h.includes('OnChatMessage'));
  check('has SendChat method', h.includes('SendChat'));
  check('has ChatHistory member', h.includes('ChatHistory'));
  check('has UnreadChatCount', h.includes('UnreadChatCount'));
  check('has MarkChatRead', h.includes('MarkChatRead'));
  check('has GetChatHistory', h.includes('GetChatHistory'));
  check('has PushChatMessage', h.includes('PushChatMessage'));
}

// ---------------------------------------------------------------------------
console.log('\n[7] Rok2Api.cpp — WS dispatch + SendChat + PushChatMessage');
// ---------------------------------------------------------------------------
const apiCppPath = join(PRIV, 'Rok2Api.cpp');
check('Rok2Api.cpp exists', existsSync(apiCppPath));
if (existsSync(apiCppPath)) {
  const c = read(apiCppPath);
  check('handles chat_message WS', c.includes('"chat_message"'));
  check('handles chat_history WS', c.includes('"chat_history"'));
  check('parses chat from snapshot', c.includes('chatHistory'));
  check('implements SendChat', c.includes('void URok2Api::SendChat'));
  check('sends chat_send via WS', c.includes('chat_send'));
  check('implements PushChatMessage', c.includes('void URok2Api::PushChatMessage'));
  check('broadcasts OnChatMessage', c.includes('OnChatMessage.Broadcast'));
  check('increments UnreadChatCount', c.includes('UnreadChatCount++'));
}

// ---------------------------------------------------------------------------
console.log('\n[8] Rok2ChatWidget.h — class definition');
// ---------------------------------------------------------------------------
const cwHPath = join(PUB, 'Rok2ChatWidget.h');
check('Rok2ChatWidget.h exists', existsSync(cwHPath));
if (existsSync(cwHPath)) {
  const h = read(cwHPath);
  check('extends UUserWidget', h.includes(': public UUserWidget'));
  check('has Api member', h.includes('URok2Api* Api'));
  check('has OnChatReceived', h.includes('OnChatReceived'));
  check('has SendChat integration', h.includes('Channel') && h.includes('Text'));
  check('has MessageScroll', h.includes('MessageScroll'));
  check('has InputField', h.includes('InputField'));
  check('has SendButton', h.includes('SendButton'));
  check('has KingdomTab', h.includes('KingdomTab'));
  check('has AllianceTab', h.includes('AllianceTab'));
  check('has MinimizeButton', h.includes('MinimizeButton'));
  check('has AddMessageBubble', h.includes('AddMessageBubble'));
  check('has GetCivColor', h.includes('GetCivColor'));
}

// ---------------------------------------------------------------------------
console.log('\n[9] Rok2ChatWidget.cpp — implementation');
// ---------------------------------------------------------------------------
const cwCppPath = join(PRIV, 'Rok2ChatWidget.cpp');
check('Rok2ChatWidget.cpp exists', existsSync(cwCppPath));
if (existsSync(cwCppPath)) {
  const c = read(cwCppPath);
  check('implements NativeConstruct', c.includes('void URok2ChatWidget::NativeConstruct'));
  check('implements BuildWidgetTree', c.includes('void URok2ChatWidget::BuildWidgetTree'));
  check('implements AddMessageBubble', c.includes('void URok2ChatWidget::AddMessageBubble'));
  check('implements OnChatReceived', c.includes('void URok2ChatWidget::OnChatReceived'));
  check('implements SendChat via Api', c.includes('Api->SendChat'));
  check('implements OnMinimizeClicked', c.includes('void URok2ChatWidget::OnMinimizeClicked'));
  check('implements SwitchChannel', c.includes('void URok2ChatWidget::SwitchChannel'));
  check('civ color: rome', c.includes('TEXT("rome")'));
  check('civ color: china', c.includes('TEXT("china")'));
  check('civ color: arabia', c.includes('TEXT("arabia")'));
  check('civ color: egypt', c.includes('TEXT("egypt")'));
  check('civ color: vikings', c.includes('TEXT("vikings")'));
  check('civ color: japan', c.includes('TEXT("japan")'));
  check('scrolls to end', c.includes('ScrollToEnd'));
  check('uses URok2MotionLibrary', c.includes('URok2MotionLibrary'));
  check('uses URok2Typography', c.includes('URok2Typography'));
}

// ---------------------------------------------------------------------------
console.log('\n[10] Rok2HudWidget.h — chat button + delegate');
// ---------------------------------------------------------------------------
const hudHPath = join(PUB, 'Rok2HudWidget.h');
check('Rok2HudWidget.h exists', existsSync(hudHPath));
if (existsSync(hudHPath)) {
  const h = read(hudHPath);
  check('has OnChatAction delegate', h.includes('OnChatAction'));
  check('has ChatButton member', h.includes('ChatButton'));
  check('has ChatBadgeText member', h.includes('ChatBadgeText'));
  check('has ChatIcon member', h.includes('ChatIcon'));
  check('has OnChatClickedHandler', h.includes('OnChatClickedHandler'));
  check('has UpdateChatBadge', h.includes('UpdateChatBadge'));
}

// ---------------------------------------------------------------------------
console.log('\n[11] Rok2HudWidget.cpp — chat button build + handler');
// ---------------------------------------------------------------------------
const hudCppPath = join(PRIV, 'Rok2HudWidget.cpp');
check('Rok2HudWidget.cpp exists', existsSync(hudCppPath));
if (existsSync(hudCppPath)) {
  const c = read(hudCppPath);
  check('builds ChatButton in BuildLeftCluster', c.includes('ChatPill') || c.includes('ChatButton'));
  check('calls OnChatAction.Broadcast', c.includes('OnChatAction.Broadcast'));
  check('calls MarkChatRead', c.includes('Api->MarkChatRead'));
  check('calls UpdateChatBadge in tick', c.includes('UpdateChatBadge()'));
  check('chat badge shows count', c.includes('ChatBadgeText'));
}

// ---------------------------------------------------------------------------
console.log('\n[12] Rok2GameMode.h — ChatWidget member + handler');
// ---------------------------------------------------------------------------
const gmHPath = join(PUB, 'Rok2GameMode.h');
check('Rok2GameMode.h exists', existsSync(gmHPath));
if (existsSync(gmHPath)) {
  const h = read(gmHPath);
  check('has ChatWidget member', h.includes('URok2ChatWidget* ChatWidget'));
  check('has HandleChatAction', h.includes('HandleChatAction'));
  check('forward declares URok2ChatWidget', h.includes('class URok2ChatWidget'));
}

// ---------------------------------------------------------------------------
console.log('\n[13] Rok2GameMode.cpp — lazy-create + bind');
// ---------------------------------------------------------------------------
const gmCppPath = join(PRIV, 'Rok2GameMode.cpp');
check('Rok2GameMode.cpp exists', existsSync(gmCppPath));
if (existsSync(gmCppPath)) {
  const c = read(gmCppPath);
  check('includes Rok2ChatWidget.h', c.includes('Rok2ChatWidget.h'));
  check('lazy-creates ChatWidget', c.includes('CreateRok2Widget(World, URok2ChatWidget'));
  check('binds OnChatAction', c.includes('OnChatAction.AddDynamic'));
  check('sets ChatWidget->Api', c.includes('ChatWidget->Api'));
  check('adds to viewport at ZOrder 50', c.includes('ChatWidget->AddToViewport(50)'));
}

// ---------------------------------------------------------------------------
console.log('\n[14] API.md — chat documentation');
// ---------------------------------------------------------------------------
const apiDocPath = join(REPO, 'game', 'docs', 'API.md');
check('API.md exists', existsSync(apiDocPath));
if (existsSync(apiDocPath)) {
  const d = read(apiDocPath);
  check('documents chat_send WS', d.includes('chat_send'));
  check('documents chat_message WS', d.includes('chat_message'));
  check('documents chat_history WS', d.includes('chat_history'));
  check('describes channels (kingdom/alliance)', d.includes('kingdom') && d.includes('alliance'));
}

// ---------------------------------------------------------------------------
console.log('\n══════════════════════════════════════════════════════════════════════');
console.log(`P6-T6 structural verification: ${passed} passed, ${failed} failed`);
if (failed === 0) {
  console.log('\n✅ ALL PASSED');
} else {
  console.log('\n❌ FAILED');
  process.exit(1);
}
