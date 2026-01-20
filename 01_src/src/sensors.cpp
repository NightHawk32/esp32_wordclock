#include "sensors.h"
#include "board.h"

Tsl2561 Tsl(Wire);
Adafruit_BME680 bme;

// Cached sensor values
static float cachedTemperature = 0;
static float cachedHumidity = 0;
static float cachedPressure = 0;
static uint32_t cachedGasResistance = 0;

static char *format( const char *fmt, ... ) {
  static char buf[128];
  va_list arg;
  va_start(arg, fmt);
  vsnprintf(buf, sizeof(buf), fmt, arg);
  buf[sizeof(buf)-1] = '\0';
  va_end(arg);
  return buf;
}

uint32_t printTSL(){
  bool found = false;
  uint32_t milliLux = 0;

  if( Tsl.begin(TSL2561_ADDR) ) {
    found = true;
    Serial.println();

    uint16_t scaledFull = 0, scaledIr;
    uint32_t full, ir;
    uint8_t id;
    bool gain = 1;
    Tsl2561::exposure_t exposure = (Tsl2561::exposure_t)2;
    Tsl.on();

    Tsl.setSensitivity(gain, exposure);
    Tsl2561Util::waitNext(exposure);
    Tsl.id(id);
    Tsl.getSensitivity(gain, exposure);
    Tsl.fullLuminosity(scaledFull);
    Tsl.irLuminosity(scaledIr);

    Serial.print(format("Tsl2561 addr: 0x%02x, id: 0x%02x, sfull: %5u, sir: %5u, gain: %d, exp: %d",
      TSL2561_ADDR, id, scaledFull, scaledIr, gain, exposure));

    if( Tsl2561Util::normalizedLuminosity(gain, exposure, full = scaledFull, ir = scaledIr) ) {
      if( Tsl2561Util::milliLux(full, ir, milliLux, Tsl2561::packageCS(id)) ) {
        Serial.print(format(", full: %5lu, ir: %5lu, lux: %5lu.%03lu\n", (unsigned long)full, (unsigned long)ir, (unsigned long)milliLux/1000, (unsigned long)milliLux%1000));
      }
    }

    Tsl.off();
  }
  return milliLux;
}

void initBME688() {
  Serial.println("Initializing BME688 sensor...");
  
  if (!bme.begin(0x76)) {  // Try alternate address 0x76 (default is 0x77)
    Serial.println("Could not find a valid BME688 sensor, check wiring!");
    return;
  }
  
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms
  
  Serial.println("BME688 sensor initialized successfully!");
}

void printBME688() {
  if (!bme.performReading()) {
    Serial.println("Failed to perform BME688 reading");
    return;
  }
  
  // Cache values
  cachedTemperature = bme.temperature;
  cachedHumidity = bme.humidity;
  cachedPressure = bme.pressure / 100.0;
  cachedGasResistance = bme.gas_resistance;
  
  Serial.println("\n=== BME688 Sensor Data ===");
  Serial.print("Temperature: ");
  Serial.print(cachedTemperature);
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.print(cachedHumidity);
  Serial.println(" %");
  
  Serial.print("Pressure: ");
  Serial.print(cachedPressure);
  Serial.println(" hPa");
  
  Serial.print("Gas Resistance: ");
  Serial.print(cachedGasResistance / 1000.0);
  Serial.println(" KOhms");
  
  Serial.print("Altitude: ");
  Serial.print(bme.readAltitude(1013.25)); // Sea level pressure
  Serial.println(" m");
  Serial.println("========================\n");
}

float getTemperature() {
  return cachedTemperature;
}

float getHumidity() {
  return cachedHumidity;
}

float getPressure() {
  return cachedPressure;
}

uint32_t getGasResistance() {
  return cachedGasResistance;
}
