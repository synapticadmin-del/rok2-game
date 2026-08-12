import { existsSync, readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, "../../..");
const errors = [];

function source(relativePath) {
  const absolutePath = join(ROOT, relativePath);
  if (!existsSync(absolutePath)) {
    errors.push(`ملف الربط مفقود: ${relativePath}`);
    return "";
  }
  return readFileSync(absolutePath, "utf8");
}

function requireEvidence(relativePath, content, pattern, description) {
  if (!pattern.test(content)) {
    errors.push(`دليل P7-T1 مفقود في ${relativePath}: ${description}`);
  }
}

const paths = {
  audioHeader: "game/client-unreal/Source/Rok2/Public/Rok2AudioManager.h",
  audioSource: "game/client-unreal/Source/Rok2/Private/Rok2AudioManager.cpp",
  motion: "game/client-unreal/Source/Rok2/Private/Rok2MotionLibrary.cpp",
  renderer: "game/client-unreal/Source/Rok2/Private/Rok2WorldRenderer.cpp",
  types: "game/client-unreal/Source/Rok2/Public/Rok2Types.h",
  apiHeader: "game/client-unreal/Source/Rok2/Public/Rok2Api.h",
  apiSource: "game/client-unreal/Source/Rok2/Private/Rok2Api.cpp",
  gameModeHeader: "game/client-unreal/Source/Rok2/Public/Rok2GameMode.h",
  gameModeSource: "game/client-unreal/Source/Rok2/Private/Rok2GameMode.cpp",
  hudHeader: "game/client-unreal/Source/Rok2/Public/Rok2HudWidget.h",
  hudSource: "game/client-unreal/Source/Rok2/Private/Rok2HudWidget.cpp",
  storyWidget: "game/client-unreal/Source/Rok2/Public/Rok2SeasonStoryWidget.h",
};
const files = Object.fromEntries(Object.entries(paths).map(([key, path]) => [key, source(path)]));

// INT-01 — أصول P6-T8 مسجلة وتُستعمل من إدخال الواجهة ومسارات حالة حقيقية.
requireEvidence(paths.audioHeader, files.audioHeader,
  /UiButtonClick[\s\S]*UiPanelOpen[\s\S]*UiPanelClose[\s\S]*UiError[\s\S]*CivWhisper/,
  "أنواع UiButtonClick/UiPanelOpen/UiPanelClose/UiError/CivWhisper");
requireEvidence(paths.audioSource, files.audioSource,
  /GetUiSfxAssetPath\(TEXT\("button_click"\)\)[\s\S]*GetUiSfxAssetPath\(TEXT\("panel_open"\)\)[\s\S]*GetUiSfxAssetPath\(TEXT\("panel_close"\)\)[\s\S]*GetUiSfxAssetPath\(TEXT\("error"\)\)/,
  "تسجيل أصول ضغط/فتح/إغلاق/خطأ من URok2ArtAssets");
requireEvidence(paths.audioSource, files.audioSource,
  /GetCivilizationWhisperAssetPath\(CivId\)[\s\S]*PlaySoundAtPath/,
  "تشغيل همس الحضارة عند التهيئة");
requireEvidence(paths.motion, files.motion, /PlaySfx\(ERok2AudioType::UiButtonClick\)/,
  "صوت ضغط موحد في BindPress");
requireEvidence(paths.apiSource, files.apiSource, /PlaySfx\(ERok2AudioType::UiError\)/,
  "صوت خطأ في فشل الاتصال الحي");
requireEvidence(paths.gameModeSource, files.gameModeSource, /PlaySfx\(ERok2AudioType::UiPanelOpen\)/,
  "صوت فتح لوحة في مسار العرض");

// INT-02 — القاموس الدلالي لـ P6-T9 يحدد اللون والمقياس والوسم في الراسم الفعلي.
requireEvidence(paths.renderer, files.renderer, /#include "Rok2WorldIconography\.h"/,
  "تضمين مكتبة أيقونات العالم");
requireEvidence(paths.renderer, files.renderer,
  /PassTargetType[\s\S]{0,240}URok2WorldIconography::Resolve\(PassTargetType, P\.Id, P\.Level\)(?=[\s\S]{0,1200}Style\.BaseColor)(?=[\s\S]{0,1200}Style\.WorldScale)(?=[\s\S]{0,1200}Style\.Glyph)/,
  "نمط بوابة الممر (لون/مقياس/وسم)");
requireEvidence(paths.renderer, files.renderer,
  /URok2WorldIconography::Resolve\(N\.Kind, N\.Kind, N\.Level\)(?=[\s\S]{0,1200}Style\.BaseColor)(?=[\s\S]{0,1200}Style\.WorldScale)(?=[\s\S]{0,1200}Style\.Glyph)/,
  "نمط عقدة المورد أو البرابرة (لون/مقياس/وسم)");

// INT-03 — اللقطة والحدث الحي يملكان نموذجاً واحداً وينتهيان عند HUD/الودجة.
requireEvidence(paths.types, files.types, /struct FRok2SeasonStoryEntry[\s\S]*FString Id[\s\S]*FString Kind/,
  "نموذج FRok2SeasonStoryEntry المشترك");
requireEvidence(paths.types, files.types, /TArray<FRok2SeasonStoryEntry> SeasonStory/,
  "حكاية الموسم داخل لقطة العالم");
requireEvidence(paths.apiHeader, files.apiHeader, /OnSeasonStoryEvent[\s\S]*GetSeasonStory/,
  "بث وحفظ حكاية الموسم في API");
requireEvidence(paths.apiSource, files.apiSource, /TryGetArrayField\(TEXT\("seasonStory"\)/,
  "تحليل seasonStory في لقطة العالم");
requireEvidence(paths.apiSource, files.apiSource, /Type == TEXT\("season_story_event"\)[\s\S]*PushSeasonStoryEvent/,
  "معالج WebSocket للحدث القصصي الحي");
requireEvidence(paths.storyWidget, files.storyWidget, /#include "Rok2Types\.h"[\s\S]*SetStoryEvents[\s\S]*AddStoryEvent/,
  "الودجة تستهلك النموذج المشترك وتحدّث الخط الزمني");
requireEvidence(paths.gameModeHeader, files.gameModeHeader, /URok2SeasonStoryWidget[\s\S]*HandleSeasonStoryAction[\s\S]*HandleSeasonStoryEvent/,
  "ملكية وضع اللعبة للشاشة ومعالجاتها");
requireEvidence(paths.gameModeSource, files.gameModeSource,
  /OnSeasonStoryEvent\.AddDynamic[\s\S]*HandleSeasonStoryAction[\s\S]*SetStoryEvents\(Api->GetSeasonStory\(\)\)/,
  "اشتراك الحدث والإنشاء الكسول والتعبئة من اللقطة");
requireEvidence(paths.hudHeader, files.hudHeader, /OnSeasonStoryAction/,
  "تفويض نقطة الدخول من HUD");
requireEvidence(paths.hudSource, files.hudSource, /حكاية المملكة[\s\S]*OnSeasonStoryClickedHandler[\s\S]*OnSeasonStoryAction\.Broadcast/,
  "زر حكاية المملكة وربطه بالتفويض");

if (errors.length) {
  console.error("فشل تحقق تكامل P7-T1:");
  errors.forEach((error) => console.error(`- ${error}`));
  process.exit(1);
}

console.log("✓ P7-T1: أُغلقت INT-01 (الصوت) وINT-02 (الأيقونات) وINT-03 (حكاية المملكة) بعقود تشغيل موثقة.");
