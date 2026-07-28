# ROK2 Backend — Cloudflare

## Stack
- **Worker** `rok2-api` — REST gateway
- **D1** `rok2-db` — accounts/players/cities/alliances
- **Durable Object** `KingdomShard` — shared map, marches, passes, WebSocket, tick

## Live deployment (already up)
- **URL:** https://rok2-api.lolelarap.workers.dev
- **Health:** https://rok2-api.lolelarap.workers.dev/v1/health
- **D1:** `rok2-db` (`297c811a-ba1e-4f64-88ca-98513cce3042`)
- Smoke remote:
```bash
cd C:\Users\kayf\Desktop\rok2\game\backend
export BASE_URL=https://rok2-api.lolelarap.workers.dev
node scripts/smoke.mjs
```

## Local run
```bash
cd C:\Users\kayf\Desktop\rok2\game\backend
npm install
npx wrangler d1 migrations apply rok2-db --local
npx wrangler dev
```

In another terminal:
```bash
cd C:\Users\kayf\Desktop\rok2\game\backend
npm run smoke
```

## Redeploy
```bash
cd C:\Users\kayf\Desktop\rok2\game\backend
npx wrangler d1 migrations apply rok2-db --remote
npx wrangler deploy
```

## Admin
Header: `x-admin-key: rok2-dev-admin`  
Change `ADMIN_KEY` / `AUTH_SECRET` in `wrangler.jsonc` before production.
