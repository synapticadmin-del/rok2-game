/* ROK2 mobile web client */
const API_BASE = localStorage.getItem("rok2_api") || "https://rok2-api.lolelarap.workers.dev";
const LS_TOKEN = "rok2_token";
const LS_DEVICE = "rok2_device";

const state = {
  token: localStorage.getItem(LS_TOKEN) || "",
  deviceId: localStorage.getItem(LS_DEVICE) || "",
  player: null,
  city: null,
  buildings: {},
  troops: {},
  civs: [],
  selectedCiv: "rome",
  mapMeta: null,
  world: null,
  selected: null, // { type: 'pass'|'node', id, data }
  ws: null,
  cam: { x: 600, y: 600, scale: 0.28 },
  dragging: false,
  lastTouch: null,
};

const $ = (id) => document.getElementById(id);
const screens = {
  boot: $("screen-boot"),
  civ: $("screen-civ"),
  city: $("screen-city"),
  map: $("screen-map"),
};

function show(name) {
  Object.values(screens).forEach((s) => s.classList.remove("active"));
  screens[name].classList.add("active");
  if (name === "map") {
    resizeCanvas();
    drawMap();
  }
}

function toast(msg) {
  const el = $("toast");
  el.textContent = msg;
  el.classList.remove("hidden");
  clearTimeout(toast._t);
  toast._t = setTimeout(() => el.classList.add("hidden"), 2500);
}

function fmt(n) {
  n = Math.floor(Number(n) || 0);
  if (n >= 1e6) return (n / 1e6).toFixed(1) + "M";
  if (n >= 1e3) return (n / 1e3).toFixed(1) + "K";
  return String(n);
}

async function api(path, { method = "GET", body, auth = true } = {}) {
  const headers = { "content-type": "application/json" };
  if (auth && state.token) headers.authorization = `Bearer ${state.token}`;
  const res = await fetch(API_BASE + path, {
    method,
    headers,
    body: body ? JSON.stringify(body) : undefined,
  });
  const data = await res.json().catch(() => ({}));
  if (!res.ok) {
    const err = new Error(data.error || res.statusText || "request failed");
    err.status = res.status;
    err.data = data;
    throw err;
  }
  return data;
}

function setToken(token) {
  state.token = token;
  localStorage.setItem(LS_TOKEN, token);
}

function ensureDevice() {
  if (!state.deviceId) {
    state.deviceId = "web_" + Math.random().toString(36).slice(2) + Date.now().toString(36);
    localStorage.setItem(LS_DEVICE, state.deviceId);
  }
  return state.deviceId;
}

/* ---------- Boot ---------- */
async function boot() {
  $("api-label").textContent = API_BASE;
  const st = $("boot-status");
  const btn = $("btn-enter");
  try {
    const h = await api("/v1/health", { auth: false });
    st.textContent = "متصل ✓ · " + (h.kingdom || "kingdom");
    st.classList.add("ok");
    btn.disabled = false;

    const civs = await api("/v1/meta/civilizations", { auth: false });
    state.civs = civs.civilizations || [];
    state.mapMeta = await api("/v1/meta/map", { auth: false });

    // try resume session
    if (state.token) {
      try {
        const me = await api("/v1/me");
        if (me.player && me.player.civ) {
          state.player = me.player;
          await loadCity();
          show("city");
          toast("مرحبًا بعودتك، " + (me.player.name || "حاكم"));
          return;
        }
      } catch {
        setToken("");
      }
    }
  } catch (e) {
    st.textContent = "فشل الاتصال: " + e.message;
    st.classList.add("err");
    btn.disabled = false;
    btn.textContent = "إعادة المحاولة";
  }
}

$("btn-enter").onclick = async () => {
  const btn = $("btn-enter");
  btn.disabled = true;
  try {
    ensureDevice();
    const guest = await api("/v1/auth/guest", {
      method: "POST",
      auth: false,
      body: { deviceId: state.deviceId },
    });
    setToken(guest.token);
    if (guest.player && guest.player.civ) {
      state.player = guest.player;
      await loadCity();
      show("city");
      return;
    }
    renderCivs();
    show("civ");
  } catch (e) {
    toast(e.message);
    // retry health
    await boot();
  } finally {
    btn.disabled = false;
  }
};

