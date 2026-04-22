#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "secrets.h"
const int   REFRESH_MS     = 30000;
const int   SLIDE_MS       = 5000;

String sessionSid = "";

// --- Display (direct draw, no sprite — heap too fragmented for 150KB buffer) ---
TFT_eSPI tft = TFT_eSPI();

#define SCR_W 320
#define SCR_H 240

// --- Colors ---
#define BG        0x0820
#define ACCENT    0x07FF
#define GREEN     0x07E0
#define RED       0xF800
#define ORANGE    0xFD20
#define WHITE     0xFFFF
#define GRAY      0x8410
#define DARKGRAY  0x2104

// --- Data ---
struct PiholeStats {
  long   dns_queries_today;
  long   ads_blocked_today;
  float  ads_percentage;
  long   domains_blocked;
  long   queries_cached;
  long   clients_ever_seen;
  String status;
  bool   valid;
};

PiholeStats stats;
int   currentSlide  = 0;
const int SLIDE_COUNT = 3;
unsigned long lastRefresh = 0;
unsigned long lastSlide   = 0;

// --- Authenticate with Pi-hole v6 API ---
bool authenticate() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.setTimeout(5000);
  http.begin("http://" + String(PIHOLE_IP) + "/api/auth");
  http.addHeader("Content-Type", "application/json");

  String body = "{\"password\":\"" + String(PIHOLE_PASS) + "\"}";
  int code = http.POST(body);

  if (code == 200) {
    JsonDocument doc;
    deserializeJson(doc, http.getString());
    sessionSid = doc["session"]["sid"].as<String>();
    http.end();
    return sessionSid.length() > 0;
  }
  http.end();
  return false;
}

// --- Fetch Pi-hole stats (v6 API) ---
void fetchStats() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (sessionSid.length() == 0) {
    if (!authenticate()) return;
  }

  HTTPClient http;
  http.setTimeout(5000);
  http.begin("http://" + String(PIHOLE_IP) + "/api/stats/summary?sid=" + sessionSid);
  int code = http.GET();

  if (code == 401) {
    sessionSid = "";
    if (authenticate()) {
      http.end();
      http.begin("http://" + String(PIHOLE_IP) + "/api/stats/summary?sid=" + sessionSid);
      code = http.GET();
    }
  }

  if (code == 200) {
    JsonDocument doc;
    deserializeJson(doc, http.getString());
    stats.dns_queries_today = doc["queries"]["total"].as<long>();
    stats.ads_blocked_today = doc["queries"]["blocked"].as<long>();
    stats.ads_percentage    = doc["queries"]["percent_blocked"].as<float>();
    stats.domains_blocked   = doc["gravity"]["domains_being_blocked"].as<long>();
    stats.queries_cached    = doc["queries"]["cached"].as<long>();
    stats.clients_ever_seen = doc["clients"]["total"].as<long>();
    stats.status            = doc["blocking"].as<String>();
    stats.valid             = true;
  }
  http.end();
}

// --- Helpers ---
void drawHeader(const char* title, uint16_t accentColor) {
  tft.fillRect(0, 0, SCR_W, 36, accentColor);
  tft.setTextColor(BG, accentColor);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString(title, SCR_W / 2, 18);

  int dotX = (SCR_W / 2) - (SLIDE_COUNT * 14) / 2;
  for (int i = 0; i < SLIDE_COUNT; i++) {
    uint16_t col = (i == currentSlide) ? WHITE : DARKGRAY;
    tft.fillCircle(dotX + i * 14, SCR_H - 8, 4, col);
  }
}

void drawBigStat(const char* label, String value, uint16_t valueColor, int y) {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(GRAY, BG);
  tft.setTextSize(1);
  tft.drawString(label, SCR_W / 2, y);
  tft.setTextColor(valueColor, BG);
  tft.setTextSize(3);
  tft.drawString(value.c_str(), SCR_W / 2, y + 22);
}

void drawBarStat(const char* label, long value, long total, uint16_t barColor, int y) {
  tft.setTextColor(GRAY, BG);
  tft.setTextSize(1);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(label, 10, y);
  tft.setTextColor(WHITE, BG);
  tft.setTextDatum(MR_DATUM);
  tft.drawString(String(value).c_str(), SCR_W - 10, y);

  int barW = total > 0 ? (int)((SCR_W - 20.0) * value / total) : 0;
  tft.fillRoundRect(10, y + 12, SCR_W - 20, 10, 5, DARKGRAY);
  tft.fillRoundRect(10, y + 12, barW, 10, 5, barColor);
}

