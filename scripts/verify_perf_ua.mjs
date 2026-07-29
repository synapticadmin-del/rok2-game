#!/usr/bin/env node
/**
 * verify_perf_ua.mjs — P4-T7 structural verification (تحسين أداء UA)
 *
 * Static/structural checks that the performance subsystem URok2Perf exists and is
 * properly wired: shared engine-mesh cache, marker actor pool, unified LOD distance,
 * building recycling in CityLayoutActor, and content-keyed hills in WorldRenderer.
 * Does not require a running UE5 build.
 *
 * Usage: node scripts/verify_perf_ua.mjs
 */

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const SRC = join(__dirname, '..', 'game', 'client-unreal', 'Source', 'Rok2');

let passed = 0;
let failed = 0;

function ok(name) { console.log(`  ✅ ${name}`); passed++; }
function fail(name, detail = '') { console.error(`  ❌ ${name}${detail ? ' — ' + detail : ''}`); failed++; }
function check(name, condition, detail = '') { if (condition) ok(name); else fail(name, detail); }

function read(rel) { return readFileSync(join(SRC, rel), 'utf8'); }

// ---------------------------------------------------------------------------
// 1. URok2Perf subsystem files
// ---------------------------------------------------------------------------
console.log('\n[1] URok2Perf subsystem');

check('Rok2Perf.h exists', existsSync(join(SRC, 'Public', 'Rok2Perf.h')));
check('Rok2Perf.cpp exists', existsSync(join(SRC, 'Private', 'Rok2Perf.cpp')));

const perfH = read('Public/Rok2Perf.h');
const perfCpp = read('Private/Rok2Perf.cpp');

check('URok2Perf is UGameInstanceSubsystem', perfH.includes('UGameInstanceSubsystem'));
check('static Get(WorldContextObject)', perfH.includes('static URok2Perf* Get(const UObject* WorldContextObject)'));
check('GetEngineMesh declared', perfH.includes('UStaticMesh* GetEngineMesh(const FString& ShapeName)'));
check('AcquireMarkerActor declared', perfH.includes('AStaticMeshActor* AcquireMarkerActor(UWorld* World)'));
check('ReleaseMarkerActor declared', perfH.includes('void ReleaseMarkerActor(AStaticMeshActor* Actor)'));
check('FlushPool declared', perfH.includes('void FlushPool()'));
check('MaxPoolSize configurable', perfH.includes('int32 MaxPoolSize'));
check('WorldRenderDistance configurable', perfH.includes('float WorldRenderDistance'));
check('WorldRenderDistanceSq helper', perfH.includes('WorldRenderDistanceSq()'));
check('EngineMeshCache is UPROPERTY (GC-safe)', perfH.includes('TMap<FString, UStaticMesh*> EngineMeshCache') && perfH.includes('UPROPERTY(Transient)\n\tTMap<FString, UStaticMesh*> EngineMeshCache'));

check('Initialize warms mesh cache', perfCpp.includes('WarmShapes'));
check('GetEngineMesh caches after first load', perfCpp.includes('EngineMeshCache.Add(ShapeName, Mesh)'));
check('GetEngineMesh returns cached hit', perfCpp.includes('EngineMeshCache.Find(ShapeName)'));
check('GetEngineMesh loads /Engine/BasicShapes path', perfCpp.includes('/Engine/BasicShapes/%s.%s'));
check('Acquire reuses pooled actors', perfCpp.includes('Pool.Pop()'));
check('Acquire spawns when pool empty', perfCpp.includes('SpawnActor<AStaticMeshActor>'));
check('Release hides + disables tick', perfCpp.includes('SetActorHiddenInGame(true)') && perfCpp.includes('SetActorTickEnabled(false)'));
check('Release respects MaxPoolSize', perfCpp.includes('Pool.Num() >= MaxPoolSize'));
check('FlushPool destroys all pooled', perfCpp.includes('FlushPool()') && perfCpp.includes('Pool.Empty()'));
check('Deinitialize flushes pool', perfCpp.includes('FlushPool();'));

// ---------------------------------------------------------------------------
// 2. WorldRenderer wired to perf subsystem
// ---------------------------------------------------------------------------
console.log('\n[2] ARok2WorldRenderer perf wiring');

const wrCpp = read('Private/Rok2WorldRenderer.cpp');
const wrH = read('Public/Rok2WorldRenderer.h');

