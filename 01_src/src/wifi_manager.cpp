#include "wifi_manager.h"
#include <WiFi.h>

static bool apMode = false;

void scanNetworks() {
  Serial.println("Scan start");

  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
      Serial.print(n);
      Serial.println(" networks found");
      Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
      for (int i = 0; i < n; ++i) {
          Serial.printf("%2d",i + 1);
          Serial.print(" | ");
          Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
          Serial.print(" | ");
          Serial.printf("%4ld", WiFi.RSSI(i));
          Serial.print(" | ");
          Serial.printf("%2ld", WiFi.channel(i));
          Serial.print(" | ");
          switch (WiFi.encryptionType(i))
          {
          case WIFI_AUTH_OPEN:
              Serial.print("open");
              break;
          case WIFI_AUTH_WEP:
              Serial.print("WEP");
              break;
          case WIFI_AUTH_WPA_PSK:
              Serial.print("WPA");
              break;
          case WIFI_AUTH_WPA2_PSK:
              Serial.print("WPA2");
              break;
          case WIFI_AUTH_WPA_WPA2_PSK:
              Serial.print("WPA+WPA2");
              break;
          case WIFI_AUTH_WPA2_ENTERPRISE:
              Serial.print("WPA2-EAP");
              break;
          case WIFI_AUTH_WPA3_PSK:
              Serial.print("WPA3");
              break;
          case WIFI_AUTH_WPA2_WPA3_PSK:
              Serial.print("WPA2+WPA3");
              break;
          case WIFI_AUTH_WAPI_PSK:
              Serial.print("WAPI");
              break;
          default:
              Serial.print("unknown");
          }
          Serial.println();
          delay(10);
      }
  }
  Serial.println("");
  WiFi.scanDelete();
}

bool setup_wifi(const char* ssid, const char* password) {
  delay(10);
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 40) {
    delay(500);
    Serial.print(".");
    counter++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi connected: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    apMode = false;
    return true;
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi");
    return false;
  }
}

void startAPMode() {
  Serial.println("Starting Access Point mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("WordClock-Setup", "wordclock123");
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.println("Connect to 'WordClock-Setup' with password 'wordclock123'");
  
  apMode = true;
}

bool isAPMode() {
  return apMode;
}

String getAPIP() {
  if (apMode) {
    return WiFi.softAPIP().toString();
  }
  return WiFi.localIP().toString();
}
