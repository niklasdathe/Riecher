// Riecher_SEN66_Zigbee.ino
// ESP32 Zigbee End Device publishing:
// EP1    : internal chip temperature (ESP32 sensor)
// EP200  : Temperature + Humidity (SEN66)
// EP201  : CO2
// EP202  : PM2.5
// EP203  : PM1.0 (Analog Input)
// EP204  : PM4.0 (Analog Input)
// EP205  : PM10  (Analog Input)
// EP206  : VOC Index (Analog Input)
// EP207  : NOx Index (Analog Input)
//
// Manufacturer/Model for Z2M discovery: DATHE / Riecher
// Switch real SEN66 vs. mock data via USE_SEN66_REAL

#ifndef ZIGBEE_MODE_ED
#error "Bitte in Arduino → Tools → Zigbee mode den End Device Modus auswählen (End Device)."
#endif

#include <Arduino.h>
#include <Wire.h>
#include "Zigbee.h"
#include <SensirionI2cSen66.h>

// ====== CONFIG ======
#define USE_SEN66_REAL 0  // 0: Mockdaten, 1: echten SEN66 lesen
#define I2C_ADDR_SEN66 SEN66_I2C_ADDR_6B
#define PUBLISH_INTERVAL_SEC 30  // Sensor data publish interval in seconds

static const char* MFG   = "DATHE";
static const char* MODEL = "Riecher";

#define EP_CHIP_TEMP 1
#define EP_TEMP_HUM  200
#define EP_CO2       201
#define EP_PM25      202
#define EP_PM1       203
#define EP_PM4       204
#define EP_PM10      205
#define EP_VOC       206
#define EP_NOX       207

static const uint8_t BUTTON_PIN = BOOT_PIN;

// ====== Zigbee Endpoints ======
ZigbeeTempSensor           zbChipTemp(EP_CHIP_TEMP);
ZigbeeTempSensor           zbTempHum(EP_TEMP_HUM);
ZigbeeCarbonDioxideSensor  zbCO2(EP_CO2);
ZigbeePM25Sensor           zbPM25(EP_PM25);
ZigbeeAnalog               zbPM1(EP_PM1);
ZigbeeAnalog               zbPM4(EP_PM4);
ZigbeeAnalog               zbPM10(EP_PM10);
ZigbeeAnalog               zbVOC(EP_VOC);
ZigbeeAnalog               zbNOx(EP_NOX);

// ====== SEN66 ======
SensirionI2cSen66 sen66;
static int16_t sen_err = 0;
static char errStr[64];

// ====== Values ======
struct Reading {
  float tempC;
  float hum;
  uint16_t co2ppm;
  float pm1;
  float pm25;
  float pm4;
  float pm10;
  float vocIndex;
  float noxIndex;
};
static Reading reading;

// ====== Helpers ======
static void errorToSerial(int16_t err, const char* ctx) {
  errorToString(err, errStr, sizeof(errStr));
  Serial.print(ctx); Serial.print(": "); Serial.println(errStr);
}

static void readMock(Reading& r) {
  static uint32_t t = 0; t++;
  r.tempC    = 22.0f + 2.0f * sinf(t*0.05f);
  r.hum      = 45.0f + 10.0f * sinf(t*0.02f + 1.0f);
  r.co2ppm   = 550 + (int)(80.0f * (sinf(t*0.03f) + 1.0f));
  r.pm1      = 3.0f  + 1.0f  * fabsf(sinf(t*0.07f));
  r.pm25     = 6.0f  + 2.0f  * fabsf(sinf(t*0.06f));
  r.pm4      = 8.0f  + 2.5f  * fabsf(sinf(t*0.05f));
  r.pm10     = 12.0f + 3.5f  * fabsf(sinf(t*0.04f));
  r.vocIndex = 80.0f + 20.0f * fabsf(sinf(t*0.03f));
  r.noxIndex = 30.0f + 10.0f * fabsf(sinf(t*0.025f));
}

static bool readSEN66(Reading& r) {
  float pm1=0, pm25=0, pm4=0, pm10=0, rh=0, tc=0, voc=0, nox=0;
  uint16_t co2=0;
  delay(1000); // SEN6x ~1 s Messintervall
  int16_t err = sen66.readMeasuredValues(pm1, pm25, pm4, pm10, rh, tc, voc, nox, co2);
  if (err != 0) { errorToSerial(err, "SEN66 read"); return false; }
  r.tempC = tc; r.hum = rh; r.co2ppm = co2;
  r.pm1 = pm1; r.pm25 = pm25; r.pm4 = pm4; r.pm10 = pm10;
  r.vocIndex = voc; r.noxIndex = nox;
  return true;
}

