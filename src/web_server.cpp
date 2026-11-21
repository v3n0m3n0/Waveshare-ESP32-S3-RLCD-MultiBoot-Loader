#include "web_server.h"
#include "web_ui_assets.h"
#include "flasher.h"
#include "display.h"
#include "board_pins.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <SD_MMC.h>
#include <esp_heap_caps.h>

extern void get_battery_status(float &voltage, int &percent);

LauncherWebServer LauncherWeb;

LauncherWebServer::LauncherWebServer() {}

bool LauncherWebServer::consume_menu_dirty() {
    if (!menu_dirty_) return false;
    menu_dirty_ = false;
    return true;
}

static bool load_shared_wifi(String &out_ssid, String &out_pass) {
    const char *namespaces[] = {"wifi", "radio"};
    Preferences p;
    for (int i = 0; i < 2; i++) {
        p.begin(namespaces[i], true);
        String s = p.getString("ssid", "");
        String pw = p.getString("pass", "");
        p.end();
        s.trim();
        if (s.length() > 0 && s != "YOUR_WIFI_SSID") {
            out_ssid = s;
            out_pass = pw;
            return true;
        }
    }
    return false;
}

static bool is_safe_basename(const String &name) {
    if (name.isEmpty()) return false;
    if (name.indexOf("..") >= 0) return false;
    if (name.indexOf('/') >= 0 || name.indexOf('\\') >= 0) return false;
    if (name.startsWith(".")) return false;
    return true;
}

static bool parse_request_json(WebServer *srv, JsonDocument &doc) {
    if (!srv->hasArg("plain")) return false;
    String body = srv->arg("plain");
    body.trim();
    if (body.isEmpty()) return false;
    return deserializeJson(doc, body) == DeserializationError::Ok;
}

bool LauncherWebServer::begin() {
    String ssid, pass;
    bool has_creds = load_shared_wifi(ssid, pass);

    if (has_creds) {
        Serial.printf("[WIFI] Connecting to '%s'...\n", ssid.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());

        uint32_t t0 = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - t0 < 10000)) {
            delay(250);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            wifi_connected_ = true;
            ip_str_ = WiFi.localIP().toString();
            active_ssid_ = WiFi.SSID();
            Serial.printf("[WIFI] Connected! IP: %s\n", ip_str_.c_str());
        } else {
            Serial.println("[WIFI] Connection timed out, starting Setup AP.");
        }
    }

    if (!wifi_connected_) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("RLCD-MultiBoot-Hub", "12345678");
        ip_str_ = WiFi.softAPIP().toString();
        active_ssid_ = has_creds ? ssid : "";
        Serial.printf("[WIFI] Setup AP Active: SSID=RLCD-MultiBoot-Hub, IP=%s\n", ip_str_.c_str());
    }

    server_ = new WebServer(80);
    setup_routes();
    server_->begin();
    Serial.printf("[WEB] Device Hub active at http://%s/\n", ip_str_.c_str());
    return true;
}

String LauncherWebServer::get_ip() const {
    return ip_str_;
}

