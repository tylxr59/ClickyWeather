#include <pebble.h>
#include "theme.h"
#include "nav.h"
#include "weather_data.h"
#include "comm.h"
#include "anim.h"
#include "settings.h"
#include "cards/cards.h"

// Card-registry indices for the toggleable cards. Must match the
// nav_register order below — kept in one place so it's obvious what
// shifts if cards are reordered.
#define IDX_ALERTS 1
#define IDX_HOURS  2
#define IDX_WEEK   3
#define IDX_PRECIP 4
#define IDX_UV     5
#define IDX_AQ     6
#define IDX_SUN    7
#define IDX_NIGHT  8
#define IDX_GOLDEN 9

static const int s_toggle_to_card_idx[SETTINGS_TOGGLEABLE_COUNT] = {
  IDX_HOURS, IDX_WEEK, IDX_PRECIP, IDX_UV, IDX_AQ, IDX_SUN, IDX_NIGHT, IDX_GOLDEN,
  IDX_ALERTS,
};

static void prv_apply_card_visibility(void) {
  for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
    nav_set_enabled(s_toggle_to_card_idx[i],
                    settings_get_enabled((ToggleId)i));
  }
  if (!nav_is_enabled(nav_current_index())) {
    nav_show_index(0);
  }
}

static void prv_handle_comm_update(void) {
  prv_apply_card_visibility();
  nav_redraw();
}

static Window *s_window;

static void prv_select_click(ClickRecognizerRef r, void *ctx) {
  (void)r; (void)ctx;
  comm_request_refresh();
}

static void prv_up_click(ClickRecognizerRef r, void *ctx) {
  (void)r; (void)ctx;
  nav_prev();
}

static void prv_down_click(ClickRecognizerRef r, void *ctx) {
  (void)r; (void)ctx;
  nav_next();
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click);
}

static void prv_window_load(Window *window) {
  theme_apply_to_window(window);
  nav_init(window);

  nav_register("Main", card_main_draw);
  nav_register("Alerts", card_alerts_draw);
  nav_register("6 Hours", card_hours_draw);
  nav_register("Week Ahead", card_week_draw);
  nav_register("Precipitation", card_precipitation_draw);
  nav_register("UV", card_uv_draw);
  nav_register("Air Quality", card_air_quality_draw);
  nav_register("Sun Cycle", card_sun_cycle_draw);
  nav_register("Night Sky", card_night_sky_draw);
  nav_register("Golden Hour", card_golden_hour_draw);
  prv_apply_card_visibility();
  nav_show_index(0);

}

static void prv_window_unload(Window *window) {
  nav_deinit();
}

static void prv_init(void) {
  theme_init();
  settings_load();
  
  // Initialize mock data only if we don't have cached weather data.
  // comm_load_cache() will load cached data if it exists; if not, mock data
  // provides sensible defaults for the UI until PKJS sends real data.
  // This prevents phantom alerts from mock data overwriting a real app state.
  comm_load_cache();
  WeatherData *d = weather_data_get();
  if (!d->valid) {
    weather_data_init_mock();
  }

  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);

  comm_set_update_callback(prv_handle_comm_update);
  comm_init();
  anim_init();
}

static void prv_deinit(void) {
  anim_deinit();
  comm_deinit();
  window_destroy(s_window);
}

int main(void) {
  if (launch_reason() == APP_LAUNCH_WAKEUP) {
    weather_data_init_mock();
    settings_load();
    comm_background_init();
    app_event_loop();
    return 0;
  }

  prv_init();
  app_event_loop();
  prv_deinit();
}
