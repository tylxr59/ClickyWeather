#include "comm.h"
#include "weather_data.h"
#include "theme.h"
#include "settings.h"
#include "anim.h"
#include "glance.h"
#include "cards/cards.h"
#include <string.h>
#include <stdlib.h>

static CommUpdateCb s_update_cb = NULL;

static int s_bg_fail_count = 0;
static AppTimer *s_bg_timeout_timer = NULL;
static AppTimer *s_bg_exit_timer = NULL;
static AppTimer *s_refresh_timeout_timer = NULL;
static AppTimer *s_settings_refresh_timer = NULL;
static bool s_is_background_mode = false;
static WakeupId s_wakeup_id = -1;
static Window *s_bg_window = NULL;
static uint32_t s_request_seq = 0;
static uint32_t s_active_request_seq = 0;
static bool s_force_refresh_context = true;

#define PERSIST_KEY_WAKEUP_ID 300
#define BG_MAX_BACKOFF_MINUTES 240
#define REFRESH_TIMEOUT_MS 35000

static void prv_reschedule_wakeup(bool success);
static void prv_background_finish(bool success);
static void prv_request_refresh(bool force);
static void prv_initial_refresh(void *ctx);

// Bumped from 100 -> 101 when last_updated was added to WeatherData,
// to avoid loading a stale-layout blob from older app versions.
// Bumped 101 -> 102 when Phase 11 added blue_am/gold_am/gold_pm/blue_pm
// (10 bytes each).
// Bumped 102 -> 103 when dew_point/use_dew_point/pollen_level were added.
// Bumped 103 -> 104 when alert_active/alert_category were added.
// Bumped 104 -> 105 when wind_gust/precip_amount were added.
// Bumped 105 -> 106 when update_failed was added.
// Bumped 106 -> 107 when refresh_in_progress was added.
// Bumped 107 -> 108 when uv_max, hourly wind/precip details, and the
// fifth week-ahead day were added.
// Bumped 108 -> 109 when update_available was added.
// Bumped 109 -> 110 for wider sun times plus UV/AQI detail fields.
// Bumped 110 -> 111 when refresh errors became a typed enum.
#define PERSIST_KEY_CACHE 111
#define PERSIST_KEY_CACHE_PREVIOUS 110

static void prv_save_cache(void) {
  WeatherData *d = weather_data_get();
  if (d->valid) {
    persist_write_data(PERSIST_KEY_CACHE, d, sizeof(WeatherData));
  }
}

static void prv_load_cache(void) {
  if (persist_exists(PERSIST_KEY_CACHE)) {
    WeatherData *d = weather_data_get();
    persist_read_data(PERSIST_KEY_CACHE, d, sizeof(WeatherData));
    // Keep the last verified alert result with the cached update timestamp.
    // A new response always replaces it with active, clear, or unavailable.
    d->refresh_in_progress = false;
    d->fetch_error = FETCH_ERROR_NONE;
  } else if (persist_exists(PERSIST_KEY_CACHE_PREVIOUS)) {
    persist_delete(PERSIST_KEY_CACHE_PREVIOUS);
  }
}

static void prv_cancel_refresh_timeout(void) {
  if (s_refresh_timeout_timer) {
    app_timer_cancel(s_refresh_timeout_timer);
    s_refresh_timeout_timer = NULL;
  }
}