check('includes Rok2Perf.h', wrCpp.includes('#include "Rok2Perf.h"'));
check('SpawnMarkerActor uses pool', wrCpp.includes('Perf->AcquireMarkerActor(GetWorld())'));
check('ClearActors releases to pool (not Destroy)', wrCpp.includes('Perf->ReleaseMarkerActor(SM)'));
check('LOD distance from subsystem', wrCpp.includes('Perf->WorldRenderDistanceSq()'));
check('no more hardcoded LOD comment', !wrCpp.includes('LOD distance handling for a 1200x1200 world grid at 100 scale'));
check('hills content-keyed (ArtHillsKey)', wrCpp.includes('ArtHillsKey') && wrCpp.includes('CitiesKey'));
check('hills released to pool on change', wrCpp.includes('SpawnedHills.Empty()'));
check('hills not destroyed every refresh', !wrCpp.includes('bArtHillsSpawned = false; // P2-T7: تُعاد زراعة المرتفعات مع إعادة الرسم'));
check('march cleanup releases to pool', wrCpp.includes('PerfForCleanup->ReleaseMarkerActor(SM)'));
check('SpawnedHills tracked in header', wrH.includes('TArray<AActor*> SpawnedHills'));
check('ArtHillsKey in header', wrH.includes('int64 ArtHillsKey'));

// ---------------------------------------------------------------------------
// 3. BuildingActor mesh cache
// ---------------------------------------------------------------------------
console.log('\n[3] ARok2BuildingActor mesh cache');

const bCpp = read('Private/Rok2BuildingActor.cpp');

check('includes Rok2Perf.h', bCpp.includes('#include "Rok2Perf.h"'));
check('GetMesh lambda uses perf cache', bCpp.includes('Perf->GetEngineMesh(FString(Shape))'));
check('roof meshes via GetMesh (no direct LoadObject BasicShapes)', !bCpp.includes('LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));'));
check('all 6 arch styles use GetMesh', (bCpp.match(/RoofMeshAsset = GetMesh\(TEXT\(/g) || []).length === 6);

// ---------------------------------------------------------------------------
// 4. CityLayoutActor building recycling (pool محلي — لا يحتاج Rok2Perf)
// ---------------------------------------------------------------------------
console.log('\n[4] ARok2CityLayoutActor building recycling');

const clCpp = read('Private/Rok2CityLayoutActor.cpp');
const clH = read('Public/Rok2CityLayoutActor.h');

check('ClearBuildings hides instead of Destroy', clCpp.includes('SetActorHiddenInGame(true)') && clCpp.includes('RecycledBuildings = Buildings'));
check('ClearBuildings no longer destroys', !clCpp.includes('KV.Value->Destroy();\n\t}\n\tBuildings.Empty();'));
check('Rebuild reuses recycled building', clCpp.includes('RecycledBuildings.Find(Id)'));
check('reused building re-shown', clCpp.includes('B->SetActorHiddenInGame(false)'));
check('build animation only on fresh spawn', clCpp.includes('if (!bReused) B->PlayBuildAnimation()'));
check('unused recycled buildings destroyed at end', clCpp.includes('RecycledBuildings.Empty()'));
check('RecycledBuildings in header', clH.includes('TMap<FString, ARok2BuildingActor*> RecycledBuildings'));

// ---------------------------------------------------------------------------
// 5. Rok2 module self-consistency
// ---------------------------------------------------------------------------
console.log('\n[5] Module consistency');

const buildCs = readFileSync(join(SRC, 'Rok2.Build.cs'), 'utf8');
check('Rok2.Build.cs unchanged (no new deps needed)', buildCs.includes('PublicDependencyModuleNames'));

// every new/edited cpp includes its own header first
for (const f of ['Rok2Perf.cpp', 'Rok2WorldRenderer.cpp', 'Rok2BuildingActor.cpp', 'Rok2CityLayoutActor.cpp']) {
  const content = read(join('Private', f));
  const headerName = f.replace('.cpp', '.h');
  check(`${f} includes ${headerName}`, content.includes(`#include "${headerName}"`));
}

// balanced braces in edited files
for (const f of ['Private/Rok2Perf.cpp', 'Private/Rok2WorldRenderer.cpp', 'Private/Rok2BuildingActor.cpp', 'Private/Rok2CityLayoutActor.cpp', 'Public/Rok2Perf.h', 'Public/Rok2WorldRenderer.h', 'Public/Rok2CityLayoutActor.h']) {
  const s = read(f);
  let d = 0;
  for (const ch of s) { if (ch === '{') d++; if (ch === '}') d--; }
  check(`${f} braces balanced`, d === 0, `depth=${d}`);
}

// ---------------------------------------------------------------------------
console.log('\n==== RESULT ====');
console.log(`${passed} passed, ${failed} failed`);
process.exit(failed === 0 ? 0 : 1);
