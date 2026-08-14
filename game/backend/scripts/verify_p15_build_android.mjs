// حارس P15 — تحقق YAML لملف workflow بناء Android (build-android.yml):
// 1) triggers على push/manual_dispatch
// 2) JDK 17 + Android SDK 34 + NDK r26+
// 3) سر UE5_TOKEN بدون توكن hard-coded
// 4) BuildCookRun + رفع APK artifact
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "../../..");
const wfPath = path.resolve(ROOT, ".github/workflows/build-android.yml");
const wf = fs.readFileSync(wfPath, "utf8");
let fails = 0, checks = 0;
function check(id, cond) {
  checks++;
  if (cond) console.log("PASS:", id);
  else { fails++; console.log("FAIL:", id); }
}
check("wf:exists", fs.existsSync(wfPath));
check("wf:triggers", /on:[\s\S]*?push:/.test(wf) || /workflow_dispatch/.test(wf));
check("wf:jdk", /openjdk-17|java-?17|java_version.*17|jdk-?17/i.test(wf));
check("wf:android-sdk", /android-34|platforms;34/.test(wf));
check("wf:ndk", /ndk[a-z0-9-]*\s*r2[67]|ndk[^@\s]*2[67]\./i.test(wf));
check("wf:ue-token", /UE5_TOKEN/.test(wf));
check("wf:build-apk", /BuildCookRun/.test(wf));
check("wf:artifact", /actions[\/\\]upload-artifact/.test(wf));
check("wf:no-hardcoded", !/ghp_[A-Za-z0-9]{10,}/.test(wf));
console.log(`\nchecks: ${checks}, failed: ${fails}\n${fails === 0 ? "ALL PASSED" : "FAILED"}`);
if (fails) process.exit(1);