static void prv_set_refresh_state(bool in_progress, FetchError error) {
  WeatherData *d = weather_data_get();
  d->refresh_in_progress = in_progress;
  d->fetch_error = error;
  if (s_update_cb) s_update_cb();
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  WeatherData *d = weather_data_get();
  bool got_anything = false;
  int fetch_error_value = FETCH_ERROR_NONE;

  Tuple *t;

  Tuple *response_seq = dict_find(iter, MESSAGE_KEY_ResponseSeq);
  if (response_seq) {
    uint32_t seq = response_seq->value->uint32;
    if (s_active_request_seq == 0 || seq != s_active_request_seq) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Ignoring stale response %lu",
              (unsigned long)seq);
      return;
    }
  }

  if ((t = dict_find(iter, MESSAGE_KEY_Theme))) {
    // Clay radiogroup with string values delivers as TUPLE_CSTRING; older
    // builds / direct AppMessage tests deliver TUPLE_INT. Accept both.
    int theme_val = 0;
    if (t->type == TUPLE_CSTRING) {
      theme_val = atoi(t->value->cstring);
    } else {
      theme_val = (int)t->value->int32;
    }
    theme_set(theme_val ? THEME_DARK : THEME_LIGHT);
    if (s_update_cb) s_update_cb();
  }
  if ((t = dict_find(iter, MESSAGE_KEY_EnabledMask))) {
    // Each bit represents a card's enabled state.
    // bit 0 = Toggle0/HOURS, ..., bit 8 = Toggle8/ALERTS.
    uint32_t mask = t->value->uint32;
    for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
      bool enabled = (mask & (1 << i)) != 0;
      settings_set_enabled((ToggleId)i, enabled);
    }
    if (s_update_cb) s_update_cb();
  }
  if ((t = dict_find(iter, MESSAGE_KEY_FetchError))) {
    fetch_error_value = (int)t->value->int32;
    if (fetch_error_value < FETCH_ERROR_NONE ||
        fetch_error_value > FETCH_ERROR_TIMEOUT) {
      fetch_error_value = FETCH_ERROR_NETWORK;
    }
  }
  if ((t = dict_find(iter, MESSAGE_KEY_UpdateAvailable))) {
    d->update_available = (t->value->int32 != 0);
    if (d->valid) {
      prv_save_cache();
    }
    if (s_update_cb) s_update_cb();
  }
  if ((t = dict_find(iter, MESSAGE_KEY_Temp))) { d->temp = t->value->int32; got_anything = true; }
  if ((t = dict_find(iter, MESSAGE_KEY_FeelsLike))) { d->feels_like = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_High))) { d->high = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Low))) { d->low = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Condition))) { d->condition = (WeatherCondition)t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Wind))) { d->wind_speed = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_WindDir))) {
    strncpy(d->wind_dir, t->value->cstring, sizeof(d->wind_dir) - 1);
    d->wind_dir[sizeof(d->wind_dir) - 1] = '\0';
  }
  if ((t = dict_find(iter, MESSAGE_KEY_Humidity))) { d->humidity = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_DewPoint))) { d->dew_point = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_UseDewPoint))) {
    d->use_dew_point = (t->value->int32 != 0);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_LoopNavigation))) {
    settings_set_loop_nav(t->value->int32 != 0);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_BackgroundUpdateInterval))) {
    int old_interval = settings_get_background_interval();
    int new_interval = t->type == TUPLE_CSTRING
      ? atoi(t->value->cstring)
      : (int)t->value->int32;
    if (new_interval != 0 && (new_interval < 300 || new_interval > 86400)) {
      new_interval = 0;
    }
    settings_set_background_interval(new_interval);
    if (new_interval != old_interval) {
      if (new_interval == 0) {
        wakeup_cancel_all();
        s_wakeup_id = -1;
        persist_delete(PERSIST_KEY_WAKEUP_ID);
      } else {
        prv_reschedule_wakeup(true);
      }
    }
  }
  if ((t = dict_find(iter, MESSAGE_KEY_AnimationsEnabled))) {
    bool enabled = t->type == TUPLE_CSTRING
      ? atoi(t->value->cstring) != 0
      : t->value->int32 != 0;
    settings_set_animations_enabled(enabled);
    anim_kick();
    if (s_update_cb) s_update_cb();
  }
  if ((t = dict_find(iter, MESSAGE_KEY_RefreshWeather)) &&
      t->value->int32 != 0 && !s_settings_refresh_timer) {
    s_settings_refresh_timer = app_timer_register(
        100, prv_initial_refresh, &s_force_refresh_context);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_PollenLevel))) {
    d->pollen_level = (int)t->value->int32;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_AlertActive))) {
    d->alert_active = (t->value->int32 != 0);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_AlertCategory))) {
    d->alert_category = (AlertCategory)t->value->int32;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_WindGust)))     { d->wind_gust = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_PrecipAmount))) { d->precip_amount = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Precip0))) { d->precip[0] = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Precip1))) { d->precip[1] = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Precip2))) { d->precip[2] = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Precip3))) { d->precip[3] = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Precip4))) { d->precip[4] = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_UV))) { d->uv = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_UVMax))) { d->uv_max = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_AQI))) { d->aqi = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_PM25))) { d->pm2_5 = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_PM10))) { d->pm10 = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_O3))) { d->o3 = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_NO2))) { d->no2 = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Sunrise))) {
    strncpy(d->sunrise, t->value->cstring, sizeof(d->sunrise) - 1);
    d->sunrise[sizeof(d->sunrise) - 1] = '\0';
  }
  if ((t = dict_find(iter, MESSAGE_KEY_Sunset))) {
    strncpy(d->sunset, t->value->cstring, sizeof(d->sunset) - 1);
    d->sunset[sizeof(d->sunset) - 1] = '\0';
  }
  if ((t = dict_find(iter, MESSAGE_KEY_RainAlertMinutes))) { d->rain_alert_min = t->value->int32; }
  if ((t = dict_find(iter, MESSAGE_KEY_Units))) {
    d->units = (Units)(t->type == TUPLE_CSTRING
      ? atoi(t->value->cstring) : (int)t->value->int32);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_LastUpdated))) {
    // PKJS sends the unix-second timestamp of the fetch. Sentinel "1" from
    // our refresh-trigger doesn't carry useful time info; treat any value
    // < 100000 as "not a real timestamp" and use device-clock time instead.
    uint32_t v = (uint32_t)t->value->int32;
    if (v >= 100000U) {
      d->last_updated = v;
    } else {
      d->last_updated = (uint32_t)time(NULL);
    }
    got_anything = true;
  }

  // Phase 10A: Next 6 Hours (was 4 in Phase 5).
  uint32_t hour_label_keys[6] = {
    MESSAGE_KEY_Hour1Label, MESSAGE_KEY_Hour2Label,
    MESSAGE_KEY_Hour3Label, MESSAGE_KEY_Hour4Label,
    MESSAGE_KEY_Hour5Label, MESSAGE_KEY_Hour6Label
  };
  uint32_t hour_temp_keys[6] = {
    MESSAGE_KEY_Hour1Temp, MESSAGE_KEY_Hour2Temp,
    MESSAGE_KEY_Hour3Temp, MESSAGE_KEY_Hour4Temp,
    MESSAGE_KEY_Hour5Temp, MESSAGE_KEY_Hour6Temp
  };
  uint32_t hour_cond_keys[6] = {
    MESSAGE_KEY_Hour1Cond, MESSAGE_KEY_Hour2Cond,
    MESSAGE_KEY_Hour3Cond, MESSAGE_KEY_Hour4Cond,
    MESSAGE_KEY_Hour5Cond, MESSAGE_KEY_Hour6Cond
  };
  uint32_t hour_pop_keys[6] = {
    MESSAGE_KEY_Hour1Pop, MESSAGE_KEY_Hour2Pop,
    MESSAGE_KEY_Hour3Pop, MESSAGE_KEY_Hour4Pop,
    MESSAGE_KEY_Hour5Pop, MESSAGE_KEY_Hour6Pop
  };
  uint32_t hour_wind_keys[6] = {
    MESSAGE_KEY_Hour1Wind, MESSAGE_KEY_Hour2Wind,
    MESSAGE_KEY_Hour3Wind, MESSAGE_KEY_Hour4Wind,
    MESSAGE_KEY_Hour5Wind, MESSAGE_KEY_Hour6Wind
  };
  uint32_t hour_wdir_keys[6] = {
    MESSAGE_KEY_Hour1WindDir, MESSAGE_KEY_Hour2WindDir,
    MESSAGE_KEY_Hour3WindDir, MESSAGE_KEY_Hour4WindDir,
    MESSAGE_KEY_Hour5WindDir, MESSAGE_KEY_Hour6WindDir
  };
  uint32_t hour_precip_keys[6] = {
    MESSAGE_KEY_Hour1Precip, MESSAGE_KEY_Hour2Precip,
    MESSAGE_KEY_Hour3Precip, MESSAGE_KEY_Hour4Precip,
    MESSAGE_KEY_Hour5Precip, MESSAGE_KEY_Hour6Precip
  };
  uint32_t hour_uv_keys[6] = {
    MESSAGE_KEY_Hour1Uv, MESSAGE_KEY_Hour2Uv,
    MESSAGE_KEY_Hour3Uv, MESSAGE_KEY_Hour4Uv,
    MESSAGE_KEY_Hour5Uv, MESSAGE_KEY_Hour6Uv
  };
  for (int i = 0; i < 6; i++) {
    if ((t = dict_find(iter, hour_label_keys[i]))) {
      strncpy(d->hours_label[i], t->value->cstring,
              sizeof(d->hours_label[i]) - 1);
      d->hours_label[i][sizeof(d->hours_label[i]) - 1] = '\0';
    }
    if ((t = dict_find(iter, hour_temp_keys[i]))) {
      d->hours_temp[i] = t->value->int32;
    }
    if ((t = dict_find(iter, hour_cond_keys[i]))) {
      d->hours_cond[i] = (WeatherCondition)t->value->int32;
    }
    if ((t = dict_find(iter, hour_pop_keys[i]))) {
      d->hours_pop[i] = (uint8_t)t->value->int32;
    }
    if ((t = dict_find(iter, hour_wind_keys[i]))) {
      d->hours_wind[i] = t->value->int32;
    }
    if ((t = dict_find(iter, hour_wdir_keys[i]))) {
      strncpy(d->hours_wind_dir[i], t->value->cstring,
              sizeof(d->hours_wind_dir[i]) - 1);
      d->hours_wind_dir[i][sizeof(d->hours_wind_dir[i]) - 1] = '\0';
    }
    if ((t = dict_find(iter, hour_precip_keys[i]))) {
      d->hours_precip_x10[i] = t->value->int32;
    }
    if ((t = dict_find(iter, hour_uv_keys[i]))) {
      d->hours_uv[i] = (int8_t)t->value->int32;
    }
  }

  // Phase 10B: Week Ahead — 5 days (was 4).
  uint32_t day_label_keys[5] = {
    MESSAGE_KEY_Day0Label, MESSAGE_KEY_Day1Label,
    MESSAGE_KEY_Day2Label, MESSAGE_KEY_Day3Label,
    MESSAGE_KEY_Day4Label
  };
  uint32_t day_high_keys[5] = {
    MESSAGE_KEY_Day0High, MESSAGE_KEY_Day1High,
    MESSAGE_KEY_Day2High, MESSAGE_KEY_Day3High,
    MESSAGE_KEY_Day4High
  };
  uint32_t day_low_keys[5] = {
    MESSAGE_KEY_Day0Low, MESSAGE_KEY_Day1Low,
    MESSAGE_KEY_Day2Low, MESSAGE_KEY_Day3Low,
    MESSAGE_KEY_Day4Low
  };
  uint32_t day_cond_keys[5] = {
    MESSAGE_KEY_Day0Cond, MESSAGE_KEY_Day1Cond,
    MESSAGE_KEY_Day2Cond, MESSAGE_KEY_Day3Cond,
    MESSAGE_KEY_Day4Cond
  };
  uint32_t day_pop_keys[5] = {
    MESSAGE_KEY_Day0Pop, MESSAGE_KEY_Day1Pop,
    MESSAGE_KEY_Day2Pop, MESSAGE_KEY_Day3Pop,
    MESSAGE_KEY_Day4Pop
  };
  for (int i = 0; i < 5; i++) {
    if ((t = dict_find(iter, day_label_keys[i]))) {
      strncpy(d->days_label[i], t->value->cstring,
              sizeof(d->days_label[i]) - 1);
      d->days_label[i][sizeof(d->days_label[i]) - 1] = '\0';
    }
    if ((t = dict_find(iter, day_high_keys[i]))) d->days_high[i] = t->value->int32;
    if ((t = dict_find(iter, day_low_keys[i])))  d->days_low[i]  = t->value->int32;
    if ((t = dict_find(iter, day_cond_keys[i]))) d->days_cond[i] = (WeatherCondition)t->value->int32;
    if ((t = dict_find(iter, day_pop_keys[i])))  d->days_pop[i]  = (uint8_t)t->value->int32;
  }

  // Phase 7: Moon.
  if ((t = dict_find(iter, MESSAGE_KEY_MoonPhase))) d->moon_phase = (uint8_t)t->value->int32;
  if ((t = dict_find(iter, MESSAGE_KEY_MoonIllum))) d->moon_illum = (uint8_t)t->value->int32;
  if ((t = dict_find(iter, MESSAGE_KEY_MoonName1))) {
    strncpy(d->moon_name1, t->value->cstring, sizeof(d->moon_name1) - 1);
    d->moon_name1[sizeof(d->moon_name1) - 1] = '\0';
  }
  if ((t = dict_find(iter, MESSAGE_KEY_MoonName2))) {
    strncpy(d->moon_name2, t->value->cstring, sizeof(d->moon_name2) - 1);
    d->moon_name2[sizeof(d->moon_name2) - 1] = '\0';
  }

  // Phase 11: Golden Hour milestone times (formatted strings).
  if ((t = dict_find(iter, MESSAGE_KEY_BlueAm))) {
    strncpy(d->blue_am, t->value->cstring, sizeof(d->blue_am) - 1);
    d->blue_am[sizeof(d->blue_am) - 1] = '\0';
  }
  if ((t = dict_find(iter, MESSAGE_KEY_GoldAm))) {
    strncpy(d->gold_am, t->value->cstring, sizeof(d->gold_am) - 1);
    d->gold_am[sizeof(d->gold_am) - 1] = '\0';
  }
  if ((t = dict_find(iter, MESSAGE_KEY_GoldPm))) {
    strncpy(d->gold_pm, t->value->cstring, sizeof(d->gold_pm) - 1);
    d->gold_pm[sizeof(d->gold_pm) - 1] = '\0';
  }
  if ((t = dict_find(iter, MESSAGE_KEY_BluePm))) {
    strncpy(d->blue_pm, t->value->cstring, sizeof(d->blue_pm) - 1);
    d->blue_pm[sizeof(d->blue_pm) - 1] = '\0';
  }

  if (fetch_error_value != FETCH_ERROR_NONE) {
    prv_cancel_refresh_timeout();
    s_active_request_seq = 0;
    d->refresh_in_progress = false;
    d->fetch_error = (FetchError)fetch_error_value;
    if (s_update_cb) s_update_cb();
    if (s_is_background_mode) prv_background_finish(false);
    return;
  }

  if (got_anything) {
    prv_cancel_refresh_timeout();
    s_active_request_seq = 0;
    d->refresh_in_progress = false;
    d->fetch_error = FETCH_ERROR_NONE;
    d->valid = true;
    prv_save_cache();
    if (!s_is_background_mode) anim_kick();
    if (s_update_cb) s_update_cb();
    if (s_is_background_mode) {
      prv_background_finish(true);
    }
  }
}

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage dropped: %d", (int)reason);
  prv_cancel_refresh_timeout();
  s_active_request_seq = 0;
  prv_set_refresh_state(false, FETCH_ERROR_NETWORK);
  if (s_is_background_mode) {
    prv_background_finish(false);
  }
}

