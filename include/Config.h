#pragma once

#include <Arduino.h>

namespace config {

constexpr uint8_t kSdaPin = 4;
constexpr uint8_t kSclPin = 5;
constexpr uint8_t kRgbPin = 8;

constexpr uint8_t kSht30Address = 0x44;
constexpr uint8_t kEns160Address = 0x53;
constexpr uint8_t kOledAddress = 0x3C;
constexpr uint8_t kOledWidth = 128;
constexpr uint8_t kOledHeight = 64;

constexpr uint32_t kSensorIntervalMs = 2000;
constexpr uint32_t kDisplayIntervalMs = 3000;
constexpr uint32_t kHistoryIntervalMs = 5 * 60 * 1000;
constexpr size_t kHistoryCapacity = 24 * 60 / 5;

constexpr char kAccessPointName[] = "ESP32-AirMonitor";
constexpr char kMdnsHostName[] = "air-monitor";
constexpr uint16_t kConfigPortalTimeoutSeconds = 180;

}  // namespace config
