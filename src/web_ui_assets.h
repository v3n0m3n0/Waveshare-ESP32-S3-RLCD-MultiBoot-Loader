#pragma once
#include <Arduino.h>

static const char LAUNCHER_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>RLCD MultiBoot Device Hub</title>
<style>
:root{
  color-scheme:dark;
  --bg:#090d16;
  --bg2:#111827;
  --card:rgba(19,27,46,0.82);
  --card-border:rgba(148,163,184,0.18);
  --text:#f1f5f9;
  --muted:#94a3b8;
  --accent:#38bdf8;
  --accent-strong:#0284c7;
  --ok:#10b981;
  --warn:#f59e0b;
  --danger:#ef4444;
  accent-color:var(--accent);
}
*{box-sizing:border-box}
body{
  margin:0;
  font-family:system-ui,-apple-system,Segoe UI,Roboto,Ubuntu,sans-serif;
  background:radial-gradient(circle at 12% 10%,#172554 0%,var(--bg) 48%,#050810 100%);
  color:var(--text);
  min-height:100vh;
  padding:18px 14px 48px;
}
.wrap{max-width:980px;margin:0 auto}
.glass{
  background:var(--card);
  backdrop-filter:blur(12px);
  -webkit-backdrop-filter:blur(12px);
  border:1px solid var(--card-border);
  border-radius:14px;
  box-shadow:0 12px 32px rgba(0,0,0,0.35);
}
header{
  padding:18px 20px;
  display:flex;
  flex-wrap:wrap;
  gap:12px;
  align-items:center;
  justify-content:space-between;
  margin-bottom:14px;
}
.brand h1{margin:0;font-size:1.25rem;letter-spacing:-0.01em;display:flex;align-items:center;gap:10px}
.brand p{margin:4px 0 0;font-size:0.82rem;color:var(--muted)}
.pills{display:flex;flex-wrap:wrap;gap:8px;align-items:center}
.pill{
  display:inline-flex;align-items:center;gap:6px;
  padding:6px 10px;border-radius:999px;font-size:0.78rem;font-weight:600;
  background:rgba(15,23,42,0.85);border:1px solid rgba(148,163,184,0.2);color:var(--text);
}
.pill .dot{width:8px;height:8px;border-radius:50%;background:var(--ok)}
.pill.warn .dot{background:var(--warn)}
.tablist{
  display:flex;
  gap:4px;
  overflow-x:auto;
  padding:6px 10px 0;
  margin-bottom:14px;
  scrollbar-width:thin;
  scroll-snap-type:inline proximity;
}
.tablist [role="tab"]{
  flex:none;
  position:relative;
  background:transparent;
  border:0;
  color:var(--muted);
  font-size:0.9rem;
  font-weight:600;
  padding:10px 14px 12px;
  cursor:pointer;
  border-radius:8px 8px 0 0;
  scroll-snap-align:start;
  transition:color .15s ease,background .15s ease;
}
.tablist [role="tab"]:hover{color:var(--text);background:rgba(56,189,248,0.08)}
.tablist [role="tab"][aria-selected="true"]{color:var(--accent)}
.tablist [role="tab"][aria-selected="true"]::after{
  content:"";
  position:absolute;
  inset:auto 8px 0;
  border-block-end:3px solid var(--accent);
}
[role="tabpanel"][hidden]{display:none}
.panel-inner{padding:18px 20px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(270px,1fr));gap:14px}
.subcard{
  background:rgba(15,23,42,0.65);
  border:1px solid rgba(148,163,184,0.16);
  border-radius:12px;
  padding:14px 16px;
}
h2{margin:0 0 12px;font-size:1.05rem;color:#e2e8f0}
h3{margin:0 0 10px;font-size:0.92rem;color:#cbd5e1}
.fw-list{display:flex;flex-direction:column;gap:10px}
.fw-item{
  display:flex;flex-wrap:wrap;gap:12px;align-items:center;justify-content:space-between;
  padding:13px 15px;border-radius:11px;
  background:rgba(15,23,42,0.72);
  border:1px solid rgba(148,163,184,0.16);
}
.fw-item.active{border-color:rgba(16,185,129,0.55);background:rgba(6,78,59,0.22)}
.fw-title{font-weight:700;font-size:0.98rem;display:flex;align-items:center;gap:8px;flex-wrap:wrap}
.fw-sub{font-size:0.8rem;color:var(--muted);margin-top:3px}
.badge{
  display:inline-block;padding:2px 8px;border-radius:999px;font-size:0.72rem;font-weight:700;
  text-transform:uppercase;letter-spacing:0.03em;
}
.b-ok{background:rgba(16,185,129,0.2);color:#34d399;border:1px solid rgba(16,185,129,0.35)}
.b-warn{background:rgba(245,158,11,0.18);color:#fbbf24;border:1px solid rgba(245,158,11,0.35)}
.b-bad{background:rgba(239,68,68,0.2);color:#f87171;border:1px solid rgba(239,68,68,0.35)}
.b-info{background:rgba(56,189,248,0.18);color:#7dd3fc;border:1px solid rgba(56,189,248,0.35)}
.actions{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
button,.btn{
  appearance:none;border:0;border-radius:8px;padding:8px 14px;
  font-size:0.85rem;font-weight:600;cursor:pointer;
  background:var(--accent-strong);color:#fff;
  transition:filter .15s ease,transform .1s ease;
}
button:hover{filter:brightness(1.12)}
button:active{transform:translateY(1px)}
button:disabled{opacity:.5;cursor:not-allowed}
.btn-ok{background:#059669}
.btn-muted{background:#334155;color:#e2e8f0}
.btn-danger{background:#b91c1c}
.btn-sm{padding:6px 10px;font-size:0.78rem}
.field{margin-bottom:11px}
.field label{display:block;font-size:0.78rem;color:var(--muted);margin-bottom:4px;font-weight:600}
input[type="text"],input[type="password"],input[type="number"],input[type="url"],select{
  width:100%;padding:9px 10px;border-radius:8px;
  background:#090d16;border:1px solid rgba(148,163,184,0.28);
  color:var(--text);font-size:0.88rem;
}
input:focus,select:focus{outline:2px solid var(--accent);border-color:transparent}
.check-row{display:flex;align-items:center;gap:9px;font-size:0.86rem;margin:8px 0;cursor:pointer}
.check-row input{width:17px;height:17px}
.upload-box{
  margin-top:14px;padding:14px;border:1px dashed rgba(56,189,248,0.45);
  border-radius:11px;background:rgba(14,165,233,0.06);
}
.upload-row{display:flex;flex-wrap:wrap;gap:10px;align-items:center}
.upload-row input[type="file"]{flex:1;min-width:200px;font-size:0.84rem;color:var(--muted)}
progress{width:100%;height:10px;border-radius:999px;margin-top:10px}
.prog-label{font-size:0.78rem;color:var(--accent);margin-top:5px;min-height:18px}
.station-row{
  display:grid;grid-template-columns:1fr 2fr;gap:8px;margin-bottom:8px;align-items:center;
}
@media(max-width:600px){.station-row{grid-template-columns:1fr}}
table{width:100%;border-collapse:collapse;font-size:0.84rem}
th,td{text-align:left;padding:9px 8px;border-bottom:1px solid rgba(148,163,184,0.14)}
th{color:var(--muted);font-size:0.75rem;text-transform:uppercase;letter-spacing:0.04em}
.toast{
  position:fixed;right:18px;bottom:18px;z-index:50;
  padding:12px 16px;border-radius:10px;font-size:0.86rem;font-weight:600;
  background:#0f172a;border:1px solid var(--accent);color:#fff;
  box-shadow:0 10px 28px rgba(0,0,0,0.5);
  opacity:0;pointer-events:none;transform:translateY(10px);transition:all .2s ease;
}
.toast.show{opacity:1;pointer-events:auto;transform:translateY(0)}
.toast.err{border-color:var(--danger);background:#450a0a}
.muted{color:var(--muted);font-size:0.82rem}
</style>
</head>
<body>
<div class="wrap">
  <header class="glass">
    <div class="brand">
      <h1>RLCD MultiBoot Device Hub</h1>
      <p id="subline">Scanning hardware, MicroSD card and installed modules...</p>
    </div>
    <div class="pills">
      <span class="pill" id="pill-bat"><span class="dot"></span><span>Bat: --%</span></span>
      <span class="pill" id="pill-sd"><span class="dot"></span><span>SD: -- MB</span></span>
      <span class="pill" id="pill-wifi"><span class="dot"></span><span>Wi-Fi: --</span></span>
      <button type="button" class="btn-muted btn-sm" id="btn-refresh">Refresh</button>
    </div>
  </header>

  <nav class="glass">
    <div class="tablist" role="tablist" aria-label="Device Hub Sections" id="tablist">
      <button type="button" role="tab" id="tab-modules" aria-selected="true" data-panel="panel-modules">Modules</button>
      <button type="button" role="tab" id="tab-webradio" aria-selected="false" data-panel="panel-webradio" hidden>Web Radio &amp; Markets</button>
      <button type="button" role="tab" id="tab-gameboy" aria-selected="false" data-panel="panel-gameboy" hidden>Game Boy Color &amp; DMG</button>
      <button type="button" role="tab" id="tab-clock" aria-selected="false" data-panel="panel-clock" hidden>Calendar &amp; Clock</button>
      <button type="button" role="tab" id="tab-miner" aria-selected="false" data-panel="panel-miner" hidden>BTC Miner</button>
      <button type="button" role="tab" id="tab-pcmon" aria-selected="false" data-panel="panel-pcmon" hidden>PC Monitor</button>
      <button type="button" role="tab" id="tab-trmnl" aria-selected="false" data-panel="panel-trmnl" hidden>TRMNL Frame</button>
      <button type="button" role="tab" id="tab-flight" aria-selected="false" data-panel="panel-flight" hidden>Flight &amp; Air</button>
      <button type="button" role="tab" id="tab-transit" aria-selected="false" data-panel="panel-transit" hidden>myBus Transit</button>
      <button type="button" role="tab" id="tab-markets" aria-selected="false" data-panel="panel-markets" hidden>NBP &amp; Crypto</button>
      <button type="button" role="tab" id="tab-system" aria-selected="false" data-panel="panel-system">System &amp; Wi-Fi</button>
    </div>
  </nav>

  <main class="glass">
    <!-- Modules Panel -->
    <section role="tabpanel" id="panel-modules" aria-labelledby="tab-modules">
      <div class="panel-inner">
        <h2>Installed MicroSD Firmwares (/firmware/)</h2>
        <div class="subcard" style="margin-bottom:14px">
          <div style="display:flex;flex-wrap:wrap;gap:12px;align-items:center;justify-content:space-between">
            <label class="check-row" style="margin:0"><input type="checkbox" id="ab-en"><span>Auto-Boot last selected module on startup (press <strong>[KEY]</strong> on Splash Screen for Menu)</span></label>
            <div style="display:flex;align-items:center;gap:8px">
              <label for="ab-sec" class="muted" style="white-space:nowrap">Delay (sec):</label>
              <input type="number" id="ab-sec" min="2" max="10" value="3" style="width:68px">
              <button type="button" class="btn-ok btn-sm" onclick="saveBootSettings()">Save Auto-Boot</button>
            </div>
          </div>
        </div>
        <div id="fw-list" class="fw-list"><p class="muted">Loading firmware list...</p></div>

        <div class="upload-box">
          <h3>Upload New Firmware (.bin)</h3>
          <form id="form-fw" class="upload-row">
            <input type="file" id="fw-file" accept=".bin" required>
            <button type="submit">Upload .bin to SD</button>
          </form>
          <progress id="fw-prog" value="0" max="100" hidden></progress>
          <div id="fw-status" class="prog-label"></div>
        </div>
      </div>
    </section>

    <!-- WebRadio Panel -->
    <section role="tabpanel" id="panel-webradio" aria-labelledby="tab-webradio" hidden>
      <div class="panel-inner">
        <h2>Web Radio &amp; Markets &mdash; Presets, Weather, RSS &amp; Financial Desk</h2>
        <form id="form-radio">
          <div class="grid">
            <div class="subcard">
              <h3>Radio Station Presets (1-10)</h3>
              <div id="station-inputs"></div>
              <hr style="border:0;border-top:1px solid rgba(148,163,184,0.16);margin:12px 0">
              <h3>Cloudless Financial Desk (NBP &amp; Crypto)</h3>
              <div class="field"><label for="r-rmode">Right Dashboard Column</label>
                <select id="r-rmode">
                  <option value="0">0 - Financial Desk (NBP &amp; Crypto)</option>
                  <option value="1">1 - Homelab Telemetry</option>
                  <option value="2">2 - System &amp; Audio</option>
                </select>
              </div>
              <label class="check-row"><input type="checkbox" id="r-fgraf"><span>Graphical Mini-Sparklines</span></label>
              <div id="fin-slots-inputs"></div>
            </div>
            <div class="subcard">
              <h3>Weather, Display &amp; Audio DSP</h3>
              <div class="grid" style="grid-template-columns:1fr 1fr;gap:8px">
                <div class="field"><label for="r-city">City Name</label><input type="text" id="r-city" maxlength="30"></div>
                <div class="field"><label for="r-profile">EQ Profile</label>
                  <select id="r-profile">
                    <option value="0">0 - Warm / Music</option>
                    <option value="1">1 - Voice / News</option>
                    <option value="2">2 - Loudness Boost</option>
                    <option value="3">3 - Neutral / Flat</option>
                  </select>
                </div>
                <div class="field"><label for="r-lat">Latitude</label><input type="number" step="0.0001" id="r-lat"></div>
                <div class="field"><label for="r-lon">Longitude</label><input type="number" step="0.0001" id="r-lon"></div>
              </div>
              <div class="field"><label for="r-view">Default Boot Screen View</label>
                <select id="r-view">
                  <option value="0">View 0 - Weather + Financial Desk</option>
                  <option value="1">View 1 - Full RSS News Reader</option>
                  <option value="2">View 2 - SD Music Player &amp; Lyrics</option>
                </select>
              </div>
              <label class="check-row"><input type="checkbox" id="r-wgraf"><span>Graphical Weather Icons &amp; Rain/Temp Gauges</span></label>
              <label class="check-row"><input type="checkbox" id="r-drc"><span>Enable Dynamic Range Compression (DRC)</span></label>
              <label class="check-row"><input type="checkbox" id="r-mono"><span>Force Mono Downmix (Speaker Clarity)</span></label>
              <hr style="border:0;border-top:1px solid rgba(148,163,184,0.16);margin:12px 0">
              <h3>RSS Reader &amp; Ticker</h3>
              <label class="check-row"><input type="checkbox" id="r-rss-en"><span>Enable Auto-Scrolling RSS Ticker</span></label>
              <div class="field"><label for="r-rss-url">RSS Feed URL</label><input type="url" id="r-rss-url"></div>
              <div class="field"><label for="r-rss-sec">Short Headline Dwell (sec, 2-20)</label><input type="number" id="r-rss-sec" min="2" max="20" value="4"></div>
              <hr style="border:0;border-top:1px solid rgba(148,163,184,0.16);margin:12px 0">
              <label class="check-row"><input type="checkbox" id="r-hl-en"><span>Enable Optional Homelab Status Card</span></label>
              <div class="field"><label for="r-hl-url">Homelab Telemetry URL (JSON)</label><input type="url" id="r-hl-url" placeholder="http://192.168.1.10:8080/api/status"></div>
              <button type="submit" class="btn-ok">Save WebRadio Settings</button>
            </div>
          </div>
        </form>
      </div>
    </section>

    <!-- Game Boy Panel -->
    <section role="tabpanel" id="panel-gameboy" aria-labelledby="tab-gameboy" hidden>
      <div class="panel-inner">
        <h2>Game Boy DMG ROM Library (/roms/) &amp; Audio</h2>
        <div class="grid" style="margin-bottom:14px">
          <div class="subcard">
            <h3>Upload ROM (.gb / .gbc)</h3>
            <form id="form-rom">
              <div class="upload-row">
                <input type="file" id="rom-file" accept=".gb,.gbc" required>
                <button type="submit">Upload ROM</button>
              </div>
              <progress id="rom-prog" value="0" max="100" hidden></progress>
              <div id="rom-status" class="prog-label"></div>
            </form>
          </div>
          <div class="subcard">
            <h3>Speaker Default Volume</h3>
            <form id="form-gb-cfg">
              <div class="field">
                <label for="gb-vol">Audio Output Step (0 = Mute, 21 = Max): <strong id="gb-vol-val">14</strong></label>
                <input type="range" id="gb-vol" min="0" max="21" value="14">
              </div>
              <button type="submit" class="btn-ok">Save Game Boy Volume</button>
            </form>
          </div>
        </div>
        <div class="subcard" style="overflow-x:auto">
          <h3>Validated ROM Files</h3>
          <table>
            <thead><tr><th>File</th><th>Header Title</th><th>Size</th><th>DMG Compatibility</th><th>Action</th></tr></thead>
            <tbody id="rom-tbody"><tr><td colspan="5" class="muted">No ROMs scanned yet.</td></tr></tbody>
          </table>
        </div>
      </div>
    </section>

    <!-- Calendar & Clock Panel -->
    <section role="tabpanel" id="panel-clock" aria-labelledby="tab-clock" hidden>
      <div class="panel-inner">
        <h2>Calendar &amp; Desk Clock Settings</h2>
        <form id="form-clock">
          <div class="grid">
            <div class="subcard">
              <h3>Display &amp; ECO Power</h3>
              <div class="field">
                <label for="c-view">Default Boot View</label>
                <select id="c-view">
                  <option value="0">View 0 - Classic Desk Calendar</option>
                  <option value="1">View 1 - Big Clock &amp; Climate</option>
                  <option value="2">View 2 - Month Browser</option>
                </select>
              </div>
              <div class="field">
                <label for="c-tz">POSIX Timezone (TZ)</label>
                <input type="text" id="c-tz" placeholder="CET-1CEST,M3.5.0,M10.5.0/3">
              </div>
              <div class="field">
                <label for="c-ntp">NTP Re-Sync Interval (hours, 1-168)</label>
                <input type="number" id="c-ntp" min="1" max="168" value="12">
              </div>
              <label class="check-row"><input type="checkbox" id="c-24h"><span>24-Hour Clock Format</span></label>
              <label class="check-row"><input type="checkbox" id="c-sec"><span>Show Seconds (disables 60s deep-ECO display rate)</span></label>
            </div>
            <div class="subcard">
              <h3>Custom Desk Event Marker</h3>
              <div class="field">
                <label for="c-ev-date">Event Date (DD.MM, e.g. 19.11)</label>
                <input type="text" id="c-ev-date" maxlength="10" placeholder="19.11">
              </div>
              <div class="field">
                <label for="c-ev-text">Event Label</label>
                <input type="text" id="c-ev-text" maxlength="28" placeholder="Hardware Release">
              </div>
              <p class="muted">Settings apply immediately from shared NVS next time Calendar &amp; Clock boots.</p>
              <button type="submit" class="btn-ok">Save Clock Settings</button>
            </div>
          </div>
        </form>
      </div>
    </section>

    <!-- BTC Miner Panel -->
    <section role="tabpanel" id="panel-miner" aria-labelledby="tab-miner" hidden>
      <div class="panel-inner">
        <h2>Bitcoin Solo Miner Settings</h2>
        <form id="form-miner">
          <div class="grid">
            <div class="subcard">
              <h3>Payout Wallet &amp; Worker</h3>
              <div class="field"><label for="m-addr">Bitcoin Payout Address</label><input type="text" id="m-addr" required></div>
              <div class="field"><label for="m-worker">Worker Name</label><input type="text" id="m-worker" maxlength="32" required></div>
            </div>
            <div class="subcard">
              <h3>Stratum Solo Pool</h3>
              <div class="field"><label for="m-host">Pool Hostname</label><input type="text" id="m-host" required></div>
              <div class="field"><label for="m-port">Pool Port</label><input type="number" id="m-port" min="1" max="65535" required></div>
              <div class="actions" style="margin-bottom:12px">
                <button type="button" class="btn-muted btn-sm" onclick="setPool('public-pool.io',21496)">public-pool.io</button>
                <button type="button" class="btn-muted btn-sm" onclick="setPool('solo.ckpool.org',3333)">solo.ckpool.org</button>
              </div>
              <button type="submit" class="btn-ok">Save Miner Settings</button>
            </div>
          </div>
        </form>
      </div>
    </section>

    <!-- PC Monitor Panel -->
    <section role="tabpanel" id="panel-pcmon" aria-labelledby="tab-pcmon" hidden>
      <div class="panel-inner">
        <h2>PC Telemetry &amp; Cyber-Pet Monitor</h2>
        <form id="form-pcmon">
          <div class="grid">
            <div class="subcard">
              <h3>Display &amp; Identity</h3>
              <div class="field"><label for="pc-view">Default Boot View</label>
                <select id="pc-view"><option value="0">View 0 - Telemetry Dashboard</option><option value="1">View 1 - Pixel Cyber-Pet</option></select>
              </div>
              <div class="field"><label for="pc-host">Default Host Label</label><input type="text" id="pc-host" maxlength="24"></div>
              <div class="field"><label for="pc-pet">Cyber-Pet Name</label><input type="text" id="pc-pet" maxlength="16"></div>
              <button type="submit" class="btn-ok">Save PC Monitor Settings</button>
            </div>
            <div class="subcard">
              <h3>Companion Agent Usage</h3>
              <p class="muted">Run <code>python rlcd-board-suite\tools\pc_telemetry_agent.py --port COM11</code> while PC Monitor is active to stream real-time CPU, RAM, Disk &amp; AI task frames over USB or Wi-Fi.</p>
            </div>
          </div>
        </form>
      </div>
    </section>

    <!-- TRMNL Frame Panel -->
    <section role="tabpanel" id="panel-trmnl" aria-labelledby="tab-trmnl" hidden>
      <div class="panel-inner">
        <h2>TRMNL BYOS, 1-Bit Gallery &amp; Noticeboard</h2>
        <form id="form-trmnl">
          <div class="grid">
            <div class="subcard">
              <h3>Mode &amp; Noticeboard</h3>
              <div class="field"><label for="tr-mode">Default Boot Mode</label>
                <select id="tr-mode"><option value="0">Mode 0 - SD 1-Bit Gallery (/images/)</option><option value="1">Mode 1 - Desk Noticeboard</option><option value="2">Mode 2 - TRMNL BYOS Server</option></select>
              </div>
              <div class="field"><label for="tr-hdr">Noticeboard Header</label><input type="text" id="tr-hdr" maxlength="32"></div>
              <div class="field"><label for="tr-body">Noticeboard Body</label><input type="text" id="tr-body" maxlength="160"></div>
            </div>
            <div class="subcard">
              <h3>BYOS Server &amp; Slideshow</h3>
              <div class="field"><label for="tr-url">TRMNL Server / BMP URL</label><input type="url" id="tr-url" placeholder="https://usetrmnl.com/api/display"></div>
              <div class="field"><label for="tr-key">Access-Token (Optional)</label><input type="text" id="tr-key"></div>
              <div class="field"><label for="tr-ref">Refresh Interval (sec, 15-3600)</label><input type="number" id="tr-ref" min="15" max="3600" value="300"></div>
              <label class="check-row"><input type="checkbox" id="tr-inv"><span>Invert 1-Bit Gallery Ink</span></label>
              <button type="submit" class="btn-ok">Save TRMNL Settings</button>
            </div>
          </div>
        </form>
      </div>
    </section>

    <!-- Flight & Air Panel -->
    <section role="tabpanel" id="panel-flight" aria-labelledby="tab-flight" hidden>
      <div class="panel-inner">
        <h2>Flight Radar (adsb.fi) &amp; Air Quality (Open-Meteo)</h2>
        <form id="form-flight">
          <div class="grid">
            <div class="subcard">
              <h3>Radar Center Coordinates</h3>
              <div class="field"><label for="fl-city">City Label</label><input type="text" id="fl-city" maxlength="24"></div>
              <div class="grid" style="grid-template-columns:1fr 1fr;gap:8px">
                <div class="field"><label for="fl-lat">Latitude</label><input type="number" step="0.0001" id="fl-lat"></div>
                <div class="field"><label for="fl-lon">Longitude</label><input type="number" step="0.0001" id="fl-lon"></div>
              </div>
            </div>
            <div class="subcard">
              <h3>Radar &amp; Polling Options</h3>
              <div class="field"><label for="fl-rad">Search Radius (NM, 5-150)</label><input type="number" id="fl-rad" min="5" max="150" value="25"></div>
              <div class="field"><label for="fl-poll">ADS-B Poll Interval (sec, 15-300)</label><input type="number" id="fl-poll" min="15" max="300" value="30"></div>
              <div class="field"><label for="fl-view">Default View</label>
                <select id="fl-view"><option value="0">View 0 - Radar Scope</option><option value="1">View 1 - Air Quality &amp; Climate</option><option value="2">View 2 - Flights Table</option></select>
              </div>
              <button type="submit" class="btn-ok">Save Flight &amp; Air Settings</button>
            </div>
          </div>
        </form>
      </div>
    </section>

    <!-- myBus Transit Board Panel -->
    <section role="tabpanel" id="panel-transit" aria-labelledby="tab-transit" hidden>
      <div class="panel-inner">
        <h2>myBus online / MUNICOM.premium / R&amp;G PLUS Transit Board</h2>
        <form id="form-transit">
          <div class="grid">
            <div class="subcard">
              <h3>City &amp; myBus Server</h3>
              <div class="field"><label for="tb-city">City Label</label><input type="text" id="tb-city" value="Starachowice 27-200"></div>
              <div class="field"><label for="tb-url">MUNICOM / myBus Server URL</label><input type="url" id="tb-url" value="http://rozklad-jazdy.zecstar.pl"></div>
              <div class="field"><label for="tb-view">Default View</label>
                <select id="tb-view"><option value="0">View 0 - Single Stop Board (6 Departures)</option><option value="1">View 1 - Dual Stop Split Board</option></select>
              </div>
            </div>
            <div class="subcard">
              <h3>Primary &amp; Secondary Bus Stops</h3>
              <div class="field"><label for="tb-s0-id">Stop #1 ID (e.g. 493)</label><input type="number" id="tb-s0-id" value="493"></div>
              <div class="field"><label for="tb-s0-lb">Stop #1 Name</label><input type="text" id="tb-s0-lb" value="Radomska / Urzad Miasta 02"></div>
              <div class="field"><label for="tb-s1-id">Stop #2 ID (e.g. 8)</label><input type="number" id="tb-s1-id" value="8"></div>
              <div class="field"><label for="tb-s1-lb">Stop #2 Name</label><input type="text" id="tb-s1-lb" value="Radomska / Urzad Miasta 01"></div>
              <button type="submit" class="btn-ok">Save Transit Board Settings</button>
            </div>
          </div>
        </form>
      </div>
    </section>

    <!-- NBP & Crypto Markets Panel -->
    <section role="tabpanel" id="panel-markets" aria-labelledby="tab-markets" hidden>
      <div class="panel-inner">
        <h2>NBP Currency Desk (Table A) &amp; Crypto Markets</h2>
        <form id="form-markets">
          <div class="grid">
            <div class="subcard">
              <h3>Default View &amp; Polling</h3>
              <div class="field"><label for="mk-view">Boot View</label>
                <select id="mk-view"><option value="0">View 0 - NBP Currency Desk (EUR/USD/CHF/GBP)</option><option value="1">View 1 - Crypto &amp; Gold Desk</option><option value="2">View 2 - Full-Screen 30D Analyst Chart</option></select>
              </div>
              <div class="field"><label for="mk-poll">Poll Interval (sec, 30-900)</label><input type="number" id="mk-poll" min="30" max="900" value="120"></div>
            </div>
            <div class="subcard">
              <h3>Top Crypto Pairs</h3>
              <div class="field"><label for="mk-c4-cd">Asset #1 Label</label><input type="text" id="mk-c4-cd" value="BTC"></div>
              <div class="field"><label for="mk-c4-sy">Asset #1 Symbol</label><input type="text" id="mk-c4-sy" value="BTCUSDT"></div>
              <div class="field"><label for="mk-c5-cd">Asset #2 Label</label><input type="text" id="mk-c5-cd" value="ETH"></div>
              <div class="field"><label for="mk-c5-sy">Asset #2 Symbol</label><input type="text" id="mk-c5-sy" value="ETHUSDT"></div>
              <button type="submit" class="btn-ok">Save Market Desk Settings</button>
            </div>
          </div>
        </form>
      </div>
    </section>

    <!-- System & Wi-Fi Panel -->
    <section role="tabpanel" id="panel-system" aria-labelledby="tab-system" hidden>
      <div class="panel-inner">
        <h2>Shared Wi-Fi &amp; System Control</h2>
        <div class="grid">
          <div class="subcard">
            <h3>Board Wi-Fi Credentials (Shared Across All Modules)</h3>
            <form id="form-wifi">
              <div class="field"><label for="w-ssid">Wi-Fi SSID</label><input type="text" id="w-ssid" required></div>
              <div class="field"><label for="w-pass">Wi-Fi Password</label><input type="password" id="w-pass" placeholder="Leave unchanged or enter password"></div>
              <button type="submit" class="btn-ok">Save Wi-Fi &amp; Reboot</button>
            </form>
          </div>
          <div class="subcard">
            <h3>Device Actions</h3>
            <p class="muted">Firmware binaries live in <code>/firmware/</code> and Game Boy ROMs in <code>/roms/</code> on your MicroSD card. Launcher always restores back on Reset.</p>
            <div class="actions">
              <button type="button" class="btn-muted" onclick="loadState()">Rescan SD Card</button>
              <button type="button" class="btn-danger" id="btn-reboot">Reboot Launcher</button>
            </div>
          </div>
        </div>
      </div>
    </section>
  </main>
</div>

<div id="toast" class="toast" role="status" aria-live="polite"></div>

<script>
const $ = s => document.querySelector(s);
const $$ = s => Array.from(document.querySelectorAll(s));
let appState = null;

function esc(str){
  return String(str ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}

function showToast(msg, isErr=false){
  const t = $('#toast');
  t.textContent = msg;
  t.className = 'toast show' + (isErr ? ' err' : '');
  clearTimeout(t._tm);
  t._tm = setTimeout(() => t.classList.remove('show'), 3800);
}

function selectTab(tabBtn){
  const tabs = $$('[role="tab"]').filter(b => !b.hidden);
  tabs.forEach(b => b.setAttribute('aria-selected', b === tabBtn ? 'true' : 'false'));
  $$('[role="tabpanel"]').forEach(p => p.hidden = (p.id !== tabBtn.dataset.panel));
  tabBtn.scrollIntoView({block:'nearest', inline:'nearest'});
}

$('#tablist').addEventListener('click', e => {
  const b = e.target.closest('[role="tab"]');
  if (b && !b.hidden) selectTab(b);
});

$('#tablist').addEventListener('keydown', e => {
  const tabs = $$('[role="tab"]').filter(b => !b.hidden);
  const idx = tabs.indexOf(document.activeElement);
  if (idx < 0) return;
  let next = -1;
  if (e.key === 'ArrowRight') next = (idx + 1) % tabs.length;
  else if (e.key === 'ArrowLeft') next = (idx - 1 + tabs.length) % tabs.length;
  if (next >= 0){
    e.preventDefault();
    tabs[next].focus();
    selectTab(tabs[next]);
  }
});

function setPool(h, p){
  $('#m-host').value = h;
  $('#m-port').value = p;
}

async function postJson(url, bodyObj){
  const r = await fetch(url, {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(bodyObj)
  });
  const txt = await r.text();
  let data = {};
  try{ data = JSON.parse(txt); }catch(_){ data = {ok: r.ok, message: txt}; }
  if (!r.ok || data.ok === false) throw new Error(data.error || data.message || ('HTTP ' + r.status));
  return data;
}

function uploadWithProgress(url, fileInput, progEl, statusEl, onDone){
  const file = fileInput.files[0];
  if (!file){ showToast('Choose a file first', true); return; }
  const fd = new FormData();
  fd.append('file', file, file.name);
  progEl.hidden = false;
  progEl.value = 0;
  statusEl.textContent = 'Uploading ' + file.name + ' (0%)...';
  const xhr = new XMLHttpRequest();
  xhr.open('POST', url, true);
  xhr.upload.onprogress = ev => {
    if (ev.lengthComputable){
      const pct = Math.round((ev.loaded / ev.total) * 100);
      progEl.value = pct;
      statusEl.textContent = 'Uploading ' + file.name + ' (' + pct + '%)...';
    }
  };
  xhr.onload = () => {
    if (xhr.status >= 200 && xhr.status < 300){
      progEl.value = 100;
      statusEl.textContent = 'Upload complete!';
      fileInput.value = '';
      showToast('Uploaded ' + file.name);
      if (onDone) onDone(xhr.responseText);
      loadState();
    } else {
      statusEl.textContent = 'Upload failed (HTTP ' + xhr.status + ')';
      showToast('Upload failed: ' + xhr.responseText, true);
    }
  };
  xhr.onerror = () => {
    statusEl.textContent = 'Network error during upload.';
    showToast('Network error during upload', true);
  };
  xhr.send(fd);
}

async function bootFw(fname){
  try{
    showToast('Flashing / Booting ' + fname + ' ... Watch device screen!');
    await postJson('/boot', {file: fname});
  }catch(e){ showToast(e.message, true); }
}

async function delFile(type, fname){
  if (!confirm('Delete ' + fname + ' from MicroSD card?')) return;
  try{
    await postJson('/api/delete', {type, file: fname});
    showToast('Deleted ' + fname);
    loadState();
  }catch(e){ showToast(e.message, true); }
}

async function moveFw(idx, delta){
  if (!appState || !appState.firmwares) return;
  const order = appState.firmwares.map(f => f.filename);
  const ni = idx + delta;
  if (ni < 0 || ni >= order.length) return;
  const tmp = order[idx];
  order[idx] = order[ni];
  order[ni] = tmp;
  try{
    await postJson('/api/firmware/order', {order});
    showToast('Module order updated');
    loadState();
  }catch(e){ showToast(e.message, true); }
}

async function saveBootSettings(){
  try{
    await postJson('/api/firmware/order', {
      autoboot: $('#ab-en').checked,
      autoboot_sec: parseInt($('#ab-sec').value, 10) || 3
    });
    showToast('Auto-Boot settings saved');
    loadState();
  }catch(e){ showToast(e.message, true); }
}

function renderState(st){
  appState = st;
  $('#pill-bat').innerHTML = `<span class="dot"></span><span>Bat: ${esc(st.battery_pct)}% (${Number(st.battery_v||0).toFixed(2)}V)</span>`;
  $('#pill-sd').innerHTML = `<span class="dot"></span><span>SD: ${esc(st.sd_used_mb)}/${esc(st.sd_total_mb)} MB</span>`;
  const wOk = !!st.wifi_connected;
  $('#pill-wifi').className = 'pill' + (wOk ? '' : ' warn');
  $('#pill-wifi').innerHTML = `<span class="dot"></span><span>${wOk ? esc(st.wifi_ssid)+' ('+esc(st.ip)+')' : 'AP: '+esc(st.ip)}</span>`;
  $('#subline').textContent = 'Active Flash App: ' + (st.active_app || 'None') + ' | Detected Modules: ' + (st.firmwares || []).length;
  if ($('#ab-en')) $('#ab-en').checked = !!st.autoboot;
  if ($('#ab-sec')) $('#ab-sec').value = st.autoboot_sec ?? 3;

  const detected = new Set((st.firmwares || []).map(f => f.module_type));
  $('#tab-webradio').hidden = !detected.has('webradio');
  $('#tab-gameboy').hidden  = !detected.has('gameboy');
  $('#tab-clock').hidden    = !detected.has('calendar-clock');
  $('#tab-miner').hidden    = !detected.has('btc-miner');
  $('#tab-pcmon').hidden    = !detected.has('pc-monitor');
  $('#tab-trmnl').hidden    = !detected.has('trmnl-frame');
  $('#tab-flight').hidden   = !detected.has('flight-air');
  $('#tab-transit').hidden  = !detected.has('transit-board');
  $('#tab-markets').hidden  = !detected.has('crypto-stocks');

  // If currently selected tab got hidden, fall back to Modules
  const activeTab = $('[role="tab"][aria-selected="true"]');
  if (activeTab && activeTab.hidden) selectTab($('#tab-modules'));

  // Firmware list
  const fwEl = $('#fw-list');
  if (!st.firmwares || !st.firmwares.length){
    fwEl.innerHTML = '<p class="muted">No firmware .bin files found in /firmware/.</p>';
  } else {
    const nFw = st.firmwares.length;
    fwEl.innerHTML = st.firmwares.map((f, idx) => `
      <div class="fw-item ${f.is_installed ? 'active' : ''}">
        <div>
          <div class="fw-title">
            ${idx + 1}. ${esc(f.title)}
            ${f.is_installed ? '<span class="badge b-ok">Installed in Flash</span>' : '<span class="badge b-info">On SD</span>'}
          </div>
          <div class="fw-sub">${esc(f.filename)} &middot; ${(f.size_bytes/1024/1024).toFixed(2)} MB &middot; ${esc(f.subtitle)}</div>
        </div>
        <div class="actions">
          <button type="button" class="btn-muted btn-sm" onclick="moveFw(${idx},-1)" ${idx===0?'disabled':''} title="Move Up">&#9650;</button>
          <button type="button" class="btn-muted btn-sm" onclick="moveFw(${idx},1)" ${idx===nFw-1?'disabled':''} title="Move Down">&#9660;</button>
          <button type="button" class="${f.is_installed ? 'btn-ok' : ''}" onclick="bootFw('${esc(f.filename)}')">${f.is_installed ? 'Boot Now' : 'Flash & Boot'}</button>
          <button type="button" class="btn-danger btn-sm" onclick="delFile('fw','${esc(f.filename)}')">Delete</button>
        </div>
      </div>
    `).join('');
  }

  // WebRadio config
  if (st.webradio){
    const r = st.webradio;
    const sts = r.stations || [];
    let sHtml = '';
    for (let i = 0; i < 10; i++){
      const s = sts[i] || {name:'', url:''};
      sHtml += `<div class="station-row">
        <input type="text" id="st-n-${i}" value="${esc(s.name)}" placeholder="Station ${i+1} Name" maxlength="24">
        <input type="url" id="st-u-${i}" value="${esc(s.url)}" placeholder="https://stream-url...">
      </div>`;
    }
    $('#station-inputs').innerHTML = sHtml;
    const fslots = r.fin_slots || [];
    let fHtml = '';
    for (let i = 0; i < 4; i++){
      const f = fslots[i] || {label:'', symbol:'', is_nbp:true};
      fHtml += `<div class="station-row" style="grid-template-columns:1fr 82px 1fr">
        <input type="text" id="f-lb-${i}" value="${esc(f.label)}" placeholder="Label" maxlength="12">
        <select id="f-nb-${i}"><option value="1" ${f.is_nbp?'selected':''}>NBP</option><option value="0" ${!f.is_nbp?'selected':''}>Binance</option></select>
        <input type="text" id="f-sy-${i}" value="${esc(f.symbol)}" placeholder="eur / BTCUSDT" maxlength="16">
      </div>`;
    }
    $('#fin-slots-inputs').innerHTML = fHtml;
    $('#r-city').value = r.city || '';
    $('#r-lat').value = r.lat ?? 52.2297;
    $('#r-lon').value = r.lon ?? 21.0122;
    $('#r-view').value = String(r.view ?? 0);
    $('#r-rmode').value = String(r.right_mode ?? 0);
    $('#r-wgraf').checked = (r.w_graf !== false);
    $('#r-fgraf').checked = (r.fin_graf !== false);
    $('#r-profile').value = String(r.profile ?? 0);
    $('#r-drc').checked = !!r.drc;
    $('#r-mono').checked = !!r.mono;
    $('#r-rss-en').checked = (r.rss_en !== false);
    $('#r-rss-url').value = r.rss_url || '';
    $('#r-rss-sec').value = r.rss_sec ?? 4;
    $('#r-hl-en').checked = !!r.homelab_en;
    $('#r-hl-url').value = r.homelab_url || '';
  }

  // Game Boy config & ROMs
  if (st.gameboy){
    $('#gb-vol').value = st.gameboy.volume ?? 14;
    $('#gb-vol-val').textContent = $('#gb-vol').value;
  }
  const romTb = $('#rom-tbody');
  if (!st.roms || !st.roms.length){
    romTb.innerHTML = '<tr><td colspan="5" class="muted">No .gb/.gbc ROMs found in /roms/. Upload one above!</td></tr>';
  } else {
    romTb.innerHTML = st.roms.map(r => `
      <tr>
        <td><strong>${esc(r.filename)}</strong></td>
        <td>${esc(r.internal_title || '-')}</td>
        <td>${Math.round((r.size_bytes||0)/1024)} KB</td>
        <td><span class="badge ${r.supported ? (r.cgb_flag===0x80 ? 'b-warn' : 'b-ok') : 'b-bad'}">${esc(r.reason)}</span></td>
        <td><button type="button" class="btn-danger btn-sm" onclick="delFile('rom','${esc(r.filename)}')">Delete</button></td>
      </tr>
    `).join('');
  }

  // Clock config
  if (st.clock){
    $('#c-view').value = String(st.clock.view ?? 0);
    $('#c-tz').value = st.clock.tz || '';
    $('#c-ntp').value = st.clock.ntp_hrs ?? 12;
    $('#c-24h').checked = !!st.clock.fmt24;
    $('#c-sec').checked = !!st.clock.show_sec;
    $('#c-ev-date').value = st.clock.ev_date || '';
    $('#c-ev-text').value = st.clock.ev_text || '';
  }

  // Miner config
  if (st.miner){
    $('#m-addr').value = st.miner.btc_addr || '';
    $('#m-worker').value = st.miner.worker || '';
    $('#m-host').value = st.miner.pool_host || 'public-pool.io';
    $('#m-port').value = st.miner.pool_port || 21496;
  }

  // PC Monitor config
  if (st.pcmon){
    $('#pc-view').value = String(st.pcmon.view ?? 0);
    $('#pc-host').value = st.pcmon.host || 'Workstation';
    $('#pc-pet').value = st.pcmon.pet_name || 'SYNA';
  }

  // TRMNL config
  if (st.trmnl){
    $('#tr-mode').value = String(st.trmnl.mode ?? 1);
    $('#tr-hdr').value = st.trmnl.note_hdr || '';
    $('#tr-body').value = st.trmnl.note_body || '';
    $('#tr-url').value = st.trmnl.srv_url || '';
    $('#tr-key').value = st.trmnl.api_key || '';
    $('#tr-ref').value = st.trmnl.refresh_s ?? 300;
    $('#tr-inv').checked = !!st.trmnl.invert;
  }

  // Flight & Air config
  if (st.flight){
    $('#fl-city').value = st.flight.city || 'Warsaw';
    $('#fl-lat').value = st.flight.lat ?? 52.2297;
    $('#fl-lon').value = st.flight.lon ?? 21.0122;
    $('#fl-rad').value = st.flight.radius_nm ?? 25;
    $('#fl-poll').value = st.flight.poll_s ?? 30;
    $('#fl-view').value = String(st.flight.view ?? 0);
  }

  // Transit Board config
  if (st.transit){
    $('#tb-city').value = st.transit.city || 'Starachowice 27-200';
    $('#tb-url').value = st.transit.url || 'http://rozklad-jazdy.zecstar.pl';
    $('#tb-view').value = String(st.transit.view ?? 0);
    $('#tb-s0-id').value = st.transit.s0_id ?? 493;
    $('#tb-s0-lb').value = st.transit.s0_lb || 'Radomska / Urzad Miasta 02';
    $('#tb-s1-id').value = st.transit.s1_id ?? 8;
    $('#tb-s1-lb').value = st.transit.s1_lb || 'Radomska / Urzad Miasta 01';
  }

  // NBP & Crypto Markets config
  if (st.markets){
    $('#mk-view').value = String(st.markets.view ?? 0);
    $('#mk-poll').value = st.markets.poll_s ?? 120;
    $('#mk-c4-cd').value = st.markets.c4_cd || 'BTC';
    $('#mk-c4-sy').value = st.markets.c4_sy || 'BTCUSDT';
    $('#mk-c5-cd').value = st.markets.c5_cd || 'ETH';
    $('#mk-c5-sy').value = st.markets.c5_sy || 'ETHUSDT';
  }

  // Wi-Fi
  if (st.wifi_ssid) $('#w-ssid').value = st.wifi_ssid;
}

async function loadState(){
  try{
    const r = await fetch('/api/state', {cache:'no-store'});
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const st = await r.json();
    renderState(st);
  }catch(e){
    showToast('Failed to load state: ' + e.message, true);
  }
}

$('#btn-refresh').addEventListener('click', loadState);
$('#gb-vol').addEventListener('input', e => $('#gb-vol-val').textContent = e.target.value);

$('#form-fw').addEventListener('submit', e => {
  e.preventDefault();
  uploadWithProgress('/upload_fw', $('#fw-file'), $('#fw-prog'), $('#fw-status'));
});

$('#form-rom').addEventListener('submit', e => {
  e.preventDefault();
  uploadWithProgress('/upload_rom', $('#rom-file'), $('#rom-prog'), $('#rom-status'), resp => {
    try{
      const j = JSON.parse(resp);
      if (j && j.supported === false) showToast('Warning: ' + j.reason, true);
    }catch(_){}
  });
});

$('#form-radio').addEventListener('submit', async e => {
  e.preventDefault();
  const stations = [];
  for (let i = 0; i < 10; i++){
    stations.push({name: $('#st-n-' + i).value.trim(), url: $('#st-u-' + i).value.trim()});
  }
  const fin_slots = [];
  for (let i = 0; i < 4; i++){
    fin_slots.push({
      label: $('#f-lb-' + i).value.trim(),
      is_nbp: $('#f-nb-' + i).value === '1',
      symbol: $('#f-sy-' + i).value.trim()
    });
  }
  try{
    await postJson('/api/config/webradio', {
      stations,
      fin_slots,
      city: $('#r-city').value.trim(),
      lat: parseFloat($('#r-lat').value),
      lon: parseFloat($('#r-lon').value),
      view: parseInt($('#r-view').value, 10),
      right_mode: parseInt($('#r-rmode').value, 10),
      w_graf: $('#r-wgraf').checked,
      fin_graf: $('#r-fgraf').checked,
      profile: parseInt($('#r-profile').value, 10),
      drc: $('#r-drc').checked,
      mono: $('#r-mono').checked,
      rss_en: $('#r-rss-en').checked,
      rss_url: $('#r-rss-url').value.trim(),
      rss_sec: parseInt($('#r-rss-sec').value, 10),
      homelab_en: $('#r-hl-en').checked,
      homelab_url: $('#r-hl-url').value.trim()
    });
    showToast('WebRadio settings saved to NVS');
  }catch(err){ showToast(err.message, true); }
});

$('#form-gb-cfg').addEventListener('submit', async e => {
  e.preventDefault();
  try{
    await postJson('/api/config/gameboy', {volume: parseInt($('#gb-vol').value, 10)});
    showToast('Game Boy volume saved');
  }catch(err){ showToast(err.message, true); }
});

$('#form-clock').addEventListener('submit', async e => {
  e.preventDefault();
  try{
    await postJson('/api/config/clock', {
      view: parseInt($('#c-view').value, 10),
      tz: $('#c-tz').value.trim(),
      ntp_hrs: parseInt($('#c-ntp').value, 10),
      fmt24: $('#c-24h').checked,
      show_sec: $('#c-sec').checked,
      ev_date: $('#c-ev-date').value.trim(),
      ev_text: $('#c-ev-text').value.trim()
    });
    showToast('Calendar & Clock settings saved');
  }catch(err){ showToast(err.message, true); }
});

$('#form-miner').addEventListener('submit', async e => {
  e.preventDefault();
  try{
    await postJson('/api/config/miner', {
      btc_addr: $('#m-addr').value.trim(),
      worker: $('#m-worker').value.trim(),
      pool_host: $('#m-host').value.trim(),
      pool_port: parseInt($('#m-port').value, 10)
    });
    showToast('BTC Miner settings saved');
  }catch(err){ showToast(err.message, true); }
});

$('#form-pcmon').addEventListener('submit', async e => {
  e.preventDefault();
  try{
    await postJson('/api/config/pcmon', {
      view: parseInt($('#pc-view').value, 10),
      host: $('#pc-host').value.trim(),
      pet_name: $('#pc-pet').value.trim()
    });
    showToast('PC Monitor settings saved');
  }catch(err){ showToast(err.message, true); }
});

$('#form-trmnl').addEventListener('submit', async e => {
  e.preventDefault();
  try{
    await postJson('/api/config/trmnl', {
      mode: parseInt($('#tr-mode').value, 10),
      note_hdr: $('#tr-hdr').value.trim(),
      note_body: $('#tr-body').value.trim(),
      srv_url: $('#tr-url').value.trim(),
      api_key: $('#tr-key').value.trim(),
      refresh_s: parseInt($('#tr-ref').value, 10),
      invert: $('#tr-inv').checked
    });
    showToast('TRMNL Frame settings saved');
  }catch(err){ showToast(err.message, true); }
});

$('#form-flight').addEventListener('submit', async e => {
  e.preventDefault();
  try{
    await postJson('/api/config/flight', {
      city: $('#fl-city').value.trim(),
      lat: parseFloat($('#fl-lat').value),
      lon: parseFloat($('#fl-lon').value),
      radius_nm: parseInt($('#fl-rad').value, 10),
      poll_s: parseInt($('#fl-poll').value, 10),
      view: parseInt($('#fl-view').value, 10)
    });
    showToast('Flight & Air settings saved');
  }catch(err){ showToast(err.message, true); }
});

$('#form-transit').addEventListener('submit', async e => {
  e.preventDefault();
  try{
    await postJson('/api/config/transit', {
      city: $('#tb-city').value.trim(),
      url: $('#tb-url').value.trim(),
      view: parseInt($('#tb-view').value, 10),
      s0_id: parseInt($('#tb-s0-id').value, 10),
      s0_lb: $('#tb-s0-lb').value.trim(),
      s1_id: parseInt($('#tb-s1-id').value, 10),
      s1_lb: $('#tb-s1-lb').value.trim()
    });
    showToast('myBus Transit Board settings saved');
  }catch(err){ showToast(err.message, true); }
});

$('#form-markets').addEventListener('submit', async e => {
  e.preventDefault();
  try{
    await postJson('/api/config/markets', {
      view: parseInt($('#mk-view').value, 10),
      poll_s: parseInt($('#mk-poll').value, 10),
      c4_cd: $('#mk-c4-cd').value.trim(),
      c4_sy: $('#mk-c4-sy').value.trim(),
      c5_cd: $('#mk-c5-cd').value.trim(),
      c5_sy: $('#mk-c5-sy').value.trim()
    });
    showToast('NBP & Crypto Market settings saved');
  }catch(err){ showToast(err.message, true); }
});

$('#form-wifi').addEventListener('submit', async e => {
  e.preventDefault();
  try{
    await postJson('/api/wifi', {ssid: $('#w-ssid').value.trim(), pass: $('#w-pass').value});
    showToast('Wi-Fi saved! Launcher is rebooting...');
  }catch(err){ showToast(err.message, true); }
});

$('#btn-reboot').addEventListener('click', async () => {
  try{
    await postJson('/api/reboot', {});
    showToast('Rebooting Launcher...');
  }catch(err){ showToast(err.message, true); }
});

loadState();
</script>
</body>
</html>
)rawliteral";
