#include "wifi_manager.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <esp_wifi.h>

#include "display.h"
#include "settings.h"
#include "time_utils.h"

// --- Tuning -----------------------------------------------------------------
// How long a single association+DHCP attempt may take before we give up on it.
static const uint32_t CONNECT_TIMEOUT_MS = 15000;
// Exponential backoff between attempts, so a rebooting router is not hammered.
static const uint32_t BACKOFF_MIN_MS = 2000;
static const uint32_t BACKOFF_MAX_MS = 60000;
// Failed attempts before the setup AP is opened as a fallback.
static const uint8_t  FAILS_BEFORE_AP = 3;
// Keep the setup AP alive this long after the station reconnects, so a client
// that is currently on the config page is not kicked off mid-edit.
static const uint32_t AP_LINGER_MS = 120000;
// Last resort: if a device that has been online before cannot get back on for
// this long, the WiFi stack itself is most likely wedged. A reboot is cheap -
// the clock re-syncs from NTP within seconds of reconnecting.
static const uint32_t RECOVERY_REBOOT_MS = 20UL * 60UL * 1000UL;
static const byte DNS_PORT = 53;

// --- State ------------------------------------------------------------------
static WifiState state = WIFI_ST_IDLE;
static bool apActive = false;
static bool everConnected = false;
static bool ntpPending = false;
static bool mdnsPending = false;

static uint32_t attemptStartedMs = 0;
static uint32_t retryAtMs = 0;
static uint32_t backoffMs = BACKOFF_MIN_MS;
static uint32_t connectedSinceMs = 0;
static uint32_t lastOnlineMs = 0;
static uint32_t apLingerUntilMs = 0;
static uint8_t  consecutiveFails = 0;
static uint32_t disconnectCount = 0;
static uint8_t  lastDisconnectReason = 0;

static WiFiSettings creds;
static DNSServer dnsServer;
static bool dnsActive = false;

// Set by the WiFi event task, consumed by wifiLoop(). Event callbacks run in a
// different task, so they only flip flags - never touch the display or NVS.
static volatile bool evGotIp = false;
static volatile bool evDisconnected = false;

// Deadline helper that is immune to the 49-day millis() rollover.
static inline bool timeReached(uint32_t deadline) {
  return (int32_t)(millis() - deadline) >= 0;
}

// --- Events -----------------------------------------------------------------
static void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      evGotIp = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      lastDisconnectReason = info.wifi_sta_disconnected.reason;
      evDisconnected = true;
      break;
    default:
      break;
  }
}

// --- Radio configuration ----------------------------------------------------
static void applyRadioTuning() {
  // Modem sleep is the single biggest source of "the clock drops off the
  // network" on a mains-powered device: the AP ages out the association while
  // the radio naps. Keep it awake.
  WiFi.setSleep(false);
  // Do not rewrite credentials into the system NVS on every begin() - we keep
  // them ourselves in Preferences and this only wears the flash.
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  // The XIAO ESP32-C3 has a small antenna; run the radio at full power.
  esp_wifi_set_max_tx_power(78); // 78 * 0.25 dBm = 19.5 dBm
}

static void startAttempt() {
  Serial.printf("[wifi] connecting to %s (attempt %u)\n", creds.ssid, consecutiveFails + 1);

  // AP_STA while the fallback AP is up, plain STA otherwise.
  WiFi.mode(apActive ? WIFI_AP_STA : WIFI_STA);
  applyRadioTuning();
  // Must be set before begin() or the router shows a generic espressif name.
  WiFi.setHostname(WIFI_HOSTNAME);

  WiFi.disconnect(false, false);
  WiFi.begin(creds.ssid, creds.password);

  attemptStartedMs = millis();
  state = WIFI_ST_CONNECTING;
}

static void scheduleRetry() {
  retryAtMs = millis() + backoffMs;
  state = WIFI_ST_RETRY_WAIT;
  Serial.printf("[wifi] retry in %lu ms\n", (unsigned long)backoffMs);
  backoffMs = min(backoffMs * 2, BACKOFF_MAX_MS);
}

