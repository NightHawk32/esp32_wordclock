#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// Network identity
#define WIFI_HOSTNAME "wordclock"
#define WIFI_AP_SSID  "WordClock-Setup"
#define WIFI_AP_PASS  "wordclock123"

// Connection state machine
enum WifiState {
  WIFI_ST_IDLE,        // no credentials stored
  WIFI_ST_CONNECTING,  // association / DHCP in progress
  WIFI_ST_CONNECTED,   // station has an IP
  WIFI_ST_RETRY_WAIT   // waiting out the backoff before the next attempt
};

// Lifecycle -----------------------------------------------------------------
// Starts the (non-blocking) connection attempt. Returns immediately.
void wifiInit();
// Drives the state machine + captive portal DNS. Call from loop().
void wifiLoop();

// Status --------------------------------------------------------------------
bool isWifiConnected();
bool isAPMode();              // soft-AP currently active
bool wifiEverConnected();     // station connected at least once since boot
WifiState getWifiState();
const char* getWifiStateName();

String getWifiIP();           // station IP, or AP IP while in AP-only mode
String getAPIP();
String getWifiSSID();
int    getWifiRSSI();
int    getWifiQuality();      // RSSI mapped to 0..100 %
uint32_t getWifiDisconnectCount();
uint8_t  getLastDisconnectReason();
uint32_t getWifiConnectedSeconds();

// Control -------------------------------------------------------------------
void wifiApplyCredentials(const char* ssid, const char* password); // save + reconnect
void wifiForceReconnect();
void startAPMode();

// Async scan ----------------------------------------------------------------
void   wifiStartScan();
int    wifiScanStatus();      // -1 = running, -2 = failed/idle, >= 0 = network count
String wifiScanResultsJson();

// Legacy helper kept for serial debugging
void scanNetworks();

#endif // WIFI_MANAGER_H
