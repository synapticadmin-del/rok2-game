const fs = require('fs');
const path = 'c:/Users/kayf/Desktop/rok2/game/backend/src/do/KingdomShard.ts';
let code = fs.readFileSync(path, 'utf8');

// 1. Add throne to MarchEntity targetType
code = code.replace(
  /targetType: "pass" \| "resource" \| "barb" \| "city" \| "point";/,
  'targetType: "pass" | "resource" | "barb" | "city" | "point" | "throne";'
);

// 2. Add ThroneEntity type
code = code.replace(
  /type Attach = {/,
  `type ThroneEntity = {
  ownerAllianceId: string | null;
  captureProgress: number;
  state: "open" | "contested";
  x: number;
  y: number;
  unlockDay: number;
};

type Attach = {`
);

// 3. Add throne properties to KingdomShard
code = code.replace(
  /private passes = new Map<string, PassEntity>\(\);/,
  `private throne: ThroneEntity = { ownerAllianceId: null, captureProgress: 0, state: "open", x: 1200, y: 1200, unlockDay: 14 };
  private throneScores = new Map<string, number>();
  private passes = new Map<string, PassEntity>();`
);

// 4. Update migrate()
code = code.replace(
  /CREATE TABLE IF NOT EXISTS resource_nodes \(/,
  `CREATE TABLE IF NOT EXISTS throne (
          id INTEGER PRIMARY KEY CHECK (id = 1),
          owner_alliance_id TEXT,
          capture_progress REAL NOT NULL,
          state TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS throne_scores (
          alliance_id TEXT PRIMARY KEY,
          points INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS resource_nodes (`
);

// 5. Update loadState()
code = code.replace(
  /for \(const row of this\.ctx\.storage\.sql\.exec<any>\("SELECT \* FROM passes"\)\.toArray\(\)\) {/,
  `const throneRow = this.ctx.storage.sql.exec<any>("SELECT * FROM throne WHERE id = 1").toArray()[0];
    if (throneRow) {
      this.throne.ownerAllianceId = throneRow.owner_alliance_id;
      this.throne.captureProgress = throneRow.capture_progress;
      this.throne.state = throneRow.state;
    }
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM throne_scores").toArray()) {
      this.throneScores.set(row.alliance_id, row.points);
    }
    for (const row of this.ctx.storage.sql.exec<any>("SELECT * FROM passes").toArray()) {`
);

// 6. Update seedWorld()
code = code.replace(
  /this\.seasonDay = 0;/,
  `this.seasonDay = 0;
    this.ctx.storage.sql.exec(
      "INSERT OR IGNORE INTO throne (id, owner_alliance_id, capture_progress, state) VALUES (1, NULL, 0, 'open')"
    );`
);

// 7. Add persist methods
code = code.replace(
  /private saveReport\(report: any\) {/,
  `private persistThrone() {
    this.ctx.storage.sql.exec(
      \`INSERT OR REPLACE INTO throne (id, owner_alliance_id, capture_progress, state) VALUES (1, ?, ?, ?)\`,
      this.throne.ownerAllianceId,
      this.throne.captureProgress,
      this.throne.state
    );
  }

  private persistThroneScore(allianceId: string, points: number) {
    this.ctx.storage.sql.exec(
      \`INSERT OR REPLACE INTO throne_scores (alliance_id, points) VALUES (?, ?)\`,
      allianceId,
      points
    );
  }

  private saveReport(report: any) {`
);

// 8. Update snapshot()
code = code.replace(
  /passes: \[\.\.\.this\.passes\.values\(\)\],/,
  `throne: this.throne,
      throneScores: [...this.throneScores.entries()],
      passes: [...this.passes.values()],`
);

// 9. Update tick() for throne scoring
code = code.replace(
  /this\.ctx\.storage\.sql\.exec\(\n\s*"INSERT OR REPLACE INTO world_meta \(id, season_day, last_tick_ms\) VALUES \(1, \?, \?\)",\n\s*this\.seasonDay,\n\s*now,\n\s*\);/,
  `if (this.throne.ownerAllianceId && this.seasonDay >= this.throne.unlockDay) {
      const current = this.throneScores.get(this.throne.ownerAllianceId) || 0;
      this.throneScores.set(this.throne.ownerAllianceId, current + 1);
      this.persistThroneScore(this.throne.ownerAllianceId, current + 1);
      changed = true;
    }

    this.ctx.storage.sql.exec(
      "INSERT OR REPLACE INTO world_meta (id, season_day, last_tick_ms) VALUES (1, ?, ?)",
      this.seasonDay,
      now,
    );`
);

// 10. Update fetch() for leaderboard endpoint
code = code.replace(
  /if \(path\.endsWith\("\/snapshot"\) && request\.method === "GET"\) {\n\s*return Response\.json\(this\.snapshot\(\)\);\n\s*}/,
  `if (path.endsWith("/snapshot") && request.method === "GET") {
      return Response.json(this.snapshot());
    }

    if (path.endsWith("/leaderboard") && request.method === "GET") {
      const scores = [...this.throneScores.entries()].map(([allianceId, points]) => ({ allianceId, points }));
      scores.sort((a, b) => b.points - a.points);
      return Response.json({ scores });
    }`
);

// 11. Update createMarch
code = code.replace(
  /let toX = Number\(body\.toX\);\n\s*let toY = Number\(body\.toY\);\n\s*let targetType = \(body\.targetType \|\| "point"\) as MarchEntity\["targetType"\];\n\s*let targetId = String\(body\.targetId \|\| "point"\);/,
  `let toX = Number(body.toX);
    let toY = Number(body.toY);
    let targetType = (body.targetType || "point") as MarchEntity["targetType"];
    let targetId = String(body.targetId || "point");

    if (targetType === "throne" || body.targetType === "throne") {
      targetType = "throne";
      targetId = "throne";
      toX = this.throne.x;
      toY = this.throne.y;
    }`
);
code = code.replace(
  /\/\/ attacking a pass: allow even if path flagged, use euclidean distance\n\s*if \(!plan\.ok && targetType === "pass"\) {/,
  `// attacking a pass or throne: allow even if path flagged, use euclidean distance
    if (!plan.ok && (targetType === "pass" || targetType === "throne")) {`
);

// 12. Update resolveMarchArrival
code = code.replace(
  /if \(m\.targetType === "resource" \|\| m\.targetType === "barb"\) {/,
  `if (m.targetType === "throne") {
      if (this.throne.unlockDay > this.seasonDay) {
        m.state = "arrived";
        this.persistMarch(m);
        return;
      }
      const garrisonCount = 2000;
      const defenderTroops: Troops = this.throne.ownerAllianceId
        ? { infantry_t1: garrisonCount, archer_t1: Math.floor(garrisonCount / 2) }
        : { infantry_t1: Math.floor(garrisonCount * 0.8) };

      if (this.throne.ownerAllianceId && m.allianceId && this.throne.ownerAllianceId === m.allianceId) {
        this.throne.captureProgress = 100;
        this.throne.state = "open";
        this.persistThrone();
        m.state = "arrived";
        this.persistMarch(m);
        return;
      }

      const result = resolveCombat(
        { name: m.ownerPlayerId, troops: m.troops },
        { name: this.throne.ownerAllianceId || "neutral_guard", troops: defenderTroops },
      );

      const report = {
        id: newId("br"),
        createdAt: now,
        kind: "throne_attack",
        attackerPlayerId: m.ownerPlayerId,
        attackerAllianceId: m.allianceId,
        result,
      };
      this.saveReport(report);

      if (result.winner === "attacker") {
        const gain = Math.min(100, 35 + Math.floor(troopPower(result.attackerRemaining) / 20));
        if (this.throne.ownerAllianceId && this.throne.ownerAllianceId !== m.allianceId) {
          this.throne.captureProgress = gain;
        } else {
          this.throne.captureProgress = Math.min(100, this.throne.captureProgress + gain);
        }
        this.throne.state = "contested";
        if (this.throne.captureProgress >= 100) {
          this.throne.ownerAllianceId = m.allianceId;
          this.throne.captureProgress = 100;
          this.throne.state = "open";
        }
        this.persistThrone();
      }

      m.troops = result.attackerRemaining;
      m.state = "arrived";
      this.persistMarch(m);
      return;
    }

    if (m.targetType === "resource" || m.targetType === "barb") {`
);

fs.writeFileSync(path, code);