static void prv_outbox_sent(DictionaryIterator *iter, void *context) {
  (void)iter;
  (void)context;
}

static void prv_outbox_failed(DictionaryIterator *iter, AppMessageResult reason,
                              void *context) {
  (void)iter;
  (void)context;
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage send failed: %d", (int)reason);
  prv_cancel_refresh_timeout();
  s_active_request_seq = 0;
  prv_set_refresh_state(false, FETCH_ERROR_NETWORK);
  if (s_is_background_mode) {
    prv_background_finish(false);
  }
}

static void prv_refresh_timeout(void *context) {
  (void)context;
  s_refresh_timeout_timer = NULL;
  s_active_request_seq = 0;
  prv_set_refresh_state(false, FETCH_ERROR_TIMEOUT);
  if (s_is_background_mode) prv_background_finish(false);
}

static void prv_request_refresh(bool force) {
  WeatherData *data = weather_data_get();
  if (data->refresh_in_progress) return;
  uint32_t now = (uint32_t)time(NULL);
  if (!force && !s_is_background_mode &&
      data->fetch_error == FETCH_ERROR_NONE &&
      data->valid && data->last_updated > 0 &&
      now >= data->last_updated && now - data->last_updated < 60) {
    data->refresh_in_progress = false;
    data->fetch_error = FETCH_ERROR_NONE;
    if (s_update_cb) s_update_cb();
    return;
  }
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    prv_set_refresh_state(false, FETCH_ERROR_NETWORK);
    if (s_is_background_mode) {
      prv_background_finish(false);
    }
    return;
  }
  s_request_seq++;
  if (s_request_seq == 0) s_request_seq++;
  s_active_request_seq = s_request_seq;
  // Send a sentinel key to trigger PKJS fetch.
  dict_write_uint8(iter, MESSAGE_KEY_LastUpdated, 1);
  dict_write_uint32(iter, MESSAGE_KEY_RequestSeq, s_active_request_seq);
  dict_write_uint8(iter, MESSAGE_KEY_ClockIs24h,
                   clock_is_24h_style() ? 1 : 0);
  dict_write_end(iter);
  prv_set_refresh_state(true, FETCH_ERROR_NONE);
  if (app_message_outbox_send() != APP_MSG_OK) {
    s_active_request_seq = 0;
    prv_set_refresh_state(false, FETCH_ERROR_NETWORK);
    if (s_is_background_mode) {
      prv_background_finish(false);
    }
    return;
  }
  prv_cancel_refresh_timeout();
  s_refresh_timeout_timer = app_timer_register(
      REFRESH_TIMEOUT_MS, prv_refresh_timeout, NULL);
}

