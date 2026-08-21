#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <ScioSense_ENS160.h>

#include <cmath>

#include "AirQuality.h"
#include "AppTypes.h"
#include "Config.h"
#include "HistoryBuffer.h"

namespace {

Adafruit_SHT31 sht30;
ScioSense_ENS160 ens160(&Wire, config::kEns160Address);
Adafruit_SSD1306 oled(config::kOledWidth, config::kOledHeight, &Wire, -1);
Adafruit_NeoPixel indicator(1, config::kRgbPin, NEO_GRB + NEO_KHZ800);
WiFiManager wifiManager;
WebServer webServer(80);

DeviceState deviceState;
HistoryBuffer<HistoryPoint, config::kHistoryCapacity> history;

bool sht30Detected = false;
bool ens160Detected = false;
bool webServerStarted = false;
uint32_t nextSensorAtMs = 0;
uint32_t nextDisplayAtMs = 0;
uint32_t nextHistoryAtMs = config::kHistoryIntervalMs;
uint32_t nextWifiReconnectAtMs = 0;
uint8_t displayPage = 0;

struct HistoryAccumulator {
  float temperatureTotal = 0;
  float humidityTotal = 0;
  uint32_t aqiTotal = 0;
  uint32_t tvocTotal = 0;
  uint32_t eco2Total = 0;
  uint16_t count = 0;

  void add(const CurrentReadings& readings) {
    if (!readings.climateValid || !readings.airValid) {
      return;
    }
    temperatureTotal += readings.temperatureC;
    humidityTotal += readings.humidityPercent;
    aqiTotal += readings.aqi;
    tvocTotal += readings.tvocPpb;
    eco2Total += readings.eco2Ppm;
    ++count;
  }

  bool flush(uint32_t recordedAtMs, HistoryPoint& point) {
    if (count == 0) {
      reset();
      return false;
    }
    point.recordedAtMs = recordedAtMs;
    point.temperatureC = temperatureTotal / count;
    point.humidityPercent = humidityTotal / count;
    point.aqi = static_cast<float>(aqiTotal) / count;
    point.tvocPpb = static_cast<float>(tvocTotal) / count;
    point.eco2Ppm = static_cast<float>(eco2Total) / count;
    reset();
    return true;
  }

  void reset() {
    temperatureTotal = 0;
    humidityTotal = 0;
    aqiTotal = 0;
    tvocTotal = 0;
    eco2Total = 0;
    count = 0;
  }
};

HistoryAccumulator historyAccumulator;

bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

const char* healthName(SensorHealth health) {
  switch (health) {
    case SensorHealth::kMissing:
      return "missing";
    case SensorHealth::kWarming:
      return "warming";
    case SensorHealth::kReady:
      return "ready";
    case SensorHealth::kError:
      return "error";
  }
  return "unknown";
}

void scanI2cBus() {
  Serial.println("I2C scan:");
  uint8_t found = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  found 0x%02X\n", address);
      ++found;
    }
  }
  Serial.printf("I2C devices: %u\n", found);
}

void initializeHardware() {
  Wire.begin(config::kSdaPin, config::kSclPin);
  scanI2cBus();

  deviceState.oledAvailable = oled.begin(SSD1306_SWITCHCAPVCC, config::kOledAddress);
  if (deviceState.oledAvailable) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.println("Air Monitor");
    oled.println("Starting...");
    oled.display();
  } else {
    Serial.println("OLED not detected");
  }

  sht30Detected = sht30.begin(config::kSht30Address);
  deviceState.sht30Health =
      sht30Detected ? SensorHealth::kError : SensorHealth::kMissing;
  Serial.printf("SHT30: %s\n", sht30Detected ? "detected" : "missing");

  ens160.setI2C(config::kSdaPin, config::kSclPin);
  ens160Detected = ens160.begin();
  if (ens160Detected) {
    ens160.setMode(ENS160_OPMODE_STD);
    deviceState.ens160Health = SensorHealth::kWarming;
  } else {
    deviceState.ens160Health = SensorHealth::kMissing;
  }
  Serial.printf("ENS160: %s\n", ens160Detected ? "detected" : "missing");

  indicator.begin();
  indicator.setBrightness(32);
  indicator.clear();
  indicator.show();
}