/* ---------- Civ select ---------- */
function renderCivs() {
  const box = $("civ-list");
  box.innerHTML = "";
  state.civs.forEach((c) => {
    const el = document.createElement("div");
    el.className = "civ" + (state.selectedCiv === c.id ? " active" : "");
    el.innerHTML = `<h3>${c.name}</h3><p>${c.fantasy || ""}</p>`;
    el.onclick = () => {
      state.selectedCiv = c.id;
      renderCivs();
      $("btn-start").disabled = false;
    };
    box.appendChild(el);
  });
  $("btn-start").disabled = !state.selectedCiv;
  if (!$("player-name").value) {
    $("player-name").value = "حاكم" + Math.floor(Math.random() * 900 + 100);
  }
}

$("btn-start").onclick = async () => {
  try {
    const name = $("player-name").value.trim() || "Governor";
    const data = await api("/v1/city/init", {
      method: "POST",
      body: { civ: state.selectedCiv, name },
    });
    if (data.token) setToken(data.token);
    state.player = data.player;
    await loadCity();
    // center camera on player
    if (state.player?.x != null) {
      state.cam.x = state.player.x;
      state.cam.y = state.player.y;
    }
    show("city");
    toast("تم تأسيس مدينتك!");
  } catch (e) {
    toast(e.message);
  }
};

/* ---------- City ---------- */
async function loadCity() {
  const data = await api("/v1/city");
  state.player = data.player;
  state.city = data.city;
  state.buildings = data.buildings || {};
  state.troops = data.troops || {};
  renderCity();
}

function renderCity() {
  const p = state.player || {};
  const c = state.city || {};
  $("city-title").textContent = p.name || "مدينتي";
  $("city-civ").textContent = `${p.civ || "—"} · قوة ${fmt(p.power)} · ${p.region_id || ""}`;

  $("resources").innerHTML = ["food", "wood", "stone", "gold"]
    .map((k) => {
      const labels = { food: "🍲 طعام", wood: "🪵 خشب", stone: "🪨 حجر", gold: "🪙 ذهب" };
      return `<div class="res"><b>${labels[k]}</b><span>${fmt(c[k])}</span></div>`;
    })
    .join("");

  const bLabels = {
    city_hall: "قاعة المدينة",
    farm: "مزرعة",
    lumber_mill: "منشرة",
    quarry: "محجر",
    goldmine: "منجم ذهب",
    barracks: "ثكنات",
    stable: "إسطبل",
    archery_range: "ميدان رماية",
    hospital: "مستشفى",
    wall: "سور",
    storehouse: "مخزن",
  };
  $("buildings").innerHTML = Object.entries(state.buildings)
    .map(
      ([id, lv]) => `<div class="item">
      <div><b>${bLabels[id] || id}</b><div class="meta">مستوى ${lv}</div></div>
      <button class="btn" data-up="${id}">ترقية</button>
    </div>`,
    )
    .join("");

  $("buildings").querySelectorAll("[data-up]").forEach((btn) => {
    btn.onclick = async () => {
      try {
        await api("/v1/city/upgrade", { method: "POST", body: { buildingId: btn.dataset.up } });
        await loadCity();
        toast("تمت الترقية");
      } catch (e) {
        toast(e.message);
      }
    };
  });

  const tLabels = { infantry_t1: "مشاة", cavalry_t1: "فرسان", archer_t1: "رماة" };
  $("troops").innerHTML = Object.entries(state.troops)
    .map(([id, n]) => `<div class="item"><b>${tLabels[id] || id}</b><span>${fmt(n)}</span></div>`)
    .join("") || `<div class="muted">لا جنود</div>`;

  if (p.alliance_id) {
    $("alliance-box").textContent = "تحالف: " + p.alliance_id;
  } else {
    $("alliance-box").textContent = "لست في تحالف — أنشئ واحدًا لاحتلال الممرات";
  }
}

