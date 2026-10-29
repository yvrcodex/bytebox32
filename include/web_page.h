// Device web page, served on both the home network and the setup AP.
#ifndef WEB_PAGE_H
#define WEB_PAGE_H

static const char WEB_PAGE[] = R"html(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>bytebox32</title>
<style>
:root {
  --bg: #111;
  --fg: #eee;
  --muted: #888;
  --card: #1c1c1c;
  --ok: #2ecc71;
  --err: #e74c3c;
  --info: #3498db;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  padding: 16px;
  font-family: system-ui, sans-serif;
  background: var(--bg);
  color: var(--fg);
}
main { max-width: 640px; margin: 0 auto; }
header { display: flex; align-items: center; justify-content: space-between; }
h1 { font-size: 1.4rem; }
h2 { margin-top: 0; font-size: 1.1rem; }
.icon {
  width: auto;
  margin: 0;
  padding: 6px 10px;
  background: transparent;
  color: var(--fg);
  font-size: 1.5rem;
}
.banner {
  padding: 12px;
  margin-bottom: 16px;
  border-radius: 8px;
  border-left: 4px solid var(--info);
  background: var(--card);
}
.banner.ok { border-color: var(--ok); }
.banner.err { border-color: var(--err); color: var(--err); }
.card {
  padding: 16px;
  margin-bottom: 16px;
  border-radius: 8px;
  background: var(--card);
}
.row { display: flex; justify-content: space-between; padding: 4px 0; }
.row span:first-child { color: var(--muted); }
.bar {
  height: 10px;
  margin-top: 12px;
  border-radius: 5px;
  background: #333;
  overflow: hidden;
}
.bar div { height: 100%; width: 0; background: var(--info); }
.err { color: var(--err); }
label { display: block; margin: 8px 0 4px; color: var(--muted); }
input {
  width: 100%;
  padding: 10px;
  border: 1px solid #333;
  border-radius: 6px;
  background: #000;
  color: var(--fg);
  font-size: 1rem;
}
button {
  width: 100%;
  margin-top: 12px;
  padding: 12px;
  border: 0;
  border-radius: 6px;
  background: var(--info);
  color: #fff;
  font-size: 1rem;
}
button.secondary { background: #333; }
ul { list-style: none; padding: 0; margin: 8px 0 0; }
li {
  display: flex;
  justify-content: space-between;
  padding: 10px;
  border-bottom: 1px solid #333;
  cursor: pointer;
}
li span { color: var(--muted); }
</style>
</head>
<body>
<main>
  <header>
    <h1>bytebox32</h1>
    <button id="gear" class="icon" type="button" title="Settings">&#9881;</button>
  </header>

  <div id="banner" class="banner">Loading...</div>

  <form id="login" class="card" hidden>
    <h2>Sign in</h2>
    <label for="login-password">Password</label>
    <input id="login-password" name="password" type="password" required autocomplete="current-password">
    <button type="submit">Sign in</button>
  </form>

  <div id="home">
    <section class="card">
      <h2>Wi-Fi</h2>
      <div class="row"><span>Network</span><span id="wifi-ssid">-</span></div>
      <div class="row"><span>State</span><span id="wifi-state">-</span></div>
    </section>

    <section class="card">
      <h2>SD card</h2>
      <div id="sd-error" class="err" hidden>No card mounted.</div>
      <div id="sd-info">
        <div class="row"><span>Type</span><span id="sd-type">-</span></div>
        <div class="row"><span>Capacity</span><span id="sd-size">-</span></div>
        <div class="row"><span>Used</span><span id="sd-used">-</span></div>
        <div class="row"><span>Free</span><span id="sd-free">-</span></div>
        <div class="bar"><div id="sd-bar"></div></div>
      </div>
    </section>
  </div>

  <div id="settings" hidden>
    <section class="card">
      <h2>Wi-Fi network</h2>
      <button id="scan" type="button" class="secondary">Scan networks</button>
      <ul id="networks"></ul>
      <form id="wifi-form">
        <label for="ssid">SSID</label>
        <input id="ssid" name="ssid" maxlength="32" required autocomplete="off">
        <label for="password">Password</label>
        <input id="password" name="password" type="password" maxlength="64" autocomplete="off">
        <button type="submit">Save and connect</button>
      </form>
    </section>

    <section class="card">
      <h2>Configuration mode</h2>
      <p>Keeps the <b>bytebox32</b> access point open for setup.</p>
      <button id="config-mode" type="button" class="secondary">-</button>
    </section>

    <section class="card">
      <button id="logout" type="button" class="secondary">Sign out</button>
    </section>
  </div>
</main>
<script>
const $ = (id) => document.getElementById(id);

const STATUS_PERIOD_MS = 2000;
const SD_PERIOD_MS = 5000;
const SCAN_POLL_MS = 1000;
const UNITS = ["B", "KB", "MB", "GB", "TB"];

let configMode = false;
let settingsOpenedForAp = false;
let signedIn = false;

class Unauthorized extends Error {}

// JSON API call. A 401 switches the page to the sign-in form.
async function api(path, options) {
  const response = await fetch(path, options);
  if (response.status === 401 && path !== "/api/login") {
    showLogin();
    throw new Unauthorized();
  }
  return response.json();
}

function showLogin() {
  if (!signedIn && !$("login").hidden) return;  // Already shown: keep the banner.
  signedIn = false;
  $("login").hidden = false;
  $("home").hidden = true;
  $("settings").hidden = true;
  $("gear").hidden = true;
  setBanner("Sign in to continue.");
}

function setBanner(text, kind) {
  const banner = $("banner");
  banner.textContent = text;
  banner.className = kind ? "banner " + kind : "banner";
}

function showSettings(show) {
  $("settings").hidden = !show;
  $("home").hidden = show;
}

function showSignedIn() {
  if (signedIn) return;
  signedIn = true;
  $("login").hidden = true;
  $("gear").hidden = false;
  showSettings(false);
}

function formatBytes(bytes) {
  let value = bytes;
  let unit = 0;
  while (value >= 1024 && unit < UNITS.length - 1) {
    value /= 1024;
    unit++;
  }
  return value.toFixed(unit === 0 ? 0 : 1) + " " + UNITS[unit];
}

async function refreshStatus() {
  try {
    const s = await api("/api/status");
    showSignedIn();

    configMode = s.config_mode;
    $("config-mode").textContent = configMode ? "Disable configuration mode" : "Enable configuration mode";
    $("wifi-ssid").textContent = s.wifi.configured ? s.wifi.ssid : "-";
    $("wifi-state").textContent = s.wifi.state;

    // Setup AP clients go straight to the settings, once.
    if (s.via_ap && !settingsOpenedForAp) {
      settingsOpenedForAp = true;
      showSettings(true);
    }

    if (!s.wifi.configured) {
      setBanner("No Wi-Fi configured. Open settings and choose a network.", "err");
    } else if (s.online) {
      setBanner("Online: " + s.wifi.ssid + (configMode ? " (configuration mode on)" : ""), "ok");
    } else if (s.wifi.state === "connecting") {
      setBanner("Connecting to " + s.wifi.ssid + "...");
    } else {
      setBanner("Error: " + s.wifi.state + ". Retrying every 3 s.", "err");
    }
  } catch (e) {
    if (e instanceof Unauthorized) return;
    // The AP may switch channels while the station connects.
    setBanner("Device unreachable. Reconnecting...", "err");
  }
}

async function refreshSd() {
  if (!signedIn || $("home").hidden) return;  // Only while the overview is shown.

  try {
    const sd = await api("/api/sd");

    $("sd-error").hidden = sd.mounted;
    $("sd-info").hidden = !sd.mounted;
    if (!sd.mounted) return;

    const free = sd.total_bytes - sd.used_bytes;
    const percent = sd.total_bytes > 0 ? (sd.used_bytes / sd.total_bytes) * 100 : 0;

    $("sd-type").textContent = sd.type;
    $("sd-size").textContent = formatBytes(sd.total_bytes);
    $("sd-used").textContent = formatBytes(sd.used_bytes) + " (" + percent.toFixed(1) + "%)";
    $("sd-free").textContent = formatBytes(free);
    $("sd-bar").style.width = percent + "%";
  } catch (e) {
    if (!(e instanceof Unauthorized)) $("sd-error").hidden = false;
  }
}

function renderNetworks(networks) {
  const list = $("networks");
  const seen = new Set();

  list.textContent = "";

  networks
    .sort((a, b) => b.rssi - a.rssi)
    .forEach((n) => {
      if (!n.ssid || seen.has(n.ssid)) return;  // Skip hidden and duplicates.
      seen.add(n.ssid);

      const item = document.createElement("li");
      const info = document.createElement("span");

      item.textContent = n.ssid;  // textContent: SSIDs are untrusted input.
      info.textContent = n.rssi + " dBm" + (n.open ? " · open" : "");
      item.appendChild(info);

      item.onclick = () => {
        $("ssid").value = n.ssid;
        $("password").focus();
      };

      list.appendChild(item);
    });
}

async function scan() {
  const button = $("scan");

  button.disabled = true;
  button.textContent = "Scanning...";

  try {
    for (;;) {
      const r = await api("/api/wifi/scan");
      if (!r.scanning) {
        renderNetworks(r.networks);
        break;
      }
      await new Promise((done) => setTimeout(done, SCAN_POLL_MS));
    }
  } catch (e) {
    if (!(e instanceof Unauthorized)) setBanner("Scan failed. Try again.", "err");
  } finally {
    button.disabled = false;
    button.textContent = "Scan networks";
  }
}

async function saveWifi(event) {
  event.preventDefault();

  try {
    const body = new URLSearchParams(new FormData(event.target));
    const r = await api("/api/wifi", { method: "POST", body });

    setBanner(r.ok ? "Saved. Connecting..." : "Error: " + r.error, r.ok ? "ok" : "err");
  } catch (e) {
    if (!(e instanceof Unauthorized)) setBanner("Could not reach the device.", "err");
  }
}

async function toggleConfigMode() {
  const body = new URLSearchParams({ enabled: configMode ? "0" : "1" });

  try {
    await api("/api/config_mode", { method: "POST", body });
    await refreshStatus();
  } catch (e) {
    if (!(e instanceof Unauthorized)) setBanner("Could not reach the device.", "err");
  }
}

async function login(event) {
  event.preventDefault();

  try {
    const body = new URLSearchParams(new FormData(event.target));
    const r = await api("/api/login", { method: "POST", body });

    if (!r.ok) {
      setBanner("Wrong password.", "err");
      return;
    }

    $("login-password").value = "";
    await refreshStatus();
    refreshSd();
  } catch {
    setBanner("Could not reach the device.", "err");
  }
}

async function logout() {
  try {
    await api("/api/logout", { method: "POST" });
  } catch {
    // Signed out either way.
  }
  showLogin();
}

$("gear").addEventListener("click", () => showSettings($("settings").hidden));
$("scan").addEventListener("click", scan);
$("wifi-form").addEventListener("submit", saveWifi);
$("config-mode").addEventListener("click", toggleConfigMode);
$("login").addEventListener("submit", login);
$("logout").addEventListener("click", logout);

refreshStatus();
refreshSd();
setInterval(refreshStatus, STATUS_PERIOD_MS);
setInterval(refreshSd, SD_PERIOD_MS);
</script>
</body>
</html>
)html";

#endif  // WEB_PAGE_H
