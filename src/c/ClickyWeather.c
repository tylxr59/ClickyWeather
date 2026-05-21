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
#define IDX_ADVICE 1
#define IDX_ALERTS 2
#define IDX_HOURS  3
#define IDX_WEEK   4
#define IDX_PRECIP 5
#define IDX_UV     6
#define IDX_AQ     7
#define IDX_SUN    8
#define IDX_NIGHT  9
#define IDX_GOLDEN 10

static const int s_toggle_to_card_idx[SETTINGS_TOGGLEABLE_COUNT] = {
  IDX_HOURS, IDX_WEEK, IDX_PRECIP, IDX_UV, IDX_AQ, IDX_SUN, IDX_NIGHT, IDX_GOLDEN,
  IDX_ADVICE, IDX_ALERTS,
};

static void prv_apply_card_visibility(void) {
  for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
    nav_set_enabled(s_toggle_to_card_idx[i],
                    settings_get_enabled((ToggleId)i));
  }
}

// Touch is plumbed for emery / gabbro hardware. Requires firmware >= 5.92.
// Flip ENABLE_TOUCH to 0 if running against an older simulator that
// doesn't ship the touch_service API.
// Currently used only for pull-down-to-refresh; tap navigation is handled
// by the UP/DOWN/SELECT buttons.
#define ENABLE_TOUCH 1

static Window *s_window;

#if ENABLE_TOUCH && defined(PBL_TOUCH)
static bool s_tracking = false;
static int16_t s_start_x = 0;
static int16_t s_start_y = 0;

static void touch_handler(const TouchEvent *event, void *context) {
  switch (event->type) {
    case TouchEvent_Touchdown:
      s_tracking = true;
      s_start_x = event->x;
      s_start_y = event->y;
      break;
    case TouchEvent_Liftoff: {
      if (!s_tracking) break;
      int16_t dx = event->x - s_start_x;
      int16_t dy = event->y - s_start_y;
      int16_t adx = dx < 0 ? -dx : dx;
      int16_t ady = dy < 0 ? -dy : dy;
      const int16_t PULLDOWN_THRESHOLD = 60;
      if (dy > PULLDOWN_THRESHOLD && ady > adx) {
        // Pull-down = manual refresh.
        comm_request_refresh();
      }
      s_tracking = false;
      break;
    }
    default:
      break;
  }
}
#endif

static void prv_select_click(ClickRecognizerRef r, void *ctx) {
  (void)r; (void)ctx;
  // Short-press SELECT to toggle Light/Dark theme.
  theme_set(theme_get() == THEME_LIGHT ? THEME_DARK : THEME_LIGHT);
  nav_redraw();
}

static void prv_select_long(ClickRecognizerRef r, void *ctx) {
  (void)r; (void)ctx;
  // Long-press SELECT toggles theme.
  theme_set(theme_get() == THEME_LIGHT ? THEME_DARK : THEME_LIGHT);
  nav_redraw();
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
  window_long_click_subscribe(BUTTON_ID_SELECT, 600, prv_select_long, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click);
}

static void prv_window_load(Window *window) {
  theme_apply_to_window(window);
  nav_init(window);

  nav_register("Main", card_main_draw);
  nav_register("Click & Go", card_advice_draw);
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

#if ENABLE_TOUCH && defined(PBL_TOUCH)
  if (touch_service_is_enabled()) {
    touch_service_subscribe(touch_handler, NULL);
  }
#endif
}

static void prv_window_unload(Window *window) {
#if ENABLE_TOUCH && defined(PBL_TOUCH)
  touch_service_unsubscribe();
#endif
  nav_deinit();
}

static void prv_init(void) {
  theme_init();
  settings_load();
  
  // Initialize mock data only if we don't have cached weather data.
  // comm_init() will load cached data if it exists; if not, mock data
  // provides sensible defaults for the UI until PKJS sends real data.
  // This prevents phantom alerts from mock data overwriting a real app state.
  comm_init();
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

  comm_set_update_callback(nav_redraw);
  anim_init();
}

static void prv_deinit(void) {
  anim_deinit();
  comm_deinit();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