$("btn-train").onclick = async () => {
  try {
    await api("/v1/city/train", {
      method: "POST",
      body: {
        unit: $("train-unit").value,
        count: Number($("train-count").value) || 1,
      },
    });
    await loadCity();
    toast("تم التدريب");
  } catch (e) {
    toast(e.message);
  }
};

$("btn-create-all").onclick = async () => {
  try {
    const name = $("all-name").value.trim();
    const tag = $("all-tag").value.trim().toUpperCase();
    const data = await api("/v1/alliance/create", { method: "POST", body: { name, tag } });
    toast("تم إنشاء " + data.alliance.tag);
    await loadCity();
  } catch (e) {
    toast(e.message);
  }
};

$("btn-refresh-city").onclick = () => loadCity().then(() => toast("تم التحديث")).catch((e) => toast(e.message));

document.querySelectorAll("[data-nav]").forEach((b) => {
  b.onclick = async () => {
    const to = b.dataset.nav;
    if (to === "map") {
      try {
        await refreshWorld();
        if (state.player?.x != null) {
          state.cam.x = state.player.x;
          state.cam.y = state.player.y;
        }
      } catch (e) {
        toast(e.message);
      }
    }
    if (to === "city") {
      try { await loadCity(); } catch (e) { toast(e.message); }
    }
    show(to);
  };
});

/* ---------- World / Map ---------- */
async function refreshWorld() {
  state.world = await api("/v1/world/snapshot");
  renderReports();
  drawMap();
  $("map-info").textContent = `يوم ${state.world.seasonDay ?? 0} · مدن ${state.world.cities?.length || 0} · مسيرات ${state.world.marches?.length || 0}`;
}

function renderReports() {
  const reps = state.world?.reports || [];
  $("reports").innerHTML =
    reps
      .slice(0, 8)
      .map((r) => {
        const w = r.result?.winner || "?";
        return `<div>${r.kind || "battle"} · ${r.passId || r.nodeId || ""} · ${w}</div>`;
      })
      .join("") || "<div>لا تقارير بعد</div>";
}

function resizeCanvas() {
  const canvas = $("map");
  const wrap = canvas.parentElement;
  const w = Math.min(wrap.clientWidth || 360, 520);
  const dpr = Math.min(window.devicePixelRatio || 1, 2);
  canvas.style.width = w + "px";
  canvas.style.height = w + "px";
  canvas.width = Math.floor(w * dpr);
  canvas.height = Math.floor(w * dpr);
  state._dpr = dpr;
  state._css = w;
}

function worldToScreen(x, y) {
  const css = state._css || 360;
  const s = state.cam.scale;
  const sx = (x - state.cam.x) * s + css / 2;
  const sy = css / 2 - (y - state.cam.y) * s; // y up in world? our map y grows up from bottom-left; canvas y down
  // map origin bottom-left: higher y is "north" -> smaller canvas y
  return { sx, sy };
}

function screenToWorld(sx, sy) {
  const css = state._css || 360;
  const s = state.cam.scale;
  const x = (sx - css / 2) / s + state.cam.x;
  const y = state.cam.y - (sy - css / 2) / s;
  return { x, y };
}

