#!/usr/bin/env node
/**
 * Generates a minimal UE5 scene file (City.umap) as a clean fallback.
 * Note: UE native binary .umap format is complex; the editor will auto-create
 * an empty map on first open if none exists. This script just ensures the
 * Content/Maps folder exists.
 */
import { mkdirSync, existsSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const mapsDir = join(__dirname, "..", "client-unreal", "Content", "Maps");
mkdirSync(mapsDir, { recursive: true });

// touch placeholder so folder is tracked
const marker = join(mapsDir, ".gitkeep");
if (!existsSync(marker)) writeFileSync(marker, "");
console.log("Maps dir ready:", mapsDir);
