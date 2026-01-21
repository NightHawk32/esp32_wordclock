#include "ota_manager.h"
#include <ArduinoOTA.h>
#include <WiFi.h>

void initOTA() {
  // Configure ArduinoOTA with increased port and timeout
  ArduinoOTA.setHostname("wordclock");
  ArduinoOTA.setPassword("admin");  // Set OTA password for security
  ArduinoOTA.setPort(3232);  // Default OTA port
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
    Serial.println("Stopping other tasks...");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update Complete!");
    Serial.println("Rebooting...");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  
  ArduinoOTA.begin();
  Serial.println("ArduinoOTA initialized");
  Serial.println("OTA Password: admin");
  Serial.print("OTA Hostname: ");
  Serial.println(ArduinoOTA.getHostname());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void handleOTA() {
  ArduinoOTA.handle();
}
