import { readFileSync } from "node:fs";
import { resolve } from "node:path";

const root = resolve(import.meta.dirname, "../../..");
const read = (relativePath) => readFileSync(resolve(root, relativePath), "utf8");
const requireMatch = (source, pattern, description) => {
  if (!pattern.test(source)) {
    console.error(`FAIL: ${description}`);
    process.exitCode = 1;
  } else {
    console.log(`OK  : ${description}`);
  }
};

const apiHeader = read("game/client-unreal/Source/Rok2/Public/Rok2Api.h");
const apiCpp = read("game/client-unreal/Source/Rok2/Private/Rok2Api.cpp");
const commanderHeader = read("game/client-unreal/Source/Rok2/Public/Rok2CommanderWidget.h");
const commanderCpp = read("game/client-unreal/Source/Rok2/Private/Rok2CommanderWidget.cpp");
const gameModeCpp = read("game/client-unreal/Source/Rok2/Private/Rok2GameMode.cpp");
const router = read("game/backend/src/http/router.ts");

console.log("=== Commander client contract checks ===");
requireMatch(apiHeader, /void FetchCommanders\(\);/, "API exposes authenticated commander synchronization");
requireMatch(apiHeader, /void LevelUpCommander\(const FString& CommanderId, int32 Tomes\);/, "API exposes commander level-up action");
requireMatch(apiHeader, /void UpgradeCommanderSkill\(const FString& CommanderId, int32 SkillSlot\);/, "API exposes commander skill-up action");
requireMatch(apiCpp, /Get\(TEXT\("\/v1\/commanders"\)/, "client loads owned commanders from server");
requireMatch(apiCpp, /TEXT\("\/v1\/commander\/levelup"\)/, "client targets supported level-up route");
requireMatch(apiCpp, /TEXT\("\/v1\/commander\/skill"\)/, "client targets supported skill-up route");
requireMatch(apiCpp, /SetStringField\(TEXT\("primaryCommanderId"\)/, "march payload uses server commander field");
requireMatch(apiCpp, /Commander\.Level = \(int32\)Rok2Json::Num\(CommanderObj, TEXT\("level"\), 1\)/, "client hydrates commander level from server response");
requireMatch(apiCpp, /Commander\.SkillLevels\.Add/, "client hydrates commander skill levels from server response");
requireMatch(apiCpp, /if \(Self->HasPlayer\(\)\) Self->FetchCommanders\(\);/, "login triggers commander synchronization for returning players");
requireMatch(apiCpp, /Self->FetchCommanders\(\);\n\t\t\t\/\/ P5-T6/, "city initialization synchronizes starter commander");
if (/Commanders\.Add\(\{TEXT\("cmd_/.test(apiCpp)) {
  console.error("FAIL: client still seeds a fake commander roster");
  process.exitCode = 1;
} else {
  console.log("OK  : no fake commander roster is seeded in the client");
}

requireMatch(commanderHeader, /class ROK2_API URok2CommanderCardHandler/, "widget retains per-card click handlers");
requireMatch(commanderHeader, /TArray<URok2CommanderCardHandler\*> CommanderCardHandlers;/, "widget owns dynamic card handlers");
requireMatch(commanderCpp, /CardBtn->SetContent\(CardBorder\);/, "commander card button owns visible card content");
requireMatch(commanderCpp, /Handler->CommanderId = Cmd\.Id;/, "each commander card carries its own id");
requireMatch(commanderCpp, /Widget->SelectCommander\(CommanderId\);/, "card click selects the matching commander");
requireMatch(commanderCpp, /Api->LevelUpCommander\(SelectedCommanderId, 1\);/, "level-up button calls the API only for an eligible commander");
requireMatch(commanderCpp, /Api->UpgradeCommanderSkill\(SelectedCommanderId, SkillIndex \+ 1\);/, "skill button calls the API with a supported slot");
if (/URok2CommanderWidget::OnCardClicked/.test(commanderCpp)) {
  console.error("FAIL: obsolete first-commander card handler remains");
  process.exitCode = 1;
} else {
  console.log("OK  : obsolete first-commander fallback is removed");
}

requireMatch(gameModeCpp, /CommanderWidget->SetupWithApi\(Api\);/, "game mode initializes commander widget through its public setup path");
if (/CommanderWidget->Api\s*=/.test(gameModeCpp)) {
  console.error("FAIL: game mode still accesses protected widget API state directly");
  process.exitCode = 1;
} else {
  console.log("OK  : game mode does not access protected widget state directly");
}

requireMatch(router, /path === "\/v1\/commanders"/, "server exposes owned commander list");
requireMatch(router, /path === "\/v1\/commander\/levelup"/, "server exposes level-up route");
requireMatch(router, /path === "\/v1\/commander\/skill"/, "server exposes skill-up route");
requireMatch(router, /commanderId: row\.commander_id/, "server returns commander identity field expected by client");
requireMatch(router, /skillSlot must be 1\.\.3/, "server validates the client skill-slot domain");

if (process.exitCode) {
  console.error("Commander client contract checks failed.");
} else {
  console.log("All commander client contract checks passed.");
}