function drawMap() {
  const canvas = $("map");
  const ctx = canvas.getContext("2d");
  const dpr = state._dpr || 1;
  const css = state._css || 360;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, css, css);

  // Background Grid Lines
  ctx.strokeStyle = "rgba(255, 255, 255, 0.04)";
  ctx.lineWidth = 1;
  const step = 40 * state.cam.scale;
  for (let x = (state.cam.x * state.cam.scale) % step; x < css; x += step) {
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, css);
    ctx.stroke();
  }
  for (let y = (state.cam.y * state.cam.scale) % step; y < css; y += step) {
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(css, y);
    ctx.stroke();
  }

  const meta = state.mapMeta || state.world?.map;
  const regions = state.mapMeta?.regions || state.world?.map?.regions || [];
  const colors = {
    1: "rgba(59, 130, 246, 0.14)",
    2: "rgba(16, 185, 129, 0.12)",
    3: "rgba(245, 158, 11, 0.14)",
  };

  // Regions
  regions.forEach((r) => {
    const [x0, y0, x1, y1] = r.aabb;
    const a = worldToScreen(x0, y1);
    const b = worldToScreen(x1, y0);
    const w = b.sx - a.sx;
    const h = b.sy - a.sy;
    ctx.fillStyle = colors[r.zone_id] || "rgba(255,255,255,.05)";
    ctx.strokeStyle = "rgba(255,255,255,.18)";
    ctx.lineWidth = 1;
    ctx.fillRect(a.sx, a.sy, w, h);
    ctx.strokeRect(a.sx, a.sy, w, h);
    ctx.fillStyle = "rgba(248, 250, 252, 0.65)";
    ctx.font = "bold 11px Cairo, sans-serif";
    ctx.fillText(r.id, a.sx + 6, a.sy + 14);
  });

  const world = state.world;
  if (!world) return;

  // Resource Nodes
  (world.nodes || []).forEach((n) => {
    const p = worldToScreen(n.x, n.y);
    const isSelected = state.selected?.id === n.id;
    if (isSelected) {
      ctx.beginPath();
      ctx.fillStyle = "rgba(59, 130, 246, 0.3)";
      ctx.arc(p.sx, p.sy, 12, 0, Math.PI * 2);
      ctx.fill();
    }
    ctx.beginPath();
    ctx.fillStyle =
      n.kind === "barb" ? "#f43f5e" : n.kind === "gold" ? "#f59e0b" : n.kind === "stone" ? "#94a3b8" : n.kind === "wood" ? "#10b981" : "#38bdf8";
    ctx.arc(p.sx, p.sy, isSelected ? 7 : 4, 0, Math.PI * 2);
    ctx.fill();
  });

  // Passes
  (world.passes || []).forEach((pass) => {
    const p = worldToScreen(pass.x, pass.y);
    const owned = !!pass.ownerAllianceId;
    const isSelected = state.selected?.id === pass.id;
    if (isSelected) {
      ctx.beginPath();
      ctx.strokeStyle = "rgba(245, 158, 11, 0.8)";
      ctx.lineWidth = 2;
      ctx.rect(p.sx - 10, p.sy - 10, 20, 20);
      ctx.stroke();
    }
    ctx.beginPath();
    ctx.fillStyle = owned ? "#8b5cf6" : "#f97316";
    ctx.strokeStyle = isSelected ? "#ffffff" : "rgba(0,0,0,0.5)";
    ctx.lineWidth = isSelected ? 2 : 1;
    ctx.rect(p.sx - 6, p.sy - 6, 12, 12);
    ctx.fill();
    ctx.stroke();
  });

  // Marches
  (world.marches || []).forEach((m) => {
    const a = worldToScreen(m.fromX, m.fromY);
    const b = worldToScreen(m.toX, m.toY);
    ctx.strokeStyle = "rgba(6, 182, 212, 0.9)";
    ctx.lineWidth = 2.5;
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(a.sx, a.sy);
    ctx.lineTo(b.sx, b.sy);
    ctx.stroke();
    ctx.setLineDash([]);
    const t = Math.min(1, Math.max(0, (Date.now() - m.startMs) / Math.max(1, m.etaMs - m.startMs)));
    const mx = a.sx + (b.sx - a.sx) * t;
    const my = a.sy + (b.sy - a.sy) * t;
    ctx.fillStyle = "#06b6d4";
    ctx.beginPath();
    ctx.arc(mx, my, 5, 0, Math.PI * 2);
    ctx.fill();
  });

  // Cities
  (world.cities || []).forEach((c) => {
    const p = worldToScreen(c.x, c.y);
    const me = c.playerId === state.player?.id;
    if (me) {
      ctx.beginPath();
      ctx.fillStyle = "rgba(16, 185, 129, 0.25)";
      ctx.arc(p.sx, p.sy, 14, 0, Math.PI * 2);
      ctx.fill();
    }
    ctx.beginPath();
    ctx.fillStyle = me ? "#10b981" : "#3b82f6";
    ctx.strokeStyle = "#ffffff";
    ctx.lineWidth = me ? 2.5 : 1;
    ctx.arc(p.sx, p.sy, me ? 7 : 5, 0, Math.PI * 2);
    ctx.fill();
    ctx.stroke();
    if (me) {
      ctx.fillStyle = "#ffffff";
      ctx.font = "bold 12px Cairo, sans-serif";
      ctx.fillText("مدينتي 🏰", p.sx + 10, p.sy - 8);
    }
  });
}