static void publishToZigbee(const Reading& r) {
  // Set values
  zbTempHum.setTemperature(r.tempC);
  zbTempHum.setHumidity(r.hum);
  zbCO2.setCarbonDioxide((float)r.co2ppm);
  zbPM25.setPM25(r.pm25);
  
  zbPM1.setAnalogInput(r.pm1);
  zbPM4.setAnalogInput(r.pm4);
  zbPM10.setAnalogInput(r.pm10);
  zbVOC.setAnalogInput(r.vocIndex);
  zbNOx.setAnalogInput(r.noxIndex);

  zbTempHum.reportTemperature(); 
  zbTempHum.reportHumidity();  
  zbCO2.report();
  zbPM25.report();
  zbPM1.reportAnalogInput();
  zbPM4.reportAnalogInput();
  zbPM10.reportAnalogInput();
  zbVOC.reportAnalogInput();
  zbNOx.reportAnalogInput();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // I2C + SEN66 init
  Wire.begin();
#if USE_SEN66_REAL
  sen66.begin(Wire, I2C_ADDR_SEN66);
  sen_err = sen66.deviceReset(); if (sen_err) errorToSerial(sen_err, "SEN66 reset");
  delay(1200);
  sen_err = sen66.startContinuousMeasurement(); if (sen_err) errorToSerial(sen_err, "SEN66 start");
#endif

  // Manufacturer/Model auf allen EPs
  ZigbeeEP* eps[] = {
    &zbChipTemp, &zbTempHum, &zbCO2, &zbPM25,
    &zbPM1, &zbPM4, &zbPM10, &zbVOC, &zbNOx
  };
  for (auto ep : eps) ep->setManufacturerAndModel(MFG, MODEL);

  // EP1: interne Chip-Temperatur (nur Temperatur-Cluster, kein Humidity)
  zbChipTemp.setMinMaxValue(-20, 125);
  zbChipTemp.setTolerance(0.1f);
  // EP1 – optional Identify-Callback (blinkt nur im Log)
  zbChipTemp.onIdentify([](uint16_t seconds){
    Serial.printf("Identify for %u s on EP1\n", seconds);
  });
  // sicherstellen, dass Hersteller/Modell gesetzt sind (du setzt es bereits für alle EPs)
  zbChipTemp.setManufacturerAndModel(MFG, MODEL);


  // EP200: Ambient Temp/Hum
  zbTempHum.setMinMaxValue(-20, 80);
  zbTempHum.setTolerance(0.1f);
  zbTempHum.addHumiditySensor(0, 100, 0.5f);

  // EP201: CO2
  zbCO2.setMinMaxValue(0, 40000);
  zbCO2.setTolerance(10.0f);

  // EP202: PM2.5
  zbPM25.setMinMaxValue(0, 1000);
  zbPM25.setTolerance(0.1f);

  // Analog Inputs konfigurieren
  zbPM1.addAnalogInput();  zbPM1.setAnalogInputDescription("PM1.0 ug/m3");  zbPM1.setAnalogInputResolution(0.1f);
  zbPM4.addAnalogInput();  zbPM4.setAnalogInputDescription("PM4.0 ug/m3");  zbPM4.setAnalogInputResolution(0.1f);
  zbPM10.addAnalogInput(); zbPM10.setAnalogInputDescription("PM10 ug/m3");  zbPM10.setAnalogInputResolution(0.1f);
  zbVOC.addAnalogInput();  zbVOC.setAnalogInputDescription("VOC Index");    zbVOC.setAnalogInputResolution(1.0f); zbVOC.setAnalogInputMinMax(0, 500);
  zbNOx.addAnalogInput();  zbNOx.setAnalogInputDescription("NOx Index");    zbNOx.setAnalogInputResolution(1.0f); zbNOx.setAnalogInputMinMax(0, 500);

  // EPs registrieren
  Zigbee.addEndpoint(&zbChipTemp);
  Zigbee.addEndpoint(&zbTempHum);
  Zigbee.addEndpoint(&zbCO2);
  Zigbee.addEndpoint(&zbPM25);
  Zigbee.addEndpoint(&zbPM1);
  Zigbee.addEndpoint(&zbPM4);
  Zigbee.addEndpoint(&zbPM10);
  Zigbee.addEndpoint(&zbVOC);
  Zigbee.addEndpoint(&zbNOx);

  // Zigbee starten
  Serial.println("Starting Zigbee...");
  if (!Zigbee.begin()) { Serial.println("Zigbee failed, rebooting..."); delay(1000); ESP.restart(); }
  Serial.print("Connecting");
  while (!Zigbee.connected()) { Serial.print("."); delay(100); }
  Serial.println("\nConnected to Zigbee network.");

  // Reporting konfigurieren
  // EP1 (chip temperature)
  zbChipTemp.setReporting(0, 30, 0.1f);  // delta 0.1°C

  // EP200 (ambient)
  zbTempHum.setReporting(0, 30, 0.1f);
  zbTempHum.setHumidityReporting(0, 30, 0.5f);

  // EP201 CO2 / EP202 PM2.5
  zbCO2.setReporting(0, 30, 0);
  zbPM25.setReporting(0, 30, 0);

  // Analog Inputs
  zbPM1.setAnalogInputReporting (0, 30, 0.2f);
  zbPM4.setAnalogInputReporting (0, 30, 0.2f);
  zbPM10.setAnalogInputReporting(0, 30, 0.2f);
  zbVOC.setAnalogInputReporting (0, 60, 1.0f);
  zbNOx.setAnalogInputReporting (0, 60, 1.0f);

  // Initial publish (damit Z2M sofort Werte sieht)
  readMock(reading);
  publishToZigbee(reading);

  // Erste EP1-Messung ebenfalls sofort senden
  float chipT = temperatureRead();
  zbChipTemp.setTemperature(chipT);
  zbChipTemp.reportTemperature();
}

