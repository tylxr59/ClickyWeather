#include "weather_data.h"
#include <string.h>

static WeatherData s_data;

void weather_data_init(void) {
  memset(&s_data, 0, sizeof(s_data));
  s_data.uv = -1;
  s_data.uv_max = -1;
  s_data.aqi = -1;
  s_data.pm2_5 = -1;
  s_data.pm10 = -1;
  s_data.o3 = -1;
  s_data.no2 = -1;
  s_data.pollen_level = -1;
  s_data.rain_alert_min = -1;
  s_data.alert_category = ALERT_CAT_UNKNOWN;
  s_data.fetch_error = FETCH_ERROR_NONE;
  for (int i = 0; i < 6; ++i) {
    s_data.hours_uv[i] = -1;
  }
}

WeatherData *weather_data_get(void) { return &s_data; }

const char *uv_label(int uv) {
  if (uv <= 2) return "LOW";
  if (uv <= 5) return "MODERATE";
  if (uv <= 7) return "HIGH";
  if (uv <= 10) return "V.HIGH";
  return "EXTREME";
}

const char *aqi_label(int aqi) {
  if (aqi < 0) return "UNKNOWN";
  if (aqi <= 50) return "GOOD";
  if (aqi <= 100) return "MODERATE";
  if (aqi <= 150) return "SENSITIVE";
  if (aqi <= 200) return "UNHEALTHY";
  if (aqi <= 300) return "V.UNHEALTHY";
  return "HAZARDOUS";
}