void LauncherWebServer::handle_file_upload(bool is_rom) {
    HTTPUpload &upload = server_->upload();

    if (upload.status == UPLOAD_FILE_START) {
        upload_is_rom_ = is_rom;
        upload_ok_ = false;
        upload_clean_name_ = "";

        if (!SDCard.is_mounted()) {
            Serial.println("[UPLOAD] SD card not mounted");
            return;
        }

        const char *dir_path = is_rom ? "/roms" : "/firmware";
        if (!SD_MMC.exists(dir_path)) SD_MMC.mkdir(dir_path);

        String clean = SDManager::sanitize_filename(upload.filename, is_rom ? ".gb" : ".bin");
        if (!is_safe_basename(clean)) {
            Serial.printf("[UPLOAD] Rejected invalid filename: %s\n", upload.filename.c_str());
            return;
        }

        upload_clean_name_ = clean;
        upload_tmp_ = String(dir_path) + "/.upload.tmp";
        upload_dst_ = String(dir_path) + "/" + clean;

        if (SD_MMC.exists(upload_tmp_)) SD_MMC.remove(upload_tmp_);
        upload_file_ = SD_MMC.open(upload_tmp_, FILE_WRITE);
        if (!upload_file_) {
            Serial.printf("[UPLOAD] Failed to open %s\n", upload_tmp_.c_str());
            return;
        }
        upload_ok_ = true;
        Serial.printf("[UPLOAD] Start -> %s\n", upload_dst_.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (upload_ok_ && upload_file_) {
            size_t written = upload_file_.write(upload.buf, upload.currentSize);
            if (written != upload.currentSize) {
                Serial.println("[UPLOAD] Short write / SD full!");
                upload_ok_ = false;
                upload_file_.close();
                SD_MMC.remove(upload_tmp_);
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (upload_file_) upload_file_.close();
        if (upload_ok_) {
            if (SD_MMC.exists(upload_dst_)) SD_MMC.remove(upload_dst_);
            if (!SD_MMC.rename(upload_tmp_, upload_dst_)) {
                Serial.println("[UPLOAD] Rename failed");
                SD_MMC.remove(upload_tmp_);
                upload_ok_ = false;
            } else {
                menu_dirty_ = true;
                Serial.printf("[UPLOAD] Completed: %s (%u bytes)\n", upload_dst_.c_str(), (unsigned)upload.totalSize);
            }
        } else {
            if (SD_MMC.exists(upload_tmp_)) SD_MMC.remove(upload_tmp_);
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (upload_file_) upload_file_.close();
        if (SD_MMC.exists(upload_tmp_)) SD_MMC.remove(upload_tmp_);
        upload_ok_ = false;
        Serial.println("[UPLOAD] Aborted");
    }
}

void LauncherWebServer::setup_routes() {
    server_->on("/", HTTP_GET, [this]() {
        server_->sendHeader("Cache-Control", "no-store");
        server_->send_P(200, "text/html", LAUNCHER_HTML);
    });

    server_->on("/api/state", HTTP_GET, [this]() {
        String active_app = Flasher.get_active_app();
        const auto &fws = SDCard.scan_firmwares(active_app);
        const auto &roms = SDCard.scan_roms();

        float vbat = 0.0f;
        int bat_pct = 0;
        get_battery_status(vbat, bat_pct);

        JsonDocument doc;
        doc["active_app"] = active_app;
        {
            Preferences lp;
            lp.begin("launcher", true);
            doc["autoboot"] = lp.getBool("autoboot", false);
            doc["autoboot_sec"] = constrain(lp.getInt("ab_sec", 3), 2, 10);
            lp.end();
        }
        doc["battery_v"] = vbat;
        doc["battery_pct"] = bat_pct;
        doc["sd_mounted"] = SDCard.is_mounted();
        doc["sd_total_mb"] = (unsigned long)(SDCard.get_total_bytes() / (1024ULL * 1024ULL));
        doc["sd_used_mb"] = (unsigned long)(SDCard.get_used_bytes() / (1024ULL * 1024ULL));
        doc["wifi_connected"] = wifi_connected_;
        doc["ip"] = ip_str_;

        String saved_ssid, saved_pass;
        load_shared_wifi(saved_ssid, saved_pass);
        doc["wifi_ssid"] = wifi_connected_ ? active_ssid_ : saved_ssid;

        JsonArray fw_arr = doc["firmwares"].to<JsonArray>();
        for (const auto &f : fws) {
            JsonObject o = fw_arr.add<JsonObject>();
            o["filename"] = f.filename;
            o["title"] = f.title;
            o["subtitle"] = f.subtitle;
            o["module_type"] = f.module_type;
            o["size_bytes"] = (unsigned long)f.size_bytes;
            o["is_installed"] = f.is_installed;
        }

        JsonArray rom_arr = doc["roms"].to<JsonArray>();
        for (const auto &r : roms) {
            JsonObject o = rom_arr.add<JsonObject>();
            o["filename"] = r.filename;
            o["internal_title"] = r.internal_title;
            o["mbc_type"] = r.mbc_type;
            o["cgb_flag"] = r.cgb_flag;
            o["supported"] = r.supported;
            o["reason"] = r.reason;
            o["size_bytes"] = (unsigned long)r.size_bytes;
        }

        // WebRadio shared NVS config (10 universal presets, RSS, Financial Desk, Graphics)
        {
            static const char *DEF_N[10] = {
                "RMF FM", "Radio ZET", "Radio 357", "Radio Nowy Swiat", "TOK FM",
                "Zlote Przeboje", "BBC World Service", "SomaFM Groove Salad", "Radio Swiss Jazz", "Radio Paradise Main"
            };
            static const char *DEF_U[10] = {
                "http://195.150.20.242:8000/rmf_fm",
                "http://zet-net-01.cdn.eurozet.pl:8400/",
                "https://stream.rcs.revma.com/ye5kghkgcm0uv",
                "https://stream.rcs.revma.com/ypqt40u0x1zuv",
                "http://radiostream.pl/tuba10-1.mp3",
                "http://radiostream.pl/tuba8-1.mp3",
                "http://stream.live.vc.bbcmedia.co.uk/bbc_world_service",
                "http://ice1.somafm.com/groovesalad-128-mp3",
                "http://stream.srg-ssr.ch/m/rsj/mp3_128",
                "http://stream.radioparadise.com/mp3-128"
            };

            JsonObject r = doc["webradio"].to<JsonObject>();
            Preferences p;
            p.begin("presets", true);
            int pver = p.getInt("ver", 0);
            r["presets_ver"] = pver;
            JsonArray st_arr = r["stations"].to<JsonArray>();
            for (int i = 0; i < 10; i++) {
                JsonObject s = st_arr.add<JsonObject>();
                char nk[6], uk[6];
                snprintf(nk, sizeof(nk), "n%d", i);
                snprintf(uk, sizeof(uk), "u%d", i);
                String sn = (pver >= 2) ? p.getString(nk, "") : "";
                String su = (pver >= 2) ? p.getString(uk, "") : "";
                s["name"] = sn.isEmpty() ? DEF_N[i] : sn;
                s["url"] = su.isEmpty() ? DEF_U[i] : su;
            }
            p.end();

            p.begin("radio_cfg", true);
            r["city"] = p.getString("city", "Warsaw");
            r["lat"] = p.getFloat("lat", 52.2297f);
            r["lon"] = p.getFloat("lon", 21.0122f);
            r["homelab_en"] = p.getBool("homelab_en", false);
            r["homelab_url"] = p.getString("homelab_url", "");
            r["rss_en"] = p.getBool("rss_en", true);
            r["rss_url"] = p.getString("rss_url", "https://www.pb.pl/rss/najnowsze.xml?utm_source=RSS&utm_medium=RSS&utm_campaign=Z%20ostatniej%20chwili");
            r["rss_sec"] = p.getInt("rss_sec", 4);
            r["w_graf"] = p.getBool("w_graf", true);
            r["view"] = p.getInt("view", 0);
            p.end();

            p.begin("radio_sound", true);
            r["profile"] = p.getInt("profile", 0);
            r["drc"] = p.getBool("drc", true);
            r["mono"] = p.getBool("mono", true);
            p.end();

            static const char *FLB[4] = {"EUR/PLN", "USD/PLN", "BTC/USD", "GOLD/USD"};
            static const char *FSY[4] = {"eur", "usd", "BTCUSDT", "PAXGUSDT"};
            static const bool FNBP[4] = {true, true, false, false};
            p.begin("radio_fin", true);
            r["right_mode"] = p.getInt("rmode", 0);
            r["fin_graf"] = p.getBool("fgraf", true);
            JsonArray fa = r["fin_slots"].to<JsonArray>();
            for (int i = 0; i < 4; i++) {
                JsonObject fo = fa.add<JsonObject>();
                char kl[10], ks[10], kn[10];
                snprintf(kl, sizeof(kl), "f%d_lb", i);
                snprintf(ks, sizeof(ks), "f%d_sy", i);
                snprintf(kn, sizeof(kn), "f%d_nbp", i);
                fo["label"] = p.getString(kl, FLB[i]);
                fo["symbol"] = p.getString(ks, FSY[i]);
                fo["is_nbp"] = p.getBool(kn, FNBP[i]);
            }
            p.end();
        }

        // Game Boy shared NVS config
        {
            Preferences p;
            p.begin("gameboy", true);
            JsonObject g = doc["gameboy"].to<JsonObject>();
            g["volume"] = p.getInt("volume", 14);
            p.end();
        }

        // Clock shared NVS config
        {
            Preferences p;
            p.begin("clock", true);
            JsonObject c = doc["clock"].to<JsonObject>();
            c["tz"] = p.getString("tz", "CET-1CEST,M3.5.0,M10.5.0/3");
            c["fmt24"] = p.getBool("fmt24", true);
            c["show_sec"] = p.getBool("show_sec", false);
            c["ntp_hrs"] = p.getInt("ntp_hrs", 12);
            c["view"] = p.getInt("view", 0);
            c["ev_date"] = p.getString("ev_date", "19.11");
            c["ev_text"] = p.getString("ev_text", "RLCD Day");
            p.end();
        }

        // BTC Miner shared NVS config
        {
            Preferences p;
            p.begin("miner", true);
            JsonObject m = doc["miner"].to<JsonObject>();
            m["btc_addr"] = p.getString("btc_addr", "");
            m["worker"] = p.getString("worker", "RLCD-Miner");
            m["pool_host"] = p.getString("pool_host", "public-pool.io");
            m["pool_port"] = p.getInt("pool_port", 21496);
            p.end();
        }

        // PC Monitor shared NVS config
        {
            Preferences p;
            p.begin("pcmon", true);
            JsonObject pc = doc["pcmon"].to<JsonObject>();
            pc["view"] = p.getInt("view", 0);
            pc["host"] = p.getString("host", "Workstation");
            pc["pet_name"] = p.getString("pet_name", "SYNA");
            p.end();
        }

        // TRMNL Frame shared NVS config
        {
            Preferences p;
            p.begin("trmnl", true);
            JsonObject tr = doc["trmnl"].to<JsonObject>();
            tr["mode"] = p.getInt("mode", 1);
            tr["srv_url"] = p.getString("srv_url", "");
            tr["api_key"] = p.getString("api_key", "");
            tr["refresh_s"] = p.getInt("refresh_s", 300);
            tr["note_hdr"] = p.getString("note_hdr", "FOCUS MODE");
            tr["note_body"] = p.getString("note_body", "Deep work session active.");
            tr["invert"] = p.getBool("invert", false);
            p.end();
        }

        // Flight Radar & Air Quality shared NVS config
        {
            Preferences p;
            p.begin("flight", true);
            JsonObject fl = doc["flight"].to<JsonObject>();
            fl["city"] = p.getString("city", "Warsaw");
            fl["lat"] = p.getFloat("lat", 52.2297f);
            fl["lon"] = p.getFloat("lon", 21.0122f);
            fl["radius_nm"] = p.getInt("radius_nm", 25);
            fl["poll_s"] = p.getInt("poll_s", 30);
            fl["view"] = p.getInt("view", 0);
            p.end();
        }

        // Transit Board (myBus R&G Starachowice 27-200) shared NVS config
        {
            Preferences p;
            p.begin("transit", true);
            JsonObject tb = doc["transit"].to<JsonObject>();
            tb["city"] = p.getString("city", "Starachowice 27-200");
            tb["url"] = p.getString("url", "http://rozklad-jazdy.zecstar.pl");
            tb["stop_idx"] = p.getInt("stop_idx", 0);
            tb["s0_id"] = p.getInt("s0_id", 493);
            tb["s0_lb"] = p.getString("s0_lb", "Radomska / Urzad Miasta 02");
            tb["s1_id"] = p.getInt("s1_id", 8);
            tb["s1_lb"] = p.getString("s1_lb", "Radomska / Urzad Miasta 01");
            tb["view"] = p.getInt("view", 0);
            p.end();
        }

        // NBP & Crypto Markets shared NVS config
        {
            Preferences p;
            p.begin("markets", true);
            JsonObject mk = doc["markets"].to<JsonObject>();
            mk["view"] = p.getInt("view", 0);
            mk["poll_s"] = p.getInt("poll_s", 120);
            mk["c4_cd"] = p.getString("c4_cd", "BTC");
            mk["c4_sy"] = p.getString("c4_sy", "BTCUSDT");
            mk["c5_cd"] = p.getString("c5_cd", "ETH");
            mk["c5_sy"] = p.getString("c5_sy", "ETHUSDT");
            p.end();
        }

        String out;
        serializeJson(doc, out);
        server_->sendHeader("Cache-Control", "no-store");
        server_->send(200, "application/json", out);
    });

    server_->on("/api/wifi", HTTP_POST, [this]() {
        JsonDocument doc;
        String ssid, pass;
        bool has_pass_key = false;
        if (parse_request_json(server_, doc)) {
            ssid = doc["ssid"] | "";
            if (!doc["pass"].isNull()) {
                pass = doc["pass"].as<String>();
                has_pass_key = true;
            }
        } else {
            ssid = server_->arg("ssid");
            if (server_->hasArg("pass")) {
                pass = server_->arg("pass");
                has_pass_key = true;
            }
        }
        ssid.trim();
        if (ssid.isEmpty()) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"SSID cannot be empty\"}");
            return;
        }
        if (!has_pass_key || pass.isEmpty()) {
            String prev_s, prev_pw;
            load_shared_wifi(prev_s, prev_pw);
            if (!has_pass_key) pass = prev_pw;
        }

        const char *ns[] = {"wifi", "radio"};
        for (int i = 0; i < 2; i++) {
            Preferences p;
            p.begin(ns[i], false);
            p.putString("ssid", ssid);
            if (has_pass_key && !pass.isEmpty()) {
                p.putString("pass", pass);
            }
            p.end();
        }

        server_->send(200, "application/json", "{\"ok\":true,\"restarting\":true}");
        pending_reboot_ = true;
        pending_action_at_ = millis() + 450;
    });

    server_->on("/api/config/webradio", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!parse_request_json(server_, doc)) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }

        if (doc["stations"].is<JsonArray>()) {
            JsonArray arr = doc["stations"].as<JsonArray>();
            if (arr.size() > 0) {
                Preferences p;
                p.begin("presets", false);
                p.putInt("ver", 3);
                int idx = 0;
                for (JsonObject s : arr) {
                    if (idx >= 10) break;
                    String nm = s["name"] | "";
                    String ur = s["url"] | "";
                    nm.trim();
                    ur.trim();
                    char nk[6], uk[6];
                    snprintf(nk, sizeof(nk), "n%d", idx);
                    snprintf(uk, sizeof(uk), "u%d", idx);
                    p.putString(nk, nm);
                    p.putString(uk, ur);
                    idx++;
                }
                p.end();
            }
        }

        {
            Preferences p;
            p.begin("radio_cfg", false);
            if (!doc["city"].isNull()) {
                String c = doc["city"].as<String>();
                c.trim();
                if (!c.isEmpty()) p.putString("city", c);
            }
            if (!doc["lat"].isNull()) p.putFloat("lat", doc["lat"].as<float>());
            if (!doc["lon"].isNull()) p.putFloat("lon", doc["lon"].as<float>());
            if (!doc["homelab_en"].isNull()) p.putBool("homelab_en", doc["homelab_en"].as<bool>());
            if (!doc["homelab_url"].isNull()) {
                String u = doc["homelab_url"].as<String>();
                u.trim();
                p.putString("homelab_url", u);
            }
            if (!doc["rss_en"].isNull()) p.putBool("rss_en", doc["rss_en"].as<bool>());
            if (!doc["rss_url"].isNull()) {
                String ru = doc["rss_url"].as<String>();
                ru.trim();
                if (!ru.isEmpty()) p.putString("rss_url", ru);
            }
            if (!doc["rss_sec"].isNull()) p.putInt("rss_sec", constrain(doc["rss_sec"].as<int>(), 2, 20));
            if (!doc["w_graf"].isNull()) p.putBool("w_graf", doc["w_graf"].as<bool>());
            if (!doc["view"].isNull()) p.putInt("view", constrain(doc["view"].as<int>(), 0, 2));
            p.end();
        }

        {
            Preferences p;
            p.begin("radio_fin", false);
            if (!doc["right_mode"].isNull()) p.putInt("rmode", constrain(doc["right_mode"].as<int>(), 0, 2));
            if (!doc["fin_graf"].isNull()) p.putBool("fgraf", doc["fin_graf"].as<bool>());
            if (doc["fin_slots"].is<JsonArray>()) {
                int idx = 0;
                for (JsonObject fo : doc["fin_slots"].as<JsonArray>()) {
                    if (idx >= 4) break;
                    String lb = fo["label"] | "";
                    String sy = fo["symbol"] | "";
                    bool isNbp = fo["is_nbp"] | true;
                    lb.trim();
                    sy.trim();
                    char kl[10], ks[10], kn[10];
                    snprintf(kl, sizeof(kl), "f%d_lb", idx);
                    snprintf(ks, sizeof(ks), "f%d_sy", idx);
                    snprintf(kn, sizeof(kn), "f%d_nbp", idx);
                    if (!lb.isEmpty()) p.putString(kl, lb);
                    if (!sy.isEmpty()) p.putString(ks, sy);
                    p.putBool(kn, isNbp);
                    idx++;
                }
            }
            p.end();
        }

        {
            Preferences p;
            p.begin("radio_sound", false);
            if (!doc["profile"].isNull()) {
                int prof = constrain(doc["profile"].as<int>(), 0, 3);
                p.putInt("profile", prof);
            }
            if (!doc["drc"].isNull()) p.putBool("drc", doc["drc"].as<bool>());
            if (!doc["mono"].isNull()) p.putBool("mono", doc["mono"].as<bool>());
            p.end();
        }

        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->on("/api/config/webradio/reset_presets", HTTP_POST, [this]() {
        Preferences p;
        p.begin("presets", false);
        p.clear();
        p.end();
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->on("/api/config/gameboy", HTTP_POST, [this]() {
        JsonDocument doc;
        int vol = -1;
        if (parse_request_json(server_, doc) && !doc["volume"].isNull()) {
            vol = doc["volume"].as<int>();
        } else if (server_->hasArg("volume")) {
            vol = server_->arg("volume").toInt();
        }
        if (vol < 0) vol = 0;
        if (vol > 21) vol = 21;

        Preferences p;
        p.begin("gameboy", false);
        p.putInt("volume", vol);
        p.end();

        JsonDocument out;
        out["ok"] = true;
        out["volume"] = vol;
        String s;
        serializeJson(out, s);
        server_->send(200, "application/json", s);
    });

    server_->on("/api/config/clock", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!parse_request_json(server_, doc)) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        Preferences p;
        p.begin("clock", false);
        if (!doc["tz"].isNull()) {
            String tz = doc["tz"].as<String>();
            tz.trim();
            if (!tz.isEmpty()) p.putString("tz", tz);
        }
        if (!doc["fmt24"].isNull()) p.putBool("fmt24", doc["fmt24"].as<bool>());
        if (!doc["show_sec"].isNull()) p.putBool("show_sec", doc["show_sec"].as<bool>());
        if (!doc["ntp_hrs"].isNull()) p.putInt("ntp_hrs", constrain(doc["ntp_hrs"].as<int>(), 1, 168));
        if (!doc["view"].isNull()) p.putInt("view", constrain(doc["view"].as<int>(), 0, 2));
        if (!doc["ev_date"].isNull()) {
            String d = doc["ev_date"].as<String>();
            d.trim();
            p.putString("ev_date", d);
        }
        if (!doc["ev_text"].isNull()) {
            String t = doc["ev_text"].as<String>();
            t.trim();
            p.putString("ev_text", t);
        }
        p.end();
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->on("/api/config/miner", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!parse_request_json(server_, doc)) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        Preferences p;
        p.begin("miner", false);
        if (!doc["btc_addr"].isNull()) {
            String a = doc["btc_addr"].as<String>();
            a.trim();
            if (!a.isEmpty()) p.putString("btc_addr", a);
        }
        if (!doc["worker"].isNull()) {
            String w = doc["worker"].as<String>();
            w.trim();
            if (!w.isEmpty()) p.putString("worker", w);
        }
        if (!doc["pool_host"].isNull()) {
            String h = doc["pool_host"].as<String>();
            h.trim();
            if (!h.isEmpty()) p.putString("pool_host", h);
        }
        if (!doc["pool_port"].isNull()) {
            int port = constrain(doc["pool_port"].as<int>(), 1, 65535);
            p.putInt("pool_port", port);
        }
        p.end();
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->on("/api/config/pcmon", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!parse_request_json(server_, doc)) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        Preferences p;
        p.begin("pcmon", false);
        if (!doc["view"].isNull()) p.putInt("view", constrain(doc["view"].as<int>(), 0, 1));
        if (!doc["host"].isNull()) {
            String h = doc["host"].as<String>(); h.trim();
            if (!h.isEmpty()) p.putString("host", h);
        }
        if (!doc["pet_name"].isNull()) {
            String pn = doc["pet_name"].as<String>(); pn.trim();
            if (!pn.isEmpty()) p.putString("pet_name", pn);
        }
        p.end();
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->on("/api/config/trmnl", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!parse_request_json(server_, doc)) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        Preferences p;
        p.begin("trmnl", false);
        if (!doc["mode"].isNull()) p.putInt("mode", constrain(doc["mode"].as<int>(), 0, 2));
        if (!doc["srv_url"].isNull()) { String u = doc["srv_url"].as<String>(); u.trim(); p.putString("srv_url", u); }
        if (!doc["api_key"].isNull()) { String k = doc["api_key"].as<String>(); k.trim(); p.putString("api_key", k); }
        if (!doc["refresh_s"].isNull()) p.putInt("refresh_s", constrain(doc["refresh_s"].as<int>(), 15, 3600));
        if (!doc["note_hdr"].isNull()) p.putString("note_hdr", doc["note_hdr"].as<String>());
        if (!doc["note_body"].isNull()) p.putString("note_body", doc["note_body"].as<String>());
        if (!doc["invert"].isNull()) p.putBool("invert", doc["invert"].as<bool>());
        p.end();
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->on("/api/config/flight", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!parse_request_json(server_, doc)) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        Preferences p;
        p.begin("flight", false);
        if (!doc["city"].isNull()) {
            String c = doc["city"].as<String>(); c.trim();
            if (!c.isEmpty()) p.putString("city", c);
        }
        if (!doc["lat"].isNull()) p.putFloat("lat", doc["lat"].as<float>());
        if (!doc["lon"].isNull()) p.putFloat("lon", doc["lon"].as<float>());
        if (!doc["radius_nm"].isNull()) p.putInt("radius_nm", constrain(doc["radius_nm"].as<int>(), 5, 150));
        if (!doc["poll_s"].isNull()) p.putInt("poll_s", constrain(doc["poll_s"].as<int>(), 15, 300));
        if (!doc["view"].isNull()) p.putInt("view", constrain(doc["view"].as<int>(), 0, 2));
        p.end();
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->on("/api/config/transit", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!parse_request_json(server_, doc)) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        Preferences p;
        p.begin("transit", false);
        if (!doc["city"].isNull()) { String c = doc["city"].as<String>(); c.trim(); if (!c.isEmpty()) p.putString("city", c); }
        if (!doc["url"].isNull()) {
            String u = doc["url"].as<String>(); u.trim();
            while (u.endsWith("/")) u.remove(u.length() - 1);
            if (!u.isEmpty()) p.putString("url", u);
        }
        if (!doc["s0_id"].isNull()) p.putInt("s0_id", doc["s0_id"].as<int>());
        if (!doc["s0_lb"].isNull()) p.putString("s0_lb", doc["s0_lb"].as<String>());
        if (!doc["s1_id"].isNull()) p.putInt("s1_id", doc["s1_id"].as<int>());
        if (!doc["s1_lb"].isNull()) p.putString("s1_lb", doc["s1_lb"].as<String>());
        if (!doc["view"].isNull()) p.putInt("view", constrain(doc["view"].as<int>(), 0, 1));
        p.end();
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->on("/api/config/markets", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!parse_request_json(server_, doc)) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        Preferences p;
        p.begin("markets", false);
        if (!doc["view"].isNull()) p.putInt("view", constrain(doc["view"].as<int>(), 0, 2));
        if (!doc["poll_s"].isNull()) p.putInt("poll_s", constrain(doc["poll_s"].as<int>(), 30, 900));
        if (!doc["c4_cd"].isNull()) p.putString("c4_cd", doc["c4_cd"].as<String>());
        if (!doc["c4_sy"].isNull()) p.putString("c4_sy", doc["c4_sy"].as<String>());
        if (!doc["c5_cd"].isNull()) p.putString("c5_cd", doc["c5_cd"].as<String>());
        if (!doc["c5_sy"].isNull()) p.putString("c5_sy", doc["c5_sy"].as<String>());
        p.end();
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    auto send_fw_result = [this]() {
        if (!upload_ok_ || upload_clean_name_.isEmpty()) {
            server_->send(500, "application/json", "{\"ok\":false,\"error\":\"Firmware upload failed\"}");
            return;
        }
        size_t sz = 0;
        File f = SD_MMC.open(upload_dst_, FILE_READ);
        if (f) { sz = f.size(); f.close(); }
        JsonDocument doc;
        doc["ok"] = true;
        doc["filename"] = upload_clean_name_;
        doc["size_bytes"] = (unsigned long)sz;
        String out;
        serializeJson(doc, out);
        server_->send(200, "application/json", out);
    };

    server_->on("/upload_fw", HTTP_POST, send_fw_result, [this]() { handle_file_upload(false); });
    server_->on("/upload", HTTP_POST, send_fw_result, [this]() { handle_file_upload(false); });

    server_->on("/upload_rom", HTTP_POST, [this]() {
        if (!upload_ok_ || upload_clean_name_.isEmpty()) {
            server_->send(500, "application/json", "{\"ok\":false,\"error\":\"ROM upload failed\"}");
            return;
        }
        File f = SD_MMC.open(upload_dst_, FILE_READ);
        size_t sz = f ? f.size() : 0;
        uint8_t *hdr = (uint8_t*)heap_caps_malloc(0x150, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        size_t got = 0;
        if (f && hdr && sz >= 0x150) got = f.read(hdr, 0x150);
        if (f) f.close();
        RomInfo info = SDManager::validate_rom_header(upload_clean_name_, sz, hdr, got);
        if (hdr) heap_caps_free(hdr);

        JsonDocument doc;
        doc["ok"] = true;
        doc["filename"] = info.filename;
        doc["internal_title"] = info.internal_title;
        doc["mbc_type"] = info.mbc_type;
        doc["cgb_flag"] = info.cgb_flag;
        doc["supported"] = info.supported;
        doc["reason"] = info.reason;
        doc["size_bytes"] = (unsigned long)info.size_bytes;
        String out;
        serializeJson(doc, out);
        server_->send(200, "application/json", out);
    }, [this]() { handle_file_upload(true); });

    server_->on("/api/delete", HTTP_POST, [this]() {
        JsonDocument doc;
        String type, fname;
        if (parse_request_json(server_, doc)) {
            type = doc["type"] | "";
            fname = doc["file"] | "";
        } else {
            type = server_->arg("type");
            fname = server_->arg("file");
        }
        type.trim();
        fname.trim();
        if (!is_safe_basename(fname) || (type != "fw" && type != "rom")) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid delete parameters\"}");
            return;
        }
        String path = String(type == "rom" ? "/roms/" : "/firmware/") + fname;
        if (!SD_MMC.exists(path)) {
            server_->send(404, "application/json", "{\"ok\":false,\"error\":\"File not found\"}");
            return;
        }
        if (!SD_MMC.remove(path)) {
            server_->send(500, "application/json", "{\"ok\":false,\"error\":\"Failed to remove file\"}");
            return;
        }
        menu_dirty_ = true;
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    auto handle_boot = [this]() {
        JsonDocument doc;
        String fname;
        if (parse_request_json(server_, doc)) {
            fname = doc["file"] | "";
        } else {
            fname = server_->arg("file");
        }
        fname.trim();
        if (!is_safe_basename(fname) || !SD_MMC.exists("/firmware/" + fname)) {
            server_->send(404, "application/json", "{\"ok\":false,\"error\":\"Firmware not found\"}");
            return;
        }
        server_->send(200, "application/json", "{\"ok\":true,\"booting\":\"" + fname + "\"}");
        pending_boot_file_ = fname;
        pending_action_at_ = millis() + 250;
    };
    server_->on("/boot", HTTP_POST, handle_boot);
    server_->on("/boot", HTTP_GET, handle_boot);

    server_->on("/api/firmware/order", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!parse_request_json(server_, doc)) {
            server_->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        if (doc["order"].is<JsonArray>()) {
            std::vector<String> ord;
            for (JsonVariant v : doc["order"].as<JsonArray>()) {
                String fn = v | "";
                fn.trim();
                if (!fn.isEmpty() && is_safe_basename(fn)) ord.push_back(fn);
            }
            SDManager::save_firmware_order(ord);
        }
        if (!doc["autoboot"].isNull() || !doc["autoboot_sec"].isNull()) {
            Preferences p;
            p.begin("launcher", false);
            if (!doc["autoboot"].isNull()) p.putBool("autoboot", doc["autoboot"].as<bool>());
            if (!doc["autoboot_sec"].isNull()) p.putInt("ab_sec", constrain(doc["autoboot_sec"].as<int>(), 2, 10));
            p.end();
        }
        menu_dirty_ = true;
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->on("/api/reboot", HTTP_POST, [this]() {
        server_->send(200, "application/json", "{\"ok\":true}");
        pending_reboot_ = true;
        pending_action_at_ = millis() + 300;
    });
}

extern void draw_boot_splash(int pct, const char *custom_banner = nullptr, const char *bottom_hint = nullptr);

void LauncherWebServer::handle() {
    if (server_) server_->handleClient();

    if (pending_reboot_ && (int32_t)(millis() - pending_action_at_) >= 0) {
        pending_reboot_ = false;
        Serial.println("[SYS] Rebooting MultiBoot Launcher...");
        ESP.restart();
    }

    if (!pending_boot_file_.isEmpty() && (int32_t)(millis() - pending_action_at_) >= 0) {
        String target = pending_boot_file_;
        pending_boot_file_ = "";

        String mod_title = target;
        for (const auto &fw : SDCard.get_firmwares()) {
            if (fw.filename == target) {
                mod_title = fw.title;
                break;
            }
        }

        if (Flasher.is_already_flashed(target)) {
            Serial.printf("[WEB-BOOT] Fast boot -> %s (%s)\n", target.c_str(), mod_title.c_str());
            draw_boot_splash(100, mod_title.c_str(), "STARTING MODULE...");
            Flasher.boot_target(target);
        } else {
            Serial.printf("[WEB-BOOT] Flashing & booting -> %s (%s)\n", target.c_str(), mod_title.c_str());
            int last_pct = -10;
            bool ok = Flasher.flash_and_boot(target, [&](size_t w, size_t tot, int pct) {
                if (pct - last_pct >= 5 || pct == 100) {
                    last_pct = pct;
                    char msg[40];
                    snprintf(msg, sizeof(msg), "FLASHING TO MEMORY: %d%%", pct);
                    draw_boot_splash(pct, mod_title.c_str(), msg);
                }
            });
            if (!ok) {
                Display.clear();
                Display.draw_string(20, 130, "FLASH FAILED! Check Serial log.", true, u8g2_font_helvB12_tr);
                Display.flush();
            }
        }
    }
}
