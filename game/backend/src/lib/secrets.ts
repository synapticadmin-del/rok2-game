import type { Env } from "../env";
import { HttpError } from "./errors";

/**
 * أسرار كانت مكتوبة نصاً صريحاً في wrangler.jsonc ومنشورة على الخادم الحي.
 * أي شخص اطّلع على المستودع يملكها، لذلك نرفضها صراحةً حتى لو أُعيد ضبطها
 * بنفس القيمة لاحقاً.
 *
 * AUTH_SECRET هو مفتاح توقيع HMAC لرموز الدخول: من يملكه يستطيع تزوير رمز
 * صالح لأي حساب. ADMIN_KEY يفتح منح الموارد وتقديم زمن الموسم.
 */
const LEAKED_SECRETS = new Set([
  "rok2-dev-admin",
  "rok2-dev-secret-change-me",
  "change-me",
  "",
]);

const MIN_SECRET_LENGTH = 24;

function isUsable(secret: string | undefined): secret is string {
  return (
    typeof secret === "string" &&
    secret.length >= MIN_SECRET_LENGTH &&
    !LEAKED_SECRETS.has(secret)
  );
}

/**
 * مقارنة ثابتة الزمن. المقارنة العادية `!==` تخرج عند أول حرف مختلف، فيمكن
 * نظرياً استنتاج المفتاح حرفاً حرفاً من فروق التوقيت.
 */
function timingSafeEqual(a: string, b: string): boolean {
  const enc = new TextEncoder();
  const ab = enc.encode(a);
  const bb = enc.encode(b);
  // نوازن الطول أولاً حتى لا يتسرّب الطول نفسه عبر مسار خروج مبكر.
  let diff = ab.length ^ bb.length;
  const len = Math.max(ab.length, bb.length);
  for (let i = 0; i < len; i++) {
    diff |= (ab[i] ?? 0) ^ (bb[i] ?? 0);
  }
  return diff === 0;
}

/**
 * يعيد مفتاح توقيع الرموز، أو يفشل الطلب إن كان غائباً أو مسرّباً.
 *
 * فشل مغلق عن قصد: تشغيل الخادم بمفتاح توقيع معروف للعامة أسوأ من توقفه،
 * لأن كل حساب يصبح قابلاً للانتحال بصمت.
 */
export function requireAuthSecret(env: Env): string {
  if (!isUsable(env.AUTH_SECRET)) {
    throw new HttpError(
      500,
      "Server misconfigured: AUTH_SECRET is unset, too short, or a known-leaked value. " +
        "Set it with: wrangler secret put AUTH_SECRET",
    );
  }
  return env.AUTH_SECRET;
}

/** يتحقق من ترويسة x-admin-key مقابل سرّ صالح، بمقارنة ثابتة الزمن. */
export function assertAdminKey(request: Request, env: Env): void {
  if (!isUsable(env.ADMIN_KEY)) {
    // لا نكشف السبب للمتصل — رسالة عامة، والتفصيل في سجل الخادم.
    console.error(
      "[security] ADMIN_KEY is unset, too short, or a known-leaked value; " +
        "admin endpoints are disabled. Set it with: wrangler secret put ADMIN_KEY",
    );
    throw new HttpError(403, "Admin access disabled");
  }

  const provided = request.headers.get("x-admin-key") || "";
  if (!timingSafeEqual(provided, env.ADMIN_KEY)) {
    throw new HttpError(403, "Invalid admin key");
  }
}
