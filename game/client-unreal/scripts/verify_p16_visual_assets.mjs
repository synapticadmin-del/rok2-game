// حارس P16 — الأصول البصرية الجديدة (P16-T1..T4):
// 1) وجود كل PNG المولّدة في Content/Art (Splash/SeasonStory/CommanderSkins/Events)
// 2) صلاحية PNG magic bytes وأبعاد >= 512px وحجم >= 50KB
// 3) import_assets.py يشمل المجلدات الأربعة الجديدة
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
const HERE = path.dirname(fileURLToPath(import.meta.url));
const BASE = path.resolve(HERE, "../Content/Art");
const MIN_BYTES = 50 * 1024;
let fails = 0, checks = 0;
function check(id, cond, why) {
  checks++;
  if (cond) console.log("PASS:", id);
  else { fails++; console.log("FAIL:", id, why || ""); }
}
const isPng = (fp) => {
  const buf = fs.readFileSync(fp);
  return buf.length >= 8 && buf[0] === 0x89 && buf[1] === 0x50 && buf[2] === 0x4e && buf[3] === 0x47;
};
const pngSize = (fp) => { const buf = fs.readFileSync(fp); return { w: buf.readUInt32BE(16), h: buf.readUInt32BE(20) }; };

const groups = {
  "P16-T1 Splash": { dir: "Splash", files: ["splash_title.png", "splash_loading.png", "splash_kingdom_storm.png"] },
  "P16-T2 SeasonStory": { dir: "SeasonStory", files: ["story_birth.png", "story_war.png", "story_alliance.png", "story_endgame.png"] },
  "P16-T3 CommanderSkins": { dir: "CommanderSkins", files: ["skin_roman_legend.png", "skin_egypt_legend.png", "skin_china_legend.png", "skin_japan_legend.png", "skin_arabia_legend.png", "skin_vikings_legend.png"] },
  "P16-T4 Events": { dir: "Events", files: ["event_tavern.png", "event_expedition.png", "event_canyon.png", "event_ark_of_osiris.png"] },
};
for (const [g, spec] of Object.entries(groups)) {
  const dirPath = path.join(BASE, spec.dir);
  const dirOk = fs.existsSync(dirPath);
  check(`${g}:dir`, dirOk, `مجلد Content/Art/${spec.dir} غير موجود`);
  if (!dirOk) continue;
  for (const file of spec.files) {
    const fp = path.join(dirPath, file);
    const present = fs.existsSync(fp);
    check(`${g}:${file}`, present, `غير موجود`);
    if (!present) continue;
    const st = fs.statSync(fp);
    check(`${g}:${file}:size`, st.size >= MIN_BYTES, `${Math.round(st.size / 1024)}KB < 50KB`);
    check(`${g}:${file}:png`, isPng(fp), "ليس PNG صالحاً");
    const { w, h } = pngSize(fp);
    check(`${g}:${file}:dims`, w >= 512 && h >= 512, `${w}x${h} < 512`);
  }
}
const importAssets = fs.readFileSync(path.resolve(HERE, "../import_assets.py"), "utf8");
check("import-assets:splash", /Art\/Splash/.test(importAssets));
check("import-assets:season", /Art\/SeasonStory/.test(importAssets));
check("import-assets:skins", /Art\/CommanderSkins/.test(importAssets));
check("import-assets:events", /Art\/Events/.test(importAssets));
console.log(`\nchecks: ${checks}, failed: ${fails}\n${fails === 0 ? "ALL PASSED" : "FAILED"}`);
if (fails) process.exit(1);