function pickAt(sx, sy) {
  const w = screenToWorld(sx, sy);
  const world = state.world;
  if (!world) return null;
  let best = null;
  let bestD = 24 / state.cam.scale; // world units threshold based on zoom

  (world.passes || []).forEach((pass) => {
    const d = Math.hypot(pass.x - w.x, pass.y - w.y);
    if (d < bestD) {
      bestD = d;
      best = { type: "pass", id: pass.id, data: pass };
    }
  });
  (world.nodes || []).forEach((n) => {
    const d = Math.hypot(n.x - w.x, n.y - w.y);
    if (d < bestD) {
      bestD = d;
      best = { type: "node", id: n.id, data: n };
    }
  });
  return best;
}

function updateSelectionUI() {
  const s = state.selected;
  const atk = $("btn-attack");
  const gath = $("btn-gather");
  if (!s) {
    $("selected-info").textContent = "اختر ممر أو مورد";
    atk.disabled = true;
    gath.disabled = true;
    return;
  }
  if (s.type === "pass") {
    $("selected-info").textContent = `ممر ${s.id} · Lv${s.data.level} · ${s.data.ownerAllianceId ? "محتل" : "محايد"} · ${Math.floor(s.data.captureProgress || 0)}%`;
    atk.disabled = false;
    gath.disabled = true;
  } else {
    $("selected-info").textContent = `${s.data.kind} ${s.id} · L${s.data.level} · ${fmt(s.data.remaining)}`;
    atk.disabled = true;
    gath.disabled = false;
  }
}

function canvasPos(e) {
  const rect = $("map").getBoundingClientRect();
  const t = e.touches ? e.touches[0] : e;
  return { x: t.clientX - rect.left, y: t.clientY - rect.top };
}

const canvas = () => $("map");

function onPointerDown(e) {
  e.preventDefault();
  state.dragging = true;
  state._moved = false;
  state.lastTouch = canvasPos(e);
}
function onPointerMove(e) {
  if (!state.dragging || !state.lastTouch) return;
  e.preventDefault();
  const p = canvasPos(e);
  const dx = p.x - state.lastTouch.x;
  const dy = p.y - state.lastTouch.y;
  if (Math.hypot(dx, dy) > 4) state._moved = true;
  // pan
  state.cam.x -= dx / state.cam.scale;
  state.cam.y += dy / state.cam.scale;
  state.lastTouch = p;
  drawMap();
}
function onPointerUp(e) {
  if (!state.dragging) return;
  state.dragging = false;
  if (!state._moved) {
    const rect = $("map").getBoundingClientRect();
    const t = e.changedTouches ? e.changedTouches[0] : e;
    const sx = t.clientX - rect.left;
    const sy = t.clientY - rect.top;
    state.selected = pickAt(sx, sy);
    updateSelectionUI();
    drawMap();
  }
  state.lastTouch = null;
}

function bindMapInput() {
  const c = canvas();
  c.addEventListener("mousedown", onPointerDown);
  window.addEventListener("mousemove", onPointerMove);
  window.addEventListener("mouseup", onPointerUp);
  c.addEventListener("touchstart", onPointerDown, { passive: false });
  c.addEventListener("touchmove", onPointerMove, { passive: false });
  c.addEventListener("touchend", onPointerUp, { passive: false });
  c.addEventListener(
    "wheel",
    (e) => {
      e.preventDefault();
      const f = e.deltaY > 0 ? 0.9 : 1.1;
      state.cam.scale = Math.min(1.2, Math.max(0.12, state.cam.scale * f));
      drawMap();
    },
    { passive: false },
  );
}

$("btn-center").onclick = () => {
  if (state.player?.x != null) {
    state.cam.x = state.player.x;
    state.cam.y = state.player.y;
    drawMap();
  }
};

