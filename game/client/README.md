# ROK2 Mobile Web Client

## Path on PC
```
C:\Users\kayf\Desktop\rok2\game\client\public
```

Files:
- `index.html`
- `styles.css`
- `app.js`

## Live URL (open on your phone)
**https://rok2-client.pages.dev/**

Latest deploy:
https://098397ea.rok2-client.pages.dev/

## Connected API
https://rok2-api.lolelarap.workers.dev

## How to play on phone
1. Open the link in Chrome/Safari
2. اضغط **دخول المملكة**
3. اختار حضارة + اسم
4. من المدينة: رقِّ المباني / درّب جنود / أنشئ تحالف
5. افتح **الخريطة** → المس بإصبعك للتحريك
6. اضغط ممر برتقالي/بنفسجي → **هجوم الممر**
7. اضغط مورد/وحش → **جمع/وحش**

## Redeploy
```bash
cd C:\Users\kayf\Desktop\rok2\game\client
npx wrangler pages deploy public --project-name rok2-client --commit-dirty=true
```