void sampleSensors(uint32_t now) {
  CurrentReadings& readings = deviceState.readings;
  readings.sampledAtMs = now;
  readings.climateValid = false;
  readings.airValid = false;

  if (sht30Detected) {
    const float temperature = sht30.readTemperature();
    const float humidity = sht30.readHumidity();
    const bool valid = std::isfinite(temperature) && std::isfinite(humidity) &&
                       temperature >= -40.0F && temperature <= 125.0F &&
                       humidity >= 0.0F && humidity <= 100.0F;
    if (valid) {
      readings.temperatureC = temperature;
      readings.humidityPercent = humidity;
      readings.climateValid = true;
      deviceState.sht30Health = SensorHealth::kReady;
      if (ens160Detected) {
        ens160.set_envdata(temperature, humidity);
      }
    } else {
      deviceState.sht30Health = SensorHealth::kError;
    }
  }

  if (ens160Detected) {
    const bool measurementReady = ens160.measure(false);
    if (measurementReady) {
      const uint8_t aqi = ens160.getAQI();
      const uint16_t tvoc = ens160.getTVOC();
      const uint16_t eco2 = ens160.geteCO2();
      if (aqi >= 1 && aqi <= 5) {
        readings.aqi = aqi;
        readings.tvocPpb = tvoc;
        readings.eco2Ppm = eco2;
        readings.airValid = true;
        deviceState.ens160Health = SensorHealth::kReady;
      } else {
        deviceState.ens160Health = SensorHealth::kError;
      }
    } else if (deviceState.ens160Health == SensorHealth::kWarming) {
      deviceState.ens160Health = SensorHealth::kWarming;
    } else {
      deviceState.ens160Health = SensorHealth::kError;
    }
  }

  historyAccumulator.add(readings);

  Serial.printf("T=%.1fC RH=%.1f%% AQI=%u TVOC=%u eCO2=%u climate=%d air=%d\n",
                readings.temperatureC, readings.humidityPercent, readings.aqi,
                readings.tvocPpb, readings.eco2Ppm, readings.climateValid,
                readings.airValid);
}

void updateHistory(uint32_t now) {
  if (!deadlineReached(now, nextHistoryAtMs)) {
    return;
  }

  HistoryPoint point;
  if (historyAccumulator.flush(now, point)) {
    history.push(point);
  }

  do {
    nextHistoryAtMs += config::kHistoryIntervalMs;
  } while (deadlineReached(now, nextHistoryAtMs));
}

void updateIndicator(uint32_t now) {
  const IndicatorLevel level = indicatorLevelForAqi(
      deviceState.readings.aqi, deviceState.readings.airValid);

  uint32_t color = 0;
  switch (level) {
    case IndicatorLevel::kGood:
      color = indicator.Color(0, 180, 55);
      break;
    case IndicatorLevel::kModerate:
      color = indicator.Color(210, 120, 0);
      break;
    case IndicatorLevel::kPoor:
      color = indicator.Color(220, 20, 18);
      break;
    case IndicatorLevel::kUnavailable:
      color = ((now / 500) % 2 == 0) ? indicator.Color(110, 0, 150) : 0;
      break;
  }
  indicator.setPixelColor(0, color);
  indicator.show();
}

void drawHeader(const char* title) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(title);
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void renderDisplay() {
  if (!deviceState.oledAvailable) {
    return;
  }

  const CurrentReadings& readings = deviceState.readings;
  switch (displayPage) {
    case 0:
      drawHeader("CLIMATE");
      oled.setTextSize(2);
      oled.setCursor(0, 17);
      if (readings.climateValid) {
        oled.printf("%.1f C\n", readings.temperatureC);
        oled.printf("%.1f %%", readings.humidityPercent);
      } else {
        oled.println("NO DATA");
      }
      break;
    case 1:
      drawHeader("AIR QUALITY");
      oled.setCursor(0, 16);
      if (readings.airValid) {
        oled.printf("AQI   %u / 5\n", readings.aqi);
        oled.printf("TVOC  %u ppb\n", readings.tvocPpb);
        oled.printf("eCO2  %u ppm", readings.eco2Ppm);
      } else {
        oled.printf("ENS160: %s", healthName(deviceState.ens160Health));
      }
      break;
    case 2:
      drawHeader("NETWORK");
      oled.setCursor(0, 16);
      if (WiFi.status() == WL_CONNECTED) {
        oled.println(WiFi.SSID());
        oled.println(WiFi.localIP());
        oled.println("air-monitor.local");
      } else {
        oled.println("Setup AP:");
        oled.println(config::kAccessPointName);
        oled.println("Pass: airmonitor");
      }
      break;
    default:
      drawHeader("DEVICE STATUS");
      oled.setCursor(0, 16);
      oled.printf("SHT30  %s\n", healthName(deviceState.sht30Health));
      oled.printf("ENS160 %s\n", healthName(deviceState.ens160Health));
      oled.printf("History %u/%u", static_cast<unsigned>(history.size()),
                  static_cast<unsigned>(history.capacity()));
      break;
  }

  oled.display();
  displayPage = (displayPage + 1) % 4;
}