static void startMDNS() {
  MDNS.end();
  if (MDNS.begin(WIFI_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[wifi] mDNS responder started");
  } else {
    Serial.println("[wifi] mDNS start failed");
  }
}

static void stopAP() {
  if (!apActive) return;
  Serial.println("[wifi] stopping setup AP");
  if (dnsActive) {
    dnsServer.stop();
    dnsActive = false;
  }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  applyRadioTuning();
  apActive = false;
}

// --- Public API -------------------------------------------------------------
void startAPMode() {
  if (apActive) return;

  Serial.println("[wifi] starting setup AP");
  // AP_STA (not AP) so the station keeps retrying the real network in the
  // background - the clock then recovers on its own once the router is back.
  WiFi.mode(creds.configured ? WIFI_AP_STA : WIFI_AP);
  applyRadioTuning();
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

  IPAddress ip = WiFi.softAPIP();
  Serial.printf("[wifi] AP %s / %s at http://%s\n", WIFI_AP_SSID, WIFI_AP_PASS, ip.toString().c_str());

  // Captive portal: answer every lookup with our own IP so phones pop the
  // config page automatically.
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsActive = dnsServer.start(DNS_PORT, "*", ip);

  apActive = true;
  apLingerUntilMs = 0;
  startMDNS();
}

void wifiInit() {
  WiFi.onEvent(onWifiEvent);
  creds = loadWiFiSettings();

  if (creds.configured && creds.ssid[0] != '\0') {
    backoffMs = BACKOFF_MIN_MS;
    consecutiveFails = 0;
    startAttempt();
  } else {
    Serial.println("[wifi] no credentials stored - opening setup AP");
    state = WIFI_ST_IDLE;
    startAPMode();
  }
}

void wifiLoop() {
  // --- drain events --------------------------------------------------------
  if (evGotIp) {
    evGotIp = false;
    everConnected = true;
    state = WIFI_ST_CONNECTED;
    connectedSinceMs = millis();
    lastOnlineMs = connectedSinceMs;
    consecutiveFails = 0;
    backoffMs = BACKOFF_MIN_MS;
    ntpPending = true;
    mdnsPending = true;
    // Give a client sitting on the config page time to finish before closing.
    if (apActive && apLingerUntilMs == 0) apLingerUntilMs = millis() + AP_LINGER_MS;
    Serial.printf("[wifi] connected, IP %s, RSSI %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    restoreBrightness();
    forceDisplayRefresh();
  }

  if (evDisconnected) {
    evDisconnected = false;
    if (state == WIFI_ST_CONNECTED) {
      disconnectCount++;
      Serial.printf("[wifi] link lost (reason %u), reconnecting\n", lastDisconnectReason);
      // A dropped link is not a failed attempt - retry immediately the first time.
      backoffMs = BACKOFF_MIN_MS;
      consecutiveFails = 0;
      state = WIFI_ST_RETRY_WAIT;
      retryAtMs = millis();
    }
  }

  // --- deferred work that must not run in the event task -------------------
  if (mdnsPending) {
    mdnsPending = false;
    startMDNS();
  }
  if (ntpPending) {
    ntpPending = false;
    TimezoneSettings tz = loadTimezoneSettings();
    initTime(tz.timezone);
  }

  // --- state machine -------------------------------------------------------
  switch (state) {
    case WIFI_ST_CONNECTING:
      // Pulse the matrix while we wait, but only on the very first connect -
      // after that the clock keeps showing the time instead of an animation.
      if (!everConnected && !apActive) showConnectingAnimation();

      if (timeReached(attemptStartedMs + CONNECT_TIMEOUT_MS)) {
        consecutiveFails++;
        Serial.printf("[wifi] attempt timed out (%u consecutive)\n", consecutiveFails);
        WiFi.disconnect(false, false);
        if (consecutiveFails >= FAILS_BEFORE_AP && !apActive) {
          startAPMode();
          restoreBrightness();
          forceDisplayRefresh();
        }
        scheduleRetry();
      }
      break;

    case WIFI_ST_RETRY_WAIT:
      if (creds.configured && timeReached(retryAtMs)) startAttempt();
      break;

    case WIFI_ST_CONNECTED:
      lastOnlineMs = millis();
      // Belt and braces: the disconnect event can be missed if the stack is busy.
      if (WiFi.status() != WL_CONNECTED) {
        disconnectCount++;
        Serial.println("[wifi] status poll says disconnected, reconnecting");
        backoffMs = BACKOFF_MIN_MS;
        state = WIFI_ST_RETRY_WAIT;
        retryAtMs = millis();
      } else if (apActive && apLingerUntilMs != 0 && timeReached(apLingerUntilMs)) {
        stopAP();
      }
      break;

    case WIFI_ST_IDLE:
    default:
      break;
  }

  // --- last-resort recovery ------------------------------------------------
  if (everConnected && state != WIFI_ST_CONNECTED &&
      timeReached(lastOnlineMs + RECOVERY_REBOOT_MS)) {
    Serial.println("[wifi] offline far too long, rebooting to clear the stack");
    delay(100);
    ESP.restart();
  }

  // --- captive portal ------------------------------------------------------
  if (dnsActive) dnsServer.processNextRequest();
}

bool isWifiConnected()   { return state == WIFI_ST_CONNECTED && WiFi.status() == WL_CONNECTED; }
bool isAPMode()          { return apActive; }
bool wifiEverConnected() { return everConnected; }
WifiState getWifiState() { return state; }
uint32_t getWifiDisconnectCount()  { return disconnectCount; }
uint8_t  getLastDisconnectReason() { return lastDisconnectReason; }

const char* getWifiStateName() {
  switch (state) {
    case WIFI_ST_CONNECTING: return "connecting";
    case WIFI_ST_CONNECTED:  return "connected";
    case WIFI_ST_RETRY_WAIT: return "reconnecting";
    default:                 return apActive ? "setup" : "idle";
  }
}

String getWifiIP() {
  if (isWifiConnected()) return WiFi.localIP().toString();
  if (apActive) return WiFi.softAPIP().toString();
  return String("0.0.0.0");
}

String getAPIP() { return apActive ? WiFi.softAPIP().toString() : getWifiIP(); }

String getWifiSSID() {
  if (isWifiConnected()) return WiFi.SSID();
  return String(creds.configured ? creds.ssid : "");
}

int getWifiRSSI() { return isWifiConnected() ? WiFi.RSSI() : 0; }

int getWifiQuality() {
  if (!isWifiConnected()) return 0;
  int rssi = WiFi.RSSI();
  if (rssi <= -100) return 0;
  if (rssi >= -50) return 100;
  return 2 * (rssi + 100);
}

uint32_t getWifiConnectedSeconds() {
  if (state != WIFI_ST_CONNECTED) return 0;
  return (millis() - connectedSinceMs) / 1000;
}

void wifiForceReconnect() {
  Serial.println("[wifi] manual reconnect requested");
  creds = loadWiFiSettings();
  backoffMs = BACKOFF_MIN_MS;
  consecutiveFails = 0;
  if (creds.configured) {
    startAttempt();
  } else {
    // Credentials were erased - drop the station so the setup AP is the only
    // way in, instead of silently staying on the old network until a reboot.
    WiFi.disconnect(false, true);
    state = WIFI_ST_IDLE;
    startAPMode();
  }
}

void wifiApplyCredentials(const char* ssid, const char* password) {
  saveWiFiSettings(ssid, password);
  wifiForceReconnect();
}

// --- Async scan -------------------------------------------------------------
void wifiStartScan() {
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
  WiFi.scanDelete();
  WiFi.scanNetworks(true, false); // async, hidden networks excluded
}

int wifiScanStatus() { return WiFi.scanComplete(); }

String wifiScanResultsJson() {
  int n = WiFi.scanComplete();
  String out = "[";
  if (n > 0) {
    // Cap the list: the page only needs the strongest handful and a long JSON
    // string is an unwelcome heap spike on a C3.
    if (n > 20) n = 20;
    for (int i = 0; i < n; i++) {
      if (i) out += ',';
      String ssid = WiFi.SSID(i);
      ssid.replace("\\", "\\\\");
      ssid.replace("\"", "\\\"");
      out += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + WiFi.RSSI(i) +
             ",\"open\":" + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
  }
  out += ']';
  return out;
}

void scanNetworks() {
  Serial.println("[wifi] blocking scan");
  int n = WiFi.scanNetworks();
  Serial.printf("[wifi] %d networks found\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("%2d | %-32.32s | %4d dBm | ch %2d\n", i + 1,
                  WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
  }
  WiFi.scanDelete();
}
