#pragma once

#include <Arduino.h>

enum class SensorHealth : uint8_t {
  kMissing,
  kWarming,
  kReady,
  kError,
};

struct CurrentReadings {
  float temperatureC = NAN;
  float humidityPercent = NAN;
  uint16_t eco2Ppm = 0;
  uint16_t tvocPpb = 0;
  uint8_t aqi = 0;
  uint32_t sampledAtMs = 0;
  bool climateValid = false;
  bool airValid = false;
};

struct DeviceState {
  CurrentReadings readings;
  SensorHealth sht30Health = SensorHealth::kMissing;
  SensorHealth ens160Health = SensorHealth::kMissing;
  bool oledAvailable = false;
};

struct HistoryPoint {
  uint32_t recordedAtMs = 0;
  float temperatureC = 0;
  float humidityPercent = 0;
  float aqi = 0;
  float tvocPpb = 0;
  float eco2Ppm = 0;
};