$("btn-snapshot").onclick = () =>
  refreshWorld()
    .then(() => toast("تم تحديث العالم"))
    .catch((e) => toast(e.message));

function marchTroops() {
  const unit = $("march-unit").value;
  const count = Math.max(1, Number($("march-count").value) || 1);
  return { [unit]: count };
}

// prototype helper: force world tick (dev admin key)
const ADMIN_KEY = "rok2-dev-admin";
async function forceTick() {
  await fetch(API_BASE + "/v1/admin/tick", {
    method: "POST",
    headers: { "content-type": "application/json", "x-admin-key": ADMIN_KEY },
    body: JSON.stringify({ force: true }),
  });
}

$("btn-attack").onclick = async () => {
  if (!state.selected || state.selected.type !== "pass") return;
  try {
    await loadCity();
    if (!state.player.alliance_id) {
      toast("أنشئ تحالفًا أولًا من شاشة المدينة");
      return;
    }
    await api("/v1/world/pass/attack", {
      method: "POST",
      body: { passId: state.selected.id, troops: marchTroops() },
    });
    toast("مسيرة هجوم انطلقت...");
    for (let i = 0; i < 4; i++) {
      await forceTick();
      await sleep(200);
    }
    await refreshWorld();
    await loadCity();
    const pass = (state.world.passes || []).find((p) => p.id === state.selected.id);
    if (pass?.ownerAllianceId) toast("الممر الآن: " + pass.ownerAllianceId.slice(0, 10));
    else toast("تقدم الاحتلال: " + Math.floor(pass?.captureProgress || 0) + "%");
    updateSelectionUI();
  } catch (e) {
    toast(e.message);
  }
};

$("btn-gather").onclick = async () => {
  if (!state.selected || state.selected.type !== "node") return;
  try {
    const node = state.selected.data;
    await api("/v1/world/march", {
      method: "POST",
      body: {
        targetType: node.kind === "barb" ? "barb" : "resource",
        targetId: node.id,
        troops: marchTroops(),
      },
    });
    toast("مسيرة جمع/وحش...");
    for (let i = 0; i < 4; i++) {
      await forceTick();
      await sleep(200);
    }
    await refreshWorld();
    await loadCity();
  } catch (e) {
    toast(e.message);
  }
};

function sleep(ms) {
  return new Promise((r) => setTimeout(r, ms));
}

$("btn-ws").onclick = () => {
  if (state.ws && state.ws.readyState === WebSocket.OPEN) {
    state.ws.close();
    state.ws = null;
    toast("تم قطع الاتصال الحي");
    return;
  }
  const url = API_BASE.replace(/^http/, "ws") + "/v1/world/ws";
  const ws = new WebSocket(url);
  state.ws = ws;
  ws.onopen = () => {
    ws.send(JSON.stringify({ type: "hello", playerId: state.player?.id }));
    toast("اتصال حي ✓");
  };
  ws.onmessage = (ev) => {
    try {
      const msg = JSON.parse(ev.data);
      if (msg.type === "snapshot" || msg.cities) {
        state.world = {
          seasonDay: msg.seasonDay ?? state.world?.seasonDay,
          cities: msg.cities || state.world?.cities,
          passes: msg.passes || state.world?.passes,
          marches: msg.marches || state.world?.marches,
          nodes: msg.nodes || state.world?.nodes,
          reports: msg.reports || state.world?.reports,
          map: msg.map || state.world?.map,
        };
        renderReports();
        drawMap();
      } else if (msg.type === "pass_owner_changed") {
        toast("تغير مالك ممر!");
        refreshWorld();
      } else if (msg.type === "battle_report") {
        toast("تقرير قتال جديد");
        refreshWorld();
      }
    } catch {
      // ignore
    }
  };
  ws.onclose = () => toast("WS أغلق");
  ws.onerror = () => toast("خطأ WS");
};

// auto redraw marches
setInterval(() => {
  if (screens.map.classList.contains("active") && state.world) drawMap();
}, 500);

window.addEventListener("resize", () => {
  if (screens.map.classList.contains("active")) {
    resizeCanvas();
    drawMap();
  }
});

bindMapInput();
boot();
