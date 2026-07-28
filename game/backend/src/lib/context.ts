import type { Env, PlayerRow, CityRow } from "../env";
import { verifyToken } from "./auth";
import { HttpError } from "./errors";

// P3-T5: تتبع نشاط اللاعب لقياس retention — upsert يوم واحد لكل لاعب (UTC) + last_seen على الحساب.
// متوافق مع قواعد لم تُرحّل بعد (أخطاء SQL تُبتلع) ولا يعطّل أي طلب.
async function trackActivity(env: Env, accountId: string, playerId: string | null): Promise<void> {
  try {
    const now = Date.now();
    const day = Math.floor(now / 86_400_000);
    await env.DB.prepare("UPDATE accounts SET last_seen_at = ? WHERE id = ?").bind(now, accountId).run();
    if (playerId) {
      await env.DB.prepare(
        `INSERT INTO player_activity (player_id, day, first_seen_ms, last_seen_ms) VALUES (?, ?, ?, ?)
         ON CONFLICT(player_id, day) DO UPDATE SET last_seen_ms = excluded.last_seen_ms`,
      ).bind(playerId, day, now, now).run();
    }
  } catch {
    // الجداول غير موجودة بعد (migration لم تُطبّق) — التتبع اختياري
  }
}

export async function requireAuth(request: Request, env: Env) {
  const header = request.headers.get("authorization") || "";
  const token = header.toLowerCase().startsWith("bearer ")
    ? header.slice(7).trim()
    : header.trim();
  if (!token) throw new HttpError(401, "Missing Authorization bearer token");

  let payload;
  try {
    payload = await verifyToken(token, env.AUTH_SECRET);
  } catch {
    throw new HttpError(401, "Invalid or expired token");
  }

  // P3-T5: تتبع النشاط على مستوى الحساب (بدون player إن لم يُنشأ بعد)
  await trackActivity(env, payload.accountId, payload.playerId ?? null);

  return {
    accountId: payload.accountId,
    playerId: payload.playerId,
    token,
  };
}

export async function requirePlayer(request: Request, env: Env) {
  const auth = await requireAuth(request, env);
  if (!auth.playerId) throw new HttpError(400, "Player not initialized. Call POST /v1/city/init");

  const player = await env.DB.prepare("SELECT * FROM players WHERE id = ?")
    .bind(auth.playerId)
    .first<PlayerRow>();
  if (!player) throw new HttpError(404, "Player not found");

  const city = await env.DB.prepare("SELECT * FROM cities WHERE player_id = ?")
    .bind(auth.playerId)
    .first<CityRow>();
  if (!city) throw new HttpError(404, "City not found");

  // P3-T5: تتبع النشاط بمعرّف اللاعب (دقة retention)
  await trackActivity(env, auth.accountId, player.id);

  return { auth, player, city };
}

export function requireAdmin(request: Request, env: Env) {
  const key = request.headers.get("x-admin-key") || "";
  if (key !== env.ADMIN_KEY) throw new HttpError(403, "Invalid admin key");
}