void comm_request_refresh(void) {
  prv_request_refresh(false);
}

void comm_set_update_callback(CommUpdateCb cb) {
  s_update_cb = cb;
}

static void prv_initial_refresh(void *ctx) {
  bool force = ctx != NULL;
  if (force) s_settings_refresh_timer = NULL;
  WeatherData *data = weather_data_get();
  uint32_t now = (uint32_t)time(NULL);
  if (!force && data->valid && data->last_updated > 0 && now >= data->last_updated &&
      now - data->last_updated < 15 * 60) {
    return;
  }
  prv_request_refresh(force);
}

void comm_load_cache(void) {
  prv_load_cache();
  // Trigger redraw if cache was loaded so the screen reconciles immediately.
  // This keeps the last verified state visible while a refresh is pending.
  if (s_update_cb && weather_data_get()->valid) {
    s_update_cb();
  }
}

static void prv_bg_exit(void *context) {
  (void)context;
  s_bg_exit_timer = NULL;
  glance_update();
  app_message_deregister_callbacks();
  if (s_bg_window) {
    window_stack_remove(s_bg_window, false);
    window_destroy(s_bg_window);
    s_bg_window = NULL;
  }
}

static void prv_background_finish(bool success) {
  if (!s_is_background_mode) return;
  s_is_background_mode = false;
  prv_cancel_refresh_timeout();
  s_active_request_seq = 0;
  if (s_bg_timeout_timer) {
    app_timer_cancel(s_bg_timeout_timer);
    s_bg_timeout_timer = NULL;
  }
  prv_reschedule_wakeup(success);
  if (!s_bg_exit_timer) {
    s_bg_exit_timer = app_timer_register(100, prv_bg_exit, NULL);
  }
}