void loop() {
  static uint32_t tick = 0;
  // Calculate tick count for publish interval (loop delay is 100ms = 0.1s)
  static const uint32_t publishIntervalTicks = (PUBLISH_INTERVAL_SEC * 10);

  // Publish sensor data at configured interval
  if ((tick++ % publishIntervalTicks) == 0) {
#if USE_SEN66_REAL
    if (!readSEN66(reading)) {
      readMock(reading);
    }
#else
    readMock(reading);
#endif
    
    // Set values and force reports to ensure updates are sent
    // (Automatic reporting may not trigger if delta thresholds aren't exceeded)
    zbTempHum.setTemperature(reading.tempC);
    zbTempHum.setHumidity(reading.hum);
    zbCO2.setCarbonDioxide((float)reading.co2ppm);
    zbPM25.setPM25(reading.pm25);
    zbPM1.setAnalogInput(reading.pm1);
    zbPM4.setAnalogInput(reading.pm4);
    zbPM10.setAnalogInput(reading.pm10);
    zbVOC.setAnalogInput(reading.vocIndex);
    zbNOx.setAnalogInput(reading.noxIndex);
    
    // EP1: chip temperature
    float chipT = temperatureRead();
    zbChipTemp.setTemperature(chipT);
    
    // Force immediate reports for all sensors
    // This ensures updates are sent even if automatic reporting thresholds aren't met
    zbChipTemp.reportTemperature();
    zbTempHum.reportTemperature();
    zbTempHum.reportHumidity();
    zbCO2.report();
    zbPM25.report();
    zbPM1.reportAnalogInput();
    zbPM4.reportAnalogInput();
    zbPM10.reportAnalogInput();
    zbVOC.reportAnalogInput();
    zbNOx.reportAnalogInput();

    Serial.printf("CHIP=%.2f°C | T=%.2f°C RH=%.2f%% CO2=%u | PM1=%.1f PM2.5=%.1f PM4=%.1f PM10=%.1f | VOC=%.0f NOx=%.0f\n",
      chipT, reading.tempC, reading.hum, reading.co2ppm,
      reading.pm1, reading.pm25, reading.pm4, reading.pm10,
      reading.vocIndex, reading.noxIndex);
  }

  // Button handling unchanged
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(100);
    int start = millis();
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(50);
      if (millis() - start > 3000) {
        Serial.println("Factory reset Zigbee...");
        delay(1000);
        Zigbee.factoryReset();
      }
    }
    // Manual report on button press
    zbChipTemp.reportTemperature();
    zbTempHum.reportTemperature();
    zbTempHum.reportHumidity();
    zbCO2.report();
    zbPM25.report();
    zbPM1.reportAnalogInput();
    zbPM4.reportAnalogInput();
    zbPM10.reportAnalogInput();
    zbVOC.reportAnalogInput();
    zbNOx.reportAnalogInput();
    }

  delay(100);
}

