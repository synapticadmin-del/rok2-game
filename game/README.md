# ROK2 Game Backend (Cloudflare)

Full authoritative multiplayer backend for the ROK2 prototype.

## Live API
**https://rok2-api.lolelarap.workers.dev**

Example:
```bash
curl https://rok2-api.lolelarap.workers.dev/v1/health
```

## Quick start (local)
```bash
cd game/backend
npm install
npx wrangler d1 migrations apply rok2-db --local
npx wrangler dev
```

Another terminal:
```bash
cd game/backend
npm run smoke
```

## Docs
- `docs/RUN.md` — run & deploy
- `docs/API.md` — endpoints
- `docs/BACKEND.md` — architecture

## What works (tested)
- Guest auth + city init (6 civs)
- Buildings upgrade + troop training + resource production
- Alliances
- Shared kingdom map (1200×1200, 8 regions, passes)
- Marches + pass capture + combat reports
- Admin tick/grant/set-day
- Smoke E2E: two alliances contest a pass