static void prv_bg_timeout(void *context) {
  (void)context;
  s_bg_timeout_timer = NULL;
  prv_set_refresh_state(false, FETCH_ERROR_TIMEOUT);
  prv_background_finish(false);
}

static void prv_reschedule_wakeup(bool success) {
  int interval = settings_get_background_interval();
  if (interval == 0) return;

  int delay_seconds = interval;
  if (success) {
    s_bg_fail_count = 0;
  } else {
    s_bg_fail_count++;
    int shift = s_bg_fail_count - 1;
    if (shift > 5) shift = 5;
    int backoff_minutes = 5 * (1 << shift);
    if (backoff_minutes > BG_MAX_BACKOFF_MINUTES) {
      backoff_minutes = BG_MAX_BACKOFF_MINUTES;
    }
    delay_seconds = backoff_minutes * 60;
  }

  wakeup_cancel_all();
  s_wakeup_id = -1;

  time_t base_time = time(NULL) + delay_seconds;
  for (int attempt = 0; attempt < 15 && s_wakeup_id < 0; ++attempt) {
    s_wakeup_id = wakeup_schedule(
      base_time + attempt * SECONDS_PER_MINUTE, 0, true);
  }

  if (s_wakeup_id >= 0) {
    persist_write_int(PERSIST_KEY_WAKEUP_ID, s_wakeup_id);
  } else {
    persist_delete(PERSIST_KEY_WAKEUP_ID);
  }
}