void handleStatusApi() {
  StaticJsonDocument<768> document;
  document["uptimeSeconds"] = millis() / 1000;
  document["historyPoints"] = history.size();

  JsonObject wifi = document.createNestedObject("wifi");
  wifi["connected"] = WiFi.status() == WL_CONNECTED;
  wifi["ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
  wifi["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  wifi["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;

  JsonObject sensors = document.createNestedObject("sensors");
  sensors["sht30"] = healthName(deviceState.sht30Health);
  sensors["ens160"] = healthName(deviceState.ens160Health);

  const CurrentReadings& readings = deviceState.readings;
  JsonObject values = document.createNestedObject("readings");
  values["sampledAgoSeconds"] = (millis() - readings.sampledAtMs) / 1000;
  if (readings.climateValid) {
    values["temperatureC"] = readings.temperatureC;
    values["humidityPercent"] = readings.humidityPercent;
  } else {
    values["temperatureC"] = nullptr;
    values["humidityPercent"] = nullptr;
  }
  if (readings.airValid) {
    values["aqi"] = readings.aqi;
    values["tvocPpb"] = readings.tvocPpb;
    values["eco2Ppm"] = readings.eco2Ppm;
  } else {
    values["aqi"] = nullptr;
    values["tvocPpb"] = nullptr;
    values["eco2Ppm"] = nullptr;
  }

  String payload;
  serializeJson(document, payload);
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", payload);
}

void handleHistoryApi() {
  const uint32_t now = millis();
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "application/json", "");
  webServer.sendContent("{\"intervalSeconds\":300,\"capacity\":288,\"points\":[");

  for (size_t i = 0; i < history.size(); ++i) {
    const HistoryPoint& point = history.at(i);
    String chunk;
    chunk.reserve(160);
    if (i > 0) {
      chunk += ',';
    }
    chunk += "{\"ageSeconds\":";
    chunk += String((now - point.recordedAtMs) / 1000);
    chunk += ",\"temperatureC\":";
    chunk += String(point.temperatureC, 1);
    chunk += ",\"humidityPercent\":";
    chunk += String(point.humidityPercent, 1);
    chunk += ",\"aqi\":";
    chunk += String(point.aqi, 2);
    chunk += ",\"tvocPpb\":";
    chunk += String(point.tvocPpb, 1);
    chunk += ",\"eco2Ppm\":";
    chunk += String(point.eco2Ppm, 1);
    chunk += '}';
    webServer.sendContent(chunk);
    if (i % 16 == 0) {
      yield();
    }
  }

  webServer.sendContent("]}");
  webServer.sendContent("");
}

void handleDashboard() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    webServer.send(500, "text/plain", "Dashboard file is missing. Upload LittleFS data.");
    return;
  }
  webServer.sendHeader("Cache-Control", "no-cache");
  webServer.streamFile(file, "text/html; charset=utf-8");
  file.close();
}

void startWebServer() {
  webServer.on("/", HTTP_GET, handleDashboard);
  webServer.on("/api/status", HTTP_GET, handleStatusApi);
  webServer.on("/api/history", HTTP_GET, handleHistoryApi);
  webServer.onNotFound([]() {
    webServer.send(404, "application/json", "{\"error\":\"not_found\"}");
  });
  webServer.begin();
  webServerStarted = true;

  if (MDNS.begin(config::kMdnsHostName)) {
    MDNS.addService("http", "tcp", 80);
  }
  Serial.printf("Dashboard: http://%s/\n", WiFi.localIP().toString().c_str());
}

void initializeNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  wifiManager.setConnectTimeout(15);
  wifiManager.setConfigPortalTimeout(config::kConfigPortalTimeoutSeconds);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.autoConnect(config::kAccessPointName, "airmonitor");
}

void serviceNetwork() {
  const uint32_t now = millis();
  if (!webServerStarted) {
    wifiManager.process();
    if (WiFi.status() == WL_CONNECTED) {
      wifiManager.stopConfigPortal();
      startWebServer();
    } else if (deadlineReached(now, nextWifiReconnectAtMs)) {
      WiFi.reconnect();
      nextWifiReconnectAtMs = now + 30000;
    }
  } else {
    webServer.handleClient();
    if (WiFi.status() != WL_CONNECTED &&
        deadlineReached(now, nextWifiReconnectAtMs)) {
      WiFi.reconnect();
      nextWifiReconnectAtMs = now + 30000;
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("\nESP32 desktop air monitor");

  initializeHardware();
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }
  initializeNetwork();

  const uint32_t now = millis();
  nextSensorAtMs = now;
  nextDisplayAtMs = now;
  nextHistoryAtMs = now + config::kHistoryIntervalMs;
  nextWifiReconnectAtMs = now + 30000;
}

void loop() {
  const uint32_t now = millis();

  if (deadlineReached(now, nextSensorAtMs)) {
    sampleSensors(now);
    do {
      nextSensorAtMs += config::kSensorIntervalMs;
    } while (deadlineReached(now, nextSensorAtMs));
  }

  updateHistory(now);
  updateIndicator(now);

  if (deadlineReached(now, nextDisplayAtMs)) {
    renderDisplay();
    do {
      nextDisplayAtMs += config::kDisplayIntervalMs;
    } while (deadlineReached(now, nextDisplayAtMs));
  }

  serviceNetwork();
  delay(2);
}
