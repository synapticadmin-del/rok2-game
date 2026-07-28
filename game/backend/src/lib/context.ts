import type { Env, PlayerRow, CityRow } from "../env";
import { verifyToken } from "./auth";
import { HttpError } from "./errors";

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

  return { auth, player, city };
}

export function requireAdmin(request: Request, env: Env) {
  const key = request.headers.get("x-admin-key") || "";
  if (key !== env.ADMIN_KEY) throw new HttpError(403, "Invalid admin key");
}