// --- Slides ---
void slideOverview() {
  drawHeader("PI-HOLE DASHBOARD", stats.status != "disabled" ? GRAY : RED);

  String pct = String(stats.ads_percentage, 1) + "%";
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(stats.ads_percentage > 10 ? GREEN : ORANGE, BG);
  tft.setTextSize(4);
  tft.drawString(pct.c_str(), SCR_W / 2, 85);
  tft.setTextColor(GRAY, BG);
  tft.setTextSize(1);
  tft.drawString("ADS BLOCKED TODAY", SCR_W / 2, 115);

  tft.drawFastHLine(20, 130, SCR_W - 40, DARKGRAY);

  drawBigStat("QUERIES TODAY", String(stats.dns_queries_today), WHITE, 148);
  drawBigStat("BLOCKED TODAY", String(stats.ads_blocked_today), GREEN, 193);
}

void slideBlocking() {
  drawHeader("BLOCKING STATS", GRAY);

  drawBarStat("QUERIES TODAY", stats.dns_queries_today, stats.dns_queries_today, ACCENT, 50);
  drawBarStat("BLOCKED",       stats.ads_blocked_today, stats.dns_queries_today, GREEN,  90);
  drawBarStat("CACHED",        stats.queries_cached,    stats.dns_queries_today, ORANGE, 130);
  drawBarStat("FORWARDED",
    stats.dns_queries_today - stats.ads_blocked_today - stats.queries_cached,
    stats.dns_queries_today, GRAY, 170);
}

void slideDomains() {
  drawHeader("BLOCKLIST", GRAY);
  drawBigStat("DOMAINS BLOCKED", String(stats.domains_blocked), RED, 80);
  tft.drawFastHLine(20, 122, SCR_W - 40, DARKGRAY);
  drawBigStat("CLIENTS SEEN",   String(stats.clients_ever_seen), ORANGE, 140);
  drawBigStat("CACHED QUERIES", String(stats.queries_cached),    ACCENT, 193);
}

void slideStatus() {
  bool enabled = stats.status == "enabled";
  drawHeader("STATUS", enabled ? GREEN : RED);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(GRAY, BG);
  tft.setTextSize(1);
  tft.drawString("PI-HOLE IS", SCR_W / 2, 65);

  tft.setTextColor(enabled ? GREEN : RED, BG);
  tft.setTextSize(3);
  tft.drawString(enabled ? "ENABLED" : "DISABLED", SCR_W / 2, 90);

  tft.drawFastHLine(20, 120, SCR_W - 40, DARKGRAY);

  tft.setTextColor(WHITE, BG);
  tft.setTextSize(1);
  tft.drawString(("IP: " + String(PIHOLE_IP)).c_str(), SCR_W / 2, 145);

  unsigned long secs = millis() / 1000;
  String uptime = "Uptime: " + String(secs / 3600) + "h " + String((secs % 3600) / 60) + "m";
  tft.drawString(uptime.c_str(), SCR_W / 2, 170);
}

void drawSlide() {
  tft.fillScreen(BG);

  if (!stats.valid) {
    tft.setTextColor(RED, BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("NO DATA", SCR_W / 2, SCR_H / 2);
  } else {
    switch (currentSlide) {
      case 0: slideOverview(); break;
      case 1: slideBlocking(); break;
      case 2: slideDomains();  break;
    }
  }
}

void connectWifi() {
  tft.fillScreen(BG);
  tft.setTextColor(WHITE, BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("Connecting...", SCR_W / 2, SCR_H / 2);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    tries++;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(BG);

  connectWifi();
  fetchStats();
  drawSlide();
}

void loop() {
  unsigned long now = millis();

  if (now - lastRefresh >= REFRESH_MS) {
    fetchStats();
    lastRefresh = now;
  }

  if (now - lastSlide >= SLIDE_MS) {
    currentSlide = (currentSlide + 1) % SLIDE_COUNT;
    drawSlide();
    lastSlide = now;
  }
}