static void prv_wakeup_handler(WakeupId id, int32_t reason) {
  (void)id;
  (void)reason;
  s_wakeup_id = -1;
  persist_delete(PERSIST_KEY_WAKEUP_ID);
  comm_request_refresh();
  prv_reschedule_wakeup(true);
}

void comm_background_init(void) {
  if (settings_get_background_interval() == 0) return;

  persist_delete(PERSIST_KEY_WAKEUP_ID);
  s_wakeup_id = -1;
  s_is_background_mode = true;

  // A wakeup launch without a window exits its event loop immediately.
  s_bg_window = window_create();
  window_stack_push(s_bg_window, false);

  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_sent(prv_outbox_sent);
  app_message_register_outbox_failed(prv_outbox_failed);
  app_message_open(2048, 256);

  s_bg_timeout_timer = app_timer_register(REFRESH_TIMEOUT_MS, prv_bg_timeout, NULL);
  comm_request_refresh();
}

void comm_init(void) {
  // Cache loading now happens separately in prv_init(), before window push.
  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_sent(prv_outbox_sent);
  app_message_register_outbox_failed(prv_outbox_failed);
  // The weather dictionary grew with hourly wind/precip context and a
  // fifth week-ahead day. Keep enough inbox room for the full PKJS payload.
  app_message_open(2048, 256);
  // Request after a short delay so AppMessage is fully open. PKJS waits for
  // this explicit sentinel instead of fetching on every ready/config event.
  app_timer_register(750, prv_initial_refresh, NULL);

  wakeup_service_subscribe(prv_wakeup_handler);
  if (settings_get_background_interval() > 0) {
    bool wakeup_valid = false;
    if (persist_exists(PERSIST_KEY_WAKEUP_ID)) {
      s_wakeup_id = persist_read_int(PERSIST_KEY_WAKEUP_ID);
      time_t wakeup_time = 0;
      wakeup_valid = wakeup_query(s_wakeup_id, &wakeup_time);
      if (!wakeup_valid) {
        s_wakeup_id = -1;
        persist_delete(PERSIST_KEY_WAKEUP_ID);
      }
    }
    if (!wakeup_valid) {
      prv_reschedule_wakeup(true);
    }
  }
}

void comm_deinit(void) {
  prv_cancel_refresh_timeout();
  if (s_settings_refresh_timer) {
    app_timer_cancel(s_settings_refresh_timer);
    s_settings_refresh_timer = NULL;
  }
  app_message_deregister_callbacks();
}
