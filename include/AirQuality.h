#pragma once

#include <cstdint>

enum class IndicatorLevel : uint8_t {
  kUnavailable,
  kGood,
  kModerate,
  kPoor,
};

inline IndicatorLevel indicatorLevelForAqi(uint8_t aqi, bool valid) {
  if (!valid || aqi < 1 || aqi > 5) {
    return IndicatorLevel::kUnavailable;
  }
  if (aqi <= 2) {
    return IndicatorLevel::kGood;
  }
  if (aqi == 3) {
    return IndicatorLevel::kModerate;
  }
  return IndicatorLevel::kPoor;
}

